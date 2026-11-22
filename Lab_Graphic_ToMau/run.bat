@echo off
cd /d "%~dp0\.."
title VKU - Lab: To Mau Boundary Fill (Stack)

echo ========================================================
echo   VKU - LAB: THUAT TOAN TO MAU KHO DE QUY BANG STACK
echo   (Boundary Fill 4-connected)
echo ========================================================
echo.
echo [1/2] Dang kiem tra va bien dich Lab To Mau...
cmake -B build -G "MinGW Makefiles"
cmake --build build --target Lab_Graphic_ToMau_main
if errorlevel 1 (
    echo.
    echo [LOI] Bien dich that bai!
    pause
    exit /b 1
)

echo.
echo [2/2] Dang khoi chay cua so do hoa...
start "" "build\Lab_Graphic_ToMau_main.exe"
