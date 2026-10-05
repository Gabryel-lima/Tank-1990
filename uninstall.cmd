@echo off
setlocal enabledelayedexpansion
title Tank-1990 - Desinstalacao

rem ===========================================================================
rem  Tank-1990 - desinstalador
rem ===========================================================================
rem
rem  Tres niveis de remocao:
rem    1) so o jogo        - mantem a distribuicao Alpine no WSL
rem    2) jogo + Alpine    - remove a distribuicao inteira (recomendado)
rem    3) tudo + WSL       - remove tambem a plataforma WSL (precisa de admin)
rem
rem ===========================================================================

set "DISTRO=Tank1990"
set "INSTALL_DIR=%LOCALAPPDATA%\Tank1990"
set "PROJECT_DIR=%~dp0"
rem  %~dp0 termina com barra invertida, que escaparia a aspa em --cd
if "%PROJECT_DIR:~-1%"=="\" set "PROJECT_DIR=%PROJECT_DIR:~0,-1%"

echo.
echo  ============================================
echo   Tank-1990 - Desinstalacao
echo  ============================================
echo.

wsl.exe -d %DISTRO% -e /bin/true >nul 2>&1
if !errorlevel! neq 0 (
    echo  A distribuicao "%DISTRO%" nao existe - parece que ja foi removida.
    echo.
    set "TEM_DISTRO=0"
) else (
    set "TEM_DISTRO=1"
)

echo   O que voce quer remover?
echo.
echo     [1] Somente o jogo
echo         Apaga o jogo, o compilador e as bibliotecas, mas mantem o
echo         Alpine instalado no WSL ^(libera ~350 MB^).
echo.
echo     [2] O jogo e a distribuicao Alpine        ^(recomendado^)
echo         Apaga tudo que este instalador criou ^(~400 MB^). O WSL
echo         continua disponivel para outras distribuicoes.
echo.
echo     [3] O jogo, a distribuicao E a plataforma WSL
echo         Remove tambem o WSL do Windows ^(libera ~1,5 GB a mais^).
echo         ATENCAO: qualquer outra distribuicao Linux que voce tenha
echo         parara de funcionar. Precisa de administrador.
echo.
echo     [0] Cancelar
echo.

set "OPCAO="
set /p "OPCAO=  Digite o numero da opcao: "

if "%OPCAO%"=="1" goto so_jogo
if "%OPCAO%"=="2" goto jogo_distro
if "%OPCAO%"=="3" goto tudo
if "%OPCAO%"=="0" goto cancelado
echo.
echo  Opcao invalida.
goto fim_erro

rem ---------------------------------------------------------------------------
:so_jogo
if "%TEM_DISTRO%"=="0" goto nada_a_fazer
echo.
echo  Removendo o jogo de dentro do WSL...
rem  Copia sem os "\r", como no install.cmd (clone com CRLF)
wsl.exe -d %DISTRO% --cd "%PROJECT_DIR%" -e /bin/sh -c "tr -d '\r' < tools/wsl-uninstall.sh > /tmp/tank1990-uninstall.sh && exec /bin/sh /tmp/tank1990-uninstall.sh"
if !errorlevel! neq 0 (
    echo  [ERRO] Falha ao remover o jogo.
    goto fim_erro
)
echo.
echo  Pronto. A distribuicao "%DISTRO%" continua instalada.
goto fim_ok

rem ---------------------------------------------------------------------------
:jogo_distro
echo.
echo  Isto vai apagar a distribuicao "%DISTRO%" inteira, sem volta.
set "CONF="
set /p "CONF=  Tem certeza? (s/N): "
if /i not "%CONF%"=="s" goto cancelado

if "%TEM_DISTRO%"=="1" (
    echo.
    echo  Removendo a distribuicao "%DISTRO%"...
    wsl.exe --unregister %DISTRO%
    if !errorlevel! neq 0 (
        echo  [ERRO] Falha ao remover a distribuicao.
        goto fim_erro
    )
)

rem  A ponte de controles (bridge\padbridge.exe) aberta prenderia a pasta
taskkill /im padbridge.exe /f >nul 2>&1
if exist "%INSTALL_DIR%" (
    echo  Apagando %INSTALL_DIR%...
    rmdir /s /q "%INSTALL_DIR%" 2>nul
)

echo.
echo  Pronto. O jogo e a distribuicao foram removidos.
echo  A plataforma WSL continua instalada.
goto fim_ok

rem ---------------------------------------------------------------------------
:tudo
echo.
echo  Isto vai remover a distribuicao "%DISTRO%" E a plataforma WSL inteira.
echo  Outras distribuicoes Linux que voce tenha deixarao de funcionar.
set "CONF="
set /p "CONF=  Tem certeza? (s/N): "
if /i not "%CONF%"=="s" goto cancelado

if "%TEM_DISTRO%"=="1" (
    echo.
    echo  Removendo a distribuicao "%DISTRO%"...
    wsl.exe --unregister %DISTRO%
)
taskkill /im padbridge.exe /f >nul 2>&1
if exist "%INSTALL_DIR%" rmdir /s /q "%INSTALL_DIR%" 2>nul

echo.
echo  Removendo a plataforma WSL...
echo  ^(vai aparecer um pedido de permissao do Windows^)
wsl.exe --uninstall
if !errorlevel! neq 0 (
    echo.
    echo  [AVISO] Nao foi possivel remover o WSL automaticamente.
    echo          Rode como administrador:
    echo.
    echo              wsl --uninstall
    echo.
    echo          Ou use: Configuracoes ^> Aplicativos ^> Aplicativos instalados
    echo          ^> "Windows Subsystem for Linux" ^> Desinstalar.
    goto fim_erro
)

echo.
echo  Pronto. Jogo, distribuicao e WSL removidos.
echo  Talvez seja preciso reiniciar o computador.
goto fim_ok

rem ---------------------------------------------------------------------------
:nada_a_fazer
echo.
echo  Nada a remover.
goto fim_ok

:cancelado
echo.
echo  Cancelado - nada foi removido.
goto fim_ok

:fim_erro
echo.
pause
exit /b 1

:fim_ok
if exist "%ProgramFiles%\usbipd-win\usbipd.exe" (
    echo.
    echo  Obs.: o usbipd-win e as autorizacoes de controles continuam no Windows.
    echo        Para remover as autorizacoes: gamepads.cmd --remove
    echo        Para remover o usbipd-win:    winget uninstall usbipd
)
echo.
pause
exit /b 0
