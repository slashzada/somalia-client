@echo off
setlocal enabledelayedexpansion

echo ========================================================
echo [SOMALIA] Iniciando Build Completo (Single-EXE Standalone)
echo ========================================================

:: 1. Compilar SomaliaNative (.asi / .dll)
echo [1/3] Compilando SomaliaNative...
cd SomaliaNative
call build.bat
if %ERRORLEVEL% neq 0 (
    echo [ERRO] Falha ao compilar SomaliaNative.
    cd ..
    exit /b 1
)
cd ..

:: 2. Gerar Payload e Compilar SomaliaLoader.exe
echo [2/3] Compilando SomaliaLoader com Payload embutido...
cd SomaliaLoader
call build_loader.bat
if %ERRORLEVEL% neq 0 (
    echo [ERRO] Falha ao compilar SomaliaLoader.
    cd ..
    exit /b 1
)
cd ..

:: 3. Conclusão
echo ========================================================
echo [SUCESSO COMPLETO] Build Finalizado com Exito!
echo.
echo Executavel Standalone gerado na raiz:
echo   - %CD%\SomaliaLoader.exe
echo   - %CD%\Somalia.exe
echo.
echo Este executavel e 100%% INDEPENDENTE e nao necessita de
echo nenhum arquivo .asi ou .dll externo na mesma pasta!
echo ========================================================
exit /b 0
