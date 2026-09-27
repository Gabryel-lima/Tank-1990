@echo off
setlocal enabledelayedexpansion
title Tank-1990

rem ===========================================================================
rem  Tank-1990 - inicia o jogo (roda dentro do WSL, janela aparece no Windows)
rem ===========================================================================
rem
rem  Uso:
rem    play.cmd           inicia o jogo
rem    play.cmd --reset   reinicia o WSL antes (use se a janela ficar cinza,
rem                       invisivel ou parada)
rem
rem  Controles USB autorizados pelo gamepads.cmd sao repassados ao WSL
rem  enquanto o jogo roda e devolvidos ao Windows quando ele fecha.
rem
rem ===========================================================================

set "DISTRO=Tank1990"
set "GAMEPADS=powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\gamepads.ps1""

if /i "%~1"=="--reset" (
    echo  Reiniciando o WSL...
    wsl.exe --shutdown
)

wsl.exe -d %DISTRO% -e /bin/true >nul 2>&1
if !errorlevel! neq 0 (
    echo.
    echo  [ERRO] O jogo ainda nao foi instalado.
    echo         Rode o install.cmd primeiro.
    echo.
    pause
    exit /b 1
)

rem  Fecha uma instancia anterior que tenha ficado aberta. Janelas esquecidas
rem  em segundo plano podem travar o WSLg e deixar as proximas cinzas.
wsl.exe -d %DISTRO% -e pkill -f /opt/tank1990/Tanks >nul 2>&1

rem  Repassa os controles ao WSL em segundo plano (inclusive os que forem
rem  conectados durante o jogo). Sem usbipd-win, nao faz nada.
start "" /b %GAMEPADS% watch

wsl.exe -d %DISTRO% -e /usr/local/bin/tank1990
set "RC=!errorlevel!"

rem  Devolve os controles ao Windows
%GAMEPADS% release

if !RC! neq 0 (
    echo.
    echo  [ERRO] O jogo terminou com erro.
    echo.
    echo  Se a janela nao abriu ou ficou cinza, o WSLg ^(suporte grafico do
    echo  WSL^) pode ter travado. Tente reiniciar o WSL:
    echo.
    echo      play.cmd --reset
    echo.
    echo  Se continuar, atualize o WSL:
    echo.
    echo      wsl --update
    echo.
    pause
    exit /b 1
)

exit /b 0
