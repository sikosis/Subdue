@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "CONFIGURATION=%~1"
if not defined CONFIGURATION set "CONFIGURATION=Release"
if /I not "%CONFIGURATION%"=="Release" if /I not "%CONFIGURATION%"=="Debug" (
    echo Usage: build-windows.bat [Release^|Debug]
    exit /b 2
)

set "ROOT=%~dp0"
set "BUILD=%ROOT%build"
set "DIST=%ROOT%dist"
set "VERSION=0.11"
set "OUTPUT=%BUILD%\%CONFIGURATION%"
set "STAGE=%BUILD%\Subdue-%VERSION%-windows-x64"
set "ARCHIVE=%DIST%\Subdue-%VERSION%-windows-x64.zip"

where cl.exe >nul 2>nul
if errorlevel 1 call :FindVisualStudio
if errorlevel 1 exit /b %errorlevel%

where cl.exe >nul 2>nul
if errorlevel 1 (
    echo ERROR: The Visual Studio C++ compiler was not found.
    echo Install the "Desktop development with C++" workload in Visual Studio Installer.
    exit /b 1
)
where rc.exe >nul 2>nul
if errorlevel 1 (
    echo ERROR: The Windows SDK resource compiler was not found.
    echo Add a Windows 10 or Windows 11 SDK in Visual Studio Installer.
    exit /b 1
)
where tar.exe >nul 2>nul
if errorlevel 1 (
    echo ERROR: tar.exe was not found. Windows 10 or later is required for ZIP packaging.
    exit /b 1
)

if not exist "%OUTPUT%" mkdir "%OUTPUT%"

echo Building Subdue %VERSION% %CONFIGURATION%...
rc.exe /nologo /d UNICODE /d _UNICODE /i "%ROOT%resources" /fo "%OUTPUT%\Subdue.res" "%ROOT%resources\Subdue.rc"
if errorlevel 1 exit /b %errorlevel%

set "COMPILE_FLAGS=/nologo /std:c++20 /W4 /EHsc /permissive- /DUNICODE /D_UNICODE /DWIN32_LEAN_AND_MEAN /DNOMINMAX"
set "LINK_FLAGS=/SUBSYSTEM:WINDOWS /MANIFEST:NO /DYNAMICBASE /NXCOMPAT /HIGHENTROPYVA comctl32.lib shell32.lib user32.lib gdi32.lib advapi32.lib ole32.lib uuid.lib"
if /I "%CONFIGURATION%"=="Debug" (
    set "COMPILE_FLAGS=!COMPILE_FLAGS! /Od /Zi /D_DEBUG"
    set "LINK_FLAGS=!LINK_FLAGS! /DEBUG"
) else (
    set "COMPILE_FLAGS=!COMPILE_FLAGS! /O2 /GL /DNDEBUG"
    set "LINK_FLAGS=!LINK_FLAGS! /LTCG /OPT:REF /OPT:ICF"
)

cl.exe !COMPILE_FLAGS! /I"%ROOT%resources" /Fo"%OUTPUT%\Subdue.obj" /Fe"%OUTPUT%\Subdue.exe" "%ROOT%src\main.cpp" "%OUTPUT%\Subdue.res" /link !LINK_FLAGS!
if errorlevel 1 exit /b %errorlevel%

if exist "%STAGE%" rmdir /s /q "%STAGE%"
mkdir "%STAGE%"
if errorlevel 1 exit /b %errorlevel%
copy /y "%OUTPUT%\Subdue.exe" "%STAGE%\Subdue.exe" >nul
if errorlevel 1 exit /b %errorlevel%
copy /y "%ROOT%README.md" "%STAGE%\README.md" >nul
if errorlevel 1 exit /b %errorlevel%

if not exist "%DIST%" mkdir "%DIST%"
if exist "%ARCHIVE%" del /q "%ARCHIVE%"
if exist "%ARCHIVE%.sha256" del /q "%ARCHIVE%.sha256"
echo Packaging %ARCHIVE%...
tar.exe -a -c -f "%ARCHIVE%" -C "%STAGE%" .
if errorlevel 1 exit /b %errorlevel%

set "HASH="
for /f "skip=1 tokens=*" %%H in ('certutil.exe -hashfile "%ARCHIVE%" SHA256') do if not defined HASH set "HASH=%%H"
if not defined HASH (
    echo ERROR: Could not calculate the SHA-256 checksum.
    exit /b 1
)
set "HASH=%HASH: =%"
for %%F in ("%ARCHIVE%") do set "ARCHIVE_NAME=%%~nxF"
>"%ARCHIVE%.sha256" echo %HASH%  %ARCHIVE_NAME%

echo.
echo Build complete:
echo   %ARCHIVE%
echo   SHA-256: %HASH%
exit /b 0

:FindVisualStudio
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo ERROR: Visual Studio Installer's vswhere.exe was not found.
    echo Install Visual Studio 2022 Build Tools with the "Desktop development with C++" workload.
    exit /b 1
)
set "VSINSTALL="
for /f "usebackq tokens=*" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%I"
if not defined VSINSTALL (
    echo ERROR: No Visual Studio installation with the x64 C++ tools was found.
    echo Add the "Desktop development with C++" workload in Visual Studio Installer.
    exit /b 1
)
if not exist "%VSINSTALL%\VC\Auxiliary\Build\vcvars64.bat" (
    echo ERROR: vcvars64.bat was not found under "%VSINSTALL%".
    exit /b 1
)
echo Initializing Visual Studio from "%VSINSTALL%"...
call "%VSINSTALL%\VC\Auxiliary\Build\vcvars64.bat"
exit /b %errorlevel%
