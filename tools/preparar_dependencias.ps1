
# Arcade Bros 1983 Recomp v0.9.0
# Prepara NESRecomp y recomp-ui sin sobrescribir cambios locales.

$ErrorActionPreference = 'Stop'

$proyecto = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
Set-Location $proyecto

$motor = Join-Path $proyecto 'deps\nesrecomp-arcade'
$ui = Join-Path $proyecto 'recomp-ui'

$revisionMotor = '5a2eddeecc646e9325bef53f3c711e119423f99c'
$revisionUI = '7c635c6b366a6e0d5d3c913a7fce6ec04dd9f256'

$parcheMotor = Join-Path $proyecto 'patches\nesrecomp\arcade-v0.9.patch'
$parcheUI = Join-Path $proyecto 'patches\recomp-ui\arcade-v0.8.patch'

function Verificar-Comando {
    param([string]$mensaje)

    if ($LASTEXITCODE -ne 0) {
        throw $mensaje
    }
}

function Aplicar-Parche {
    param(
        [string]$carpeta,
        [string]$parche,
        [string]$nombre
    )

    # Comprobar si el parche ya esta aplicado.
    # Esta comprobacion puede fallar normalmente
# cuando el parche aun no esta aplicado.
$preferenciaAnterior = $ErrorActionPreference

try {
    $ErrorActionPreference = 'SilentlyContinue'
    git -C $carpeta apply --reverse --check $parche 2>$null
}
finally {
    $ErrorActionPreference = $preferenciaAnterior
}

    if ($LASTEXITCODE -eq 0) {
        Write-Host "$nombre ya tiene el parche aplicado." -ForegroundColor Green
        return
    }

    # Comprobar si puede aplicarse sin conflictos.
    git -C $carpeta apply --check $parche
    Verificar-Comando "El parche de $nombre no se puede aplicar."

    git -C $carpeta apply $parche
    Verificar-Comando "Error al aplicar el parche de $nombre."

    Write-Host "Parche de $nombre aplicado." -ForegroundColor Green
}

Write-Host "`n=== PREPARANDO DEPENDENCIAS v0.9.0 ===" -ForegroundColor Cyan

# 1. Comprobar los parches
foreach ($parche in @($parcheMotor, $parcheUI)) {
    if (!(Test-Path $parche -PathType Leaf)) {
        throw "No se encontro el parche: $parche"
    }
}

# 2. Preparar NESRecomp
if (!(Test-Path $motor)) {
    Write-Host "`nDescargando NESRecomp..." -ForegroundColor Cyan

    New-Item -ItemType Directory -Force `
        (Join-Path $proyecto 'deps') | Out-Null

    git clone --recurse-submodules `
        'https://github.com/mstan/nesrecomp.git' $motor

    Verificar-Comando 'No se pudo descargar NESRecomp.'

    git -C $motor checkout --detach $revisionMotor
    Verificar-Comando 'No se pudo seleccionar la revision de NESRecomp.'

    git -C $motor submodule update --init --recursive
    Verificar-Comando 'No se pudieron preparar los submodulos de NESRecomp.'
} else {
    Write-Host "`nNESRecomp ya existe. No se sobrescribira." -ForegroundColor Yellow
}

if (!(Test-Path (Join-Path $motor '.git'))) {
    throw 'La carpeta de NESRecomp no es un repositorio Git valido.'
}

$actualMotor = (git -C $motor rev-parse HEAD).Trim()
Verificar-Comando 'No se pudo verificar la revision de NESRecomp.'

if ($actualMotor -ne $revisionMotor) {
    throw 'NESRecomp tiene una revision diferente. No se modificara automaticamente.'
}

Aplicar-Parche $motor $parcheMotor 'NESRecomp'

# 3. Preparar recomp-ui
if (!(Test-Path (Join-Path $ui '.git'))) {
    Write-Host "`nInicializando recomp-ui..." -ForegroundColor Cyan

    git -C $proyecto submodule update --init --recursive -- recomp-ui
    Verificar-Comando 'No se pudo inicializar recomp-ui.'
} else {
    Write-Host "`nrecomp-ui ya existe. No se sobrescribira." -ForegroundColor Yellow
}

if (!(Test-Path (Join-Path $ui '.git'))) {
    throw 'La carpeta recomp-ui no es un submodulo Git valido.'
}

$actualUI = (git -C $ui rev-parse HEAD).Trim()
Verificar-Comando 'No se pudo verificar la revision de recomp-ui.'

if ($actualUI -ne $revisionUI) {
    throw 'recomp-ui tiene una revision diferente. No se modificara automaticamente.'
}

Aplicar-Parche $ui $parcheUI 'recomp-ui'

# 4. Compilar el recompilador de NES
$carpetaBuild = Join-Path $motor 'build\recompiler'
$exeRecompiler = Join-Path $carpetaBuild 'Release\NESRecomp.exe'

if (!(Test-Path $exeRecompiler)) {
    Write-Host "`nCompilando NESRecomp..." -ForegroundColor Cyan

    cmake -S (Join-Path $motor 'recompiler') `
        -B $carpetaBuild `
        -G 'Visual Studio 17 2022' -A x64

    Verificar-Comando 'Fallo la configuracion del recompilador.'

    cmake --build $carpetaBuild --config Release --parallel 4
    Verificar-Comando 'Fallo la compilacion del recompilador.'
} else {
    Write-Host "`nEl recompilador ya esta compilado." -ForegroundColor Green
}

Write-Host "`n=== DEPENDENCIAS PREPARADAS ===" -ForegroundColor Green
Write-Host 'NESRecomp: revision correcta y parche aplicado.'
Write-Host 'recomp-ui: revision correcta y traduccion aplicada.'
Write-Host 'Recompilador: disponible.'
Write-Host ''
Write-Host 'Ya puedes compilar Arcade Bros 1983 Recomp.'
