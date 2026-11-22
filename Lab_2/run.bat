@echo off
cd /d "%~dp0\.."
title VKU - Lab 2: Ve duong tron, Ellipse, Parabol

echo ========================================================
echo   VKU - LAB 2: VE DUONG TRON, ELLIPSE, PARABOL
echo ========================================================
echo.
echo [1/2] Dang kiem tra va bien dich Lab 2...
cmake --build build --target Lab_2_lab2
if errorlevel 1 (
    echo.
    echo [LOI] Bien dich that bai!
    pause
    exit /b 1
)

echo.
echo [2/2] Dang khoi chay cua so do hoa...
start "" "build\Lab_2_lab2.exe"
