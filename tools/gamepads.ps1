# ============================================================================
# Tank-1990 - repassa controles (gamepads) USB do Windows para o WSL
# ============================================================================
#
# O WSL nao enxerga controles USB. O usbipd-win "empresta" o dispositivo USB
# para o Linux do WSL enquanto o jogo roda e devolve ao Windows no fim.
#
# Uso (normalmente chamado pelos .cmd, nao a mao):
#   gamepads.ps1 setup    detecta os controles conectados e autoriza cada
#                         modelo no usbipd (pede administrador uma vez)
#   gamepads.ps1 list     mostra os controles autorizados e os conectados
#   gamepads.ps1 watch    enquanto o jogo roda, conecta ao WSL todo controle
#                         autorizado que for plugado (roda em segundo plano)
#   gamepads.ps1 release  para o watch e devolve os controles ao Windows
#   gamepads.ps1 remove   remove as autorizacoes (pede administrador)
#
# Autorizacao = politica "AutoBind" do usbipd para o VID:PID do controle.
# Com ela, conectar ao WSL nao exige administrador. Teclados e mouses nunca
# sao autorizados: so dispositivos HID de uso "gamepad"/"joystick" e os
# controles Xbox (que nao sao HID no Windows).
#
# ============================================================================

param(
    [Parameter(Position = 0)]
    [ValidateSet('setup', 'list', 'watch', 'release', 'remove')]
    [string]$Action = 'list'
)

$ErrorActionPreference = 'Stop'

$Usbipd   = Join-Path $env:ProgramFiles 'usbipd-win\usbipd.exe'
$Distro   = 'Tank1990'
$StateDir = Join-Path $env:LOCALAPPDATA 'Tank1990'
$RunFile  = Join-Path $StateDir 'gamepads.run'
$LogFile  = Join-Path $StateDir 'gamepads.log'

# PIDs de controles Xbox da Microsoft (VID 045E). Os de outras marcas
# licenciadas aparecem como classe XnaComposite/XboxComposite no Windows.
$XboxPids = @('028E', '028F', '0291', '02A1', '02D1', '02DD', '02E3', '02EA',
              '02FD', '0719', '0B00', '0B05', '0B12', '0B13', '0B20', '0B22')

function Log([string]$msg) {
    try { Add-Content -Path $LogFile -Value ("{0:HH:mm:ss} {1}" -f (Get-Date), $msg) } catch {}
}

