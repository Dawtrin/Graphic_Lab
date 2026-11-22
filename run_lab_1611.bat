@echo off
cd /d "%~dp0"
title "VKU - LAB THUC HANH 16/11: HINH CHOP SABC"

echo ========================================================
echo   TRUONG DAI HOC CONG NGHE THONG TIN VA TRUYEN THONG VIET - HAN
echo   BAI TAP THUC HANH TAI LOP (16/11): HINH CHOP SABC
echo   1. Khu khuat bang thuat toan Backface va Deep-buffer
echo   2. To bong hinh chop bang phuong phap Gouraud
echo   3. Xuat 3 hinh anh nop Zalo de check cot "16/11"
echo ========================================================
echo.
echo [1/2] Dang kiem tra va bien dich Lab TH 16/11...
cmake -B build
cmake --build build --target Lab_TH_1611_main
if errorlevel 1 (
    echo.
    echo [LOI] Bien dich that bai!
    pause
    exit /b 1
)

echo.
echo [2/2] Dang khoi chay ung dung do hoa Lab TH 16/11...
start "" "build\Lab_TH_1611_main.exe"
