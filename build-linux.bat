@echo off
rem Runs the portable Linux build (Docker) from Windows via WSL2.
rem Requires: WSL2 installed, Docker Desktop with WSL integration enabled
rem for your default distro (Settings -> Resources -> WSL Integration).
rem
rem Usage:
rem   build-linux.bat            (Release, default)
rem   build-linux.bat --debug
setlocal

set "WINROOT=%~dp0"
if "%WINROOT:~-1%"=="\" set "WINROOT=%WINROOT:~0,-1%"

where wsl >nul 2>&1
if errorlevel 1 (
  echo [ERROR] WSL not found. Install it first: wsl --install
  exit /b 1
)

set "LINUXROOT="
for /f "usebackq delims=" %%i in (`wsl wslpath -a "%WINROOT%"`) do set "LINUXROOT=%%i"

if "%LINUXROOT%"=="" (
  echo [ERROR] Could not resolve a WSL path for: %WINROOT%
  exit /b 1
)

wsl bash -lc "cd '%LINUXROOT%' && ./build-linux.sh --docker %*"
set "ERR=%ERRORLEVEL%"
if %ERR% neq 0 pause
exit /b %ERR%
