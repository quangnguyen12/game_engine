#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>

class AssetManager;
class Scene;
struct ImDrawList;
struct ImVec2;

enum class AssetCategory {
    TARGETS,
    ENEMIES,
    ANIMATIONS,
    ALL_MODELS,
    CITY,
    WAREHOUSE,
    NATURE,
    PROPS,
    PRIMITIVES,
    TEXTURES,
    SCRIPTS
};

enum class AssetIconType {
    TARGET_IPSC_METRIC,
    TARGET_IPSC,
    TARGET_IPSC_MINI,
    TARGET_POPPER,
    TARGET_POPPER_MINI,
    TARGET_PLATE_ROUND,
    TARGET_PLATE_SQUARE,
    TARGET_PLATE_ROUND_LOW,
    TARGET_PLATE_SQUARE_LOW,
    TARGET_POPPER_TALL,
    TARGET_BARRICADE,
    TARGET_GENERIC,
    MODEL_CHARACTER,
    MODEL_TREE,
    MODEL_ROCK,
    MODEL_LOG,
    MODEL_FOLIAGE,
    MODEL_PROP,
    MODEL_WAREHOUSE,
    MODEL_BUILDING,
    MODEL_VEHICLE,
    MODEL_STREET_PROP,
    PRIMITIVE_CUBE,
    PRIMITIVE_SPHERE,
    PRIMITIVE_PLANE,
    PRIMITIVE_CYLINDER,
    PRIMITIVE_CONE,
    TEXTURE,
    SCRIPT,
    ANIMATION_CLIP,
    GENERIC
};

struct AssetItem {
    std::string id;
    std::string name;          // Clean human-friendly name
    std::string path;          // Primary model or asset path
    std::string filename;
    std::string ext;           // .fbx, .obj, .glb, etc.
    std::string displayExt;    // FBX, OBJ, GLB, PNG, LUA
    std::string texturePath;   // Associated texture file if any
    AssetCategory category = AssetCategory::ALL_MODELS;
    AssetIconType iconType = AssetIconType::GENERIC;
    uintmax_t fileSize = 0;
    bool isPrimitive = false;
    std::string primitivePayload;
};

class AssetBrowserPanel
{
public:
    AssetBrowserPanel();

    void draw(AssetManager* assetManager, Scene* scene, float windowWidth, float bottomPanelHeight);
    static void openFileInExternalEditor(const std::string& filePath);
    static void openFileLocationInExplorer(const std::string& filePath);

    bool& getVisible() { return showAssetBrowserPanel; }
    void setVisible(bool visible) { showAssetBrowserPanel = visible; }

    void refreshAssetRegistry(AssetManager* assetManager);

private:
    void renderAssetCard(const AssetItem& item, AssetManager* assetManager, Scene* scene, float cardWidth, float cardHeight);
    void drawIconSilhouette(ImDrawList* drawList, const ImVec2& minP, const ImVec2& maxP, AssetIconType type);

    bool showAssetBrowserPanel = true;
    AssetCategory selectedCategory = AssetCategory::TARGETS; // Default to TARGETS matching New_UI.jpg!
    char searchFilter[64] = "";
    bool needsInitialScan = true;
    std::vector<AssetItem> cachedAssets;
    std::string selectedAssetId = "";
};
