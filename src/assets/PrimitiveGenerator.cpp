#include "assets/PrimitiveGenerator.h"
#include <glm/gtc/constants.hpp>
#include <cmath>
#include <vector>

Mesh PrimitiveGenerator::generateCube()
{
    Mesh mesh;

    struct FaceDef { glm::vec3 normal; glm::vec3 verts[4]; glm::vec3 colors[4]; };
    std::vector<FaceDef> faces = {
        // Front (+Z)
        { {0,0,1}, { {-0.5f,-0.5f,0.5f},{0.5f,-0.5f,0.5f},{0.5f,0.5f,0.5f},{-0.5f,0.5f,0.5f} },
          { {1.0f,0.2f,0.3f},{1.0f,0.5f,0.2f},{1.0f,0.8f,0.2f},{1.0f,0.3f,0.5f} } },
        // Back (-Z)
        { {0,0,-1}, { {0.5f,-0.5f,-0.5f},{-0.5f,-0.5f,-0.5f},{-0.5f,0.5f,-0.5f},{0.5f,0.5f,-0.5f} },
          { {0.1f,0.9f,0.3f},{0.2f,1.0f,0.5f},{0.3f,0.9f,0.7f},{0.1f,0.8f,0.4f} } },
        // Top (-Y in Vulkan)
        { {0,-1,0}, { {-0.5f,-0.5f,-0.5f},{0.5f,-0.5f,-0.5f},{0.5f,-0.5f,0.5f},{-0.5f,-0.5f,0.5f} },
          { {0.1f,0.5f,1.0f},{0.2f,0.6f,1.0f},{0.4f,0.2f,1.0f},{0.1f,0.4f,1.0f} } },
        // Bottom (+Y in Vulkan)
        { {0,1,0}, { {-0.5f,0.5f,0.5f},{0.5f,0.5f,0.5f},{0.5f,0.5f,-0.5f},{-0.5f,0.5f,-0.5f} },
          { {1.0f,0.9f,0.1f},{1.0f,0.7f,0.2f},{1.0f,0.9f,0.3f},{1.0f,0.8f,0.1f} } },
        // Right (+X)
        { {1,0,0}, { {0.5f,-0.5f,0.5f},{0.5f,-0.5f,-0.5f},{0.5f,0.5f,-0.5f},{0.5f,0.5f,0.5f} },
          { {0.9f,0.2f,0.9f},{0.7f,0.1f,1.0f},{1.0f,0.3f,0.9f},{0.8f,0.2f,1.0f} } },
        // Left (-X)
        { {-1,0,0}, { {-0.5f,-0.5f,-0.5f},{-0.5f,-0.5f,0.5f},{-0.5f,0.5f,0.5f},{-0.5f,0.5f,-0.5f} },
          { {0.1f,0.9f,0.9f},{0.2f,1.0f,0.8f},{0.1f,0.8f,1.0f},{0.3f,0.9f,1.0f} } },
    };

    glm::vec2 uvs[4] = { {0,1},{1,1},{1,0},{0,0} };

    for (auto& face : faces) {
        uint32_t base = static_cast<uint32_t>(mesh.vertices.size());
        for (int v = 0; v < 4; v++) {
            Vertex vert{};
            vert.pos      = face.verts[v];
            vert.color    = face.colors[v];
            vert.normal   = face.normal;
            vert.texCoord = uvs[v];
            mesh.vertices.push_back(vert);
        }
        mesh.indices.insert(mesh.indices.end(), {base,base+1,base+2, base+2,base+3,base});
    }

    mesh.indexCount = static_cast<uint32_t>(mesh.indices.size());
    return mesh;
}

