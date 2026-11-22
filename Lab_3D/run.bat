@echo off
cd /d "%~dp0\.."
title "VKU - LAB 3D: HINH CHOP ABCD & CAC PHEP BIEN DOI 3D (26/10)"

echo ========================================================
echo   VKU - LAB 3D: HINH CHOP ABCD ^& BIEN DOI 3D
echo   - Mo hinh: Mat da giac / Khung ket noi (Wireframe)
echo   - Phep chieu: Song song / Phoi canh (XOY, YOZ, ZOX)
echo   - Bien doi 3D: Tinh tien, Ti le quanh dinh, Quay Ox/Oy/Oz
echo ========================================================
echo.
echo [1/2] Dang kiem tra va bien dich ung dung...
cmake -B build
cmake --build build --target Lab_3D_main
if errorlevel 1 (
    echo.
    echo [LOI] Bien dich that bai!
    pause
    exit /b 1
)

echo.
echo [2/2] Dang khoi chay ung dung Lab 3D...
start "" "build\Lab_3D_main.exe"
