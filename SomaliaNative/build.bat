@echo off
setlocal enabledelayedexpansion

echo ========================================================
echo [SOMALIA NATIVE] Iniciando compilacao da Fase 1...
echo ========================================================

:: 1. Localiza vcvarsall.bat do Visual Studio dinamicamente via vswhere ou caminhos padrão
set "VS_PATH="

:: Tentativa 1: Localizador oficial vswhere (detecta qualquer edição e disco)
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        if exist "%%i\VC\Auxiliary\Build\vcvarsall.bat" (
            set "VS_PATH=%%i\VC\Auxiliary\Build\vcvarsall.bat"
        )
    )
)

:: Tentativa 2: Caminhos comuns no drive C: e D:
if "%VS_PATH%"=="" (
    for %%D in ("C:\Program Files\Microsoft Visual Studio\2022" "C:\Program Files\Microsoft Visual Studio\18" "C:\Program Files (x86)\Microsoft Visual Studio\2019" "D:\Program Files\Microsoft Visual Studio\2022") do (
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

if not exist "build" mkdir "build"
cd build

echo [INFO] Compilando SomaliaNative.asi...

cl /nologo /O2 /MT /std:c++20 /EHsc /W3 /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "SOMALIANATIVE_EXPORTS" /D "_CRT_SECURE_NO_WARNINGS" ^
   /I ".." ^
   /I "..\Render\ImGui" ^
   /I "..\UI" ^
   /I "..\Core" ^
   /I "..\Engine" ^
   /I "..\Config" ^
   /I "..\Input" ^
   /I "..\Features" ^
   ..\Core\Main.cpp ^
   ..\Core\Logger.cpp ^
   ..\Core\RuntimeState.cpp ^
   ..\Render\D3D9Hook.cpp ^
   ..\Render\ImGui\imgui.cpp ^
   ..\Render\ImGui\imgui_draw.cpp ^
   ..\Render\ImGui\imgui_widgets.cpp ^
   ..\Render\ImGui\imgui_tables.cpp ^
   ..\Render\ImGui\imgui_impl_dx9.cpp ^
   ..\Render\ImGui\imgui_impl_win32.cpp ^
   ..\UI\Menu.cpp ^
   ..\UI\Theme.cpp ^
   ..\UI\custom.cpp ^
   ..\Input\InputManager.cpp ^
   ..\Config\Config.cpp ^
   ..\Config\ConfigManager.cpp ^
   ..\Engine\GTA\GTA.cpp ^
   ..\Engine\SAMP\SAMP.cpp ^
   ..\Features\Visuals\ESP.cpp ^
   ..\Features\Aimbot\TargetSelector.cpp ^
   ..\Features\Aimbot\Aimbot.cpp ^
   ..\Features\Aimbot\AimAssist.cpp ^
   ..\Features\Aimbot\RageBot.cpp ^
   ..\Features\LocalMods\LocalMods.cpp ^
   ..\Features\Slide\Slide.cpp ^
   ..\Features\AntiAim\AntiAim.cpp ^
   /link /DLL /OUT:"SomaliaNative.asi" user32.lib gdi32.lib d3d9.lib shell32.lib wininet.lib

if %ERRORLEVEL% EQU 0 (
    echo ========================================================
    echo [SUCESSO] SomaliaNative.asi compilado com sucesso!
    echo Local: %CD%\SomaliaNative.asi
    echo ========================================================
    copy /Y "SomaliaNative.asi" "..\..\dist\SomaliaNative.asi" >nul
    copy /Y "SomaliaNative.asi" "..\..\SomaliaNative.asi" >nul
    if exist "C:\Users\Usuario\Downloads\slash again\slash again" (
        copy /Y "SomaliaNative.asi" "C:\Users\Usuario\Downloads\slash again\slash again\SomaliaNative.asi" >nul
    )
    if exist "D:\sa-mpo\slash again" (
        copy /Y "SomaliaNative.asi" "D:\sa-mpo\slash again\SomaliaNative.asi" >nul
    )
    cd ..
    exit /b 0
) else (
    echo ========================================================
    echo [FALHA] Erro durante a compilacao.
    echo ========================================================
    cd ..
    exit /b 1
)
