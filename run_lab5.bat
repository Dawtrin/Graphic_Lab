@echo off
cd /d "%~dp0"
title "VKU - LAB 5: DUONG VA MAT CONG 3D"

echo ========================================================
echo   TRUONG DAI HOC CONG NGHE THONG TIN VA TRUYEN THONG VIET - HAN
echo   LAB 5: CHUONG TRINH VE DUONG VA MAT CONG TRONG KHONG GIAN 3D
echo   - 5.1: Mat co quy tac: Cylinder, Cone, Hyperbolic Paraboloid, Conoid
echo   - 5.2: Mat tron xoay: Sphere, Ellipsoid, Hyperboloid, Toroid, Paraboloid
echo   - 5.3: Duong cong Bezier: Bernstein, De Casteljau, Animation t
echo   - 5.4: Mat cong Bezier 4x4: Bicubic Surface, Control Net, Y-Heights
echo ========================================================
echo.
echo [1/2] Dang kiem tra va bien dich Lab 5...
cmake -B build
cmake --build build --target Lab_5_main
if errorlevel 1 (
    echo.
    echo [LOI] Bien dich that bai!
    pause
    exit /b 1
)

echo.
echo [2/2] Dang khoi chay cua so do hoa Lab 5...
start "" "build\Lab_5_main.exe"
