@echo off
cd /d "%~dp0"

g++ -std=c++17 -Wall -Wextra -pedantic digital_twin.cpp -o digital_twin.exe
if errorlevel 1 (
    echo Failed to compile digital_twin.cpp.
    pause
    exit /b 1
)

start "SmartTowerTwin C++ Logic Demo" "%~dp0digital_twin.exe"
start "" "%~dp0index.html"
