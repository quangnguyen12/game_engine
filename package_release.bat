@echo off
chcp 65001 >nul
echo ========================================================
echo    DONG GOI BO CAI DAT - UNITY HUB 3D VULKAN ENGINE
echo ========================================================
echo.

set "DIST_DIR=dist\UnityHub3D_Vulkan"
set "BUILD_DIR=build\Release"

if not exist "%BUILD_DIR%\ShapeRenderer.exe" (
    echo [THONG BAO] Chua tim thay ShapeRenderer.exe trong build\Release.
    echo Dang tien hanh bien dich ban Release...
    cmake --build build --config Release
    if errorlevel 1 (
        echo [LOI] Bien dich that bai! Vui long kiem tra lai ma nguon.
        pause
        exit /b 1
    )
)

echo [1/5] Tao thu muc phan phoi: %DIST_DIR%...
if exist "%DIST_DIR%" rd /s /q "%DIST_DIR%"
mkdir "%DIST_DIR%"
mkdir "%DIST_DIR%\assets"

echo [2/5] Sao chep file thuc thi, thu vien DLL va Shaders...
copy /y "%BUILD_DIR%\ShapeRenderer.exe" "%DIST_DIR%\" >nul
copy /y "%BUILD_DIR%\glfw3.dll" "%DIST_DIR%\" >nul
copy /y "%BUILD_DIR%\lua.dll" "%DIST_DIR%\" >nul
copy /y "%BUILD_DIR%\vert.spv" "%DIST_DIR%\" >nul
copy /y "%BUILD_DIR%\frag.spv" "%DIST_DIR%\" >nul
copy /y "%BUILD_DIR%\shadow_vert.spv" "%DIST_DIR%\" >nul
if exist "%BUILD_DIR%\imgui.ini" copy /y "%BUILD_DIR%\imgui.ini" "%DIST_DIR%\" >nul

echo [3/5] Sao chep thu muc tai nguyen assets...
xcopy /e /i /y "assets" "%DIST_DIR%\assets" >nul

echo [4/5] Tao file huong dan su dung trong bo cai...
echo ======================================================== > "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"
echo    HUONG DAN SU DUNG BO CAI DAT UNITY HUB 3D VULKAN     >> "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"
echo ======================================================== >> "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"
echo. >> "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"
echo 1. YEU CAU HE THONG: >> "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"
echo    - He dieu hanh: Windows 10 / 11 64-bit. >> "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"
echo    - GPU: Ho tro Vulkan 1.2+ (NVIDIA GeForce, AMD Radeon, Intel Iris/Arc). >> "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"
echo    - Driver: Da cai driver card man hinh moi nhat. >> "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"
echo    - Runtime: Microsoft Visual C++ 2015-2022 Redistributable (x64). >> "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"
echo. >> "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"
echo 2. CACH KHOI CHAY: >> "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"
echo    - Nhan dup chuot vao file: ShapeRenderer.exe >> "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"
echo    - Khong can cai dat phuc tap, bo cai hoat dong doc lap (Portable). >> "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"
echo. >> "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"
echo 3. CAU TRUC CAC FILE BAT BUOC PHAI CO CUNG THU MUC: >> "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"
echo    - ShapeRenderer.exe : File chay chuong trinh >> "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"
echo    - glfw3.dll, lua.dll: Thu vien he thong lien ket dong >> "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"
echo    - *.spv             : Cac shader do hoa Vulkan da bien dich >> "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"
echo    - assets/           : Thu muc chua Model 3D, Texture, Lua Scripts >> "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"
echo. >> "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"
echo 4. XU LY SU CO THUONG GAP: >> "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"
echo    - Neu thieu MSVCP140.dll hoac VCRUNTIME140.dll: >> "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"
echo      Cai dat Visual C++ Redistributable tu trang chu Microsoft. >> "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"
echo    - Neu bao loi Vulkan / Physical Device: >> "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"
echo      Cap nhat driver GPU len ban moi nhat hoac chon chay bang GPU roi. >> "%DIST_DIR%\HUONG_DAN_SU_DUNG.txt"

if exist "HUONG_DAN_CAI_DAT.md" copy /y "HUONG_DAN_CAI_DAT.md" "%DIST_DIR%\" >nul

echo [5/5] Tao file nen Portable ZIP...
powershell -NoProfile -Command "if (Test-Path 'dist\UnityHub3D_Vulkan_Portable.zip') { Remove-Item 'dist\UnityHub3D_Vulkan_Portable.zip' -Force }; Compress-Archive -Path '%DIST_DIR%\*' -DestinationPath 'dist\UnityHub3D_Vulkan_Portable.zip' -Force"

echo.
echo ========================================================
echo [HOAN TAT] Bo cai dat da duoc tao thanh cong tai:
echo    - Thu muc chay truc tiep: %DIST_DIR%
echo    - File nen Portable ZIP : dist\UnityHub3D_Vulkan_Portable.zip
echo ========================================================
echo.
