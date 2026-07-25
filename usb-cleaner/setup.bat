@echo off
chcp 65001 >nul 2>&1
setlocal enabledelayedexpansion

:: ============================================================
:: USB Cleaner v2.0 - Automatic Setup Script
:: Downloads Nana GUI and configures the project
:: ============================================================

title USB Cleaner Setup

echo.
echo ╔════════════════════════════════════════════════════╗
echo ║         USB Cleaner v2.0 - Setup Script            ║
echo ╚════════════════════════════════════════════════════╝
echo.

:: Check if running from correct directory
if not exist "src\main.cpp" (
    echo [ERROR] Please run this script from the project root directory!
    pause
    exit /b 1
)

:: Create directories
echo [1/4] Creating directories...
if not exist "external" mkdir external
if not exist "external\nana" mkdir external\nana
if not exist "build" mkdir build
if not exist "release" mkdir release
echo       ✓ Directories created

:: Download Nana GUI Library
echo.
echo [2/4] Downloading Nana GUI Library...
cd external\nana

if exist "include\nana/gui.hpp" (
    echo       ✓ Nana already installed, skipping download...
) else (
    echo       Downloading Nana v1.7.4...
    
    :: Try with PowerShell first (more reliable)
    powershell -Command ^
        "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; " ^
        "Invoke-WebRequest -Uri 'https://github.com/cnjinhao/nana/releases/download/v1.7.4/nana-1.7.4.zip' " ^
        "-OutFile 'nana.zip'" 2>nul
    
    if exist "nana.zip" (
        echo       ✓ Download complete
        
        :: Extract using PowerShell
        echo       Extracting archive...
        powershell -Command "Expand-Archive -Path 'nana.zip' -DestinationPath '.' -Force"
        
        :: Move files to correct location
        if exist "nana" (
            xcopy /E /Y /I "nana\*" "." >nul 2>&1
            rmdir /S /Q "nana" >nul 2>&1
        )
        
        :: Cleanup zip file
        del /Q "nana.zip" >nul 2>&1
        
        echo       ✓ Extraction complete
    ) else (
        echo       [WARN] Automatic download failed!
        echo       Please download manually:
        echo       https://github.com/cnjinhao/nana/releases/download/v1.7.4/nana-1.7.4.zip
        echo       Extract to: external\nana\
        pause
        exit /b 1
    )
)

cd ..\..

:: Verify installation
echo.
echo [3/4] Verifying installation...

if exist "external\nana\include\nana\gui.hpp" (
    echo       ✓ Nana GUI library found
) else (
    echo       [ERROR] Nana GUI library not found correctly!
    echo       Please check the external\nana folder structure
    pause
    exit /b 1
)

:: Configure CMake
echo.
echo [4/4] Configuring build system...

if exist "build\CMakeCache.txt" (
    echo       CMake already configured, reconfiguring...
    cd build
    cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DUSE_NANA=ON -DNANA_INSTALL_DIR="%CD%\..\external\nana"
    cd ..
) else (
    echo       Running CMake configuration...
    cd build
    cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DUSE_NANA=ON -DNANA_INSTALL_DIR="%CD%\..\external\nana"
    cd ..
)

if %ERRORLEVEL% EQU 0 (
    echo       ✓ Build system configured successfully
) else (
    echo       [WARN] CMake configuration had issues, but you can still build manually
)

:: Summary
echo.
echo ╔════════════════════════════════════════════════════╗
echo ║              Setup Complete! ✅                    ║
echo ╠════════════════════════════════════════════════════╣
echo ║                                                    ║
echo ║  Next steps:                                        ║
echo ║  1. Build:   cmake --build build --config Release  ║
echo ║  2. Or use:  mingw32-make -C build                 ║
echo ║  3. Run:     build\usb_cleaner.exe                ║
echo ║                                                    ║
echo ║  Quick build command:                              ║
echo ║  g++ -std=c++17 -o usb_cleaner.exe src/main.cpp   ║
echo ║      -I./external/nana/include                     ║
echo ║      -DNANA_AUTOMATIC_GUI_TESTING                  ║
echo ║      -lcomctl32 -lshell32 -lole32 -luuid           ║
echo ║      -lgdi32 -lsetupapi                            ║
echo ║      -mwindows -static -DNDEBUG -O2               ║
echo ║                                                    ║
echo ╚════════════════════════════════════════════════════╝
echo.

pause
