#include "animation/AnimationLoader.h"

#include "vendor/ufbx/ufbx.h"

#include <glm/gtc/matrix_transform.hpp>

#include <glm/gtc/quaternion.hpp>

#include <iostream>

#include <filesystem>

#include <algorithm>

#include <queue>

#include <set>



static glm::mat4 toGlmMat4(const ufbx_matrix& m)

{

    return glm::mat4(

        static_cast<float>(m.cols[0].x), static_cast<float>(m.cols[0].y), static_cast<float>(m.cols[0].z), 0.0f,

        static_cast<float>(m.cols[1].x), static_cast<float>(m.cols[1].y), static_cast<float>(m.cols[1].z), 0.0f,

        static_cast<float>(m.cols[2].x), static_cast<float>(m.cols[2].y), static_cast<float>(m.cols[2].z), 0.0f,

        static_cast<float>(m.cols[3].x), static_cast<float>(m.cols[3].y), static_cast<float>(m.cols[3].z), 1.0f

    );

}



static glm::mat4 toGlmMat4(const ufbx_transform& t)

{

    glm::vec3 pos(static_cast<float>(t.translation.x), static_cast<float>(t.translation.y), static_cast<float>(t.translation.z));

    glm::quat rot(static_cast<float>(t.rotation.w), static_cast<float>(t.rotation.x), static_cast<float>(t.rotation.y), static_cast<float>(t.rotation.z));

    glm::vec3 scl(static_cast<float>(t.scale.x), static_cast<float>(t.scale.y), static_cast<float>(t.scale.z));



    glm::mat4 T = glm::translate(glm::mat4(1.0f), pos);

    glm::mat4 R = glm::mat4_cast(rot);

    glm::mat4 S = glm::scale(glm::mat4(1.0f), scl);

    return T * R * S;

}



static std::string resolveClipName(const ufbx_anim_stack* stack, const std::filesystem::path& fbxP, size_t stackIndex, size_t totalStacks)

{

    std::string sName = (stack && stack->name.data) ? stack->name.data : "";

    std::string lower = sName;

    for (char& c : lower) c = static_cast<char>(tolower(c));



    bool isGeneric = sName.empty() || sName == "Take 001" || sName == "default" ||

                     lower == "mixamo.com" || lower.find("mixamo") != std::string::npos ||

                     lower.rfind("armature", 0) == 0 || lower == "layer0" || lower == "base_layer" ||

                     lower == "anim" || lower == "scene" || lower == "action";



    if (isGeneric) {

        std::string stem = fbxP.stem().string();

        if (totalStacks > 1) {

            return stem + "_" + std::to_string(stackIndex + 1);

        }

        return stem;

    }

    return sName;

}



static std::string resolveFBXTexture(const std::string& texFile, const std::string& matName, const std::string& fbxDir, const std::string& fbxStem = "")