Mesh PrimitiveGenerator::generateSphere(int stacks, int slices)
{
    Mesh mesh;
    const float PI = glm::pi<float>();

    for (int i = 0; i <= stacks; ++i) {
        float phi = PI * float(i) / float(stacks);
        for (int j = 0; j <= slices; ++j) {
            float theta = 2.0f * PI * float(j) / float(slices);
            Vertex v{};
            v.pos = { std::sin(phi)*std::cos(theta)*0.5f,
                     -std::cos(phi)*0.5f,
                      std::sin(phi)*std::sin(theta)*0.5f };
            v.normal   = glm::normalize(v.pos);
            v.color    = { 0.9f + 0.1f*std::sin(theta), 0.7f, 0.3f };
            v.texCoord = { float(j)/float(slices), float(i)/float(stacks) };
            mesh.vertices.push_back(v);
        }
    }

    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            uint32_t r0 = (uint32_t)((i  )*(slices+1) + j);
            uint32_t r1 = (uint32_t)((i+1)*(slices+1) + j);
            mesh.indices.insert(mesh.indices.end(), {r0, r1, r0+1, r1, r1+1, r0+1});
        }
    }

    mesh.indexCount = static_cast<uint32_t>(mesh.indices.size());
    return mesh;
}

Mesh PrimitiveGenerator::generatePlane()
{
    Mesh mesh;
    glm::vec3 normal = {0, 1, 0};
    glm::vec3 cols[4] = {
        {0.35f,0.35f,0.4f}, {0.3f,0.3f,0.38f},
        {0.4f,0.4f,0.45f},  {0.32f,0.32f,0.4f}
    };
    glm::vec3 pos[4] = {
        {-0.5f, 0.0f, -0.5f}, {0.5f, 0.0f, -0.5f},
        {0.5f,  0.0f,  0.5f}, {-0.5f, 0.0f, 0.5f}
    };
    glm::vec2 uvs[4] = { {0,0},{1,0},{1,1},{0,1} };
    for (int i = 0; i < 4; i++) {
        Vertex v{};
        v.pos = pos[i]; v.normal = normal; v.color = cols[i]; v.texCoord = uvs[i];
        mesh.vertices.push_back(v);
    }
    mesh.indices = {0,2,1, 0,3,2};
    mesh.indexCount = static_cast<uint32_t>(mesh.indices.size());
    return mesh;
}

