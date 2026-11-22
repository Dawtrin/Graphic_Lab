@echo off
cd /d "%~dp0\.."
title "VKU - LAB 4: QUAN SAT VAT THE 3D"

echo ========================================================
echo   TRUONG DAI HOC CONG NGHE THONG TIN VA TRUYEN THONG VIET - HAN
echo   LAB 4: CHUONG TRINH QUAN SAT VAT THE 3D
echo   - 4.1: Phep quay 3D quanh truc Ox, Oy, Oz
echo   - 4.2: Ma tran bien doi he quan sat (Viewing Matrix)
echo          + Chieu Song song / Phoi canh
echo          + Goc quan sat Theta, Phi
echo          + Vi tri quan sat (Diem nhin)
echo          + Phong to, thu nho vat the (Scale & Zoom)
echo   - Mo hinh: Mat da giac (Polygon Mesh Model)
echo ========================================================
echo.
echo [1/2] Dang kiem tra va bien dich ung dung Lab 4...
cmake -B build
cmake --build build --target Lab4_3D_main
if errorlevel 1 (
    echo.
    echo [LOI] Bien dich that bai!
    pause
    exit /b 1
)

echo.
echo [2/2] Dang khoi chay ung dung Lab 4...
start "" "build\Lab4_3D_main.exe"