{

    std::vector<std::string> candidates;



    auto addCandidatePaths = [&](const std::string& relPath) {

        if (relPath.empty()) return;

        candidates.push_back(relPath);

        candidates.push_back(fbxDir + "/" + relPath);

        candidates.push_back(fbxDir + "/textures/" + relPath);

        if (!fbxStem.empty()) {

            candidates.push_back(fbxDir + "/" + fbxStem + ".fbm/" + relPath);

            candidates.push_back(fbxDir + "/" + fbxStem + "/" + relPath);

        }

        candidates.push_back("assets/textures/" + relPath);

        candidates.push_back("assets/models/" + relPath);

        candidates.push_back("assets/source/" + relPath);

        if (!fbxStem.empty()) {

            candidates.push_back("assets/source/" + fbxStem + ".fbm/" + relPath);

            candidates.push_back("assets/textures/" + fbxStem + ".fbm/" + relPath);

        }

        candidates.push_back("build/Release/" + relPath);

    };



    // 1. Direct path from FBX

    if (!texFile.empty()) {

        std::filesystem::path tp(texFile);

        std::string fname = tp.filename().string();

        addCandidatePaths(fname);

        if (fname != texFile) {

            addCandidatePaths(texFile);

        }

    }



    // 2. Strip suffixes like _Diffuse, _diffuse, _BaseColor, _Albedo, etc.

    std::string baseTex = std::filesystem::path(texFile).filename().string();

    size_t dotPos = baseTex.rfind('.');

    std::string ext = (dotPos != std::string::npos) ? baseTex.substr(dotPos) : ".png";

    std::string stem = (dotPos != std::string::npos) ? baseTex.substr(0, dotPos) : baseTex;



    const std::vector<std::string> suffixes = {

        "_Diffuse", "_diffuse", "_BaseColor", "_basecolor", "_Albedo", "_albedo",

        "_D", "_d", "_Col", "_col", "_Pbr_Diffuse", "_Pbr_diffuse"

    };



    if (!stem.empty()) {

        for (const auto& suf : suffixes) {

            if (stem.size() > suf.size() && stem.rfind(suf) == stem.size() - suf.size()) {

                std::string stripped = stem.substr(0, stem.size() - suf.size()) + ext;

                addCandidatePaths(stripped);

            }

        }

        // Variations with space vs underscore

        std::string underStem = stem;

        std::replace(underStem.begin(), underStem.end(), ' ', '_');

        if (underStem != stem) addCandidatePaths(underStem + ext);



        std::string spaceStem = stem;

        std::replace(spaceStem.begin(), spaceStem.end(), '_', ' ');

        if (spaceStem != stem) addCandidatePaths(spaceStem + ext);

    }



    // 3. By Material Name

    if (!matName.empty()) {

        for (const std::string& e : { ".png", ".jpg", ".tga", ".jpeg" }) {

            addCandidatePaths(matName + e);

            std::string underMat = matName;

            std::replace(underMat.begin(), underMat.end(), ' ', '_');

            if (underMat != matName) addCandidatePaths(underMat + e);

            std::string spaceMat = matName;

            std::replace(spaceMat.begin(), spaceMat.end(), '_', ' ');

            if (spaceMat != matName) addCandidatePaths(spaceMat + e);

        }

    }



    // 4. Special fallback for tm_professional (Mixamo classy shooter character)

    if (matName == "Material_002" || stem.find("Material_002") != std::string::npos) {

        addCandidatePaths("tm_professional_upperbody_var5.jpg");

        addCandidatePaths("tm_professional_upperbody_var1.jpg");

    }



    // Check all candidates

    for (const auto& c : candidates) {

        if (std::filesystem::exists(c)) return c;

    }



    // 5. Case-insensitive search in relevant directories

    std::vector<std::string> searchDirs = { "assets/textures", fbxDir };

    if (!fbxStem.empty()) {

        searchDirs.push_back(fbxDir + "/" + fbxStem + ".fbm");

        searchDirs.push_back("assets/source/" + fbxStem + ".fbm");

    }



    std::string lowerStem = stem;

    for (char& ch : lowerStem) ch = static_cast<char>(tolower(ch));



    for (const auto& dir : searchDirs) {

        if (!std::filesystem::exists(dir)) continue;

        for (const auto& entry : std::filesystem::directory_iterator(dir)) {

            if (!entry.is_regular_file()) continue;

            std::string fStem = entry.path().stem().string();

            std::string lowerFStem = fStem;

            for (char& ch : lowerFStem) ch = static_cast<char>(tolower(ch));

            if (!lowerStem.empty() && (lowerFStem == lowerStem || lowerFStem.find(lowerStem) != std::string::npos || lowerStem.find(lowerFStem) != std::string::npos)) {

                return entry.path().string();

            }

            if (!matName.empty()) {

                std::string lowerMat = matName;

                for (char& ch : lowerMat) ch = static_cast<char>(tolower(ch));

                if (lowerFStem == lowerMat || lowerFStem.find(lowerMat) != std::string::npos) {

                    return entry.path().string();

                }

            }

        }

    }



    return "";

}



bool AnimationLoader::loadFBXModel(const std::string& path, Mesh& outMesh,

                                   std::shared_ptr<Skeleton>& outSkeleton,

                                   std::vector<std::shared_ptr<AnimationClip>>& outClips,

                                   TextureLoaderCallback textureLoader,

                                   MemoryTextureLoaderCallback memTextureLoader)

