@echo off
cd /d "%~dp0\.."
title "VKU - LAB 5: DUONG VA MAT CONG 3D"

echo ========================================================
echo   TRUONG DAI HOC CONG NGHE THONG TIN VA TRUYEN THONG VIET - HAN
echo   LAB 5: CHUONG TRINH VE DUONG VA MAT CONG TRONG KHONG GIAN 3D
echo   - 5.1: Mat co quy tac (Ruled Surfaces): Cylinder, Cone, Hyperbolic Paraboloid, Conoid
echo   - 5.2: Mat tron xoay (Surfaces of Revolution): Sphere, Ellipsoid, Hyperboloid, Toroid
echo   - 5.3: Duong cong Bezier (Bezier Curves): Da thuc Bernstein & De Casteljau
echo   - 5.4: Mat cong Bezier (Bezier Surfaces): Bicubic 4x4, Control Net, Y-Heights
echo ========================================================
echo.
echo [1/2] Dang kiem tra va bien dich ung dung Lab 5...
cmake -B build
cmake --build build --target Lab_5_main
if errorlevel 1 (
    echo.
    echo [LOI] Bien dich that bai!
    pause
    exit /b 1
)

echo.
echo [2/2] Dang khoi chay ung dung do hoa Lab 5...
start "" "build\Lab_5_main.exe"
