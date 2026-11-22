@echo off
cd /d "%~dp0\.."
title "VKU - LAB 3: CAC PHEP BIEN DOI 2D & TIM ANH TAM GIAC ABC"

echo ========================================================
echo   VKU - LAB 3: CAC PHEP BIEN DOI 2D
echo   (Tinh tien, Ti le, Quay, Doi xung y=mx+b)
echo   Ap dung tim anh tam giac ABC voi tuong tac chuot
echo ========================================================
echo.
echo [1/2] Dang kiem tra va bien dich ung dung...
cmake --build build --target Lab_BienDoi2D_TiLe_scalling_lab
if errorlevel 1 (
    echo.
    echo [LOI] Bien dich that bai!
    pause
    exit /b 1
)

echo.
echo [2/2] Dang khoi chay ung dung Bien Doi 2D Lab...
start "" "build\Lab_BienDoi2D_TiLe_scalling_lab.exe"
