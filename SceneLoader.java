package com.mygame;

import com.jme3.app.SimpleApplication;
import com.jme3.asset.AssetManager;
import com.jme3.bullet.BulletAppState;
import com.jme3.bullet.collision.shapes.CollisionShape;
import com.jme3.bullet.control.RigidBodyControl;
import com.jme3.bullet.util.CollisionShapeFactory;
import com.jme3.light.AmbientLight;
import com.jme3.light.DirectionalLight;
import com.jme3.material.Material;
import com.jme3.math.ColorRGBA;
import com.jme3.math.Quaternion;
import com.jme3.math.Vector3f;
import com.jme3.scene.Geometry;
import com.jme3.scene.Node;
import com.jme3.scene.Spatial;
import com.jme3.scene.shape.Box;
import com.jme3.scene.shape.Cylinder;
import com.jme3.scene.shape.Sphere;
import com.google.gson.*;

import java.io.File;
import java.io.FileInputStream;
import java.io.InputStreamReader;

/**
 * SceneLoader - Nap canh xuat tu Unity Hub 3D Vulkan Engine vao jMonkeyEngine 3.x
 */
public class SceneLoader {

    public static Node loadScene(String jsonFilePath, SimpleApplication app, BulletAppState bulletAppState) {
        Node sceneRoot = new Node("ImportedScene");
        AssetManager assetManager = app.getAssetManager();

        try {
            File f = new File(jsonFilePath);
            if (!f.exists()) {
                System.err.println("[SceneLoader] File not found: " + jsonFilePath);
                return sceneRoot;
            }

            JsonObject root = JsonParser.parseReader(new InputStreamReader(new FileInputStream(f), "UTF-8")).getAsJsonObject();
            System.out.println("[SceneLoader] Loading scene: " + (root.has("environment") ? root.get("environment").getAsString() : "Default"));

            // 1. Sun Light
            if (root.has("sunLight")) {
                JsonObject sun = root.getAsJsonObject("sunLight");
                JsonArray dir = sun.getAsJsonArray("direction");
                JsonArray col = sun.getAsJsonArray("color");
                float intensity = sun.has("intensity") ? sun.get("intensity").getAsFloat() : 1.0f;

                DirectionalLight dl = new DirectionalLight();
                dl.setDirection(new Vector3f(dir.get(0).getAsFloat(), dir.get(1).getAsFloat(), dir.get(2).getAsFloat()).normalizeLocal());
                dl.setColor(new ColorRGBA(col.get(0).getAsFloat() * intensity, col.get(1).getAsFloat() * intensity, col.get(2).getAsFloat() * intensity, 1.0f));
                app.getRootNode().addLight(dl);

                AmbientLight al = new AmbientLight();
                al.setColor(new ColorRGBA(0.25f, 0.25f, 0.28f, 1.0f));
                app.getRootNode().addLight(al);
            }

            // 2. Camera Settings
            if (root.has("camera")) {
                JsonObject camObj = root.getAsJsonObject("camera");
                JsonArray pos = camObj.getAsJsonArray("position");
                JsonArray target = camObj.getAsJsonArray("target");
                app.getCamera().setLocation(new Vector3f(pos.get(0).getAsFloat(), pos.get(1).getAsFloat(), pos.get(2).getAsFloat()));
                app.getCamera().lookAt(new Vector3f(target.get(0).getAsFloat(), target.get(1).getAsFloat(), target.get(2).getAsFloat()), Vector3f.UNIT_Y);
            }

            // 3. Load Objects
            if (root.has("objects")) {
                JsonArray objects = root.getAsJsonArray("objects");
                for (JsonElement elem : objects) {
                    JsonObject obj = elem.getAsJsonObject();
                    String name = obj.get("name").getAsString();
                    String type = obj.has("type") ? obj.get("type").getAsString() : "model";

                    Spatial spatial = null;
                    if ("model".equalsIgnoreCase(type) || "terrain".equalsIgnoreCase(type)) {
                        String modelPath = obj.has("modelPath") ? obj.get("modelPath").getAsString() : "";
                        if (!modelPath.isEmpty()) {
                            try {
                                spatial = assetManager.loadModel(modelPath);
                            } catch (Exception ex) {
                                System.err.println("[SceneLoader] Warning: Failed to load model '" + modelPath + "' (" + ex.getMessage() + ")");
                            }
                        }
                    } else if ("primitive".equalsIgnoreCase(type)) {
                        String primType = obj.has("primitiveType") ? obj.get("primitiveType").getAsString() : "CUBE";
                        Geometry geom = null;
                        if ("SPHERE".equalsIgnoreCase(primType)) {
                            geom = new Geometry(name, new Sphere(16, 16, 0.5f));
                        } else if ("CYLINDER".equalsIgnoreCase(primType)) {
                            geom = new Geometry(name, new Cylinder(16, 16, 0.5f, 1.0f, true));
                        } else {
                            geom = new Geometry(name, new Box(0.5f, 0.5f, 0.5f));
                        }
                        Material mat = new Material(assetManager, "Common/MatDefs/Misc/Unshaded.j3md");
                        if (obj.has("color")) {
                            JsonArray col = obj.getAsJsonArray("color");
                            mat.setColor("Color", new ColorRGBA(col.get(0).getAsFloat(), col.get(1).getAsFloat(), col.get(2).getAsFloat(), col.get(3).getAsFloat()));
                        }
                        geom.setMaterial(mat);
                        spatial = geom;
                    }

                    if (spatial == null) continue;
                    spatial.setName(name);

                    // Position
                    if (obj.has("position")) {
                        JsonArray p = obj.getAsJsonArray("position");
                        spatial.setLocalTranslation(p.get(0).getAsFloat(), p.get(1).getAsFloat(), p.get(2).getAsFloat());
                    }

                    // Rotation (Quaternion)
                    if (obj.has("rotationQuat")) {
                        JsonArray q = obj.getAsJsonArray("rotationQuat");
                        spatial.setLocalRotation(new Quaternion(q.get(0).getAsFloat(), q.get(1).getAsFloat(), q.get(2).getAsFloat(), q.get(3).getAsFloat()));
                    }

                    // Scale
                    if (obj.has("scale")) {
                        JsonArray s = obj.getAsJsonArray("scale");
                        spatial.setLocalScale(s.get(0).getAsFloat(), s.get(1).getAsFloat(), s.get(2).getAsFloat());
                    }

                    // Physics
                    if (bulletAppState != null && obj.has("physics")) {
                        JsonObject phys = obj.getAsJsonObject("physics");
                        boolean enabled = phys.has("enabled") && phys.get("enabled").getAsBoolean();
                        boolean hasRb = phys.has("hasRigidBody") && phys.get("hasRigidBody").getAsBoolean();
                        if (enabled || hasRb) {
                            float mass = phys.has("mass") ? phys.get("mass").getAsFloat() : 0.0f;
                            CollisionShape shape = (mass == 0.0f)
                                ? CollisionShapeFactory.createMeshShape(spatial)
                                : CollisionShapeFactory.createDynamicMeshShape(spatial);
                            RigidBodyControl rbc = new RigidBodyControl(shape, mass);
                            spatial.addControl(rbc);
                            bulletAppState.getPhysicsSpace().add(rbc);
                        }
                    }

                    sceneRoot.attachChild(spatial);
                }
            }
            System.out.println("[SceneLoader] Loaded successfully into jMonkeyEngine! Total spatials: " + sceneRoot.getQuantity());
        } catch (Exception e) {
            e.printStackTrace();
        }
        return sceneRoot;
    }
}
