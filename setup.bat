@echo off
chcp 65001 > nul
echo =======================================================
echo    DANG KHOI CHAY CAU HINH MOI TRUONG OPENGL LAB...
echo =======================================================
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0setup.ps1"
echo.
pause
