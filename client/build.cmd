@echo off
setlocal

:: ============================================================
:: Configuration -- adjust these paths to match your installation
:: ============================================================
set QT_DIR=C:\Qt\6.8.0\msvc2022_64

:: Auto-detect MSVC: prefer Community, fall back to Build Tools
set VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat
if not exist "%VCVARS%" set VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat
:: ============================================================

set BUILD_DIR=%~dp0GameLogger\build-tmp
set SOURCE_DIR=%~dp0GameLogger

if not exist "%VCVARS%" (
    echo ERROR: MSVC environment script not found. Install Visual Studio 2022
    echo ^(Community or Build Tools with the C++ workload^).
    exit /b 1
)

if not exist "%QT_DIR%\bin\qmake.exe" (
    echo ERROR: qmake not found under:
    echo   %QT_DIR%
    echo Edit the QT_DIR variable at the top of this script.
    exit /b 1
)

echo Setting up MSVC environment...
call "%VCVARS%"
if errorlevel 1 ( echo MSVC setup failed & exit /b 1 )

set PATH=%QT_DIR%\bin;%PATH%

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

echo Running qmake...
cd /d "%BUILD_DIR%"
qmake "%SOURCE_DIR%\GameLogger.pro" -spec win32-msvc "CONFIG+=release"
if errorlevel 1 ( echo qmake failed & exit /b 1 )

echo Building...
where jom >nul 2>&1
if not errorlevel 1 (
    jom /J %NUMBER_OF_PROCESSORS% release
) else (
    nmake release
)
if errorlevel 1 ( echo Build failed & exit /b 1 )

echo Deploying Qt DLLs...
windeployqt --release "%BUILD_DIR%\release\GameLogger.exe"
if errorlevel 1 ( echo windeployqt failed & exit /b 1 )

echo.
echo Done: %BUILD_DIR%\release\GameLogger.exe
