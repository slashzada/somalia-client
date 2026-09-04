@echo off
setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
cd /d "%SCRIPT_DIR%"

set "TARGET_MODE=Release"
if /i "%1"=="Debug" set "TARGET_MODE=Debug"

echo ========================================================
echo [SOMALIA LOADER] Iniciando compilacao (%TARGET_MODE% - x86)...
echo ========================================================

:: 1. Localiza vcvarsall.bat do Visual Studio dinamicamente
set "VS_PATH="
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        if exist "%%i\VC\Auxiliary\Build\vcvarsall.bat" (
            set "VS_PATH=%%i\VC\Auxiliary\Build\vcvarsall.bat"
        )
    )
)

if "%VS_PATH%"=="" (
    for %%D in ("%ProgramFiles%\Microsoft Visual Studio\2022" "%ProgramFiles%\Microsoft Visual Studio\18" "%ProgramFiles(x86)%\Microsoft Visual Studio\2019") do (
        for %%E in (Community Professional Enterprise BuildTools) do (
            if exist "%%~D\%%E\VC\Auxiliary\Build\vcvarsall.bat" (
                set "VS_PATH=%%~D\%%E\VC\Auxiliary\Build\vcvarsall.bat"
            )
        )
    )
)

if "%VS_PATH%"=="" (
    echo [ERRO] Visual Studio nao localizado.
    exit /b 1
)

echo [INFO] Configurando ambiente MSVC x86 (32-bit)...
call "%VS_PATH%" x86

if not exist "build" mkdir "build"
cd build

if /i "%TARGET_MODE%"=="Debug" (
    set "CL_OPTS=/Od /MTd /Zi /FS /D "_DEBUG" /D "SOMALIA_DEBUG""
) else (
    set "CL_OPTS=/O2 /MT /D "NDEBUG""
)

echo [INFO] Compilando SomaliaLoader.exe (%TARGET_MODE%)...

cl /nologo %CL_OPTS% /std:c++20 /EHsc /W3 /utf-8 /D "WIN32" /D "_WINDOWS" /D "_CRT_SECURE_NO_WARNINGS" ^
   /I ".." ^
   /I "..\..\Common" ^
   /I "..\..\SomaliaNative\Render\ImGui" ^
   /I "..\..\SomaliaNative\Render" ^
   /I "..\..\SomaliaNative\UI" ^
   /I "..\UI" ^
   /I "..\Auth" ^
   /I "..\Config" ^
   /I "..\Injector" ^
   ..\Main.cpp ^
   ..\Auth\KeyAuth.cpp ^
   ..\Config\LoaderConfig.cpp ^
   ..\Injector\Injector.cpp ^
   ..\UI\LoaderMenu.cpp ^
   ..\..\SomaliaNative\Render\ImGui\imgui.cpp ^
   ..\..\SomaliaNative\Render\ImGui\imgui_draw.cpp ^
   ..\..\SomaliaNative\Render\ImGui\imgui_widgets.cpp ^
   ..\..\SomaliaNative\Render\ImGui\imgui_tables.cpp ^
   ..\..\SomaliaNative\Render\ImGui\imgui_impl_dx9.cpp ^
   ..\..\SomaliaNative\Render\ImGui\imgui_impl_win32.cpp ^
   /Fe:"SomaliaLoader.exe" ^
   /link /SUBSYSTEM:WINDOWS ^
   user32.lib gdi32.lib d3d9.lib wininet.lib shell32.lib ole32.lib dwmapi.lib advapi32.lib

if %ERRORLEVEL% equ 0 (
    echo ========================================================
    echo [SUCESSO] SomaliaLoader.exe compilado com sucesso! (%TARGET_MODE%)
    echo Local: %CD%\SomaliaLoader.exe
    echo ========================================================
    copy /Y "SomaliaLoader.exe" "..\..\SomaliaLoader.exe" >nul
    exit /b 0
) else (
    echo ========================================================
    echo [FALHA] Erro durante a compilacao do SomaliaLoader.exe
    echo ========================================================
    exit /b 1
)