function Test-Admin {
    $id = [Security.Principal.WindowsIdentity]::GetCurrent()
    ([Security.Principal.WindowsPrincipal]$id).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

# Extrai "VID:PID" (minusculo, formato do usbipd) de um InstanceId do Windows
function Get-HwId([string]$instanceId) {
    if ($instanceId -match 'VID_([0-9A-F]{4})&PID_([0-9A-F]{4})') {
        return ('{0}:{1}' -f $Matches[1], $Matches[2]).ToLower()
    }
    return $null
}

# Dispositivos USB vistos pelo usbipd
function Get-UsbDevices {
    $json = & $Usbipd state | Out-String
    $state = $json | ConvertFrom-Json
    foreach ($d in $state.Devices) {
        if (-not $d.BusId) { continue }   # so os conectados agora
        [pscustomobject]@{
            BusId       = $d.BusId
            HwId        = Get-HwId $d.InstanceId
            Description = $d.Description
            Attached    = [bool]$d.ClientIPAddress
        }
    }
}

# VID:PID dos controles conectados, segundo o Windows
function Find-Gamepads {
    $found = @{}
    $entities = Get-CimInstance Win32_PnPEntity -Filter "Present = TRUE"
    foreach ($e in $entities) {
        $hw = Get-HwId $e.DeviceID
        if (-not $hw) { continue }
        $isPad = $false
        # HID com Usage Page 0x01 (Generic Desktop), Usage 0x04 (joystick) ou 0x05 (gamepad)
        if ($e.HardwareID -match 'HID_DEVICE_UP:0001_U:000[45]') { $isPad = $true }
        # Controles Xbox usam driver proprio (nao HID)
        if ($e.PNPClass -match '^(XnaComposite|XboxComposite)$') { $isPad = $true }
        if ($hw -match '^045e:(.{4})$' -and $XboxPids -contains $Matches[1].ToUpper()) { $isPad = $true }
        if ($isPad -and -not $found.ContainsKey($hw)) { $found[$hw] = $e.Name }
    }
    return $found
}

# VID:PID autorizados (politicas AutoBind "Allow" do usbipd)
function Get-AllowedHwIds {
    $ids = @()
    foreach ($line in (& $Usbipd policy list)) {
        if ($line -match 'Allow\s+AutoBind\s+.*?([0-9a-fA-F]{4}:[0-9a-fA-F]{4})') {
            $ids += $Matches[1].ToLower()
        }
    }
    return $ids
}

function Get-PolicyGuids {
    $guids = @()
    foreach ($line in (& $Usbipd policy list)) {
        if ($line -match '^([0-9a-fA-F-]{36})\s+Allow\s+AutoBind') { $guids += $Matches[1] }
    }
    return $guids
}

function Invoke-Elevated([string]$action) {
    Write-Host '  Pedindo permissao de administrador (aprove a janela do Windows)...'
    $p = Start-Process powershell.exe -Verb RunAs -Wait -PassThru -ArgumentList @(
        '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', "`"$PSCommandPath`"", $action)
    exit $p.ExitCode
}

if (-not (Test-Path $Usbipd)) {
    if ($Action -in 'watch', 'release') { exit 0 }   # sem usbipd: joga so no teclado
    Write-Host ''
    Write-Host '  [ERRO] usbipd-win nao esta instalado. Rode o gamepads.cmd.'
    exit 1
}
New-Item -ItemType Directory -Force -Path $StateDir | Out-Null

switch ($Action) {

    'setup' {
        if (-not (Test-Admin)) { Invoke-Elevated 'setup' }
        Write-Host ''
        Write-Host '  Procurando controles conectados...'
        $pads = Find-Gamepads
        $allowed = Get-AllowedHwIds
        if ($pads.Count -eq 0) {
            Write-Host ''
            Write-Host '  Nenhum controle encontrado. Conecte-o por cabo USB e rode de novo.'
            Write-Host '  (Controles por Bluetooth nao podem ser repassados ao WSL.)'
        }
        foreach ($hw in $pads.Keys) {
            if ($allowed -contains $hw) {
                Write-Host ("  [ OK ] {0}  {1}  (ja autorizado)" -f $hw, $pads[$hw])
            } else {
                & $Usbipd policy add --effect Allow --operation AutoBind --hardware-id $hw | Out-Null
                Write-Host ("  [ OK ] {0}  {1}  (autorizado)" -f $hw, $pads[$hw])
            }
        }
        Write-Host ''
        Write-Host '  Pronto. Os controles autorizados funcionam no jogo pelo play.cmd.'
        Write-Host '  Durante o jogo eles ficam indisponiveis para o Windows e voltam ao fechar.'
        Write-Host ''
        if ([Environment]::UserInteractive -and $Host.Name -eq 'ConsoleHost') {
            Write-Host '  Pressione qualquer tecla para fechar.'
            try { [void][Console]::ReadKey($true) } catch {}
        }
    }

    'list' {
        $allowed = Get-AllowedHwIds
        Write-Host ''
        Write-Host '  Controles autorizados (VID:PID):'
        if ($allowed.Count -eq 0) { Write-Host '    nenhum - rode o gamepads.cmd' }
        foreach ($hw in $allowed) { Write-Host "    $hw" }
        Write-Host ''
        Write-Host '  Conectados agora:'
        foreach ($d in Get-UsbDevices) {
            if ($allowed -contains $d.HwId) {
                $st = if ($d.Attached) { 'no WSL' } else { 'no Windows' }
                Write-Host ("    {0}  {1}  {2}  [{3}]" -f $d.BusId, $d.HwId, $d.Description, $st)
            }
        }
        Write-Host ''
    }

    'watch' {
        # Roda em segundo plano enquanto o jogo esta aberto. Termina quando o
        # release apaga o arquivo de controle (ou apos 12 h, por seguranca).
        Set-Content -Path $RunFile -Value $PID
        Set-Content -Path $LogFile -Value ("{0:HH:mm:ss} watch iniciado" -f (Get-Date))
        $allowed = Get-AllowedHwIds
        if ($allowed.Count -eq 0) { Log 'nenhum controle autorizado'; exit 0 }
        $failed = @{}
        $deadline = (Get-Date).AddHours(12)
        while ((Test-Path $RunFile) -and (Get-Date) -lt $deadline) {
            try {
                foreach ($d in Get-UsbDevices) {
                    if ($d.Attached -or -not ($allowed -contains $d.HwId)) { continue }
                    # Depois de uma falha, espera 10 s antes de tentar o mesmo de novo
                    if ($failed.ContainsKey($d.BusId) -and ((Get-Date) - $failed[$d.BusId]).TotalSeconds -lt 10) { continue }
                    $out = & $Usbipd attach --wsl $Distro --busid $d.BusId 2>&1 | Out-String
                    if ($LASTEXITCODE -eq 0) {
                        Log "conectado ao WSL: $($d.BusId) $($d.HwId) $($d.Description)"
                        $failed.Remove($d.BusId)
                    } else {
                        Log "falha ao conectar $($d.BusId) $($d.HwId): $($out.Trim())"
                        $failed[$d.BusId] = Get-Date
                    }
                }
            } catch {
                Log "erro: $_"
            }
            Start-Sleep -Milliseconds 1000
        }
        Log 'watch encerrado'
    }

    'release' {
        Remove-Item -Path $RunFile -ErrorAction SilentlyContinue
        Start-Sleep -Milliseconds 1200   # deixa o watch terminar a volta atual
        foreach ($hw in Get-AllowedHwIds) {
            & $Usbipd detach --hardware-id $hw 2>&1 | Out-Null
        }
        Log 'controles devolvidos ao Windows'
    }

    'remove' {
        if (-not (Test-Admin)) { Invoke-Elevated 'remove' }
        foreach ($g in Get-PolicyGuids) { & $Usbipd policy remove --guid $g | Out-Null }
        Write-Host '  Autorizacoes de controles removidas.'
    }
}
exit 0
