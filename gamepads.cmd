@echo off
setlocal enabledelayedexpansion
title Tank-1990 - Controles

rem ===========================================================================
rem  Tank-1990 - configura controles (gamepads) USB para jogar no WSL
rem ===========================================================================
rem
rem  O WSL nao enxerga controles USB sozinho. Este script:
rem    1. instala o usbipd-win, se precisar (pede administrador)
rem    2. detecta os controles conectados por cabo USB e autoriza cada
rem       modelo a ser repassado ao WSL (pede administrador)
rem
rem  Rode uma vez com os controles conectados, e de novo so quando tiver um
rem  modelo de controle novo. Depois disso o play.cmd cuida de tudo: repassa
rem  os controles ao abrir o jogo e os devolve ao Windows ao fechar.
rem
rem  Uso:
rem    gamepads.cmd            instala/autoriza os controles conectados
rem    gamepads.cmd --list     mostra os controles autorizados
rem    gamepads.cmd --remove   remove todas as autorizacoes
rem
rem ===========================================================================

set "PS1=%~dp0tools\gamepads.ps1"
set "USBIPD=%ProgramFiles%\usbipd-win\usbipd.exe"
set "PS=powershell.exe -NoProfile -ExecutionPolicy Bypass -File"

echo.
echo  ============================================
echo   Tank-1990 - Controles
echo  ============================================

if /i "%~1"=="--list" (
    %PS% "%PS1%" list
    pause
    exit /b 0
)
if /i "%~1"=="--remove" (
    %PS% "%PS1%" remove
    pause
    exit /b 0
)

rem ---------------------------------------------------------------------------
rem  1. usbipd-win
rem ---------------------------------------------------------------------------
if exist "%USBIPD%" (
    echo.
    echo  [1/2] usbipd-win ja instalado.
) else (
    echo.
    echo  [1/2] Instalando o usbipd-win ^(aprove a janela de administrador^)...
    where winget.exe >nul 2>&1
    if !errorlevel! neq 0 (
        echo.
        echo  [ERRO] winget nao encontrado. Instale o usbipd-win manualmente:
        echo         https://github.com/dorssel/usbipd-win/releases
        echo.
        pause
        exit /b 1
    )
    winget.exe install --id dorssel.usbipd-win --exact --accept-package-agreements --accept-source-agreements
    if not exist "%USBIPD%" (
        echo.
        echo  [ERRO] A instalacao do usbipd-win falhou ou foi cancelada.
        echo.
        pause
        exit /b 1
    )
)

rem ---------------------------------------------------------------------------
rem  2. Autorizar os controles conectados
rem ---------------------------------------------------------------------------
echo.
echo  [2/2] Conecte os controles por cabo USB agora, se ainda nao conectou.
echo        ^(controles por Bluetooth nao podem ser repassados ao WSL^)
echo.
pause
%PS% "%PS1%" setup
%PS% "%PS1%" list
pause
exit /b 0
