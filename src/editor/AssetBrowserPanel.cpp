#include "editor/AssetBrowserPanel.h"
#include "assets/AssetManager.h"
#include "scene/Scene.h"
#include "animation/AnimationLoader.h"
#include "imgui.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#endif

#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cmath>

AssetBrowserPanel::AssetBrowserPanel()
    : showAssetBrowserPanel(true), selectedCategory(AssetCategory::TARGETS), needsInitialScan(true)
{
    searchFilter[0] = '\0';
}

void AssetBrowserPanel::openFileInExternalEditor(const std::string& filePath)
{
#ifdef _WIN32
    ShellExecuteA(NULL, "open", filePath.c_str(), NULL, NULL, SW_SHOWNORMAL);
#else
    std::string cmd = "xdg-open \"" + filePath + "\" &";
    system(cmd.c_str());
#endif
}

void AssetBrowserPanel::openFileLocationInExplorer(const std::string& filePath)
{
#ifdef _WIN32
    std::string param = "/select,\"" + filePath + "\"";
    ShellExecuteA(NULL, "open", "explorer.exe", param.c_str(), NULL, SW_SHOWNORMAL);
#endif
}

static std::string formatCleanName(const std::string& stem)
{
    std::string s = stem;
    if (s.rfind("SM_", 0) == 0) s = s.substr(3);
    std::replace(s.begin(), s.end(), '_', ' ');

    std::string lower = stem;
    for (auto& c : lower) c = (char)tolower(c);

    if (lower == "ipsc_metric_target") return "IPSC Metric Target";
    if (lower == "ipsc_target") return "IPSC Target";
    if (lower == "ipsc_target_mini") return "IPSC Target Mini";
    if (lower == "pepper_popper") return "Pepper Popper";
    if (lower == "pepper_popper_mini") return "Pepper Popper Mini";
    if (lower == "plate_a") return "Plate A";
    if (lower == "plate_b") return "Plate B";
    if (lower == "plate_c") return "Plate C";
    if (lower == "plate_d") return "Plate D";
    if (lower == "shootingtarget") return "Shooting Target";
    if (lower.find("target_vinashoot_swivel_fold") != std::string::npos || lower.find("vinashoot_swivel_fold") != std::string::npos) return "VINASHOOT Swiveling & Folding Target";
    if (lower.find("target_portable_motorized") != std::string::npos || lower.find("portable_motorized") != std::string::npos) return "Portable Motorized Target (VINASHOOT)";
    if (lower.find("biaso4") != std::string::npos) return "Vietnam Military Target - Bia So 4";
    if (lower.find("biaso7_tripod") != std::string::npos) return "Vietnam Military Target - Bia So 7 (Tripod)";
    if (lower.find("biaso7") != std::string::npos) return "Vietnam Military Target - Bia So 7";
    if (lower.find("hyster_forklift") != std::string::npos) return "Hyster Forklift";
    if (lower.find("pallet_wood") != std::string::npos) return "Wooden Pallet";
    if (lower.find("plasticbarrel") != std::string::npos) return "Plastic Barrel";
    if (lower.find("air_conditioner") != std::string::npos) return "HVAC Unit";
    if (lower.find("palleted_4_barrels") != std::string::npos) return "Palleted Barrels";
    if (lower.find("barrel_01_damaged") != std::string::npos) return "Damaged Barrel";
    if (lower.find("barrellabeled") != std::string::npos) return "Hazard Barrel";
    if (lower.find("handtruck_red") != std::string::npos) return "Red Pallet Truck";
    if (lower.find("handtruck_standard") != std::string::npos) return "Hand Truck";
    if (lower.find("tool_desk_populated") != std::string::npos) return "Tool Workbench";
    if (lower.find("palletcementbags") != std::string::npos) return "Pallet Cement Bags";
    if (lower.find("palletboxes") != std::string::npos) return "Pallet Boxes";
    if (lower.find("aisle_red") != std::string::npos) return "Red Shelving Aisle";
    if (lower.find("aisle_white") != std::string::npos) return "White Shelving Aisle";
    if (lower.find("crate_wood") != std::string::npos) return "Wood Crate";
    if (lower.find("stackcrate") != std::string::npos) return "Stackable Crate";
    if (lower == "tree_pine") return "Pine Tree";
    if (lower == "tree_cedar") return "Cedar Tree";
    if (lower == "tree_redwood") return "Redwood Tree";
    if (lower == "tree_oak") return "Oak Tree";
    if (lower == "tree_mossy") return "Mossy Tree";
    if (lower == "tree_savanna") return "Savanna Tree";
    if (lower == "tree9" || lower == "trees9") return "Deciduous Tree";
    if (lower == "rock") return "Valley Boulder";
    if (lower == "stone") return "Meadow Stone";
    if (lower == "bush") return "Green Bush";
    if (lower == "log_pile") return "Log Pile";
    if (lower == "tree_log") return "Fallen Tree Log";
    if (lower == "damagedhelmet") return "Damaged Helmet";
    if (lower == "fox") return "Low-poly Fox";
    if (lower == "duck") return "Rubber Duck";
    if (lower == "lantern") return "Antique Lantern";
    if (lower == "cesiumman") return "Cesium Man";
    if (lower == "avocado") return "Avocado";
    if (lower == "box") return "Shipping Box";

    // Enemy Characters / Bank Robbers
    if (lower == "tm_classy_bank_robber") return "Bank Robber Classic";
    if (lower == "tm_classy_bank_robber2") return "Bank Robber Var 1";
    if (lower == "tm_classy_bank_robber3") return "Bank Robber Var 2";
    if (lower == "tm_classy_bank_robber_leader") return "Bank Robber Leader";
    if (lower == "tm_classy_black_guy") return "Bank Robber Var 4";
    if (lower == "tm_classy_bank_robber_ak47") return "Bank Robber Classic (AK-47 Rigged)";
    if (lower == "test_ak47") return "Enemy Var 4 (AK-47 Rigged)";

    // Weapons
    if (lower == "ak47") return "AK-47 Assault Rifle (Ultra PBR)";
    if (lower == "ak47_gameready") return "AK-47 (Game-Ready 18k)";

    // City & NYC Urban Street Models
    if (lower == "nyc_taxi" || lower == "taxi") return "NYC Yellow Taxi";
    if (lower == "nypd_cruiser" || lower == "police_car") return "NYPD Cruiser";
    if (lower == "nyc_lamppost") return "NYC Lamppost";
    if (lower == "nyc_fire_hydrant") return "Fire Hydrant";
    if (lower == "nyc_bus_stop_shelter") return "Bus Stop Shelter";
    if (lower == "nyc_street_sign") return "Street Sign";
    if (lower == "nyc_newsbox_a") return "News Box A";
    if (lower == "nyc_newsbox_b") return "News Box B";
    if (lower == "nyc_manhole_cover") return "Manhole Cover";
    if (lower == "nyc_sewer_grate") return "Sewer Grate";
    if (lower == "nyc_street_tree") return "NYC Street Tree";
    if (lower == "nypd_barricade") return "NYPD Barricade";
    if (lower == "police_traffic_cone") return "Traffic Cone";
    if (lower == "police_cordon_tape") return "Police Cordon Tape";
    if (lower == "street_road_asphalt") return "Asphalt Road";
    if (lower == "sidewalk_curb") return "Sidewalk & Curb";
    if (lower == "waterfront_pier") return "Waterfront Pier";
    if (lower == "waterfront_railing") return "Waterfront Railing";
    if (lower == "background_manhattan") return "Manhattan Skyline";
    if (lower == "building_left_near") return "Building Left Near";
    if (lower == "building_left_far") return "Building Left Far";
    if (lower == "building_right_near") return "Building Right Near";
    if (lower == "building_right_far") return "Building Right Far";
    if (lower == "building_modular_bottom") return "Modular Base";
    if (lower == "building_modular_mid") return "Modular Mid";
    if (lower == "building_modular_top") return "Modular Top";
    if (lower == "building_depth_midrise") return "Depth Midrise";
    if (lower == "building_depth_small") return "Depth Small";
    if (lower == "building_depth_tower") return "Depth Tower";
    if (lower == "building_5story") return "Building 5-Story";
    if (lower == "building_6story") return "Building 6-Story";
    if (lower == "building_7story") return "Building 7-Story";

    return s;
}

