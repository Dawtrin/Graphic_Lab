@echo off
cd /d "%~dp0"
title VKU - Lab 3: Thuat toan xen hinh (Cohen-Sutherland & Chia nhi phan)

echo ========================================================
echo   VKU - LAB 3: THUAT TOAN XEN HINH (LINE CLIPPING)
echo   Cohen-Sutherland ^& Chia nhi phan (Midpoint)
echo ========================================================
echo.
echo [1/2] Dang kiem tra va bien dich Lab 3...
cmake -B build
cmake --build build --target Lab_3_lab3
if errorlevel 1 (
    echo.
    echo [LOI] Bien dich that bai!
    pause
    exit /b 1
)

echo.
echo [2/2] Dang khoi chay cua so do hoa...
start "" "build\Lab_3_lab3.exe"
