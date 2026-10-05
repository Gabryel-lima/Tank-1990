@echo off
setlocal enabledelayedexpansion
title Tank-1990 - Instalacao

rem ===========================================================================
rem  Tank-1990 - instalador para Windows via WSL2
rem ===========================================================================
rem
rem  Cria uma distribuicao Alpine Linux minima dentro do WSL2, compila o jogo
rem  nela e mantem o compilador para recompilar. Resultado: ~400 MB de disco
rem  (com --slim o compilador e removido no fim: ~250 MB).
rem
rem  Nao precisa de permissao de administrador, DESDE QUE o WSL ja esteja
rem  instalado. Se nao estiver, o script mostra o comando a executar.
rem
rem  Para desinstalar:  uninstall.cmd
rem  Para jogar:        play.cmd
rem
rem ===========================================================================

set "DISTRO=Tank1990"
set "ALPINE_VERSION=3.24.2"
set "ALPINE_BRANCH=v3.24"
set "INSTALL_DIR=%LOCALAPPDATA%\Tank1990"
set "ROOTFS_URL=https://dl-cdn.alpinelinux.org/alpine/%ALPINE_BRANCH%/releases/x86_64/alpine-minirootfs-%ALPINE_VERSION%-x86_64.tar.gz"
set "ROOTFS_TMP=%TEMP%\alpine-minirootfs-%ALPINE_VERSION%.tar.gz"
set "PROJECT_DIR=%~dp0"
rem  %~dp0 termina com barra invertida, que escaparia a aspa em --cd
if "%PROJECT_DIR:~-1%"=="\" set "PROJECT_DIR=%PROJECT_DIR:~0,-1%"

call :agora T_TOTAL

echo.
echo  ============================================
echo   Tank-1990 - Instalacao no WSL
echo  ============================================
echo.

rem ---------------------------------------------------------------------------
rem  1. O WSL esta instalado?
rem ---------------------------------------------------------------------------
where wsl.exe >nul 2>&1
if !errorlevel! neq 0 goto sem_wsl

wsl.exe --status >nul 2>&1
if !errorlevel! neq 0 goto sem_wsl

echo  [1/5] WSL detectado.

rem ---------------------------------------------------------------------------
rem  2. A distribuicao ja existe?
rem ---------------------------------------------------------------------------
wsl.exe -d %DISTRO% -e /bin/true >nul 2>&1
if !errorlevel! equ 0 (
    echo  [2/5] Distribuicao "%DISTRO%" ja existe - reaproveitando.
    goto compilar
)

rem ---------------------------------------------------------------------------
rem  3. Baixar o Alpine minirootfs (~3,5 MB)
rem ---------------------------------------------------------------------------
if exist "%ROOTFS_TMP%" (
    echo  [2/5] Alpine %ALPINE_VERSION% ja baixado.
) else (
    echo  [2/5] Baixando o Alpine Linux %ALPINE_VERSION% ^(~3,5 MB^)...
    curl.exe -fL --progress-bar -w "        baixado em %%{time_total}s\n" -o "%ROOTFS_TMP%" "%ROOTFS_URL%"
    if !errorlevel! neq 0 (
        echo.
        echo  [ERRO] Falha ao baixar o Alpine. Verifique a conexao.
        echo         URL: %ROOTFS_URL%
        goto fim_erro
    )
)

rem ---------------------------------------------------------------------------
rem  4. Importar a distribuicao
rem ---------------------------------------------------------------------------
echo  [3/5] Criando a distribuicao "%DISTRO%" em:
echo        %INSTALL_DIR%
if not exist "%INSTALL_DIR%" mkdir "%INSTALL_DIR%"
call :agora T_PASSO
wsl.exe --import %DISTRO% "%INSTALL_DIR%" "%ROOTFS_TMP%" --version 2
if !errorlevel! neq 0 (
    echo.
    echo  [ERRO] Falha ao importar a distribuicao no WSL.
    goto fim_erro
)
call :duracao %T_PASSO% DUR
echo        distribuicao criada em %DUR%

