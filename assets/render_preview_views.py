# Render views using Blender internal Python
import bpy
import os
import math
from mathutils import Vector

blend_path = r"D:\Game_prj\game_engine\assets\models\Target_Vinashoot_Swivel_Fold.blend"
out_dir = r"D:\Game_prj\game_engine\assets"

bpy.ops.wm.open_mainfile(filepath=blend_path)

scene = bpy.context.scene
scene.render.engine = 'BLENDER_EEVEE'
scene.render.resolution_x = 800
scene.render.resolution_y = 800
scene.render.film_transparent = True

# World lighting
world = scene.world
if not world:
    world = bpy.data.worlds.new("World")
    scene.world = world
world.use_nodes = True
bg = world.node_tree.nodes.get("Background")
if bg:
    bg.inputs[0].default_value = (0.15, 0.16, 0.18, 1.0)
    bg.inputs[1].default_value = 1.2

# Remove existing lights/cameras
for obj in list(scene.objects):
    if obj.type in ('LIGHT', 'CAMERA'):
        bpy.data.objects.remove(obj, do_unlink=True)

# Add 3-point lighting + ambient
def add_light(name, light_type, pos, energy, color=(1,1,1)):
    l_data = bpy.data.lights.new(name, light_type)
    l_data.energy = energy
    l_data.color = color
    l_obj = bpy.data.objects.new(name, l_data)
    l_obj.location = pos
    scene.collection.objects.link(l_obj)
    return l_obj

add_light("KeyLight", 'SUN', (3.0, -4.0, 5.0), 3.0)
add_light("FillLight", 'SUN', (-4.0, 3.0, 4.0), 1.5)
add_light("RimLight", 'SUN', (0.0, 4.0, 3.0), 2.0)
add_light("BottomFill", 'POINT', (0.0, -1.0, 0.2), 50.0)

# Create camera
cam_data = bpy.data.cameras.new("RenderCam")
cam_obj = bpy.data.objects.new("RenderCam", cam_data)
scene.collection.objects.link(cam_obj)
scene.camera = cam_obj

def point_at(cam, target):
    direction = target - cam.location
    rot_quat = direction.to_track_quat('-Z', 'Y')
    cam.rotation_euler = rot_quat.to_euler()

# Target center is roughly (0, 0, 0.53)
target_center = Vector((0.0, 0.0, 0.53))

views = {
    "preview_front.png": {
        "ortho": True,
        "ortho_scale": 1.25,
        "loc": Vector((0.0, -2.5, 0.53)),
    },
    "preview_back.png": {
        "ortho": True,
        "ortho_scale": 1.25,
        "loc": Vector((0.0, 2.5, 0.53)),
    },
    "preview_side.png": {
        "ortho": True,
        "ortho_scale": 1.25,
        "loc": Vector((-2.5, 0.0, 0.53)),
    },
    "preview_top.png": {
        "ortho": True,
        "ortho_scale": 0.9,
        "loc": Vector((0.0, 0.0, 2.5)),
    },
    "preview_iso.png": {
        "ortho": False,
        "lens": 50,
        "loc": Vector((1.6, -1.8, 1.2)),
    }
}

rendered_paths = {}

for filename, cfg in views.items():
    cam_obj.location = cfg["loc"]
    if cfg.get("ortho", False):
        cam_data.type = 'ORTHO'
        cam_data.ortho_scale = cfg["ortho_scale"]
    else:
        cam_data.type = 'PERSP'
        cam_data.lens = cfg.get("lens", 50)
    
    if filename == "preview_top.png":
        cam_obj.rotation_euler = (0, 0, -math.pi / 2)
    else:
        point_at(cam_obj, target_center)
    
    out_path = os.path.join(out_dir, filename)
    scene.render.filepath = out_path
    bpy.ops.render.render(write_still=True)
    rendered_paths[filename] = out_path
    print(f"Rendered {filename} -> {out_path}")

