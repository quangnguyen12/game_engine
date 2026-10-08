#include "assets/ModelLoader.h"
#include "assets/AssetManager.h"

#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stb_image.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include <tiny_gltf.h>

#define TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_DONT_INCLUDE_FAST_FLOAT
#include "vendor/tiny_obj_loader.h"

#include <iostream>
#include <filesystem>
#include <algorithm>
#include <functional>
#include <map>
#include <cmath>
#include <glm/gtc/quaternion.hpp>

void ModelLoader::loadModel(const std::string& path, Mesh& mesh, AssetManager* assetManager)
{
    mesh.vertices.clear();
    mesh.indices.clear();
    mesh.submeshes.clear();
    mesh.isFoliage = false;
    mesh.defaultTextureId = -1;
    mesh.defaultRoughness = 0.5f;
    mesh.defaultMetallic = 0.0f;

    std::string ext = path.substr(path.find_last_of('.') + 1);
    for (auto& c : ext) c = static_cast<char>(tolower(c));

    if (ext == "glb" || ext == "gltf") {
        tinygltf::Model gltfModel;
        tinygltf::TinyGLTF loader;
        std::string err, warn;
        bool ret = false;
        
        loader.SetImageLoader([](tinygltf::Image* image, const int, std::string* err, std::string*, int, int, const unsigned char* bytes, int size, void*) -> bool {
            int w = 0, h = 0, comp = 0;
            unsigned char* data = stbi_load_from_memory(bytes, size, &w, &h, &comp, 4);
            if (!data) {
                if (err) *err = "Failed to decode glTF image with stb_image";
                return false;
            }
            image->width = w;
            image->height = h;
            image->component = 4;
            image->image.assign(data, data + (w * h * 4));
            stbi_image_free(data);
            return true;
        }, nullptr);
        
        if (ext == "glb") {
            ret = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, path);
        } else {
            ret = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, path);
        }
        
        if (!warn.empty()) printf("GLTF Warn: %s\n", warn.c_str());
        if (!err.empty()) printf("GLTF Err: %s\n", err.c_str());
        if (!ret) throw std::runtime_error("Failed to parse glTF: " + path);
        
        const tinygltf::Scene& scene = gltfModel.scenes[gltfModel.defaultScene > -1 ? gltfModel.defaultScene : 0];
        
        std::function<void(int, const glm::mat4&)> processNode = [&](int nodeIndex, const glm::mat4& parentMatrix) {
            const tinygltf::Node& node = gltfModel.nodes[nodeIndex];
            glm::mat4 matrix = parentMatrix;
            
            if (node.matrix.size() == 16) {
                glm::mat4 localMat;
                for (int i = 0; i < 16; ++i) localMat[i / 4][i % 4] = static_cast<float>(node.matrix[i]);
                matrix = matrix * localMat;
            } else {
                glm::mat4 t(1.0f), r(1.0f), s(1.0f);
                if (node.translation.size() == 3) t = glm::translate(glm::mat4(1.0f), glm::vec3((float)node.translation[0], (float)node.translation[1], (float)node.translation[2]));
                if (node.rotation.size() == 4) {
                    glm::quat q((float)node.rotation[3], (float)node.rotation[0], (float)node.rotation[1], (float)node.rotation[2]);
                    r = glm::mat4_cast(q);
                }
                if (node.scale.size() == 3) s = glm::scale(glm::mat4(1.0f), glm::vec3((float)node.scale[0], (float)node.scale[1], (float)node.scale[2]));
                matrix = matrix * t * r * s;
            }
            
            if (node.mesh > -1) {
                const tinygltf::Mesh& gmesh = gltfModel.meshes[node.mesh];
                for (size_t i = 0; i < gmesh.primitives.size(); ++i) {
                    const tinygltf::Primitive& primitive = gmesh.primitives[i];
                    
                    uint32_t vertexStart = static_cast<uint32_t>(mesh.vertices.size());
                    
                    const unsigned char* positionDataBytes = nullptr;
                    size_t vertexCount = 0;
                    size_t posStride = sizeof(float) * 3;
                    if (primitive.attributes.find("POSITION") != primitive.attributes.end()) {
                        const tinygltf::Accessor& accessor = gltfModel.accessors[primitive.attributes.find("POSITION")->second];
                        const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
                        const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];
                        positionDataBytes = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
                        vertexCount = accessor.count;
                        int stride = accessor.ByteStride(bufferView);
                        posStride = (stride > 0) ? static_cast<size_t>(stride) : sizeof(float) * 3;
                    }
                    
                    const unsigned char* normalDataBytes = nullptr;
                    size_t normStride = sizeof(float) * 3;
                    if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
                        const tinygltf::Accessor& accessor = gltfModel.accessors[primitive.attributes.find("NORMAL")->second];
                        const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
                        const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];
                        normalDataBytes = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
                        int stride = accessor.ByteStride(bufferView);
                        normStride = (stride > 0) ? static_cast<size_t>(stride) : sizeof(float) * 3;
                    }
                    
                    const unsigned char* texcoordDataBytes = nullptr;
                    size_t texStride = sizeof(float) * 2;
                    if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
                        const tinygltf::Accessor& accessor = gltfModel.accessors[primitive.attributes.find("TEXCOORD_0")->second];
                        const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
                        const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];
                        texcoordDataBytes = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
                        int stride = accessor.ByteStride(bufferView);
                        texStride = (stride > 0) ? static_cast<size_t>(stride) : sizeof(float) * 2;
                    }
                    
                    glm::mat3 m3 = glm::mat3(matrix);
                    float det = glm::determinant(m3);
                    glm::mat3 normalMatrix = (std::abs(det) > 1e-6f) ? glm::transpose(glm::inverse(m3)) : m3;

                    for (size_t v = 0; v < vertexCount; ++v) {
                        Vertex vertex{};
                        if (positionDataBytes) {
                            const float* pos = reinterpret_cast<const float*>(positionDataBytes + v * posStride);
                            glm::vec4 localPos = glm::vec4(pos[0], pos[1], pos[2], 1.0f);
                            vertex.pos = glm::vec3(matrix * localPos);
                        } else {
                            vertex.pos = glm::vec3(0.0f);
                        }
                        
                        if (std::isnan(vertex.pos.x) || std::isinf(vertex.pos.x)) vertex.pos.x = 0.0f;
                        if (std::isnan(vertex.pos.y) || std::isinf(vertex.pos.y)) vertex.pos.y = 0.0f;
                        if (std::isnan(vertex.pos.z) || std::isinf(vertex.pos.z)) vertex.pos.z = 0.0f;

                        if (normalDataBytes) {
                            const float* norm = reinterpret_cast<const float*>(normalDataBytes + v * normStride);
                            glm::vec3 n = normalMatrix * glm::vec3(norm[0], norm[1], norm[2]);
                            vertex.normal = (glm::length(n) > 1e-5f) ? glm::normalize(n) : glm::vec3(0.0f, 1.0f, 0.0f);
                        } else {
                            vertex.normal = {0.0f, 1.0f, 0.0f};
                        }
                        
                        if (texcoordDataBytes) {
                            const float* tex = reinterpret_cast<const float*>(texcoordDataBytes + v * texStride);
                            vertex.texCoord = {tex[0], tex[1]};
                        } else {
                            vertex.texCoord = {0.0f, 0.0f};
                        }
                        
                        vertex.color = {1.0f, 1.0f, 1.0f};
                        mesh.vertices.push_back(vertex);
                    }
                    
                    if (primitive.indices > -1) {
                        const tinygltf::Accessor& accessor = gltfModel.accessors[primitive.indices];
                        const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
                        const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];
                        
                        if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                            const uint32_t* indexData = reinterpret_cast<const uint32_t*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
                            for (size_t ind = 0; ind < accessor.count; ++ind) {
                                mesh.indices.push_back(vertexStart + indexData[ind]);
                            }
                        } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                            const uint16_t* indexData = reinterpret_cast<const uint16_t*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
                            for (size_t ind = 0; ind < accessor.count; ++ind) {
                                mesh.indices.push_back(vertexStart + indexData[ind]);
                            }
                        } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                            const uint8_t* indexData = reinterpret_cast<const uint8_t*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
                            for (size_t ind = 0; ind < accessor.count; ++ind) {
                                mesh.indices.push_back(vertexStart + indexData[ind]);
                            }
                        }
                    } else {
                        for (size_t v = 0; v < vertexCount; ++v) {
                            mesh.indices.push_back(static_cast<uint32_t>(vertexStart + v));
                        }
                    }
                }
            }
            
            for (int child : node.children) {
                processNode(child, matrix);
            }
        };
        
        for (int nodeIdx : scene.nodes) {
            processNode(nodeIdx, glm::mat4(1.0f));
        }

        int primaryTexIndex = -1;
        for (const auto& mat : gltfModel.materials) {
            if (mat.pbrMetallicRoughness.baseColorTexture.index >= 0) {
                primaryTexIndex = mat.pbrMetallicRoughness.baseColorTexture.index;
                mesh.defaultRoughness = static_cast<float>(mat.pbrMetallicRoughness.roughnessFactor);
                mesh.defaultMetallic = static_cast<float>(mat.pbrMetallicRoughness.metallicFactor);
                break;
            }
        }

        int imageIndex = -1;
        if (primaryTexIndex >= 0 && primaryTexIndex < static_cast<int>(gltfModel.textures.size())) {
            imageIndex = gltfModel.textures[primaryTexIndex].source;
        } else if (!gltfModel.textures.empty() && gltfModel.textures[0].source >= 0) {
            imageIndex = gltfModel.textures[0].source;
        } else if (!gltfModel.images.empty()) {
            imageIndex = 0;
        }

        if (assetManager && imageIndex >= 0 && imageIndex < static_cast<int>(gltfModel.images.size())) {
            const auto& gltfImg = gltfModel.images[imageIndex];
            if (!gltfImg.image.empty() && gltfImg.width > 0 && gltfImg.height > 0) {
                std::string cacheKey = path + "#img" + std::to_string(imageIndex);
                auto& cache = assetManager->getCachedTextureIds();
                if (cache.find(cacheKey) != cache.end()) {
                    mesh.defaultTextureId = cache[cacheKey];
                } else {
                    try {
                        Texture newTex;
                        assetManager->createTextureFromRawPixels(gltfImg.image.data(), gltfImg.width, gltfImg.height, newTex);
                        assetManager->getTextures().push_back(newTex);
                        int newTexId = static_cast<int>(assetManager->getTextures().size()) - 1;
                        cache[cacheKey] = newTexId;
                        mesh.defaultTextureId = newTexId;
                    } catch (const std::exception& ex) {
                        printf("Failed to create Vulkan texture from glTF image %d: %s\n", imageIndex, ex.what());
                    }
                }
            } else if (!gltfImg.uri.empty()) {
                std::filesystem::path p(path);
                std::filesystem::path imgPath = p.parent_path() / gltfImg.uri;
                int texId = assetManager->getOrLoadTextureAsset(imgPath.string());
                if (texId >= 0) mesh.defaultTextureId = texId;
            }
        }
        
        std::string lowerGltf = path;
        std::transform(lowerGltf.begin(), lowerGltf.end(), lowerGltf.begin(), ::tolower);
        if (lowerGltf.find("tree") != std::string::npos || lowerGltf.find("foliage") != std::string::npos || lowerGltf.find("plant") != std::string::npos) {
            mesh.isFoliage = true;
        }
    } else {
        // Fallback to OBJ
        std::filesystem::path objPath(path);
        std::string mtlBaseDir = objPath.parent_path().string();
        if (mtlBaseDir.empty()) mtlBaseDir = ".";

        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str(), mtlBaseDir.c_str()))
            throw std::runtime_error(warn + err);
        
        std::string lowerObjPath = path;
        std::transform(lowerObjPath.begin(), lowerObjPath.end(), lowerObjPath.begin(), ::tolower);
        bool isTreeAsset = (lowerObjPath.find("tree") != std::string::npos || lowerObjPath.find("foliage") != std::string::npos);
        for (const auto& mat : materials) {
            std::string mtex = mat.diffuse_texname;
            std::transform(mtex.begin(), mtex.end(), mtex.begin(), ::tolower);
            if (mtex.find("bark") != std::string::npos || mtex.find("walnut") != std::string::npos ||
                mtex.find("oak") != std::string::npos || mtex.find("leaf") != std::string::npos ||
                mtex.find("leav") != std::string::npos || mtex.find("mossy") != std::string::npos ||
                mtex.find("bottom_t") != std::string::npos || mtex.find("sonnerat") != std::string::npos) {
                isTreeAsset = true;
                break;
            }
        }

        std::vector<int> matTexIds(materials.size(), -1);
        if (assetManager) {
            for (size_t m = 0; m < materials.size(); ++m) {
                std::string texName = materials[m].diffuse_texname;
                if (!texName.empty()) {
                    std::string normTex = texName;
                    std::replace(normTex.begin(), normTex.end(), '\\', '/');
                    std::filesystem::path tp(normTex);
                    std::string filename = tp.filename().string();
                    std::string stem = tp.stem().string();

                    std::vector<std::string> candidates = {
                        normTex,
                        mtlBaseDir + "/" + normTex,
                        mtlBaseDir + "/../" + normTex,
                        mtlBaseDir + "/textures/" + filename,
                        mtlBaseDir + "/../textures/" + filename,
                        mtlBaseDir + "/Texture/" + filename,
                        mtlBaseDir + "/../Texture/" + filename,
                        "assets/" + normTex,
                        "assets/textures/" + filename,
                        "assets/Texture/" + filename,
                        "assets/models/Texture/" + filename,
                        "assets/" + filename,
                        "assets/models/" + filename
                    };

                    if (tp.extension() == ".jpg" || tp.extension() == ".JPG") {
                        std::vector<std::string> pngCandidates = {
                            mtlBaseDir + "/textures/" + stem + ".png",
                            mtlBaseDir + "/../textures/" + stem + ".png",
                            mtlBaseDir + "/Texture/" + stem + ".png",
                            mtlBaseDir + "/" + stem + ".png",
                            "assets/textures/" + stem + ".png",
                            "assets/Texture/" + stem + ".png",
                            "assets/models/Texture/" + stem + ".png",
                            "assets/" + stem + ".png"
                        };
                        candidates.insert(candidates.begin(), pngCandidates.begin(), pngCandidates.end());
                    }

                    for (const auto& cand : candidates) {
                        if (std::filesystem::exists(cand)) {
                            int texId = assetManager->getOrLoadTextureAsset(cand);
                            if (texId >= 0) {
                                matTexIds[m] = texId;
                                break;
                            }
                        }
                    }
                }
            }
        }

        std::map<int, std::vector<tinyobj::index_t>> matFaces;
        for (const auto& shape : shapes) {
            if (shape.mesh.num_face_vertices.empty() && !shape.mesh.indices.empty()) {
                int defMat = (!shape.mesh.material_ids.empty()) ? shape.mesh.material_ids[0] : -1;
                for (const auto& idx : shape.mesh.indices) {
                    matFaces[defMat].push_back(idx);
                }
                continue;
            }
            size_t index_offset = 0;
            for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); ++f) {
                size_t fv = shape.mesh.num_face_vertices[f];
                int mat_id = -1;
                if (f < shape.mesh.material_ids.size()) {
                    mat_id = shape.mesh.material_ids[f];
                }
                if (fv == 3) {
                    matFaces[mat_id].push_back(shape.mesh.indices[index_offset + 0]);
                    matFaces[mat_id].push_back(shape.mesh.indices[index_offset + 1]);
                    matFaces[mat_id].push_back(shape.mesh.indices[index_offset + 2]);
                } else if (fv > 3) {
                    for (size_t v = 1; v + 1 < fv; ++v) {
                        matFaces[mat_id].push_back(shape.mesh.indices[index_offset + 0]);
                        matFaces[mat_id].push_back(shape.mesh.indices[index_offset + v]);
                        matFaces[mat_id].push_back(shape.mesh.indices[index_offset + v + 1]);
                    }
                }
                index_offset += fv;
            }
        }

        for (const auto& pair : matFaces) {
            int mat_id = pair.first;
            const auto& faceIndices = pair.second;
            if (faceIndices.empty()) continue;

            uint32_t subIndexOffset = static_cast<uint32_t>(mesh.indices.size());

            for (const auto& idx : faceIndices) {
                if (idx.vertex_index < 0 || (size_t)(3 * idx.vertex_index + 2) >= attrib.vertices.size()) {
                    continue;
                }

                Vertex vertex{};
                vertex.pos = {
                    attrib.vertices[3 * idx.vertex_index + 0],
                    attrib.vertices[3 * idx.vertex_index + 1],
                    attrib.vertices[3 * idx.vertex_index + 2]
                };

                if (std::isnan(vertex.pos.x) || std::isinf(vertex.pos.x)) vertex.pos.x = 0.0f;
                if (std::isnan(vertex.pos.y) || std::isinf(vertex.pos.y)) vertex.pos.y = 0.0f;
                if (std::isnan(vertex.pos.z) || std::isinf(vertex.pos.z)) vertex.pos.z = 0.0f;

                if (idx.texcoord_index >= 0 && (size_t)(2 * idx.texcoord_index + 1) < attrib.texcoords.size()) {
                    float u = attrib.texcoords[2 * idx.texcoord_index + 0];
                    float v = attrib.texcoords[2 * idx.texcoord_index + 1];
                    vertex.texCoord = {
                        u,
                        isTreeAsset ? v : (1.0f - v)
                    };
                } else {
                    vertex.texCoord = {0.0f, 0.0f};
                }

                if (idx.normal_index >= 0 && (size_t)(3 * idx.normal_index + 2) < attrib.normals.size()) {
                    vertex.normal = {
                        attrib.normals[3 * idx.normal_index + 0],
                        attrib.normals[3 * idx.normal_index + 1],
                        attrib.normals[3 * idx.normal_index + 2]
                    };
                    float nLen = glm::length(vertex.normal);
                    if (nLen > 1e-5f && !std::isnan(nLen)) {
                        vertex.normal /= nLen;
                    } else {
                        vertex.normal = {0.0f, 0.0f, 0.0f};
                    }
                } else {
                    vertex.normal = {0.0f, 0.0f, 0.0f};
                }

                vertex.color = {1.0f, 1.0f, 1.0f};

                mesh.vertices.push_back(vertex);
                mesh.indices.push_back(static_cast<uint32_t>(mesh.vertices.size()) - 1);
            }

            uint32_t subIndexCount = static_cast<uint32_t>(mesh.indices.size()) - subIndexOffset;
            if (subIndexCount == 0) continue;

            SubMesh sub{};
            sub.indexOffset = subIndexOffset;
            sub.indexCount = subIndexCount;
            if (mat_id >= 0 && mat_id < static_cast<int>(matTexIds.size())) {
                sub.textureId = matTexIds[mat_id];
                if (mat_id < static_cast<int>(materials.size())) {
                    sub.roughness = materials[mat_id].roughness > 0.0f ? materials[mat_id].roughness : 0.5f;
                    sub.metallic = materials[mat_id].metallic;

                    std::string mName = materials[mat_id].name;
                    std::string mTex = materials[mat_id].diffuse_texname;
                    std::string mCombined = mName + " " + mTex;
                    std::transform(mCombined.begin(), mCombined.end(), mCombined.begin(), ::tolower);

                    if (mCombined.find("leav") != std::string::npos ||
                        mCombined.find("leaf") != std::string::npos ||
                        mCombined.find("foliage") != std::string::npos ||
                        mCombined.find("walnut") != std::string::npos ||
                        mCombined.find("oak") != std::string::npos ||
                        mCombined.find("sonnerat") != std::string::npos) {
                        sub.isFoliage = true;
                        sub.twoSided = true;
                        sub.roughness = 0.55f;
                        sub.metallic = 0.0f;
                        mesh.isFoliage = true;
                    } else if (mCombined.find("bark") != std::string::npos ||
                               mCombined.find("trunk") != std::string::npos ||
                               mCombined.find("wood") != std::string::npos) {
                        sub.isFoliage = false;
                        sub.roughness = 0.88f;
                        sub.metallic = 0.0f;
                    }
                }
            } else {
                sub.textureId = -1;
            }
            mesh.submeshes.push_back(sub);
        }

        bool needsNormals = attrib.normals.empty();
        if (!needsNormals) {
            for (const auto& v : mesh.vertices) {
                if (glm::length(v.normal) < 0.001f || std::isnan(v.normal.x)) {
                    needsNormals = true;
                    break;
                }
            }
        }

        if (needsNormals) {
            for (auto& v : mesh.vertices) {
                v.normal = glm::vec3(0.0f);
            }
            for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
                uint32_t i0 = mesh.indices[i];
                uint32_t i1 = mesh.indices[i + 1];
                uint32_t i2 = mesh.indices[i + 2];
                if (i0 < mesh.vertices.size() && i1 < mesh.vertices.size() && i2 < mesh.vertices.size()) {
                    glm::vec3 v0 = mesh.vertices[i0].pos;
                    glm::vec3 v1 = mesh.vertices[i1].pos;
                    glm::vec3 v2 = mesh.vertices[i2].pos;
                    glm::vec3 fn = glm::cross(v1 - v0, v2 - v0);
                    mesh.vertices[i0].normal += fn;
                    mesh.vertices[i1].normal += fn;
                    mesh.vertices[i2].normal += fn;
                }
            }
            for (auto& v : mesh.vertices) {
                float len = glm::length(v.normal);
                if (len > 1e-5f && !std::isnan(len)) {
                    v.normal = v.normal / len;
                } else {
                    v.normal = glm::vec3(0.0f, 1.0f, 0.0f);
                }
            }
        }

        if (mesh.indices.size() % 3 != 0) {
            mesh.indices.resize(mesh.indices.size() - (mesh.indices.size() % 3));
        }

        for (const auto& sub : mesh.submeshes) {
            if (sub.textureId >= 0) {
                mesh.defaultTextureId = sub.textureId;
                break;
            }
        }

        if (mesh.defaultTextureId < 0 && assetManager) {
            std::string stem = objPath.stem().string();
            std::string parentDir = objPath.parent_path().string();
            std::string texName = "";
            if (std::filesystem::exists(parentDir + "/" + stem + ".png")) texName = parentDir + "/" + stem + ".png";
            else if (std::filesystem::exists(parentDir + "/" + stem + ".jpg")) texName = parentDir + "/" + stem + ".jpg";
            else if (stem.find("tree") != std::string::npos || stem.find("Tree") != std::string::npos) {
                if (std::filesystem::exists("assets/Texture/Bark___0.jpg")) texName = "assets/Texture/Bark___0.jpg";
                else if (std::filesystem::exists("assets/Texture/Oak_Leav.jpg")) texName = "assets/Texture/Oak_Leav.jpg";
            }
            if (!texName.empty()) {
                mesh.defaultTextureId = assetManager->getOrLoadTextureAsset(texName);
            }
        }
    }
    
    mesh.indexCount = static_cast<uint32_t>(mesh.indices.size());
}
