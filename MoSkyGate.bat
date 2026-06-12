@echo off
chcp 65001 >nul
title SkyGate Airport Management

:: Mở trình duyệt sau 1 giây
start "" "http://localhost:3000"

:: Chạy server
echo ============================================
echo   SkyGate dang chay tai http://localhost:3000
echo   KHONG DONG cua so nay! Nhan Ctrl+C de tat.
echo ============================================
cd /d "%~dp0"
skygate_web.exe 3000
pause