void AssetBrowserPanel::refreshAssetRegistry(AssetManager* assetManager)
{
    cachedAssets.clear();

    // 1. Register Built-in Primitives
    {
        AssetItem cube;
        cube.id = "prim_cube";
        cube.name = "Cube Primitive";
        cube.path = "PRIMITIVE_CUBE";
        cube.filename = "Cube";
        cube.ext = "";
        cube.displayExt = "PRIM";
        cube.category = AssetCategory::PRIMITIVES;
        cube.iconType = AssetIconType::PRIMITIVE_CUBE;
        cube.isPrimitive = true;
        cube.primitivePayload = "PRIMITIVE_CUBE";
        cachedAssets.push_back(cube);

        AssetItem sphere;
        sphere.id = "prim_sphere";
        sphere.name = "Sphere Primitive";
        sphere.path = "PRIMITIVE_SPHERE";
        sphere.filename = "Sphere";
        sphere.ext = "";
        sphere.displayExt = "PRIM";
        sphere.category = AssetCategory::PRIMITIVES;
        sphere.iconType = AssetIconType::PRIMITIVE_SPHERE;
        sphere.isPrimitive = true;
        sphere.primitivePayload = "PRIMITIVE_SPHERE";
        cachedAssets.push_back(sphere);

        AssetItem plane;
        plane.id = "prim_plane";
        plane.name = "Plane Primitive";
        plane.path = "PRIMITIVE_PLANE";
        plane.filename = "Plane";
        plane.ext = "";
        plane.displayExt = "PRIM";
        plane.category = AssetCategory::PRIMITIVES;
        plane.iconType = AssetIconType::PRIMITIVE_PLANE;
        plane.isPrimitive = true;
        plane.primitivePayload = "PRIMITIVE_PLANE";
        cachedAssets.push_back(plane);
    }

    // 2. Discover ALL 3D Models (Both OBJ and GLB/FBX without deduplicating or dropping)
    std::unordered_set<std::string> visitedPaths;
    std::vector<AssetItem> targetItems;
    std::vector<AssetItem> enemyItems;
    std::vector<AssetItem> animationItems;
    std::vector<AssetItem> cityItems;
    std::vector<AssetItem> warehouseItems;
    std::vector<AssetItem> natureItems;
    std::vector<AssetItem> propItems;

    std::vector<std::string> searchDirs = { "assets/models", "assets/animations", "assets" };

    for (const auto& dir : searchDirs)
    {
        if (!std::filesystem::exists(dir)) continue;

        for (const auto& entry : std::filesystem::recursive_directory_iterator(dir))
        {
            if (!entry.is_regular_file()) continue;

            std::string pStr = entry.path().string();
            std::replace(pStr.begin(), pStr.end(), '\\', '/');

            // Skip textures subdirectories inside models for 3D model scanning
            if (pStr.find("/Tex/") != std::string::npos || pStr.find("/textures/") != std::string::npos || pStr.find("/Texture/") != std::string::npos) continue;

            // Prevent duplicate visits across search directories
            if (visitedPaths.count(pStr)) continue;
            visitedPaths.insert(pStr);

            std::string ext = entry.path().extension().string();
            for (auto& c : ext) c = (char)tolower(c);

            if (ext == ".obj" || ext == ".glb" || ext == ".gltf" || ext == ".fbx")
            {
                std::string stem = entry.path().stem().string();
                std::string lowerStem = stem;
                for (auto& c : lowerStem) c = (char)tolower(c);

                AssetItem item;
                item.id = "model_" + pStr;
                item.name = formatCleanName(stem);
                item.path = pStr;
                item.filename = entry.path().filename().string();
                item.ext = ext;
                item.fileSize = entry.file_size();

                std::string dExt = ext.substr(1);
                for (auto& c : dExt) c = (char)toupper(c);
                item.displayExt = dExt;

                // Category & Icon Type Determination
                std::string lowerPath = pStr;
                for (auto& c : lowerPath) c = (char)tolower(c);

                bool isAnimation = false;
                if (ext == ".fbx")
                {
                    if (lowerPath.find("/animations/") != std::string::npos ||
                        lowerPath.find("/anim/") != std::string::npos ||
                        lowerPath.find("/motion/") != std::string::npos)
                    {
                        isAnimation = true;
                    }
                    else if (AnimationLoader::isAnimationOnlyFBX(pStr))
                    {
                        isAnimation = true;
                    }
                }

                if (isAnimation)
                {
                    item.category = AssetCategory::ANIMATIONS;
                    item.displayExt = "ANIM";
                    item.iconType = AssetIconType::ANIMATION_CLIP;
                    animationItems.push_back(item);
                    continue;
                }

                bool isCity = (lowerPath.find("nyc_street") != std::string::npos ||
                               lowerPath.find("/nyc") != std::string::npos ||
                               lowerStem.rfind("nyc_", 0) == 0 ||
                               lowerStem.rfind("nypd_", 0) == 0 ||
                               lowerStem == "taxi" || lowerStem == "police_car" ||
                               lowerStem.rfind("building_", 0) == 0 ||
                               lowerStem.rfind("sidewalk_", 0) == 0 ||
                               lowerStem.rfind("street_road", 0) == 0 ||
                               lowerStem.rfind("waterfront_", 0) == 0 ||
                               lowerStem.rfind("police_traffic", 0) == 0 ||
                               lowerStem.rfind("police_cordon", 0) == 0 ||
                               lowerStem == "background_manhattan" ||
                               lowerStem == "test_hydrant" || lowerStem == "test_taxi");

                bool isEnemy = (lowerStem.find("tm_classy") != std::string::npos ||
                                lowerStem.find("bank_robber") != std::string::npos ||
                                lowerStem.find("black_guy") != std::string::npos ||
                                lowerStem.find("enemy") != std::string::npos);

                if (isCity) {
                    item.category = AssetCategory::CITY;
                    if (lowerStem.find("building") != std::string::npos || lowerStem.find("manhattan") != std::string::npos) {
                        item.iconType = AssetIconType::MODEL_BUILDING;
                    } else if (lowerStem.find("taxi") != std::string::npos || lowerStem.find("cruiser") != std::string::npos || lowerStem.find("police_car") != std::string::npos) {
                        item.iconType = AssetIconType::MODEL_VEHICLE;
                    } else if (lowerStem.find("tree") != std::string::npos) {
                        item.iconType = AssetIconType::MODEL_TREE;
                    } else {
                        item.iconType = AssetIconType::MODEL_STREET_PROP;
                    }
                } else if (isEnemy) {
                    item.category = AssetCategory::ENEMIES;
                    item.iconType = AssetIconType::MODEL_CHARACTER;
                    if (lowerStem == "tm_classy_bank_robber" || lowerStem == "tm_classy_bank_robber_ak47") {
                        item.texturePath = "assets/textures/tm_professional_head_variantd.jpg";
                    } else if (lowerStem == "tm_classy_bank_robber2") {
                        item.texturePath = "assets/textures/tm_professional_head_varainta.jpg";
                    } else if (lowerStem == "tm_classy_bank_robber3") {
                        item.texturePath = "assets/textures/tm_professional_head_variantb.jpg";
                    } else if (lowerStem == "tm_classy_bank_robber_leader") {
                        item.texturePath = "assets/textures/tm_professional_head_varainta.jpg";
                    } else if (lowerStem == "tm_classy_black_guy" || lowerStem == "test_ak47") {
                        item.texturePath = "assets/textures/tm_professional_head_variante.jpg";
                    }
                } else if (lowerStem.find("ipsc_metric") != std::string::npos) {
                    item.category = AssetCategory::TARGETS;
                    item.iconType = AssetIconType::TARGET_IPSC_METRIC;
                } else if (lowerStem.find("ipsc_target_mini") != std::string::npos) {
                    item.category = AssetCategory::TARGETS;
                    item.iconType = AssetIconType::TARGET_IPSC_MINI;
                } else if (lowerStem.find("ipsc") != std::string::npos) {
                    item.category = AssetCategory::TARGETS;
                    item.iconType = AssetIconType::TARGET_IPSC;
                } else if (lowerStem.find("pepper_popper_mini") != std::string::npos) {
                    item.category = AssetCategory::TARGETS;
                    item.iconType = AssetIconType::TARGET_POPPER_MINI;
                } else if (lowerStem.find("pepper_popper") != std::string::npos) {
                    item.category = AssetCategory::TARGETS;
                    item.iconType = AssetIconType::TARGET_POPPER;
                } else if (lowerStem.find("plate_a") != std::string::npos) {
                    item.category = AssetCategory::TARGETS;
                    item.iconType = AssetIconType::TARGET_PLATE_ROUND;
                } else if (lowerStem.find("plate_b") != std::string::npos) {
                    item.category = AssetCategory::TARGETS;
                    item.iconType = AssetIconType::TARGET_PLATE_SQUARE;
                } else if (lowerStem.find("plate_c") != std::string::npos) {
                    item.category = AssetCategory::TARGETS;
                    item.iconType = AssetIconType::TARGET_PLATE_ROUND_LOW;
                } else if (lowerStem.find("plate_d") != std::string::npos) {
                    item.category = AssetCategory::TARGETS;
                    item.iconType = AssetIconType::TARGET_PLATE_SQUARE_LOW;
                } else if (lowerStem.find("popper") != std::string::npos) {
                    item.category = AssetCategory::TARGETS;
                    item.iconType = AssetIconType::TARGET_POPPER_TALL;
                } else if (lowerStem.find("barricade") != std::string::npos) {
                    item.category = AssetCategory::TARGETS;
                    item.iconType = AssetIconType::TARGET_BARRICADE;
                } else if (lowerStem.find("shootingtarget") != std::string::npos || lowerStem.find("target") != std::string::npos) {
                    item.category = AssetCategory::TARGETS;
                    item.iconType = AssetIconType::TARGET_GENERIC;
                    if (lowerStem.find("biaso4") != std::string::npos) {
                        item.texturePath = "assets/models/Target_BiaSo4_Cardboard.png";
                    } else if (lowerStem.find("biaso7_tripod") != std::string::npos) {
                        item.texturePath = "assets/models/Target_BiaSo7_Tripod_Paper.png";
                    } else if (lowerStem.find("biaso7") != std::string::npos) {
                        item.texturePath = "assets/models/Target_BiaSo7_BoardFace_V5.png";
                    } else if (lowerStem.find("swivel_fold") != std::string::npos || lowerStem.find("vinashoot_swivel") != std::string::npos) {
                        item.texturePath = "assets/models/tex_new_target_front.png";
                    } else if (lowerStem.find("portable") != std::string::npos || lowerStem.find("motorized") != std::string::npos) {
                        item.texturePath = "assets/models/soldier_target_board.png";
                    } else if (lowerStem.find("shootingtarget") != std::string::npos) {
                        item.texturePath = "assets/models/ShootingTarget.png";
                    }
                } else if (lowerPath.find("warehouse") != std::string::npos || lowerStem.find("warehouse") != std::string::npos ||
                           lowerStem.find("forklift") != std::string::npos || lowerStem.find("pallet") != std::string::npos ||
                           lowerStem.find("aisle") != std::string::npos || lowerStem.find("barrel") != std::string::npos ||
                           lowerStem.find("hvac") != std::string::npos || lowerStem.find("conditioner") != std::string::npos ||
                           lowerStem.find("truck") != std::string::npos || lowerStem.find("crate") != std::string::npos ||
                           lowerStem.find("desk") != std::string::npos || lowerStem.find("shelf") != std::string::npos ||
                           lowerStem.find("shelv") != std::string::npos || lowerStem.find("spanner") != std::string::npos ||
                           lowerStem.find("tool") != std::string::npos) {
                    item.category = AssetCategory::WAREHOUSE;
                    item.iconType = AssetIconType::MODEL_WAREHOUSE;
                } else if (lowerStem.find("tree") != std::string::npos) {
                    item.category = AssetCategory::NATURE;
                    if (lowerStem.find("log") != std::string::npos) item.iconType = AssetIconType::MODEL_LOG;
                    else item.iconType = AssetIconType::MODEL_TREE;
                } else if (lowerStem.find("rock") != std::string::npos || lowerStem.find("stone") != std::string::npos) {
                    item.category = AssetCategory::NATURE;
                    item.iconType = AssetIconType::MODEL_ROCK;
                } else if (lowerStem.find("bush") != std::string::npos || lowerStem.find("foliage") != std::string::npos) {
                    item.category = AssetCategory::NATURE;
                    item.iconType = AssetIconType::MODEL_FOLIAGE;
                } else if (lowerStem.find("log") != std::string::npos) {
                    item.category = AssetCategory::NATURE;
                    item.iconType = AssetIconType::MODEL_LOG;
                } else if (lowerStem.find("ak47") != std::string::npos || lowerStem.find("weapon") != std::string::npos) {
                    item.category = AssetCategory::PROPS;
                    item.iconType = AssetIconType::MODEL_PROP;
                    item.texturePath = "assets/textures/AK_Base_color.png";
                } else {
                    item.category = AssetCategory::PROPS;
                    item.iconType = AssetIconType::MODEL_PROP;
                }

                // Check for matching texture file
                if (item.texturePath.empty()) {
                    if (item.category == AssetCategory::CITY) {
                        if (lowerStem.find("taxi") != std::string::npos && std::filesystem::exists("assets/textures/tex_taxi_body_diffuse.png"))
                            item.texturePath = "assets/textures/tex_taxi_body_diffuse.png";
                        else if (lowerStem.find("cruiser") != std::string::npos && std::filesystem::exists("assets/textures/tex_nypd_cruiser_diffuse.png"))
                            item.texturePath = "assets/textures/tex_nypd_cruiser_diffuse.png";
                        else if (lowerStem.find("traffic_cone") != std::string::npos && std::filesystem::exists("assets/textures/tex_traffic_cone.png"))
                            item.texturePath = "assets/textures/tex_traffic_cone.png";
                        else if (lowerStem.find("barricade") != std::string::npos && std::filesystem::exists("assets/textures/tex_barricade_nypd_beam.png"))
                            item.texturePath = "assets/textures/tex_barricade_nypd_beam.png";
                        else if (lowerStem.find("street_road") != std::string::npos && std::filesystem::exists("assets/textures/tex_nyc_asphalt_road.png"))
                            item.texturePath = "assets/textures/tex_nyc_asphalt_road.png";
                    }

                    if (item.texturePath.empty()) {
                        std::string parentDir = entry.path().parent_path().string();
                        std::string candPng = parentDir + "/" + stem + ".png";
                        std::string candJpg = parentDir + "/" + stem + ".jpg";
                        if (std::filesystem::exists(candPng)) item.texturePath = candPng;
                        else if (std::filesystem::exists(candJpg)) item.texturePath = candJpg;
                        else if (std::filesystem::exists(parentDir + "/Stylized_Industrial_TextureMap_V02.png")) {
                            item.texturePath = parentDir + "/Stylized_Industrial_TextureMap_V02.png";
                        }
                    }
                }

                if (item.category == AssetCategory::TARGETS) targetItems.push_back(item);
                else if (item.category == AssetCategory::ENEMIES) enemyItems.push_back(item);
                else if (item.category == AssetCategory::CITY) cityItems.push_back(item);
                else if (item.category == AssetCategory::WAREHOUSE) warehouseItems.push_back(item);
                else if (item.category == AssetCategory::NATURE) natureItems.push_back(item);
                else propItems.push_back(item);
            }
        }
    }

    // Sort target models so they appear in clean logical order matching New_UI.jpg
    std::vector<std::string> targetOrder = {
        "target_biaso4", "target_biaso7_tripod", "target_biaso7",
        "ipsc_metric_target", "ipsc_target", "ipsc_target_mini", "shootingtarget",
        "pepper_popper", "pepper_popper_mini", "popper",
        "plate_a", "plate_b", "plate_c", "plate_d", "wall_barricade"
    };

    for (const auto& tKey : targetOrder)
    {
        for (auto it = targetItems.begin(); it != targetItems.end(); )
        {
            std::string lStem = it->filename;
            for (auto& c : lStem) c = (char)tolower(c);
            if (lStem.find(tKey) != std::string::npos)
            {
                cachedAssets.push_back(*it);
                it = targetItems.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
    for (const auto& item : targetItems) cachedAssets.push_back(item);

    // Sort city models so they appear in clean logical order (vehicles, street props, infrastructure, buildings)
    std::vector<std::string> cityOrder = {
        "nyc_taxi", "taxi", "nypd_cruiser", "police_car",
        "nyc_lamppost", "nyc_fire_hydrant", "nyc_bus_stop_shelter",
        "nyc_street_sign", "nyc_newsbox_a", "nyc_newsbox_b",
        "nypd_barricade", "police_traffic_cone", "police_cordon_tape",
        "nyc_manhole_cover", "nyc_sewer_grate", "nyc_street_tree",
        "street_road_asphalt", "sidewalk_curb", "waterfront_pier", "waterfront_railing",
        "building_left_near", "building_left_far", "building_right_near", "building_right_far",
        "building_modular_bottom", "building_modular_mid", "building_modular_top",
        "building_5story", "building_6story", "building_7story",
        "building_depth_midrise", "building_depth_small", "building_depth_tower",
        "background_manhattan"
    };

    for (const auto& cKey : cityOrder)
    {
        for (auto it = cityItems.begin(); it != cityItems.end(); )
        {
            std::string lStem = it->filename;
            for (auto& c : lStem) c = (char)tolower(c);
            if (lStem.find(cKey) != std::string::npos)
            {
                cachedAssets.push_back(*it);
                it = cityItems.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
    for (const auto& item : cityItems) cachedAssets.push_back(item);

    // Sort enemy models in logical order
    std::vector<std::string> enemyOrder = {
        "tm_classy_bank_robber_leader",
        "tm_classy_bank_robber",
        "tm_classy_bank_robber2",
        "tm_classy_bank_robber3",
        "tm_classy_black_guy"
    };

    for (const auto& eKey : enemyOrder)
    {
        for (auto it = enemyItems.begin(); it != enemyItems.end(); )
        {
            std::string lStem = it->filename;
            for (auto& c : lStem) c = (char)tolower(c);
            if (lStem.find(eKey) != std::string::npos)
            {
                cachedAssets.push_back(*it);
                it = enemyItems.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
    for (const auto& item : enemyItems) cachedAssets.push_back(item);
    for (const auto& item : animationItems) cachedAssets.push_back(item);

    for (const auto& item : warehouseItems) cachedAssets.push_back(item);
    for (const auto& item : natureItems) cachedAssets.push_back(item);
    for (const auto& item : propItems) cachedAssets.push_back(item);

    // 3. Discover Textures
    if (std::filesystem::exists("assets"))
    {
        for (const auto& entry : std::filesystem::directory_iterator("assets"))
        {
            if (!entry.is_regular_file()) continue;
            std::string ext = entry.path().extension().string();
            for (auto& c : ext) c = (char)tolower(c);
            if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp")
            {
                AssetItem item;
                item.id = "tex_" + entry.path().stem().string();
                item.name = entry.path().stem().string();
                item.path = entry.path().string();
                std::replace(item.path.begin(), item.path.end(), '\\', '/');
                item.filename = entry.path().filename().string();
                item.ext = ext;
                item.displayExt = "TEX";
                item.category = AssetCategory::TEXTURES;
                item.iconType = AssetIconType::TEXTURE;
                item.texturePath = item.path;
                item.fileSize = entry.file_size();
                cachedAssets.push_back(item);
            }
        }
    }

    // 4. Discover Lua Scripts
    if (std::filesystem::exists("assets"))
    {
        for (const auto& entry : std::filesystem::directory_iterator("assets"))
        {
            if (!entry.is_regular_file()) continue;
            std::string ext = entry.path().extension().string();
            for (auto& c : ext) c = (char)tolower(c);
            if (ext == ".lua")
            {
                AssetItem item;
                item.id = "script_" + entry.path().stem().string();
                item.name = entry.path().stem().string();
                item.path = entry.path().string();
                std::replace(item.path.begin(), item.path.end(), '\\', '/');
                item.filename = entry.path().filename().string();
                item.ext = ext;
                item.displayExt = "LUA";
                item.category = AssetCategory::SCRIPTS;
                item.iconType = AssetIconType::SCRIPT;
                item.fileSize = entry.file_size();
                cachedAssets.push_back(item);
            }
        }
    }

    needsInitialScan = false;
}

void AssetBrowserPanel::drawIconSilhouette(ImDrawList* drawList, const ImVec2& minP, const ImVec2& maxP, AssetIconType type)
{
    float cx = (minP.x + maxP.x) * 0.5f;
    float cy = (minP.y + maxP.y) * 0.5f;
    float w = maxP.x - minP.x;
    float h = maxP.y - minP.y;

    switch (type)
    {
    case AssetIconType::TARGET_IPSC_METRIC:
    case AssetIconType::TARGET_IPSC:
    case AssetIconType::TARGET_IPSC_MINI:
    {
        float scale = (type == AssetIconType::TARGET_IPSC_MINI) ? 0.75f : 0.95f;
        // Wooden stand sticks
        drawList->AddRectFilled(ImVec2(cx - 13.0f * scale, cy - 8.0f), ImVec2(cx - 9.0f * scale, maxP.y - 6.0f), IM_COL32(140, 95, 55, 255));
        drawList->AddRectFilled(ImVec2(cx + 9.0f * scale, cy - 8.0f), ImVec2(cx + 13.0f * scale, maxP.y - 6.0f), IM_COL32(140, 95, 55, 255));
        // Base steel feet
        drawList->AddRectFilled(ImVec2(cx - 20.0f * scale, maxP.y - 8.0f), ImVec2(cx + 20.0f * scale, maxP.y - 4.0f), IM_COL32(40, 42, 48, 255), 2.0f);
        drawList->AddRect(ImVec2(cx - 20.0f * scale, maxP.y - 8.0f), ImVec2(cx + 20.0f * scale, maxP.y - 4.0f), IM_COL32(80, 85, 95, 255), 2.0f);

        // Cardboard Torso Polygon
        float hw = 17.0f * scale; // Half width torso
        float tw = 7.0f * scale;  // Half width head
        float yHeadTop = cy - 28.0f * scale;
        float yHeadBot = cy - 18.0f * scale;
        float yShoulder = cy - 12.0f * scale;
        float yTorsoBot = cy + 10.0f * scale;

        ImVec2 pts[8] = {
            ImVec2(cx - tw, yHeadTop),
            ImVec2(cx + tw, yHeadTop),
            ImVec2(cx + tw, yHeadBot),
            ImVec2(cx + hw, yShoulder),
            ImVec2(cx + hw * 0.85f, yTorsoBot),
            ImVec2(cx - hw * 0.85f, yTorsoBot),
            ImVec2(cx - hw, yShoulder),
            ImVec2(cx - tw, yHeadBot)
        };
        drawList->AddConvexPolyFilled(pts, 8, IM_COL32(205, 165, 120, 255));
        drawList->AddPolyline(pts, 8, IM_COL32(120, 85, 50, 255), ImDrawFlags_Closed, 1.6f);

        // Inner scoring zone (A-Zone & C-Zone) lines
        drawList->AddRect(ImVec2(cx - 6.0f * scale, cy - 8.0f * scale), ImVec2(cx + 6.0f * scale, cy + 6.0f * scale), IM_COL32(160, 120, 80, 200), 1.0f);
        drawList->AddRect(ImVec2(cx - 4.0f * scale, yHeadTop + 3.0f * scale), ImVec2(cx + 4.0f * scale, yHeadBot - 2.0f * scale), IM_COL32(160, 120, 80, 200), 1.0f);
        break;
    }

    case AssetIconType::TARGET_POPPER:
    case AssetIconType::TARGET_POPPER_MINI:
    case AssetIconType::TARGET_POPPER_TALL:
    {
        float scale = (type == AssetIconType::TARGET_POPPER_MINI) ? 0.75f : 1.0f;
        // Black steel base
        drawList->AddRectFilled(ImVec2(cx - 18.0f * scale, maxP.y - 8.0f), ImVec2(cx + 18.0f * scale, maxP.y - 4.0f), IM_COL32(35, 37, 42, 255), 2.0f);
        drawList->AddRectFilled(ImVec2(cx - 6.0f * scale, maxP.y - 12.0f), ImVec2(cx + 6.0f * scale, maxP.y - 8.0f), IM_COL32(45, 48, 55, 255));

        // White Steel Body
        float headR = 11.0f * scale;
        float yHead = cy - 14.0f * scale;
        // Bulb Head
        drawList->AddCircleFilled(ImVec2(cx, yHead), headR, IM_COL32(235, 238, 242, 255));
        drawList->AddCircle(ImVec2(cx, yHead), headR, IM_COL32(60, 65, 75, 255), 0, 1.5f);

        // Tapered Stem
        ImVec2 stemPts[4] = {
            ImVec2(cx - 7.0f * scale, yHead + headR * 0.6f),
            ImVec2(cx + 7.0f * scale, yHead + headR * 0.6f),
            ImVec2(cx + 5.0f * scale, maxP.y - 12.0f),
            ImVec2(cx - 5.0f * scale, maxP.y - 12.0f)
        };
        drawList->AddConvexPolyFilled(stemPts, 4, IM_COL32(225, 228, 235, 255));
        drawList->AddPolyline(stemPts, 4, IM_COL32(60, 65, 75, 255), ImDrawFlags_Closed, 1.5f);
        break;
    }

    case AssetIconType::TARGET_PLATE_ROUND:
    case AssetIconType::TARGET_PLATE_ROUND_LOW:
    {
        float scale = (type == AssetIconType::TARGET_PLATE_ROUND_LOW) ? 0.8f : 1.0f;
        float yPlate = (type == AssetIconType::TARGET_PLATE_ROUND_LOW) ? cy + 2.0f : cy - 10.0f;
        // Steel base
        drawList->AddRectFilled(ImVec2(cx - 16.0f, maxP.y - 8.0f), ImVec2(cx + 16.0f, maxP.y - 4.0f), IM_COL32(35, 37, 42, 255), 2.0f);
        // Vertical post
        drawList->AddLine(ImVec2(cx, maxP.y - 8.0f), ImVec2(cx, yPlate + 12.0f * scale), IM_COL32(50, 53, 62, 255), 3.0f);

        // Circular Steel Plate
        float r = 13.0f * scale;
        drawList->AddCircleFilled(ImVec2(cx, yPlate), r, IM_COL32(240, 242, 245, 255));
        drawList->AddCircle(ImVec2(cx, yPlate), r, IM_COL32(60, 65, 75, 255), 0, 1.6f);
        break;
    }

    case AssetIconType::TARGET_PLATE_SQUARE:
    case AssetIconType::TARGET_PLATE_SQUARE_LOW:
    {
        float scale = (type == AssetIconType::TARGET_PLATE_SQUARE_LOW) ? 0.8f : 1.0f;
        float yPlate = (type == AssetIconType::TARGET_PLATE_SQUARE_LOW) ? cy + 2.0f : cy - 10.0f;
        // Steel base
        drawList->AddRectFilled(ImVec2(cx - 16.0f, maxP.y - 8.0f), ImVec2(cx + 16.0f, maxP.y - 4.0f), IM_COL32(35, 37, 42, 255), 2.0f);
        // Vertical post
        drawList->AddLine(ImVec2(cx, maxP.y - 8.0f), ImVec2(cx, yPlate + 12.0f * scale), IM_COL32(50, 53, 62, 255), 3.0f);

        // Square Steel Plate
        float hs = 12.0f * scale;
        drawList->AddRectFilled(ImVec2(cx - hs, yPlate - hs), ImVec2(cx + hs, yPlate + hs), IM_COL32(240, 242, 245, 255), 2.0f);
        drawList->AddRect(ImVec2(cx - hs, yPlate - hs), ImVec2(cx + hs, yPlate + hs), IM_COL32(60, 65, 75, 255), 2.0f, 0, 1.6f);
        break;
    }

    case AssetIconType::TARGET_BARRICADE:
    {
        // Wood feet
        drawList->AddRectFilled(ImVec2(cx - 24.0f, maxP.y - 8.0f), ImVec2(cx - 16.0f, maxP.y - 4.0f), IM_COL32(130, 90, 50, 255));
        drawList->AddRectFilled(ImVec2(cx + 16.0f, maxP.y - 8.0f), ImVec2(cx + 24.0f, maxP.y - 4.0f), IM_COL32(130, 90, 50, 255));
        // Plywood main panel
        drawList->AddRectFilled(ImVec2(cx - 26.0f, cy - 22.0f), ImVec2(cx + 26.0f, maxP.y - 8.0f), IM_COL32(185, 145, 100, 255), 2.0f);
        drawList->AddRect(ImVec2(cx - 26.0f, cy - 22.0f), ImVec2(cx + 26.0f, maxP.y - 8.0f), IM_COL32(100, 70, 40, 255), 2.0f, 0, 1.5f);
        // Wood framing rails
        drawList->AddLine(ImVec2(cx - 26.0f, cy - 8.0f), ImVec2(cx + 26.0f, cy - 8.0f), IM_COL32(130, 90, 50, 255), 2.0f);
        drawList->AddLine(ImVec2(cx, cy - 22.0f), ImVec2(cx, maxP.y - 8.0f), IM_COL32(130, 90, 50, 255), 2.0f);
        break;
    }

    case AssetIconType::TARGET_GENERIC:
    {
        // Tripod legs
        drawList->AddLine(ImVec2(cx, cy + 6.0f), ImVec2(cx - 16.0f, maxP.y - 6.0f), IM_COL32(70, 75, 85, 255), 2.0f);
        drawList->AddLine(ImVec2(cx, cy + 6.0f), ImVec2(cx + 16.0f, maxP.y - 6.0f), IM_COL32(70, 75, 85, 255), 2.0f);
        drawList->AddLine(ImVec2(cx, cy + 6.0f), ImVec2(cx, maxP.y - 6.0f), IM_COL32(90, 95, 105, 255), 2.0f);

        // Concentric target circles
        float r = 18.0f;
        drawList->AddCircleFilled(ImVec2(cx, cy - 8.0f), r, IM_COL32(245, 245, 245, 255));
        drawList->AddCircleFilled(ImVec2(cx, cy - 8.0f), r * 0.75f, IM_COL32(40, 40, 45, 255));
        drawList->AddCircleFilled(ImVec2(cx, cy - 8.0f), r * 0.5f, IM_COL32(230, 60, 50, 255));
        drawList->AddCircleFilled(ImVec2(cx, cy - 8.0f), r * 0.25f, IM_COL32(245, 195, 35, 255));
        break;
    }

    case AssetIconType::MODEL_CHARACTER:
    {
        // Dark tactical silhouette: Head and torso
        float rHead = 10.0f;
        float yHead = cy - 10.0f;
        drawList->AddCircleFilled(ImVec2(cx, yHead), rHead, IM_COL32(230, 80, 80, 240));
        drawList->AddCircle(ImVec2(cx, yHead), rHead, IM_COL32(255, 255, 255, 200), 0, 1.5f);

        // Shoulders / torso trapezoid
        ImVec2 torsoPts[4] = {
            ImVec2(cx - 7.0f, yHead + rHead * 0.8f),
            ImVec2(cx + 7.0f, yHead + rHead * 0.8f),
            ImVec2(cx + 18.0f, maxP.y - 8.0f),
            ImVec2(cx - 18.0f, maxP.y - 8.0f)
        };
        drawList->AddConvexPolyFilled(torsoPts, 4, IM_COL32(180, 45, 45, 230));
        drawList->AddPolyline(torsoPts, 4, IM_COL32(255, 120, 120, 220), ImDrawFlags_Closed, 1.5f);
        break;
    }

    case AssetIconType::MODEL_TREE:
    {
        // Trunk
        drawList->AddRectFilled(ImVec2(cx - 3.0f, cy + 8.0f), ImVec2(cx + 3.0f, maxP.y - 8.0f), IM_COL32(110, 75, 45, 255));
        // Pine layers
        drawList->AddTriangleFilled(ImVec2(cx, cy - 25.0f), ImVec2(cx - 13.0f, cy - 8.0f), ImVec2(cx + 13.0f, cy - 8.0f), IM_COL32(46, 125, 50, 255));
        drawList->AddTriangleFilled(ImVec2(cx, cy - 14.0f), ImVec2(cx - 17.0f, cy + 2.0f), ImVec2(cx + 17.0f, cy + 2.0f), IM_COL32(40, 110, 45, 255));
        drawList->AddTriangleFilled(ImVec2(cx, cy - 4.0f), ImVec2(cx - 21.0f, cy + 12.0f), ImVec2(cx + 21.0f, cy + 12.0f), IM_COL32(35, 95, 40, 255));
        break;
    }

    case AssetIconType::MODEL_ROCK:
    {
        ImVec2 pts[6] = {
            ImVec2(cx - 20.0f, cy + 10.0f),
            ImVec2(cx - 15.0f, cy - 10.0f),
            ImVec2(cx + 2.0f, cy - 18.0f),
            ImVec2(cx + 18.0f, cy - 6.0f),
            ImVec2(cx + 16.0f, cy + 12.0f),
            ImVec2(cx - 6.0f, cy + 14.0f)
        };
        drawList->AddConvexPolyFilled(pts, 6, IM_COL32(120, 135, 145, 255));
        drawList->AddPolyline(pts, 6, IM_COL32(65, 75, 85, 255), ImDrawFlags_Closed, 1.5f);
        drawList->AddLine(ImVec2(cx - 15.0f, cy - 10.0f), ImVec2(cx - 2.0f, cy + 2.0f), IM_COL32(160, 175, 185, 255), 1.2f);
        drawList->AddLine(ImVec2(cx + 2.0f, cy - 18.0f), ImVec2(cx - 2.0f, cy + 2.0f), IM_COL32(90, 105, 115, 255), 1.2f);
        drawList->AddLine(ImVec2(cx + 18.0f, cy - 6.0f), ImVec2(cx - 2.0f, cy + 2.0f), IM_COL32(75, 85, 95, 255), 1.2f);
        break;
    }

    case AssetIconType::MODEL_LOG:
    {
        // Stack of 3 logs
        drawList->AddRectFilled(ImVec2(cx - 20.0f, cy + 2.0f), ImVec2(cx + 15.0f, cy + 12.0f), IM_COL32(130, 88, 50, 255), 3.0f);
        drawList->AddCircleFilled(ImVec2(cx + 15.0f, cy + 7.0f), 5.0f, IM_COL32(200, 160, 120, 255));

        drawList->AddRectFilled(ImVec2(cx - 16.0f, cy - 8.0f), ImVec2(cx + 18.0f, cy + 2.0f), IM_COL32(115, 75, 42, 255), 3.0f);
        drawList->AddCircleFilled(ImVec2(cx + 18.0f, cy - 3.0f), 5.0f, IM_COL32(190, 150, 110, 255));
        break;
    }

    case AssetIconType::MODEL_FOLIAGE:
    {
        drawList->AddCircleFilled(ImVec2(cx - 8.0f, cy + 2.0f), 12.0f, IM_COL32(55, 135, 60, 255));
        drawList->AddCircleFilled(ImVec2(cx + 8.0f, cy + 4.0f), 11.0f, IM_COL32(45, 120, 50, 255));
        drawList->AddCircleFilled(ImVec2(cx, cy - 6.0f), 13.0f, IM_COL32(65, 155, 70, 255));
        break;
    }

    case AssetIconType::PRIMITIVE_CUBE:
    {
        // 3D Isometric Cube
        ImVec2 top[4] = { ImVec2(cx, cy - 16.0f), ImVec2(cx + 15.0f, cy - 8.0f), ImVec2(cx, cy), ImVec2(cx - 15.0f, cy - 8.0f) };
        ImVec2 left[4] = { ImVec2(cx - 15.0f, cy - 8.0f), ImVec2(cx, cy), ImVec2(cx, cy + 16.0f), ImVec2(cx - 15.0f, cy + 8.0f) };
        ImVec2 right[4] = { ImVec2(cx, cy), ImVec2(cx + 15.0f, cy - 8.0f), ImVec2(cx + 15.0f, cy + 8.0f), ImVec2(cx, cy + 16.0f) };

        drawList->AddConvexPolyFilled(top, 4, IM_COL32(130, 185, 245, 255));
        drawList->AddConvexPolyFilled(left, 4, IM_COL32(70, 130, 215, 255));
        drawList->AddConvexPolyFilled(right, 4, IM_COL32(45, 95, 180, 255));

        drawList->AddPolyline(top, 4, IM_COL32(20, 50, 100, 255), ImDrawFlags_Closed, 1.2f);
        drawList->AddPolyline(left, 4, IM_COL32(20, 50, 100, 255), ImDrawFlags_Closed, 1.2f);
        drawList->AddPolyline(right, 4, IM_COL32(20, 50, 100, 255), ImDrawFlags_Closed, 1.2f);
        break;
    }

    case AssetIconType::PRIMITIVE_SPHERE:
    {
        drawList->AddCircleFilled(ImVec2(cx, cy), 16.0f, IM_COL32(230, 120, 80, 255));
        drawList->AddCircleFilled(ImVec2(cx - 5.0f, cy - 5.0f), 7.0f, IM_COL32(255, 180, 140, 255));
        drawList->AddCircle(ImVec2(cx, cy), 16.0f, IM_COL32(160, 70, 40, 255), 0, 1.5f);
        break;
    }

    case AssetIconType::PRIMITIVE_PLANE:
    {
        ImVec2 planePts[4] = {
            ImVec2(cx, cy - 12.0f),
            ImVec2(cx + 22.0f, cy),
            ImVec2(cx, cy + 12.0f),
            ImVec2(cx - 22.0f, cy)
        };
        drawList->AddConvexPolyFilled(planePts, 4, IM_COL32(100, 180, 140, 255));
        drawList->AddPolyline(planePts, 4, IM_COL32(50, 120, 90, 255), ImDrawFlags_Closed, 1.5f);
        break;
    }

    case AssetIconType::MODEL_WAREHOUSE:
    {
        // Warehouse storage rack / shelving silhouette
        // Upright columns
        drawList->AddRectFilled(ImVec2(cx - 20.0f, cy - 22.0f), ImVec2(cx - 16.0f, cy + 22.0f), IM_COL32(230, 110, 30, 255));
        drawList->AddRectFilled(ImVec2(cx + 16.0f, cy - 22.0f), ImVec2(cx + 20.0f, cy + 22.0f), IM_COL32(230, 110, 30, 255));
        // Horizontal rack beams
        drawList->AddRectFilled(ImVec2(cx - 20.0f, cy - 14.0f), ImVec2(cx + 20.0f, cy - 10.0f), IM_COL32(245, 166, 35, 255));
        drawList->AddRectFilled(ImVec2(cx - 20.0f, cy + 4.0f), ImVec2(cx + 20.0f, cy + 8.0f), IM_COL32(245, 166, 35, 255));
        drawList->AddRectFilled(ImVec2(cx - 20.0f, cy + 18.0f), ImVec2(cx + 20.0f, cy + 22.0f), IM_COL32(75, 80, 92, 255));
        // Goods / Boxes on shelves
        drawList->AddRectFilled(ImVec2(cx - 13.0f, cy - 10.0f), ImVec2(cx - 1.0f, cy + 4.0f), IM_COL32(175, 130, 75, 255), 1.0f);
        drawList->AddRectFilled(ImVec2(cx + 2.0f, cy - 6.0f), ImVec2(cx + 13.0f, cy + 4.0f), IM_COL32(145, 105, 60, 255), 1.0f);
        break;
    }

    case AssetIconType::MODEL_BUILDING:
    {
        // High-rise city building silhouette with illuminated windows
        drawList->AddRectFilled(ImVec2(cx - 16.0f, cy - 22.0f), ImVec2(cx + 16.0f, cy + 22.0f), IM_COL32(45, 65, 88, 255), 2.0f);
        drawList->AddRect(ImVec2(cx - 16.0f, cy - 22.0f), ImVec2(cx + 16.0f, cy + 22.0f), IM_COL32(80, 115, 150, 255), 2.0f, 0, 1.2f);
        // Rooftop antenna / spire
        drawList->AddLine(ImVec2(cx, cy - 22.0f), ImVec2(cx, cy - 28.0f), IM_COL32(180, 205, 230, 255), 1.5f);
        // Windows grid
        for (int row = -2; row <= 2; ++row) {
            float wy = cy + row * 8.0f;
            drawList->AddRectFilled(ImVec2(cx - 11.0f, wy - 2.0f), ImVec2(cx - 5.0f, wy + 2.0f), IM_COL32(245, 215, 110, 230), 1.0f);
            drawList->AddRectFilled(ImVec2(cx - 2.0f, wy - 2.0f), ImVec2(cx + 4.0f, wy + 2.0f), IM_COL32(180, 220, 255, 230), 1.0f);
            drawList->AddRectFilled(ImVec2(cx + 7.0f, wy - 2.0f), ImVec2(cx + 11.0f, wy + 2.0f), IM_COL32(245, 215, 110, 230), 1.0f);
        }
        break;
    }

    case AssetIconType::MODEL_VEHICLE:
    {
        // City vehicle (cab/cruiser) silhouette
        ImVec2 roofPts[4] = {
            ImVec2(cx - 11.0f, cy - 8.0f),
            ImVec2(cx + 9.0f, cy - 8.0f),
            ImVec2(cx + 18.0f, cy + 3.0f),
            ImVec2(cx - 18.0f, cy + 3.0f)
        };
        drawList->AddConvexPolyFilled(roofPts, 4, IM_COL32(235, 180, 45, 255));
        drawList->AddRectFilled(ImVec2(cx - 22.0f, cy + 3.0f), ImVec2(cx + 22.0f, cy + 13.0f), IM_COL32(215, 155, 30, 255), 2.0f);
        // Wheels
        drawList->AddCircleFilled(ImVec2(cx - 13.0f, cy + 14.0f), 5.0f, IM_COL32(30, 32, 38, 255));
        drawList->AddCircleFilled(ImVec2(cx - 13.0f, cy + 14.0f), 2.0f, IM_COL32(170, 175, 185, 255));
        drawList->AddCircleFilled(ImVec2(cx + 13.0f, cy + 14.0f), 5.0f, IM_COL32(30, 32, 38, 255));
        drawList->AddCircleFilled(ImVec2(cx + 13.0f, cy + 14.0f), 2.0f, IM_COL32(170, 175, 185, 255));
        // Windshield & windows
        drawList->AddRectFilled(ImVec2(cx - 8.0f, cy - 6.0f), ImVec2(cx + 6.0f, cy + 1.0f), IM_COL32(65, 130, 195, 230), 1.0f);
        // Taxi roof sign / police siren
        drawList->AddRectFilled(ImVec2(cx - 4.0f, cy - 12.0f), ImVec2(cx + 4.0f, cy - 8.0f), IM_COL32(255, 235, 120, 255), 1.0f);
        break;
    }

    case AssetIconType::MODEL_STREET_PROP:
    {
        // Street lamp & furniture silhouette
        drawList->AddRectFilled(ImVec2(cx - 2.0f, cy - 16.0f), ImVec2(cx + 2.0f, cy + 18.0f), IM_COL32(140, 150, 165, 255));
        drawList->AddRectFilled(ImVec2(cx - 6.0f, cy + 16.0f), ImVec2(cx + 6.0f, cy + 20.0f), IM_COL32(95, 105, 120, 255), 1.0f);
        // Curved overhanging arm
        drawList->AddLine(ImVec2(cx, cy - 16.0f), ImVec2(cx + 13.0f, cy - 20.0f), IM_COL32(140, 150, 165, 255), 2.5f);
        // Glowing lantern head
        drawList->AddTriangleFilled(ImVec2(cx + 13.0f, cy - 22.0f), ImVec2(cx + 8.0f, cy - 16.0f), ImVec2(cx + 18.0f, cy - 16.0f), IM_COL32(245, 205, 60, 255));
        drawList->AddCircleFilled(ImVec2(cx + 13.0f, cy - 15.0f), 3.0f, IM_COL32(255, 255, 210, 255));
        break;
    }

    case AssetIconType::ANIMATION_CLIP:
    {
        // Film clapper / Runner silhouette with cyan & teal styling
        drawList->AddRectFilled(ImVec2(cx - 18.0f, cy - 14.0f), ImVec2(cx + 18.0f, cy + 14.0f), IM_COL32(25, 38, 55, 255), 3.0f);
        drawList->AddRect(ImVec2(cx - 18.0f, cy - 14.0f), ImVec2(cx + 18.0f, cy + 14.0f), IM_COL32(50, 185, 230, 255), 3.0f, 0, 1.5f);
        // Play triangle
        ImVec2 triPts[3] = {
            ImVec2(cx - 5.0f, cy - 8.0f),
            ImVec2(cx + 8.0f, cy),
            ImVec2(cx - 5.0f, cy + 8.0f)
        };
        drawList->AddConvexPolyFilled(triPts, 3, IM_COL32(50, 225, 165, 255));
        // Film sprocket notches
        for (int sp = -2; sp <= 2; ++sp) {
            drawList->AddRectFilled(ImVec2(cx + sp * 7.0f - 1.5f, cy - 12.0f), ImVec2(cx + sp * 7.0f + 1.5f, cy - 9.0f), IM_COL32(180, 220, 255, 200));
            drawList->AddRectFilled(ImVec2(cx + sp * 7.0f - 1.5f, cy + 9.0f), ImVec2(cx + sp * 7.0f + 1.5f, cy + 12.0f), IM_COL32(180, 220, 255, 200));
        }
        break;
    }

    default:
    {
        // Generic asset icon
        drawList->AddRectFilled(ImVec2(cx - 14.0f, cy - 16.0f), ImVec2(cx + 14.0f, cy + 16.0f), IM_COL32(70, 75, 90, 255), 3.0f);
        drawList->AddRect(ImVec2(cx - 14.0f, cy - 16.0f), ImVec2(cx + 14.0f, cy + 16.0f), IM_COL32(110, 115, 135, 255), 3.0f);
        break;
    }
    }
}

void AssetBrowserPanel::renderAssetCard(const AssetItem& item, AssetManager* assetManager, Scene* scene, float cardWidth, float cardHeight)
{
    ImGui::PushID(item.id.c_str());
    ImGui::BeginGroup();

    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    bool isSelected = (selectedAssetId == item.id);
    bool isHovered = ImGui::IsMouseHoveringRect(p, ImVec2(p.x + cardWidth, p.y + cardHeight));

    // Card Background & Border (Matching New_UI.jpg dark neutral tones with orange/gold accent)
    ImU32 bgCol = isSelected ? IM_COL32(40, 44, 56, 255) : (isHovered ? IM_COL32(36, 39, 48, 255) : IM_COL32(26, 28, 34, 255));
    ImU32 borderCol = isSelected ? IM_COL32(245, 166, 35, 255) : (isHovered ? IM_COL32(245, 166, 35, 200) : IM_COL32(48, 52, 62, 255));
    float borderThickness = (isSelected || isHovered) ? 1.6f : 1.0f;

    drawList->AddRectFilled(p, ImVec2(p.x + cardWidth, p.y + cardHeight), bgCol, 4.0f);
    drawList->AddRect(p, ImVec2(p.x + cardWidth, p.y + cardHeight), borderCol, 4.0f, 0, borderThickness);

    // Thumbnail Recessed Box
    ImVec2 thumbMin(p.x + 6.0f, p.y + 6.0f);
    ImVec2 thumbMax(p.x + cardWidth - 6.0f, p.y + cardHeight - 26.0f);
    drawList->AddRectFilled(thumbMin, thumbMax, IM_COL32(18, 20, 24, 255), 3.0f);
    drawList->AddRect(thumbMin, thumbMax, IM_COL32(38, 42, 50, 255), 3.0f, 0, 1.0f);

    // Draw Thumbnail Content
    auto& thumbnails = assetManager->getThumbnails();
    bool renderedTexture = false;

    if (!item.texturePath.empty())
    {
        if (thumbnails.find(item.texturePath) == thumbnails.end())
        {
            try {
                Texture tex;
                assetManager->loadTexture(item.texturePath, tex);
                thumbnails[item.texturePath] = tex;
            } catch (...) {}
        }

        if (thumbnails.find(item.texturePath) != thumbnails.end())
        {
            float thumbW = thumbMax.x - thumbMin.x;
            float thumbH = thumbMax.y - thumbMin.y;
            drawList->AddImage((ImTextureID)thumbnails[item.texturePath].descriptorSet,
                               ImVec2(thumbMin.x + 2.0f, thumbMin.y + 2.0f),
                               ImVec2(thumbMax.x - 2.0f, thumbMax.y - 2.0f));
            renderedTexture = true;
        }
    }

    if (!renderedTexture)
    {
        drawIconSilhouette(drawList, thumbMin, thumbMax, item.iconType);
    }

    // Format Badge with Distinct Format Colors (OBJ: Cyan, GLB: Green, FBX: Purple, etc.)
    if (!item.displayExt.empty())
    {
        ImU32 badgeBg = IM_COL32(20, 24, 30, 220);
        ImU32 badgeText = IM_COL32(180, 195, 210, 255);
        ImU32 badgeBorder = IM_COL32(50, 55, 68, 200);

        if (item.displayExt == "OBJ") {
            badgeBg = IM_COL32(16, 42, 70, 230);
            badgeText = IM_COL32(80, 195, 255, 255);
            badgeBorder = IM_COL32(40, 120, 200, 220);
        } else if (item.displayExt == "GLB" || item.displayExt == "GLTF") {
            badgeBg = IM_COL32(14, 52, 30, 230);
            badgeText = IM_COL32(75, 230, 140, 255);
            badgeBorder = IM_COL32(35, 160, 85, 220);
        } else if (item.displayExt == "FBX") {
            badgeBg = IM_COL32(55, 22, 75, 230);
            badgeText = IM_COL32(225, 130, 255, 255);
            badgeBorder = IM_COL32(150, 60, 210, 220);
        } else if (item.displayExt == "PRIM") {
            badgeBg = IM_COL32(65, 45, 12, 230);
            badgeText = IM_COL32(255, 195, 60, 255);
            badgeBorder = IM_COL32(190, 130, 30, 220);
        } else if (item.displayExt == "TEX" || item.displayExt == "PNG" || item.displayExt == "JPG") {
            badgeBg = IM_COL32(15, 50, 60, 230);
            badgeText = IM_COL32(80, 220, 220, 255);
            badgeBorder = IM_COL32(40, 160, 170, 220);
        } else if (item.displayExt == "LUA") {
            badgeBg = IM_COL32(30, 40, 65, 230);
            badgeText = IM_COL32(120, 170, 255, 255);
            badgeBorder = IM_COL32(65, 110, 200, 220);
        }

        ImVec2 txtSz = ImGui::CalcTextSize(item.displayExt.c_str());
        ImVec2 badgeMin(thumbMin.x + 3.0f, thumbMin.y + 3.0f);
        ImVec2 badgeMax(badgeMin.x + txtSz.x + 8.0f, badgeMin.y + 13.0f);
        drawList->AddRectFilled(badgeMin, badgeMax, badgeBg, 3.0f);
        drawList->AddRect(badgeMin, badgeMax, badgeBorder, 3.0f, 0, 1.0f);
        drawList->AddText(ImGui::GetFont(), 10.0f, ImVec2(badgeMin.x + 4.0f, badgeMin.y + 1.0f), badgeText, item.displayExt.c_str());
    }

    // Bottom Label Area
    std::string displayName = item.name;
    if (displayName.size() > 14) displayName = displayName.substr(0, 12) + "..";

    ImVec2 labelPos(p.x + 6.0f, p.y + cardHeight - 20.0f);
    drawList->AddText(labelPos, isHovered ? IM_COL32(255, 255, 255, 255) : IM_COL32(190, 195, 205, 255), displayName.c_str());

    // Invisible Button for Click & Drag Events
    ImGui::Dummy(ImVec2(cardWidth, cardHeight));
    ImGui::SetCursorScreenPos(p);
    ImGui::Selectable("##CardBtn", isSelected, 0, ImVec2(cardWidth, cardHeight));

    if (ImGui::IsItemClicked())
    {
        selectedAssetId = item.id;
    }

    // Drag-and-Drop Source
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
    {
        if (item.isPrimitive)
        {
            ImGui::SetDragDropPayload("DND_ASSET_MODEL", item.primitivePayload.c_str(), item.primitivePayload.size() + 1);
            ImGui::Text("🎲 Dragging '%s'\nDrop into Scene View to place!", item.name.c_str());
        }
        else if (item.category == AssetCategory::TEXTURES)
        {
            ImGui::SetDragDropPayload("DND_ASSET_TEXTURE", item.path.c_str(), item.path.size() + 1);
            ImGui::Text("🖼️ Dragging Texture '%s'\nDrop onto Object to apply!", item.name.c_str());
        }
        else if (item.category == AssetCategory::SCRIPTS)
        {
            ImGui::SetDragDropPayload("DND_ASSET_LUA", item.path.c_str(), item.path.size() + 1);
            ImGui::Text("📜 Dragging Script '%s'\nDrop to attach!", item.name.c_str());
        }
        else if (item.category == AssetCategory::ANIMATIONS)
        {
            ImGui::SetDragDropPayload("DND_ASSET_ANIM", item.path.c_str(), item.path.size() + 1);
            ImGui::Text("🎬 Dragging Animation '%s'\nDrop onto Character with Animator to attach & play!", item.name.c_str());
        }
        else
        {
            ImGui::SetDragDropPayload("DND_ASSET_MODEL", item.path.c_str(), item.path.size() + 1);
            ImGui::Text("🎯 Dragging 3D Model '%s'\nDrop into Scene View to place target!", item.name.c_str());
        }
        ImGui::EndDragDropSource();
    }

    // Double-Click: Instant Spawning or Animation Application
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
    {
        if (item.isPrimitive)
        {
            scene->saveHistory();
            SceneObject newObj;
            newObj.id = static_cast<int>(scene->getObjects().size());
            newObj.name = item.name;
            newObj.position = glm::vec3(0.0f, 0.5f, 0.0f);
            newObj.scale = glm::vec3(1.0f);
            newObj.color = glm::vec4(1.0f);
            if (item.primitivePayload == "PRIMITIVE_CUBE") { newObj.type = ObjectType::CUBE; newObj.meshId = assetManager->getCubeMeshId(); }
            else if (item.primitivePayload == "PRIMITIVE_SPHERE") { newObj.type = ObjectType::SPHERE; newObj.meshId = assetManager->getSphereMeshId(); }
            else if (item.primitivePayload == "PRIMITIVE_PLANE") { newObj.type = ObjectType::PLANE; newObj.meshId = assetManager->getPlaneMeshId(); }
            newObj.syncComponents();
            scene->getObjects().push_back(newObj);
            scene->setSelectedObjectIndex(static_cast<int>(scene->getObjects().size()) - 1);
        }
        else if (item.category == AssetCategory::ANIMATIONS)
        {
            auto selectedObj = scene->getSelectedObject();
            if (selectedObj)
            {
                selectedObj->autoAttachAnimatorIfSkinned(assetManager->getMeshes());
                auto animComp = selectedObj->getAnimator();
                if (animComp)
                {
                    auto clip = AnimationLoader::loadFBXClip(item.path, animComp->skeleton.get());
                    if (clip)
                    {
                        animComp->addClip(clip);
                        animComp->play(clip->name, true);
                    }
                }
            }
        }
        else if (item.category != AssetCategory::TEXTURES && item.category != AssetCategory::SCRIPTS)
        {
            scene->saveHistory();
            int meshId = assetManager->load3DModelAsset(item.path);
            if (meshId >= 0)
            {
                SceneObject newObj;
                newObj.id = static_cast<int>(scene->getObjects().size());
                newObj.name = item.name;
                newObj.type = ObjectType::CUBE;

                // Spawn at origin or centered front view
                newObj.position = glm::vec3(0.0f, 0.0f, 0.0f);
                newObj.scale = glm::vec3(1.0f);
                newObj.rotation = (item.category == AssetCategory::TARGETS) ? glm::vec3(0.0f, 180.0f, 0.0f) : glm::vec3(0.0f);
                newObj.color = glm::vec4(1.0f);
                newObj.meshId = meshId;
                newObj.modelPath = item.path;

                const auto& meshes = assetManager->getMeshes();
                bool isMultiMaterial = false;
                if (meshId < static_cast<int>(meshes.size()))
                {
                    isMultiMaterial = (meshes[meshId].submeshes.size() > 1);
                    newObj.textureId = isMultiMaterial ? -1 : meshes[meshId].defaultTextureId;
                    newObj.roughness = meshes[meshId].defaultRoughness;
                    newObj.metallic = meshes[meshId].defaultMetallic;
                }

                if (!item.texturePath.empty() && !isMultiMaterial)
                {
                    int texId = assetManager->getOrLoadTextureAsset(item.texturePath);
                    if (texId >= 0) {
                        newObj.textureId = texId;
                        newObj.texturePath = item.texturePath;
                    }
                }

                newObj.syncComponents();
                newObj.autoAttachAnimatorIfSkinned(assetManager->getMeshes());
                scene->getObjects().push_back(newObj);
                scene->setSelectedObjectIndex(static_cast<int>(scene->getObjects().size()) - 1);
            }
        }
    }

    // Context Menu
    if (ImGui::BeginPopupContextItem())
    {
        ImGui::TextDisabled("Asset: %s", item.name.c_str());
        ImGui::Separator();

        if (item.category == AssetCategory::ANIMATIONS)
        {
            if (ImGui::MenuItem("🏃 Apply Animation to Selected Character"))
            {
                auto selectedObj = scene->getSelectedObject();
                if (selectedObj)
                {
                    selectedObj->autoAttachAnimatorIfSkinned(assetManager->getMeshes());
                    auto animComp = selectedObj->getAnimator();
                    if (animComp)
                    {
                        auto clip = AnimationLoader::loadFBXClip(item.path, animComp->skeleton.get());
                        if (clip)
                        {
                            animComp->addClip(clip);
                            animComp->play(clip->name, true);
                        }
                    }
                }
            }
        }
        else if (ImGui::MenuItem("➕ Spawn in Scene (At Origin)"))
        {
            int meshId = assetManager->load3DModelAsset(item.path);
            if (meshId >= 0)
            {
                scene->saveHistory();
                SceneObject newObj;
                newObj.id = static_cast<int>(scene->getObjects().size());
                newObj.name = item.name;
                newObj.type = ObjectType::CUBE;
                newObj.position = glm::vec3(0.0f);
                newObj.scale = glm::vec3(1.0f);
                newObj.rotation = (item.category == AssetCategory::TARGETS) ? glm::vec3(0.0f, 180.0f, 0.0f) : glm::vec3(0.0f);
                newObj.color = glm::vec4(1.0f);
                newObj.meshId = meshId;
                newObj.modelPath = item.path;
                const auto& curMeshes = assetManager->getMeshes();
                bool isMultiMatDnd = (meshId >= 0 && meshId < static_cast<int>(curMeshes.size()) && curMeshes[meshId].submeshes.size() > 1);
                if (meshId >= 0 && meshId < static_cast<int>(curMeshes.size())) {
                    newObj.textureId = isMultiMatDnd ? -1 : curMeshes[meshId].defaultTextureId;
                    newObj.roughness = curMeshes[meshId].defaultRoughness;
                    newObj.metallic = curMeshes[meshId].defaultMetallic;
                }
                if (!item.texturePath.empty() && !isMultiMatDnd) {
                    int texId = assetManager->getOrLoadTextureAsset(item.texturePath);
                    if (texId >= 0) {
                        newObj.textureId = texId;
                        newObj.texturePath = item.texturePath;
                    }
                }
                newObj.syncComponents();
                newObj.autoAttachAnimatorIfSkinned(assetManager->getMeshes());
                scene->getObjects().push_back(newObj);
                scene->setSelectedObjectIndex(static_cast<int>(scene->getObjects().size()) - 1);
            }
        }

        if (!item.path.empty() && item.path.find("PRIMITIVE_") == std::string::npos)
        {
            if (ImGui::MenuItem("📂 Show in Windows Explorer"))
            {
                openFileLocationInExplorer(item.path);
            }
            if (ImGui::MenuItem("📋 Copy File Path"))
            {
                ImGui::SetClipboardText(item.path.c_str());
            }
        }
        ImGui::EndPopup();
    }

    // Hover Tooltip
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextColored(ImVec4(0.96f, 0.65f, 0.14f, 1.0f), "%s", item.name.c_str());
        if (!item.path.empty() && item.path.find("PRIMITIVE_") == std::string::npos) {
            ImGui::TextDisabled("Path: %s", item.path.c_str());
            if (item.fileSize > 0) {
                float sizeMB = item.fileSize / (1024.0f * 1024.0f);
                if (sizeMB >= 1.0f) ImGui::TextDisabled("Size: %.2f MB", sizeMB);
                else ImGui::TextDisabled("Size: %.1f KB", item.fileSize / 1024.0f);
            }
        }
        ImGui::Separator();
        if (item.category == AssetCategory::ANIMATIONS) {
            ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.4f, 1.0f), "🖱️ Double-click: Apply to Selected Character");
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "✋ Drag & Drop: Drop onto Character to Play");
        } else {
            ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.4f, 1.0f), "🖱️ Double-click: Spawn in Scene");
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "✋ Drag & Drop: Place into Viewport");
        }
        ImGui::EndTooltip();
    }

    ImGui::EndGroup();
    ImGui::PopID();
}

void AssetBrowserPanel::draw(AssetManager* assetManager, Scene* scene, float windowWidth, float bottomPanelHeight)
{
    if (!showAssetBrowserPanel || !assetManager || !scene) return;

    if (needsInitialScan)
    {
        refreshAssetRegistry(assetManager);
    }

    // Docked at bottom of viewport
    ImGui::SetNextWindowPos(ImVec2(0.0f, ImGui::GetIO().DisplaySize.y - bottomPanelHeight), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(windowWidth, bottomPanelHeight), ImGuiCond_Always);

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 6.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.11f, 0.12f, 0.15f, 0.96f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.22f, 0.24f, 0.30f, 1.0f));

    if (ImGui::Begin("##AssetDrawerPanel", &showAssetBrowserPanel, windowFlags))
    {
        // 1. TOP HEADER & CATEGORY TABS (Styled exactly like New_UI.jpg)
        ImGui::BeginGroup();

        auto renderCategoryBtn = [&](const char* label, AssetCategory cat) {
            bool active = (selectedCategory == cat);
            if (active) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.96f, 0.65f, 0.14f, 0.9f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.20f, 0.25f, 0.8f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.75f, 0.78f, 0.85f, 1.0f));
            }
            if (ImGui::Button(label)) {
                selectedCategory = cat;
            }
            ImGui::PopStyleColor(2);
            ImGui::SameLine();
        };

        renderCategoryBtn("🎯 TARGETS", AssetCategory::TARGETS);
        renderCategoryBtn("👤 ENEMIES", AssetCategory::ENEMIES);
        renderCategoryBtn("🎬 ANIMATIONS", AssetCategory::ANIMATIONS);
        renderCategoryBtn("🏙️ THÀNH PHỐ", AssetCategory::CITY);
        renderCategoryBtn("🏭 WAREHOUSE", AssetCategory::WAREHOUSE);
        renderCategoryBtn("📦 ALL 3D (OBJ/GLB)", AssetCategory::ALL_MODELS);
        renderCategoryBtn("🌲 NATURE", AssetCategory::NATURE);
        renderCategoryBtn("🏺 PROPS", AssetCategory::PROPS);
        renderCategoryBtn("🎲 PRIMITIVES", AssetCategory::PRIMITIVES);
        renderCategoryBtn("🖼️ TEXTURES", AssetCategory::TEXTURES);
        renderCategoryBtn("📜 SCRIPTS", AssetCategory::SCRIPTS);

        // Right side controls: Search filter, item count, refresh, and close button (TARGETS X)
        float availRight = ImGui::GetContentRegionAvail().x;
        float searchWidth = 140.0f;
        float rightWidgetsWidth = searchWidth + 120.0f;

        if (availRight > rightWidgetsWidth)
        {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + availRight - rightWidgetsWidth);
        }

        ImGui::PushItemWidth(searchWidth);
        ImGui::InputTextWithHint("##SearchFilter", "🔍 Filter...", searchFilter, IM_ARRAYSIZE(searchFilter));
        ImGui::PopItemWidth();
        ImGui::SameLine();

        if (ImGui::Button("🔄"))
        {
            refreshAssetRegistry(assetManager);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Rescan assets folder");
        ImGui::SameLine();

        // Close drawer button (matching TARGETS X in New_UI.jpg)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.15f, 0.15f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button(" X "))
        {
            showAssetBrowserPanel = false;
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Close / Collapse Target Drawer");
        ImGui::PopStyleColor(2);

        ImGui::EndGroup();
        ImGui::Separator();

        // 2. HORIZONTAL ASSET DRAWER ROW
        float cardWidth = 104.0f;
        float cardHeight = 118.0f;

        std::string filterStr = searchFilter;
        std::transform(filterStr.begin(), filterStr.end(), filterStr.begin(), ::tolower);

        // Child window with horizontal scrollbar
        ImGui::BeginChild("##AssetScrollArea", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_HorizontalScrollbar);

        int displayedCount = 0;
        for (const auto& item : cachedAssets)
        {
            // Category filter: ALL_MODELS shows all 3D mesh assets (OBJ, GLB, FBX, PRIM)
            if (selectedCategory == AssetCategory::ALL_MODELS)
            {
                if (item.category == AssetCategory::TEXTURES || item.category == AssetCategory::SCRIPTS || item.category == AssetCategory::ANIMATIONS)
                    continue;
            }
            else if (selectedCategory != item.category)
            {
                continue;
            }

            // Keyword filter
            if (!filterStr.empty())
            {
                std::string lowerName = item.name;
                std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
                if (lowerName.find(filterStr) == std::string::npos) continue;
            }

            if (displayedCount > 0)
            {
                ImGui::SameLine(0.0f, 10.0f);
            }

            renderAssetCard(item, assetManager, scene, cardWidth, cardHeight);
            displayedCount++;
        }

        if (displayedCount == 0)
        {
            ImGui::SetCursorPos(ImVec2(20.0f, 30.0f));
            ImGui::TextDisabled("No assets found matching current filter.");
        }

        ImGui::EndChild();
    }
    ImGui::End();

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}
