@echo off
setlocal
title Tank-1990

rem ===========================================================================
rem  Tank-1990 - inicia o jogo (roda dentro do WSL, janela aparece no Windows)
rem ===========================================================================

set "DISTRO=Tank1990"

wsl.exe -d %DISTRO% -e /bin/true >nul 2>&1
if errorlevel 1 (
    echo.
    echo  [ERRO] O jogo ainda nao foi instalado.
    echo         Rode o instalar.cmd primeiro.
    echo.
    pause
    exit /b 1
)

wsl.exe -d %DISTRO% -e /usr/local/bin/tank1990
if errorlevel 1 (
    echo.
    echo  [ERRO] O jogo terminou com erro.
    echo.
    echo  Se a janela nao abriu, o WSLg ^(suporte grafico do WSL^) pode estar
    echo  desatualizado. Atualize o WSL com este comando e tente de novo:
    echo.
    echo      wsl --update
    echo.
    pause
    exit /b 1
)

exit /b 0
