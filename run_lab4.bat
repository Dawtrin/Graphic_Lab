@echo off
cd /d "%~dp0"
title "VKU - LAB 4: QUAN SAT VAT THE 3D"

echo ========================================================
echo   TRUONG DAI HOC CONG NGHE THONG TIN VA TRUYEN THONG VIET - HAN
echo   LAB 4: CHUONG TRINH QUAN SAT VAT THE 3D
echo ========================================================
echo.
echo [1/2] Dang kiem tra va bien dich Lab 4...
cmake -B build
cmake --build build --target Lab4_3D_main
if errorlevel 1 (
    echo.
    echo [LOI] Bien dich that bai!
    pause
    exit /b 1
)

echo.
echo [2/2] Dang khoi chay cua so do hoa Lab 4 3D...
start "" "build\Lab4_3D_main.exe"
