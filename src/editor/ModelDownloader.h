#pragma once

#include <string>

class Scene;
class AssetManager;

struct OnlineCatalogItem
{
    std::string name;
    std::string category;
    std::string url;
    std::string filename;
    std::string description;
};

class ModelDownloader
{
public:
    ModelDownloader();

    bool downloadModelFromUrl(const std::string& urlStr, const std::string& customFileName, Scene* scene, AssetManager* assetManager);
    void draw(Scene* scene, AssetManager* assetManager);

    bool& getVisible() { return showOnlineDownloaderPanel; }

private:
    bool showOnlineDownloaderPanel = true;
    char downloaderUrlBuffer[512] = "";
    char downloaderFilenameBuffer[128] = "";
    std::string downloaderStatusMsg = "Sẵn sàng tải Model 3D trực tuyến từ URL hoặc Catalog mẫu...";
    bool isDownloadingModel = false;
};
