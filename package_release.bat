@echo off
chcp 65001 >nul
echo ========================================================
echo   DONG GOI BO CAI DAT - UNITY HUB 3D VULKAN ENGINE + JME
echo ========================================================
echo.

set "SCRIPT_DIR=%~dp0"
cd /d "%SCRIPT_DIR%"

set "DIST_DIR=dist\UnityHub3D_Vulkan"
set "BUILD_DIR=build\Release"
set "JME_DIR=..\Jme_scene"

echo [1/7] Kiem tra va dong cac tien trinh cu dang chay...
taskkill /f /im ShapeRenderer.exe 2>nul
echo Da dong tien trinh cu (neu co).

echo.
echo [2/7] Tien hanh bien dich ban Release C++ (Vulkan Scene Editor)...
if not exist "build\CMakeCache.txt" (
    echo [THONG TIN] Khong tim thay build\CMakeCache.txt, tien hanh cau hinh CMake truoc...
    if exist "%USERPROFILE%\vcpkg\scripts\buildsystems\vcpkg.cmake" (
        cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="%USERPROFILE%\vcpkg\scripts\buildsystems\vcpkg.cmake"
    ) else (
        cmake -B build -S .
    )
    if errorlevel 1 (
        echo [LOI] Cau hinh CMake that bai!
        pause
        exit /b 1
    )
)
cmake --build build --config Release
if errorlevel 1 (
    echo [LOI] Bien dich C++ that bai! Vui long kiem tra lai ma nguon C++.
    pause
    exit /b 1
)

echo.
echo [3/7] Tien hanh bien dich va dong goi Java 21 / jMonkeyEngine 3.7...
if exist "%JME_DIR%\gradlew.bat" (
    pushd "%JME_DIR%"
    call gradlew.bat installDist --no-daemon
    if errorlevel 1 (
        echo [LOI] Bien dich Java / jME that bai!
        popd
        pause
        exit /b 1
    )
    popd
) else (
    echo [CANH BAO] Khong tim thay thu muc %JME_DIR%! Bo qua buoc build Java.
)

echo.
echo [4/7] Tao thu muc phan phoi: %DIST_DIR%...
if exist "%DIST_DIR%" rd /s /q "%DIST_DIR%"
mkdir "%DIST_DIR%"
mkdir "%DIST_DIR%\assets"
mkdir "%DIST_DIR%\Jme_scene"
mkdir "%DIST_DIR%\Jme_scene\build\install\jme-scene"

echo.
echo [5/7] Sao chep C++ binaries, thu vien DLL, Shaders va Scene Configurations...
copy /y "%BUILD_DIR%\ShapeRenderer.exe" "%DIST_DIR%\" >nul
copy /y "%BUILD_DIR%\glfw3.dll" "%DIST_DIR%\" >nul
copy /y "%BUILD_DIR%\lua.dll" "%DIST_DIR%\" >nul
copy /y "%BUILD_DIR%\vert.spv" "%DIST_DIR%\" >nul
copy /y "%BUILD_DIR%\frag.spv" "%DIST_DIR%\" >nul
copy /y "%BUILD_DIR%\shadow_vert.spv" "%DIST_DIR%\" >nul
if exist "%BUILD_DIR%\imgui.ini" copy /y "%BUILD_DIR%\imgui.ini" "%DIST_DIR%\" >nul
copy /y "%BUILD_DIR%\scene*.json" "%DIST_DIR%\" >nul
if exist "scene.json" copy /y "scene.json" "%DIST_DIR%\" >nul
if exist "scene_jme.json" copy /y "scene_jme.json" "%DIST_DIR%\" >nul
if exist "scene_forest_clearing.json" copy /y "scene_forest_clearing.json" "%DIST_DIR%\" >nul
if exist "scene_industrial_warehouse.json" copy /y "scene_industrial_warehouse.json" "%DIST_DIR%\" >nul
if exist "scene_nyc_street.json" copy /y "scene_nyc_street.json" "%DIST_DIR%\" >nul
if exist "scene_nyc_street_scene.json" copy /y "scene_nyc_street_scene.json" "%DIST_DIR%\" >nul

echo Sao chep thu muc tai nguyen assets...
xcopy /e /i /y "assets" "%DIST_DIR%\assets" >nul

echo.
echo [6/7] Sao chep ban phan phoi jMonkeyEngine Runtime vao bo cai...
if exist "%JME_DIR%\build\install\jme-scene" (
    xcopy /e /i /y "%JME_DIR%\build\install\jme-scene" "%DIST_DIR%\Jme_scene\build\install\jme-scene" >nul
)
if exist "%JME_DIR%\Jme_scene.exe" copy /y "%JME_DIR%\Jme_scene.exe" "%DIST_DIR%\Jme_scene\" >nul
if exist "%JME_DIR%\run_jme.bat" copy /y "%JME_DIR%\run_jme.bat" "%DIST_DIR%\Jme_scene\" >nul
if exist "%JME_DIR%\scene.json" copy /y "%JME_DIR%\scene.json" "%DIST_DIR%\Jme_scene\" >nul

echo.
echo [7/7] Tao file huong dan su dung va tao file nen Portable ZIP...
echo ======================================================== > "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"
echo    HUONG DAN SU DUNG BO CAI DAT UNITY HUB 3D VULKAN     >> "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"
echo ======================================================== >> "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"
echo. >> "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"
echo 1. YEU CAU HE THONG: >> "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"
echo    - He dieu hanh: Windows 10 / 11 64-bit. >> "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"
echo    - GPU: Ho tro Vulkan 1.2+ (NVIDIA GeForce, AMD Radeon, Intel Iris/Arc). >> "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"
echo    - Java: Java 21 (JDK hoac JRE) de chay che do PLAY (jMonkeyEngine 3.7). >> "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"
echo    - Runtime: Microsoft Visual C++ 2015-2022 Redistributable (x64). >> "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"
echo. >> "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"
echo 2. CACH KHOI CHAY: >> "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"
echo    - Nhan dup chuot vao file: ShapeRenderer.exe >> "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"
echo    - Nhan nut [PLAY (jMonkeyEngine)] hoac F5 de mo trinh dien sinh canh 3D. >> "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"
echo. >> "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"
echo 3. CAU TRUC CAC THU MUC PHAN PHOI: >> "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"
echo    - ShapeRenderer.exe : Trinh bien tap C++ Vulkan Scene Editor >> "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"
echo    - assets/           : Model 3D, Texture, Lua Scripts >> "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"
echo    - Jme_scene/        : Trinh dien sinh canh 3D jMonkeyEngine Player >> "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"
echo    - scene*.json       : Cac cau hinh sinh canh 3D dong bo >> "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"

if exist "HUONG_DAN_CAI_DAT.md" copy /y "HUONG_DAN_CAI_DAT.md" "%DIST_DIR%\" >nul

powershell -NoProfile -Command "if (Test-Path 'dist\UnityHub3D_Vulkan_Portable.zip') { Remove-Item 'dist\UnityHub3D_Vulkan_Portable.zip' -Force }; Compress-Archive -Path '%DIST_DIR%\*' -DestinationPath 'dist\UnityHub3D_Vulkan_Portable.zip' -Force"

echo.
echo ========================================================
echo [HOAN TAT] Bo cai dat da duoc tao thanh cong tai:
echo    - Thu muc chay truc tiep: %DIST_DIR%
echo    - File nen Portable ZIP : dist\UnityHub3D_Vulkan_Portable.zip
echo ========================================================
echo.
