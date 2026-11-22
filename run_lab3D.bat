@echo off
cd /d "%~dp0"
title "VKU - LAB 3D: HINH CHOP ABCD & CAC PHEP BIEN DOI 3D (26/10)"

echo ========================================================
echo   VKU - LAB 3D: HINH CHOP ABCD ^& BIEN DOI 3D
echo ========================================================
echo.
echo [1/2] Dang kiem tra va bien dich Lab 3D...
cmake -B build
cmake --build build --target Lab_3D_main
if errorlevel 1 (
    echo.
    echo [LOI] Bien dich that bai!
    pause
    exit /b 1
)

echo.
echo [2/2] Dang khoi chay cua so do hoa 3D...
start "" "build\Lab_3D_main.exe"
