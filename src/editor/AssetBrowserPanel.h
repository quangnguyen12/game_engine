#pragma once

#include <string>

class AssetManager;
class Scene;

class AssetBrowserPanel
{
public:
    AssetBrowserPanel();

    void draw(AssetManager* assetManager, Scene* scene, float windowWidth, float bottomPanelHeight);
    static void openFileInExternalEditor(const std::string& filePath);

    bool& getVisible() { return showAssetBrowserPanel; }

private:
    bool showAssetBrowserPanel = true;
};
