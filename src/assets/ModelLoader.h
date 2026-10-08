#pragma once

#include "core/Vertex.h"
#include <string>

class AssetManager;

class ModelLoader
{
public:
    static void loadModel(const std::string& path, Mesh& mesh, AssetManager* assetManager);
};