{

    ufbx_load_opts opts = { 0 };

    opts.target_axes = ufbx_axes_right_handed_y_up;

    opts.target_unit_meters = 1.0f;

    opts.clean_skin_weights = true;

    opts.evaluate_skinning = false;

    opts.generate_missing_normals = true;



    ufbx_error error;

    ufbx_scene* scene = ufbx_load_file(path.c_str(), &opts, &error);

    if (!scene) {

        printf("[AnimationLoader] Failed to load FBX '%s': %s\n", path.c_str(), error.description.data);

        return false;

    }



    outMesh.sourcePath = path;

    outMesh.vertices.clear();

    outMesh.indices.clear();

    outMesh.submeshes.clear();



    // 1. Discover Bones & Build Skeleton

    std::vector<ufbx_node*> boneNodes;

    std::unordered_map<ufbx_node*, ufbx_skin_cluster*> nodeToCluster;



    for (size_t mi = 0; mi < scene->meshes.count; ++mi) {

        ufbx_mesh* m = scene->meshes.data[mi];

        for (size_t si = 0; si < m->skin_deformers.count; ++si) {

            ufbx_skin_deformer* skin = m->skin_deformers.data[si];

            for (size_t ci = 0; ci < skin->clusters.count; ++ci) {

                ufbx_skin_cluster* cluster = skin->clusters.data[ci];

                if (cluster->bone_node) {

                    nodeToCluster[cluster->bone_node] = cluster;

                }

            }

        }

    }



    // Identify all nodes that act as bones

    std::set<ufbx_node*> boneSet;

    for (auto& pair : nodeToCluster) {

        ufbx_node* curr = pair.first;

        while (curr && !curr->is_root) {

            boneSet.insert(curr);

            curr = curr->parent;

        }

    }



    // If no skin deformers found, check if nodes have bone attribute or animation

    if (boneSet.empty()) {

        for (size_t ni = 0; ni < scene->nodes.count; ++ni) {

            ufbx_node* node = scene->nodes.data[ni];

            if (node->is_root) continue;

            if (node->bone || node->attrib_type == UFBX_ELEMENT_BONE) {

                ufbx_node* curr = node;

                while (curr && !curr->is_root) {

                    boneSet.insert(curr);

                    curr = curr->parent;

                }

            }

        }

    }



    std::unordered_map<ufbx_node*, int> nodeToBoneIndex;

    if (!boneSet.empty()) {

        outSkeleton = std::make_shared<Skeleton>();



        // Breadth-First-Search from roots so parents are added before children

        std::queue<ufbx_node*> queue;

        for (ufbx_node* bNode : boneSet) {

            if (!bNode->parent || bNode->parent->is_root || boneSet.find(bNode->parent) == boneSet.end()) {

                queue.push(bNode);

            }

        }



        while (!queue.empty()) {

            ufbx_node* curr = queue.front();

            queue.pop();



            int parentIndex = -1;

            if (curr->parent && nodeToBoneIndex.find(curr->parent) != nodeToBoneIndex.end()) {

                parentIndex = nodeToBoneIndex[curr->parent];

            }



            glm::mat4 invBind = glm::mat4(1.0f);

            auto itCluster = nodeToCluster.find(curr);

            if (itCluster != nodeToCluster.end()) {

                invBind = toGlmMat4(itCluster->second->geometry_to_bone);

            } else {

                glm::mat4 nodeWorld = toGlmMat4(curr->node_to_world);

                invBind = glm::inverse(nodeWorld);

            }



            glm::mat4 localTrans = toGlmMat4(curr->local_transform);
            if (parentIndex < 0) {
                // Root bone: use node_to_world to ensure proper metric scale and target axes
                localTrans = toGlmMat4(curr->node_to_world);
            }

            std::string boneName = curr->name.data;



            int boneIdx = outSkeleton->addBone(boneName, parentIndex, invBind, localTrans);

            nodeToBoneIndex[curr] = boneIdx;



            for (size_t ch = 0; ch < curr->children.count; ++ch) {

                ufbx_node* child = curr->children.data[ch];

                if (boneSet.find(child) != boneSet.end()) {

                    queue.push(child);

                }

            }

        }

    }



    // 2. Extract Geometry (Vertices, Normals, UVs, Skin Weights)

    std::filesystem::path fbxP(path);

    std::string fbxDir = fbxP.parent_path().string();

    if (fbxDir.empty()) fbxDir = ".";

    std::string fbxStem = fbxP.stem().string();



    glm::vec3 minB(1e9f), maxB(-1e9f);



    // Helper lambda to resolve texture for a material (from embedded memory or external file)

    auto resolveMaterialTexture = [&](ufbx_material* mat) -> int {

        if (!mat) return -1;

        std::string matName = (mat->name.data) ? mat->name.data : "";

        ufbx_texture* tex = mat->pbr.base_color.texture ? mat->pbr.base_color.texture : mat->fbx.diffuse_color.texture;

        if (!tex && mat->textures.count > 0) {

            tex = mat->textures.data[0].texture;

        }



        // 1. Embedded texture in FBX

        if (tex && tex->content.size > 0 && memTextureLoader) {

            std::string texIdentifier = (tex->name.length > 0) ? tex->name.data : ("tex_" + std::to_string(tex->element_id));

            std::string cacheKey = path + "#embedded_" + matName + "_" + texIdentifier;

            int id = memTextureLoader(reinterpret_cast<const unsigned char*>(tex->content.data), tex->content.size, cacheKey);

            if (id >= 0) return id;

        }



        // 2. File texture resolution

        if (textureLoader) {

            std::string texFile = "";

            if (tex && tex->filename.length > 0) {

                texFile = tex->filename.data;

            } else if (tex && tex->relative_filename.length > 0) {

                texFile = tex->relative_filename.data;

            }



            std::string resolved = resolveFBXTexture(texFile, matName, fbxDir, fbxStem);

            if (!resolved.empty()) {

                return textureLoader(resolved);

            }

        }

        return -1;

    };



    for (size_t mi = 0; mi < scene->meshes.count; ++mi) {

        ufbx_mesh* mesh = scene->meshes.data[mi];

        if (mesh->num_faces == 0) continue;



        ufbx_skin_deformer* skin = (mesh->skin_deformers.count > 0) ? mesh->skin_deformers.data[0] : nullptr;



        // Map cluster index to skeleton bone index

        std::vector<int> clusterToBoneIndex;

        if (skin && outSkeleton) {

            clusterToBoneIndex.resize(skin->clusters.count, -1);

            for (size_t ci = 0; ci < skin->clusters.count; ++ci) {

                ufbx_skin_cluster* cl = skin->clusters.data[ci];

                if (cl->bone_node && nodeToBoneIndex.find(cl->bone_node) != nodeToBoneIndex.end()) {

                    clusterToBoneIndex[ci] = nodeToBoneIndex[cl->bone_node];

                }

            }

        }



        std::vector<uint32_t> triIndices(mesh->max_face_triangles * 3);



        bool isSkinnedMesh = (outSkeleton && outSkeleton->getBoneCount() > 0);

        ufbx_matrix rotGeomToWorld{};

        bool hasRotGeom = false;

        if (!isSkinnedMesh && mesh->instances.count > 0) {

            const ufbx_matrix* m = &mesh->instances.data[0]->geometry_to_world;

            rotGeomToWorld = *m;

            // Normalize axes to strip non-uniform or accidental DCC node scaling, keeping 1:1 metric scale

            for (int c = 0; c < 3; ++c) {

                ufbx_vec3 col = rotGeomToWorld.cols[c];

                double len = std::sqrt(col.x * col.x + col.y * col.y + col.z * col.z);

                if (len > 1e-6) {

                    rotGeomToWorld.cols[c].x /= len;

                    rotGeomToWorld.cols[c].y /= len;

                    rotGeomToWorld.cols[c].z /= len;

                }

            }

            rotGeomToWorld.cols[3].x = 0;

            rotGeomToWorld.cols[3].y = 0;

            rotGeomToWorld.cols[3].z = 0;

            hasRotGeom = true;

        }



        // Process mesh parts (materials)

        size_t numParts = mesh->material_parts.count > 0 ? mesh->material_parts.count : 1;

        for (size_t pi = 0; pi < numParts; ++pi) {

            SubMesh submesh;

            submesh.indexOffset = static_cast<uint32_t>(outMesh.indices.size());

            submesh.roughness = 0.5f;

            submesh.metallic = 0.0f;

            submesh.textureId = -1;



            // Resolve material texture

            if (mesh->material_parts.count > 0) {

                ufbx_mesh_part part = mesh->material_parts.data[pi];

                if (part.num_faces == 0) continue;



                uint32_t matIdx = part.index;

                if (mesh->face_material.data && part.num_faces > 0) {

                    uint32_t firstFace = part.face_indices.data[0];

                    if (firstFace < mesh->face_material.count) {

                        matIdx = mesh->face_material.data[firstFace];

                    }

                }

                ufbx_material* mat = (matIdx < mesh->materials.count) ? mesh->materials.data[matIdx] : nullptr;

                std::string matName = (mat && mat->name.data) ? mat->name.data : "";

                std::string lowerMat = matName;

                for (char& c : lowerMat) c = static_cast<char>(tolower(c));



                if (mat) {

                    if (mat->pbr.roughness.has_value) submesh.roughness = static_cast<float>(mat->pbr.roughness.value_real);

                    if (mat->pbr.metalness.has_value) submesh.metallic = static_cast<float>(mat->pbr.metalness.value_real);

                }



                glm::vec3 defaultVertexColor(1.0f);

                if (mat) {
                    if (mat->pbr.base_color.has_value) {
                        defaultVertexColor = glm::vec3(
                            static_cast<float>(mat->pbr.base_color.value_vec4.x),
                            static_cast<float>(mat->pbr.base_color.value_vec4.y),
                            static_cast<float>(mat->pbr.base_color.value_vec4.z)
                        );
                    } else if (mat->fbx.diffuse_color.has_value) {
                        defaultVertexColor = glm::vec3(
                            static_cast<float>(mat->fbx.diffuse_color.value_vec3.x),
                            static_cast<float>(mat->fbx.diffuse_color.value_vec3.y),
                            static_cast<float>(mat->fbx.diffuse_color.value_vec3.z)
                        );
                    }
                }

                if (lowerMat.find("glove") != std::string::npos) {

                    defaultVertexColor = glm::vec3(0.12f, 0.12f, 0.14f);

                    submesh.roughness = 0.65f;

                } else if (lowerMat.find("skin") != std::string::npos) {

                    defaultVertexColor = glm::vec3(0.65f, 0.45f, 0.35f);

                    submesh.roughness = 0.50f;

                }



                if (mat) {

                    submesh.textureId = resolveMaterialTexture(mat);

                }

                glm::vec3 finalVertexColor = (submesh.textureId >= 0) ? glm::vec3(1.0f) : defaultVertexColor;



                for (size_t pfi = 0; pfi < part.num_faces; ++pfi) {

                    uint32_t faceIdx = part.face_indices.data[pfi];

                    ufbx_face face = mesh->faces.data[faceIdx];

                    uint32_t numTris = ufbx_triangulate_face(triIndices.data(), triIndices.size(), mesh, face);



                    for (uint32_t vi = 0; vi < numTris * 3; ++vi) {

                        uint32_t meshIdx = triIndices[vi];

                        Vertex v{};



                        ufbx_vec3 pos = ufbx_get_vertex_vec3(&mesh->vertex_position, meshIdx);

                        if (hasRotGeom) pos = ufbx_transform_position(&rotGeomToWorld, pos);

                        v.pos = glm::vec3(static_cast<float>(pos.x), static_cast<float>(pos.y), static_cast<float>(pos.z));

                        v.color = finalVertexColor;



                        minB = glm::min(minB, v.pos);

                        maxB = glm::max(maxB, v.pos);



                        if (mesh->vertex_normal.exists) {

                            ufbx_vec3 n = ufbx_get_vertex_vec3(&mesh->vertex_normal, meshIdx);

                            if (hasRotGeom) n = ufbx_transform_direction(&rotGeomToWorld, n);

                            v.normal = glm::vec3(static_cast<float>(n.x), static_cast<float>(n.y), static_cast<float>(n.z));

                        } else {

                            v.normal = glm::vec3(0.0f, 1.0f, 0.0f);

                        }



                        if (mesh->vertex_uv.exists) {

                            ufbx_vec2 uv = ufbx_get_vertex_vec2(&mesh->vertex_uv, meshIdx);

                            v.texCoord = glm::vec2(static_cast<float>(uv.x), 1.0f - static_cast<float>(uv.y));

                        } else {

                            v.texCoord = glm::vec2(0.0f);

                        }



                        // Skin weights

                        if (skin && outSkeleton) {

                            uint32_t logicalV = mesh->vertex_indices.data[meshIdx];

                            if (logicalV < skin->vertices.count) {

                                ufbx_skin_vertex svert = skin->vertices.data[logicalV];

                                float totalWeight = 0.0f;

                                uint32_t count = std::min(svert.num_weights, 4u);

                                for (uint32_t w = 0; w < count; ++w) {

                                    ufbx_skin_weight sw = skin->weights.data[svert.weight_begin + w];

                                    if (sw.cluster_index < clusterToBoneIndex.size()) {

                                        int bIdx = clusterToBoneIndex[sw.cluster_index];

                                        if (bIdx >= 0) {

                                            v.boneIDs[w] = bIdx;

                                            v.weights[w] = static_cast<float>(sw.weight);

                                            totalWeight += v.weights[w];

                                        }

                                    }

                                }

                                if (totalWeight > 1e-4f) {

                                    v.weights /= totalWeight;

                                }

                            }

                        }



                        uint32_t idx = static_cast<uint32_t>(outMesh.vertices.size());

                        outMesh.vertices.push_back(v);

                        outMesh.indices.push_back(idx);

                    }

                }

            } else {

                // Entire mesh without material parts

                ufbx_material* mat = (mesh->materials.count > 0) ? mesh->materials.data[0] : nullptr;

                if (mat) {

                    if (mat->pbr.roughness.has_value) submesh.roughness = static_cast<float>(mat->pbr.roughness.value_real);

                    if (mat->pbr.metalness.has_value) submesh.metallic = static_cast<float>(mat->pbr.metalness.value_real);

                    submesh.textureId = resolveMaterialTexture(mat);

                }



                for (size_t fi = 0; fi < mesh->faces.count; ++fi) {

                    ufbx_face face = mesh->faces.data[fi];

                    uint32_t numTris = ufbx_triangulate_face(triIndices.data(), triIndices.size(), mesh, face);



                    for (uint32_t vi = 0; vi < numTris * 3; ++vi) {

                        uint32_t meshIdx = triIndices[vi];

                        Vertex v{};

                        ufbx_vec3 pos = ufbx_get_vertex_vec3(&mesh->vertex_position, meshIdx);

                        if (hasRotGeom) pos = ufbx_transform_position(&rotGeomToWorld, pos);

                        v.pos = glm::vec3(static_cast<float>(pos.x), static_cast<float>(pos.y), static_cast<float>(pos.z));

                        glm::vec3 matColor(1.0f);

                        if (mat) {

                            ufbx_vec3 c = mat->pbr.base_color.value_vec3;

                            if (c.x > 0.001f || c.y > 0.001f || c.z > 0.001f) {

                                matColor = glm::vec3(static_cast<float>(c.x), static_cast<float>(c.y), static_cast<float>(c.z));

                            } else {

                                ufbx_vec3 dc = mat->fbx.diffuse_color.value_vec3;

                                if (dc.x > 0.001f || dc.y > 0.001f || dc.z > 0.001f) {

                                    matColor = glm::vec3(static_cast<float>(dc.x), static_cast<float>(dc.y), static_cast<float>(dc.z));

                                }

                            }

                        }

                        v.color = matColor;



                        minB = glm::min(minB, v.pos);

                        maxB = glm::max(maxB, v.pos);



                        if (mesh->vertex_normal.exists) {

                            ufbx_vec3 n = ufbx_get_vertex_vec3(&mesh->vertex_normal, meshIdx);

                            if (hasRotGeom) n = ufbx_transform_direction(&rotGeomToWorld, n);

                            v.normal = glm::vec3(static_cast<float>(n.x), static_cast<float>(n.y), static_cast<float>(n.z));

                        } else {

                            v.normal = glm::vec3(0.0f, 1.0f, 0.0f);

                        }



                        if (mesh->vertex_uv.exists) {

                            ufbx_vec2 uv = ufbx_get_vertex_vec2(&mesh->vertex_uv, meshIdx);

                            v.texCoord = glm::vec2(static_cast<float>(uv.x), 1.0f - static_cast<float>(uv.y));

                        } else {

                            v.texCoord = glm::vec2(0.0f);

                        }



                        if (skin && outSkeleton) {

                            uint32_t logicalV = mesh->vertex_indices.data[meshIdx];

                            if (logicalV < skin->vertices.count) {

                                ufbx_skin_vertex svert = skin->vertices.data[logicalV];

                                float totalWeight = 0.0f;

                                uint32_t count = std::min(svert.num_weights, 4u);

                                for (uint32_t w = 0; w < count; ++w) {

                                    ufbx_skin_weight sw = skin->weights.data[svert.weight_begin + w];

                                    if (sw.cluster_index < clusterToBoneIndex.size()) {

                                        int bIdx = clusterToBoneIndex[sw.cluster_index];

                                        if (bIdx >= 0) {

                                            v.boneIDs[w] = bIdx;

                                            v.weights[w] = static_cast<float>(sw.weight);

                                            totalWeight += v.weights[w];

                                        }

                                    }

                                }

                                if (totalWeight > 1e-4f) v.weights /= totalWeight;

                            }

                        }



                        uint32_t idx = static_cast<uint32_t>(outMesh.vertices.size());

                        outMesh.vertices.push_back(v);

                        outMesh.indices.push_back(idx);

                    }

                }

            }



            submesh.indexCount = static_cast<uint32_t>(outMesh.indices.size()) - submesh.indexOffset;

            if (submesh.indexCount > 0) {

                outMesh.submeshes.push_back(submesh);

            }

        }

    }



    // Ground and center static meshes (e.g. ShootingTarget prop)
    // IMPORTANT: Weapons and equipable socket props (e.g. AK47_GameReady, pistols, knives)
    // MUST PRESERVE their native artist-designed pivot (trigger/receiver tang) at (0,0,0).
    // Never auto-ground or auto-center weapons!
    bool isSkinnedMesh = (outSkeleton && outSkeleton->getBoneCount() > 0);

    std::string lowerStem = fbxStem;
    for (char& c : lowerStem) c = static_cast<char>(tolower(c));
    std::string lowerPath = path;
    for (char& c : lowerPath) c = static_cast<char>(tolower(c));

    bool isWeaponOrSocketProp = (
        lowerStem.find("ak47") != std::string::npos || lowerStem.find("ak_") != std::string::npos ||
        lowerStem.find("rifle") != std::string::npos || lowerStem.find("weapon") != std::string::npos ||
        lowerStem.find("gun") != std::string::npos || lowerStem.find("pistol") != std::string::npos ||
        lowerStem.find("knife") != std::string::npos || lowerStem.find("sword") != std::string::npos ||
        lowerStem.find("magazine") != std::string::npos || lowerStem.find("scope") != std::string::npos ||
        lowerPath.find("weapon") != std::string::npos || lowerPath.find("gun") != std::string::npos ||
        lowerPath.find("rifle") != std::string::npos || lowerPath.find("ak47") != std::string::npos
    );

    if (!isSkinnedMesh && !isWeaponOrSocketProp && !outMesh.vertices.empty()) {

        float minY = minB.y;

        float centerOffX = (minB.x + maxB.x) * 0.5f;

        float centerOffZ = (minB.z + maxB.z) * 0.5f;

        for (auto& v : outMesh.vertices) {

            v.pos.x -= centerOffX;

            v.pos.y -= minY;

            v.pos.z -= centerOffZ;

        }

        minB.x -= centerOffX;

        maxB.x -= centerOffX;

        minB.y -= minY;

        maxB.y -= minY;

        minB.z -= centerOffZ;

        maxB.z -= centerOffZ;

    }



    // Determine automatic metric scale factor

    float scaleFactor = 1.0f;

    glm::vec3 rawExtent = maxB - minB;

    float maxDim = std::max({rawExtent.x, rawExtent.y, rawExtent.z});



    if (outSkeleton && outSkeleton->getBoneCount() > 0) {

        // Skinned character: standard human height is ~1.80m

        // If coordinate space is non-metric (e.g. Maya units ~7744, cm ~180, mm ~1800)

        if (maxDim > 5.0f || (maxDim > 0.0f && maxDim < 0.2f)) {

            scaleFactor = 1.80f / maxDim;

        }

    } else {

        // Static mesh / props (e.g. ShootingTarget is 1.75m, leave unchanged if <= 15m)

        if (maxDim > 2000.0f) {

            scaleFactor = 0.001f; // millimeters to meters

        } else if (maxDim > 100.0f) {

            scaleFactor = 0.01f;  // centimeters to meters

        }

    }



    if (scaleFactor != 1.0f) {

        for (auto& v : outMesh.vertices) {

            v.pos *= scaleFactor;

        }

        minB *= scaleFactor;

        maxB *= scaleFactor;

    }



    outMesh.indexCount = static_cast<uint32_t>(outMesh.indices.size());

    outMesh.originalMinB = minB;

    outMesh.originalMaxB = maxB;



    for (const auto& sm : outMesh.submeshes) {

        if (sm.textureId >= 0) {

            outMesh.defaultTextureId = sm.textureId;

            break;

        }

    }



    if (outMesh.defaultTextureId < 0 && textureLoader) {

        std::filesystem::path fbxPObj(fbxP);

        std::string stem = fbxPObj.stem().string();

        std::string parentDir = fbxPObj.parent_path().string();

        std::vector<std::string> compCands = {

            parentDir + "/" + stem + ".png",

            parentDir + "/" + stem + ".jpg",

            "assets/models/" + stem + ".png",

            "assets/models/" + stem + ".jpg",

            "assets/textures/" + stem + ".png",

            "assets/textures/" + stem + ".jpg"

        };

        for (const auto& cand : compCands) {

            if (std::filesystem::exists(cand)) {

                int texId = textureLoader(cand);

                if (texId >= 0) {

                    outMesh.defaultTextureId = texId;

                    for (auto& sm : outMesh.submeshes) {

                        if (sm.textureId < 0) sm.textureId = texId;

                    }

                    break;

                }

            }

        }

    }



    if (outSkeleton) {

        outSkeleton->scaleFactor = scaleFactor;

        if (scaleFactor != 1.0f) {

            for (auto& bone : outSkeleton->bones) {

                bone.localTransform[3] = glm::vec4(glm::vec3(bone.localTransform[3]) * scaleFactor, 1.0f);

                bone.inverseBindMatrix[3] = glm::vec4(glm::vec3(bone.inverseBindMatrix[3]) * scaleFactor, 1.0f);

            }

        }

    }



    // 3. Extract Baked Animations

    for (size_t ai = 0; ai < scene->anim_stacks.count; ++ai) {

        ufbx_anim_stack* stack = scene->anim_stacks.data[ai];

        ufbx_bake_opts bake_opts = { 0 };

        ufbx_error bake_err;

        ufbx_baked_anim* bake = ufbx_bake_anim(scene, stack->anim, &bake_opts, &bake_err);

        if (!bake) continue;



        auto clip = std::make_shared<AnimationClip>();

        clip->name = resolveClipName(stack, fbxP, ai, scene->anim_stacks.count);

        clip->duration = static_cast<float>(stack->time_end - stack->time_begin);

        if (clip->duration <= 0.0f) clip->duration = 1.0f;



        for (size_t ni = 0; ni < bake->nodes.count; ++ni) {

            ufbx_baked_node bnode = bake->nodes.data[ni];

            ufbx_node* sceneNode = scene->nodes.data[bnode.typed_id];

            if (!sceneNode) continue;



            BoneTrack track;

            track.boneName = sceneNode->name.data;



            // Translation keys

            for (size_t k = 0; k < bnode.translation_keys.count; ++k) {

                KeyframeVec3 kv;

                kv.time = static_cast<float>(bnode.translation_keys.data[k].time - stack->time_begin);

                ufbx_vec3 val = bnode.translation_keys.data[k].value;

                kv.value = glm::vec3(static_cast<float>(val.x), static_cast<float>(val.y), static_cast<float>(val.z)) * scaleFactor;

                track.positions.push_back(kv);

            }



            // Rotation keys

            for (size_t k = 0; k < bnode.rotation_keys.count; ++k) {

                KeyframeQuat kq;

                kq.time = static_cast<float>(bnode.rotation_keys.data[k].time - stack->time_begin);

                ufbx_quat q = bnode.rotation_keys.data[k].value;

                kq.value = glm::quat(static_cast<float>(q.w), static_cast<float>(q.x), static_cast<float>(q.y), static_cast<float>(q.z));

                track.rotations.push_back(kq);

            }



            // Scale keys

            for (size_t k = 0; k < bnode.scale_keys.count; ++k) {

                KeyframeVec3 kv;

                kv.time = static_cast<float>(bnode.scale_keys.data[k].time - stack->time_begin);

                ufbx_vec3 val = bnode.scale_keys.data[k].value;

                kv.value = glm::vec3(static_cast<float>(val.x), static_cast<float>(val.y), static_cast<float>(val.z));

                track.scales.push_back(kv);

            }



            if (!track.positions.empty() || !track.rotations.empty() || !track.scales.empty()) {

                clip->addTrack(track);

            }

        }



        outClips.push_back(clip);

        ufbx_free_baked_anim(bake);

    }



    ufbx_free_scene(scene);

    printf("[AnimationLoader] Loaded FBX '%s': %zu vertices, %zu indices, %zu bones, %zu animation clips (scaleFactor=%.6f)\n",

           path.c_str(), outMesh.vertices.size(), outMesh.indices.size(),

           outSkeleton ? outSkeleton->getBoneCount() : 0, outClips.size(), scaleFactor);

    return true;

}