Mesh PrimitiveGenerator::generateCylinder(float radiusTop, float radiusBottom, float height, int slices)
{
    Mesh mesh;
    const float PI = glm::pi<float>();
    float halfH = height * 0.5f;

    // Side wall
    for (int i = 0; i <= slices; ++i) {
        float theta = 2.0f * PI * float(i) / float(slices);
        float cosT = std::cos(theta);
        float sinT = std::sin(theta);

        glm::vec3 normal = glm::normalize(glm::vec3(cosT, (radiusBottom - radiusTop) / height, sinT));

        Vertex vBot{};
        vBot.pos = glm::vec3(radiusBottom * cosT, -halfH, radiusBottom * sinT);
        vBot.normal = normal;
        vBot.color = glm::vec3(0.75f, 0.75f, 0.75f);
        vBot.texCoord = glm::vec2(float(i) / float(slices), 1.0f);
        mesh.vertices.push_back(vBot);

        Vertex vTop{};
        vTop.pos = glm::vec3(radiusTop * cosT, halfH, radiusTop * sinT);
        vTop.normal = normal;
        vTop.color = glm::vec3(0.9f, 0.9f, 0.9f);
        vTop.texCoord = glm::vec2(float(i) / float(slices), 0.0f);
        mesh.vertices.push_back(vTop);
    }

    for (int i = 0; i < slices; ++i) {
        uint32_t b0 = i * 2;
        uint32_t t0 = i * 2 + 1;
        uint32_t b1 = (i + 1) * 2;
        uint32_t t1 = (i + 1) * 2 + 1;

        mesh.indices.insert(mesh.indices.end(), { b0, b1, t0, t0, b1, t1 });
    }

    // Top cap
    uint32_t topCenterIdx = static_cast<uint32_t>(mesh.vertices.size());
    Vertex vTopCenter{};
    vTopCenter.pos = glm::vec3(0.0f, halfH, 0.0f);
    vTopCenter.normal = glm::vec3(0.0f, 1.0f, 0.0f);
    vTopCenter.color = glm::vec3(0.95f);
    vTopCenter.texCoord = glm::vec2(0.5f, 0.5f);
    mesh.vertices.push_back(vTopCenter);

    uint32_t topRingStart = static_cast<uint32_t>(mesh.vertices.size());
    for (int i = 0; i <= slices; ++i) {
        float theta = 2.0f * PI * float(i) / float(slices);
        Vertex v{};
        v.pos = glm::vec3(radiusTop * std::cos(theta), halfH, radiusTop * std::sin(theta));
        v.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        v.color = glm::vec3(0.95f);
        v.texCoord = glm::vec2(0.5f + 0.5f * std::cos(theta), 0.5f + 0.5f * std::sin(theta));
        mesh.vertices.push_back(v);
    }
    for (int i = 0; i < slices; ++i) {
        mesh.indices.insert(mesh.indices.end(), { topCenterIdx, topRingStart + i, topRingStart + i + 1 });
    }

    // Bottom cap
    uint32_t botCenterIdx = static_cast<uint32_t>(mesh.vertices.size());
    Vertex vBotCenter{};
    vBotCenter.pos = glm::vec3(0.0f, -halfH, 0.0f);
    vBotCenter.normal = glm::vec3(0.0f, -1.0f, 0.0f);
    vBotCenter.color = glm::vec3(0.6f);
    vBotCenter.texCoord = glm::vec2(0.5f, 0.5f);
    mesh.vertices.push_back(vBotCenter);

    uint32_t botRingStart = static_cast<uint32_t>(mesh.vertices.size());
    for (int i = 0; i <= slices; ++i) {
        float theta = 2.0f * PI * float(i) / float(slices);
        Vertex v{};
        v.pos = glm::vec3(radiusBottom * std::cos(theta), -halfH, radiusBottom * std::sin(theta));
        v.normal = glm::vec3(0.0f, -1.0f, 0.0f);
        v.color = glm::vec3(0.6f);
        v.texCoord = glm::vec2(0.5f + 0.5f * std::cos(theta), 0.5f + 0.5f * std::sin(theta));
        mesh.vertices.push_back(v);
    }
    for (int i = 0; i < slices; ++i) {
        mesh.indices.insert(mesh.indices.end(), { botCenterIdx, botRingStart + i + 1, botRingStart + i });
    }

    mesh.indexCount = static_cast<uint32_t>(mesh.indices.size());
    return mesh;
}

Mesh PrimitiveGenerator::generateCone(float radius, float height, int slices)
{
    return generateCylinder(0.001f, radius, height, slices);
}

