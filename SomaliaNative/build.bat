@echo off
setlocal enabledelayedexpansion

set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Release"

echo ========================================================
echo [SOMALIA NATIVE] Compilacao: %CONFIG% (x86 32-bit)
echo ========================================================

:: 1. Localiza vcvarsall.bat do Visual Studio dinamicamente via vswhere ou caminhos padrao
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
    for %%D in ("%ProgramFiles%\Microsoft Visual Studio\2022" "%ProgramFiles%\Microsoft Visual Studio\18" "%ProgramFiles(x86)%\Microsoft Visual Studio\2019" "%ProgramFiles(x86)%\Microsoft Visual Studio\2022") do (
        for %%E in (Community Professional Enterprise BuildTools) do (
            if exist "%%~D\%%E\VC\Auxiliary\Build\vcvarsall.bat" (
                set "VS_PATH=%%~D\%%E\VC\Auxiliary\Build\vcvarsall.bat"
            )
        )
    )
)

if "%VS_PATH%"=="" (
    echo [ERRO] Visual Studio nao localizado. Certifique-se de que a carga "Desenvolvimento para desktop com C++" foi instalada.
    exit /b 1
)

echo [INFO] Configurando ambiente MSVC x86 (32-bit)...
call "%VS_PATH%" x86

if not exist "%~dp0build" mkdir "%~dp0build"
pushd "%~dp0build"

if /i "%CONFIG%"=="Debug" (
    set "CL_FLAGS=/nologo /Od /MTd /Zi /std:c++20 /EHsc /W3 /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "SOMALIANATIVE_EXPORTS" /D "_CRT_SECURE_NO_WARNINGS" /D "_DEBUG""
    set "LINK_FLAGS=/DEBUG"
) else (
    set "CL_FLAGS=/nologo /O2 /MT /std:c++20 /EHsc /W3 /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "SOMALIANATIVE_EXPORTS" /D "_CRT_SECURE_NO_WARNINGS" /D "NDEBUG""
    set "LINK_FLAGS="
)

echo [INFO] Compilando SomaliaNative.asi (%CONFIG%)...

cl %CL_FLAGS% ^
   /I "%~dp0." ^
   /I "%~dp0..\Common" ^
   /I "%~dp0Render\ImGui" ^
   /I "%~dp0UI" ^
   /I "%~dp0Core" ^
   /I "%~dp0Engine" ^
   /I "%~dp0Config" ^
   /I "%~dp0Input" ^
   /I "%~dp0Features" ^
   "%~dp0Core\Main.cpp" ^
   "%~dp0Core\Logger.cpp" ^
   "%~dp0Core\RuntimeState.cpp" ^
   "%~dp0Render\D3D9Hook.cpp" ^
   "%~dp0Render\ImGui\imgui.cpp" ^
   "%~dp0Render\ImGui\imgui_draw.cpp" ^
   "%~dp0Render\ImGui\imgui_widgets.cpp" ^
   "%~dp0Render\ImGui\imgui_tables.cpp" ^
   "%~dp0Render\ImGui\imgui_impl_dx9.cpp" ^
   "%~dp0Render\ImGui\imgui_impl_win32.cpp" ^
   "%~dp0UI\Menu.cpp" ^
   "%~dp0UI\Theme.cpp" ^
   "%~dp0UI\custom.cpp" ^
   "%~dp0Input\InputManager.cpp" ^
   "%~dp0Config\Config.cpp" ^
   "%~dp0Config\ConfigManager.cpp" ^
   "%~dp0Engine\GTA\GTA.cpp" ^
   "%~dp0Engine\SAMP\SAMP.cpp" ^
   "%~dp0Features\Visuals\ESP.cpp" ^
   "%~dp0Features\Aimbot\TargetSelector.cpp" ^
   "%~dp0Features\Aimbot\Aimbot.cpp" ^
   "%~dp0Features\Aimbot\AimAssist.cpp" ^
   "%~dp0Features\Aimbot\RageBot.cpp" ^
   "%~dp0Features\LocalMods\LocalMods.cpp" ^
   "%~dp0Features\Slide\Slide.cpp" ^
   "%~dp0Features\AntiAim\AntiAim.cpp" ^
   /link /DLL %LINK_FLAGS% /OUT:"SomaliaNative.asi" user32.lib gdi32.lib d3d9.lib shell32.lib wininet.lib advapi32.lib

if %ERRORLEVEL% EQU 0 (
    echo ========================================================
    echo [SUCESSO] SomaliaNative.asi compilado com sucesso! [%CONFIG%]
    echo Local: %CD%\SomaliaNative.asi
    echo ========================================================
    if not exist "%~dp0..\dist" mkdir "%~dp0..\dist"
    copy /Y "SomaliaNative.asi" "%~dp0..\dist\SomaliaNative.asi" >nul
    copy /Y "SomaliaNative.asi" "%~dp0..\SomaliaNative.asi" >nul
    popd
    exit /b 0
) else (
    echo ========================================================
    echo [FALHA] Erro durante a compilacao [%CONFIG%].
    echo ========================================================
    popd
    exit /b 1
)