bool AnimationLoader::loadFBXAnimations(const std::string& path,

                                       std::vector<std::shared_ptr<AnimationClip>>& outClips,

                                       const Skeleton* targetSkeleton)

{

    ufbx_load_opts opts = { 0 };

    opts.target_axes = ufbx_axes_right_handed_y_up;

    opts.target_unit_meters = 1.0f;

    opts.evaluate_skinning = false;



    ufbx_error error;

    ufbx_scene* scene = ufbx_load_file(path.c_str(), &opts, &error);

    if (!scene) {

        printf("[AnimationLoader] Failed to load animation FBX '%s': %s\n", path.c_str(), error.description.data);

        return false;

    }



    std::filesystem::path fbxP(path);

    float clipScale = (targetSkeleton && targetSkeleton->scaleFactor > 0.0f) ? targetSkeleton->scaleFactor : 1.0f;



    for (size_t ai = 0; ai < scene->anim_stacks.count; ++ai) {

        ufbx_anim_stack* stack = scene->anim_stacks.data[ai];

        ufbx_bake_opts bake_opts = { 0 };

        ufbx_error bake_err;

        ufbx_baked_anim* bake = ufbx_bake_anim(scene, stack->anim, &bake_opts, &bake_err);

        if (!bake) continue;



        // Auto-detect animation translation scale across all baked nodes
        float maxTransLen = 0.0f;
        for (size_t ni = 0; ni < bake->nodes.count; ++ni) {
            ufbx_baked_node bnode = bake->nodes.data[ni];
            for (size_t k = 0; k < bnode.translation_keys.count; ++k) {
                ufbx_vec3 val = bnode.translation_keys.data[k].value;
                float len = static_cast<float>(std::sqrt(val.x * val.x + val.y * val.y + val.z * val.z));
                if (len > maxTransLen) maxTransLen = len;
            }
        }

        // 1. If targetSkeleton is provided and has a calibrated scaleFactor (e.g. CS:GO / CC character: 0.0002324f),
        // PRESERVE IT 100%! Never overwrite it with rough heuristic guesses!
        float effectiveScale = clipScale;
        if (targetSkeleton && targetSkeleton->scaleFactor > 0.0f && std::abs(targetSkeleton->scaleFactor - 1.0f) > 1e-5f) {
            effectiveScale = targetSkeleton->scaleFactor;
        } else {
            // Heuristic detection when importing standalone animation clips without a target skeleton
            bool hasCSGOBones = false;
            for (size_t ni = 0; ni < bake->nodes.count; ++ni) {
                ufbx_baked_node bnode = bake->nodes.data[ni];
                ufbx_node* sceneNode = scene->nodes.data[bnode.typed_id];
                if (sceneNode && sceneNode->name.data) {
                    std::string bname = sceneNode->name.data;
                    if (bname.find("CC_Base_") != std::string::npos || bname.find("RL_BoneRoot") != std::string::npos ||
                        bname.find("ValveBiped") != std::string::npos) {
                        hasCSGOBones = true;
                        break;
                    }
                }
            }

            if (hasCSGOBones || maxTransLen > 400.0f) {
                // CS:GO / Source Engine / Reallusion CC raw units (7744.14 units ~ 1.80m)
                effectiveScale = 1.80f / 7744.14f;
            } else if (maxTransLen > 50.0f) {
                // Standard centimeters to meters (Mixamo cm)
                effectiveScale = 0.01f;
            } else if (maxTransLen > 5.0f && maxTransLen <= 50.0f) {
                // Decimeters to meters
                effectiveScale = 0.1f;
            } else if (effectiveScale == 1.0f && scene->settings.unit_meters > 0.0f && scene->settings.unit_meters < 0.9f) {
                effectiveScale = static_cast<float>(scene->settings.unit_meters);
            }
        }

        auto clip = std::make_shared<AnimationClip>();

        clip->name = resolveClipName(stack, fbxP, ai, scene->anim_stacks.count);

        clip->duration = static_cast<float>(stack->time_end - stack->time_begin);

        if (clip->duration <= 0.0f) clip->duration = 1.0f;



        for (size_t ni = 0; ni < bake->nodes.count; ++ni) {

            ufbx_baked_node bnode = bake->nodes.data[ni];

            ufbx_node* sceneNode = scene->nodes.data[bnode.typed_id];

            if (!sceneNode) continue;



            BoneTrack track;

            track.boneName = sceneNode->name.data;



            for (size_t k = 0; k < bnode.translation_keys.count; ++k) {

                KeyframeVec3 kv;

                kv.time = static_cast<float>(bnode.translation_keys.data[k].time - stack->time_begin);

                ufbx_vec3 val = bnode.translation_keys.data[k].value;

                kv.value = glm::vec3(static_cast<float>(val.x), static_cast<float>(val.y), static_cast<float>(val.z)) * effectiveScale;

                track.positions.push_back(kv);

            }



            for (size_t k = 0; k < bnode.rotation_keys.count; ++k) {

                KeyframeQuat kq;

                kq.time = static_cast<float>(bnode.rotation_keys.data[k].time - stack->time_begin);

                ufbx_quat q = bnode.rotation_keys.data[k].value;

                kq.value = glm::quat(static_cast<float>(q.w), static_cast<float>(q.x), static_cast<float>(q.y), static_cast<float>(q.z));

                track.rotations.push_back(kq);

            }



            for (size_t k = 0; k < bnode.scale_keys.count; ++k) {

                KeyframeVec3 kv;

                kv.time = static_cast<float>(bnode.scale_keys.data[k].time - stack->time_begin);

                ufbx_vec3 val = bnode.scale_keys.data[k].value;

                kv.value = glm::vec3(static_cast<float>(val.x), static_cast<float>(val.y), static_cast<float>(val.z));

                track.scales.push_back(kv);

            }



            if (!track.positions.empty() || !track.rotations.empty() || !track.scales.empty()) {

                clip->addTrack(track);

            }

        }



        outClips.push_back(clip);

        ufbx_free_baked_anim(bake);

    }



    ufbx_free_scene(scene);

    printf("[AnimationLoader] Extracted %zu animation clips from '%s' (scale=%.6f)\n", outClips.size(), path.c_str(), clipScale);

    return !outClips.empty();

}



std::shared_ptr<AnimationClip> AnimationLoader::loadFBXClip(const std::string& path, Skeleton* targetSkeleton)

{

    std::vector<std::shared_ptr<AnimationClip>> clips;

    if (loadFBXAnimations(path, clips, targetSkeleton) && !clips.empty()) {

        return clips[0];

    }

    return nullptr;

}



bool AnimationLoader::isAnimationOnlyFBX(const std::string& path)

{

    ufbx_load_opts opts = { 0 };

    opts.ignore_geometry = true;

    opts.evaluate_skinning = false;

    opts.evaluate_caches = false;

    opts.ignore_animation = false;

    opts.ignore_embedded = true;



    ufbx_error error;

    ufbx_scene* scene = ufbx_load_file(path.c_str(), &opts, &error);

    if (!scene) return false;



    bool animOnly = (scene->meshes.count == 0 && scene->anim_stacks.count > 0);

    ufbx_free_scene(scene);

    return animOnly;

}