Mesh PrimitiveGenerator::generateTerrain(int gridW, int gridD, float cellSize, float heightScale, int seed, int biomeType)
{
    Mesh mesh;
    float halfW = (gridW - 1) * cellSize * 0.5f;
    float halfD = (gridD - 1) * cellSize * 0.5f;

    auto noiseFunc = [seed, heightScale, biomeType](float x, float z) -> float {
        float sx = x * 0.1f + seed * 0.13f;
        float sz = z * 0.1f + seed * 0.17f;

        float h = 0.0f;
        if (biomeType == 0) { // Forest & Mountains
            h += std::sin(sx) * std::cos(sz) * 0.6f;
            h += std::sin(sx * 2.3f + 1.2f) * std::cos(sz * 2.1f + 0.5f) * 0.3f;
            h += std::sin(sx * 4.7f) * std::cos(sz * 4.9f) * 0.1f;
            h = std::pow(std::abs(h), 1.3f) * (h >= 0 ? 1.0f : -0.5f);
        } else if (biomeType == 1) { // Desert Dunes
            h += std::sin(sx * 0.7f + sz * 0.5f) * 0.8f;
            h += std::cos(sx * 1.8f - sz * 1.2f) * 0.2f;
        } else if (biomeType == 2) { // Cyberpunk City Floor
            float cx = std::floor(x * 0.2f);
            float cz = std::floor(z * 0.2f);
            h = (std::sin(cx * 12.3f + cz * 45.6f + seed) > 0.3f) ? 0.05f : 0.0f;
        } else if (biomeType == 3) { // Medieval Village Valley
            float distFromCenter = std::sqrt(x * x + z * z);
            h = (1.0f - std::exp(-distFromCenter * 0.05f)) * 0.8f;
            h += std::sin(sx * 1.5f) * std::cos(sz * 1.5f) * 0.2f;
        } else if (biomeType == 4) { // Floating Islands
            h = std::sin(sx * 1.2f) * std::cos(sz * 1.2f) * 1.2f;
        } else { // 3D Maze Floor
            h = 0.0f;
        }
        return h * heightScale;
    };

    std::vector<std::vector<glm::vec3>> positions(gridD, std::vector<glm::vec3>(gridW));
    std::vector<std::vector<glm::vec3>> normals(gridD, std::vector<glm::vec3>(gridW));

    for (int z = 0; z < gridD; ++z) {
        for (int x = 0; x < gridW; ++x) {
            float worldX = -halfW + x * cellSize;
            float worldZ = -halfD + z * cellSize;
            float worldY = noiseFunc(worldX, worldZ);
            positions[z][x] = glm::vec3(worldX, worldY, worldZ);
        }
    }

    for (int z = 0; z < gridD; ++z) {
        for (int x = 0; x < gridW; ++x) {
            float xL = (x > 0) ? positions[z][x - 1].y : positions[z][x].y;
            float xR = (x < gridW - 1) ? positions[z][x + 1].y : positions[z][x].y;
            float zU = (z > 0) ? positions[z - 1][x].y : positions[z][x].y;
            float zD = (z < gridD - 1) ? positions[z + 1][x].y : positions[z][x].y;

            glm::vec3 normal = glm::normalize(glm::vec3(xL - xR, 2.0f * cellSize, zU - zD));
            normals[z][x] = normal;
        }
    }

    for (int z = 0; z < gridD; ++z) {
        for (int x = 0; x < gridW; ++x) {
            Vertex v{};
            v.pos = positions[z][x];
            v.normal = normals[z][x];
            v.texCoord = glm::vec2(float(x) / float(gridW - 1) * 12.0f, float(z) / float(gridD - 1) * 12.0f);

            float y = v.pos.y;
            if (biomeType == 0) { // Forest
                if (y < heightScale * 0.2f) v.color = glm::vec3(0.2f, 0.6f, 0.25f);
                else if (y < heightScale * 0.6f) v.color = glm::vec3(0.45f, 0.4f, 0.25f);
                else v.color = glm::vec3(0.9f, 0.92f, 0.95f);
            } else if (biomeType == 1) { // Desert
                v.color = glm::vec3(0.85f + 0.1f * std::sin(y), 0.68f, 0.35f);
            } else if (biomeType == 2) { // Cyberpunk Floor
                v.color = glm::vec3(0.12f, 0.14f, 0.18f);
            } else if (biomeType == 3) { // Medieval Valley
                v.color = (y > heightScale * 0.5f) ? glm::vec3(0.5f, 0.5f, 0.5f) : glm::vec3(0.3f, 0.65f, 0.3f);
            } else if (biomeType == 4) { // Floating Island
                v.color = glm::vec3(0.4f, 0.45f, 0.35f);
            } else { // Dungeon Stone
                v.color = glm::vec3(0.3f, 0.3f, 0.32f);
            }

            mesh.vertices.push_back(v);
        }
    }

    for (int z = 0; z < gridD - 1; ++z) {
        for (int x = 0; x < gridW - 1; ++x) {
            uint32_t topLeft = z * gridW + x;
            uint32_t topRight = topLeft + 1;
            uint32_t botLeft = (z + 1) * gridW + x;
            uint32_t botRight = botLeft + 1;

            mesh.indices.insert(mesh.indices.end(), { topLeft, botLeft, topRight, topRight, botLeft, botRight });
        }
    }

    mesh.indexCount = static_cast<uint32_t>(mesh.indices.size());
    return mesh;
}