:compilar
rem ---------------------------------------------------------------------------
rem  5. Instalar dependencias e compilar dentro do WSL
rem ---------------------------------------------------------------------------
echo  [4/5] Instalando dependencias e compilando o jogo...
echo        ^(a primeira vez baixa ~150 MB de pacotes e leva alguns minutos^)
echo.
rem  Roda uma copia do script sem os "\r": um clone feito antes do .gitattributes, ou
rem  com core.autocrlf=true, traz o .sh com CRLF, e o sh do WSL para logo na primeira linha
wsl.exe -d %DISTRO% --cd "%PROJECT_DIR%" -e /bin/sh -c "tr -d '\r' < tools/wsl-setup.sh > /tmp/tank1990-setup.sh && exec /bin/sh /tmp/tank1990-setup.sh %*"
if !errorlevel! neq 0 (
    echo.
    echo  [ERRO] Falha ao compilar o jogo dentro do WSL.
    goto fim_erro
)

rem  Ponte de controles Bluetooth: o WSL nao enxerga os controles Bluetooth, so o
rem  Windows. O padbridge.exe (compilado no WSL pelo wsl-setup.sh) roda no Windows
rem  durante o jogo e manda os controles para dentro do WSL (ver CONTROLES.md).
set "BRIDGE_DIR=%INSTALL_DIR%\bridge"
if not exist "%BRIDGE_DIR%" mkdir "%BRIDGE_DIR%"
taskkill /im padbridge.exe /f >nul 2>&1
wsl.exe -d %DISTRO% --cd "%BRIDGE_DIR%" -e sh -c "cp /opt/tank1990/windows/padbridge.exe /opt/tank1990/windows/SDL2.dll . 2>/dev/null"
if exist "%BRIDGE_DIR%\padbridge.exe" (
    echo        Ponte de controles Bluetooth instalada em %BRIDGE_DIR%
) else (
    echo        [AVISO] Ponte de controles nao instalada: controles Bluetooth
    echo                nao chegam ao jogo ^(teclado e cabo USB funcionam^).
)

rem ---------------------------------------------------------------------------
call :duracao %T_TOTAL% DUR
echo  [5/5] Pronto^^!
echo.
echo  ============================================
echo   Instalacao concluida em %DUR%
echo  ============================================
echo.
echo   Para jogar:       play.cmd
echo   Controles USB:    gamepads.cmd ^(uma vez, com o controle conectado^)
echo   Bluetooth:        pareie o controle no Windows; o play.cmd o leva ao jogo
echo   Para desinstalar: uninstall.cmd
echo.
goto fim_ok

rem ---------------------------------------------------------------------------
:sem_wsl
echo  [ERRO] O WSL nao esta instalado nesta maquina.
echo.
echo   Abra o PowerShell ou o Prompt de Comando COMO ADMINISTRADOR
echo   ^(botao direito no menu Iniciar ^> Terminal ^(Admin^)^) e rode:
echo.
echo       wsl --install --no-distribution
echo.
echo   Reinicie o computador e execute este install.cmd de novo.
echo.
echo   Obs.: "--no-distribution" instala so a base do WSL, sem o Ubuntu.
echo         O jogo usa o Alpine Linux, que e bem menor.
echo.
goto fim_erro

rem ---------------------------------------------------------------------------
:fim_erro
echo.
pause
exit /b 1

:fim_ok
pause
exit /b 0

rem ---------------------------------------------------------------------------
rem  Cronometro
rem ---------------------------------------------------------------------------
rem  call :agora VAR           guarda em VAR os segundos desde a meia-noite
rem  call :duracao INICIO VAR  guarda em VAR o tempo desde INICIO (mm:ss)
:agora
set "_t=%time: =0%"
set /a "%1=(1%_t:~0,2%-100)*3600 + (1%_t:~3,2%-100)*60 + (1%_t:~6,2%-100)"
exit /b 0

:duracao
call :agora _agora
set /a "_d=_agora-%1"
if %_d% lss 0 set /a "_d+=86400"
set /a "_m=_d/60, _s=_d%%60"
if %_m% lss 10 set "_m=0%_m%"
if %_s% lss 10 set "_s=0%_s%"
set "%2=%_m%:%_s%"
exit /b 0
