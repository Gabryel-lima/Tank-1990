@echo off
setlocal enabledelayedexpansion
title Tank-1990 - Instalacao

rem ===========================================================================
rem  Tank-1990 - instalador para Windows via WSL2
rem ===========================================================================
rem
rem  Cria uma distribuicao Alpine Linux minima dentro do WSL2, compila o jogo
rem  nela e remove o compilador no fim. Resultado: ~250 MB de disco.
rem
rem  Nao precisa de permissao de administrador, DESDE QUE o WSL ja esteja
rem  instalado. Se nao estiver, o script mostra o comando a executar.
rem
rem  Para desinstalar:  desinstalar.cmd
rem  Para jogar:        jogar.cmd
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

echo.
echo  ============================================
echo   Tank-1990 - Instalacao no WSL
echo  ============================================
echo.

rem ---------------------------------------------------------------------------
rem  1. O WSL esta instalado?
rem ---------------------------------------------------------------------------
where wsl.exe >nul 2>&1
if errorlevel 1 goto sem_wsl

wsl.exe --status >nul 2>&1
if errorlevel 1 goto sem_wsl

echo  [1/5] WSL detectado.

rem ---------------------------------------------------------------------------
rem  2. A distribuicao ja existe?
rem ---------------------------------------------------------------------------
wsl.exe -d %DISTRO% -e /bin/true >nul 2>&1
if not errorlevel 1 (
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
    curl.exe -fL --progress-bar -o "%ROOTFS_TMP%" "%ROOTFS_URL%"
    if errorlevel 1 (
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
wsl.exe --import %DISTRO% "%INSTALL_DIR%" "%ROOTFS_TMP%" --version 2
if errorlevel 1 (
    echo.
    echo  [ERRO] Falha ao importar a distribuicao no WSL.
    goto fim_erro
)

:compilar
rem ---------------------------------------------------------------------------
rem  5. Instalar dependencias e compilar dentro do WSL
rem ---------------------------------------------------------------------------
echo  [4/5] Instalando dependencias e compilando o jogo...
echo        ^(a primeira vez baixa ~150 MB de pacotes e leva alguns minutos^)
echo.
wsl.exe -d %DISTRO% --cd "%PROJECT_DIR%" -- /bin/sh tools/wsl-setup.sh %*
if errorlevel 1 (
    echo.
    echo  [ERRO] Falha ao compilar o jogo dentro do WSL.
    goto fim_erro
)

rem ---------------------------------------------------------------------------
echo  [5/5] Pronto!
echo.
echo  ============================================
echo   Instalacao concluida
echo  ============================================
echo.
echo   Para jogar:       jogar.cmd
echo   Para desinstalar: desinstalar.cmd
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
echo   Reinicie o computador e execute este instalar.cmd de novo.
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
