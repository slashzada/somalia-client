@echo off
setlocal enabledelayedexpansion

echo ========================================================
echo [TESTS] Compilando e Executando Suite de Testes x86...
echo ========================================================

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
    echo [ERRO] Visual Studio nao localizado.
    exit /b 1
)

call "%VS_PATH%" x86

if not exist "%~dp0build" mkdir "%~dp0build"
pushd "%~dp0build"

cl /nologo /O2 /MT /std:c++20 /EHsc /W3 /D "WIN32" /D "_CONSOLE" /D "NDEBUG" /D "_CRT_SECURE_NO_WARNINGS" ^
   /I "%~dp0..\Common" ^
   /I "%~dp0..\SomaliaLoader\Auth" ^
   "%~dp0TestRunner.cpp" ^
   "%~dp0..\SomaliaLoader\Auth\KeyAuth.cpp" ^
   /link /OUT:"TestRunner.exe" user32.lib advapi32.lib wininet.lib

if %ERRORLEVEL% NEQ 0 (
    echo [ERRO] Falha ao compilar TestRunner.exe
    popd
    exit /b 1
)

echo [INFO] Executando suite de testes...
TestRunner.exe
set "TEST_RES=%ERRORLEVEL%"
popd
exit /b %TEST_RES%
