#pragma once

#include "core/Vertex.h"

class PrimitiveGenerator
{
public:
    static Mesh generateCube();
    static Mesh generateSphere(int stacks = 16, int slices = 32);
    static Mesh generatePlane();
    static Mesh generateCylinder(float radiusTop = 0.5f, float radiusBottom = 0.5f, float height = 1.0f, int slices = 16);
    static Mesh generateCone(float radius = 0.5f, float height = 1.0f, int slices = 16);
    static Mesh generateTerrain(int gridW, int gridD, float cellSize, float heightScale, int seed, int biomeType);
};
