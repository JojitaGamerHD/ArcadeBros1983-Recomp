
# Arcade Bros 1983 Recomp v0.4.0
# Instalacion portable PERSONAL. No publicar dist/ en GitHub.

$ErrorActionPreference = 'Stop'

$proyecto = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
Set-Location $proyecto

$carpetaCompilacion = Join-Path $proyecto 'build\arcade-test'
$carpetaRelease = Join-Path $carpetaCompilacion 'Release'

$destino = Join-Path $proyecto 'dist\Arcade Bros 1983 Recomp'
$bin = Join-Path $destino 'Bin'
$datos = Join-Path $destino 'Datos'
$rom = Join-Path $datos 'ROM'
$records = Join-Path $datos 'Records'

Write-Host '=== Arcade Bros 1983 Recomp v0.4.0 ===' -ForegroundColor Cyan

# 1. Compilar nuestra version actual
Write-Host 'Configurando CMake...' -ForegroundColor Cyan

cmake -S . -B $carpetaCompilacion -G 'Visual Studio 17 2022' -A x64
if ($LASTEXITCODE -ne 0) {
    throw 'Fallo la configuracion de CMake.'
}

Write-Host 'Compilando juego y launcher...' -ForegroundColor Cyan

cmake --build $carpetaCompilacion --config Release --parallel 4
if ($LASTEXITCODE -ne 0) {
    throw 'Fallo la compilacion.'
}

# 2. Comprobar ejecutables
$exeJuego = Join-Path $carpetaRelease 'ArcadeBros1983Recomp.exe'
$exeLauncher = Join-Path $carpetaRelease 'ArcadeBrosLauncher.exe'
$assetsOrigen = Join-Path $carpetaRelease 'assets'

if (!(Test-Path $exeJuego)) {
    throw 'No se encontro el ejecutable del juego.'
}

if (!(Test-Path $exeLauncher)) {
    throw 'No se encontro el launcher.'
}

if (!(Test-Path $assetsOrigen -PathType Container)) {
    throw 'No se encontro la carpeta assets de recomp-ui.'
}

# 3. Crear estructura portable
Write-Host 'Preparando carpetas...' -ForegroundColor Cyan

New-Item -ItemType Directory -Force `
    $bin, $rom, $records | Out-Null

# 4. Copiar ejecutables
Copy-Item $exeJuego (Join-Path $bin 'ArcadeBros1983Recomp.exe') -Force

Copy-Item $exeLauncher `
    (Join-Path $destino 'ArcadeBros1983Recomp.exe') -Force

# 5. Copiar dependencias DLL
Get-ChildItem $carpetaRelease -Filter '*.dll' -File |
    ForEach-Object {
        Copy-Item $_.FullName (Join-Path $bin $_.Name) -Force
    }

# 6. Copiar recursos graficos de recomp-ui
Write-Host 'Copiando assets de recomp-ui...' -ForegroundColor Cyan

$assetsDestino = Join-Path $bin 'assets'
New-Item -ItemType Directory -Force $assetsDestino | Out-Null

Copy-Item (Join-Path $assetsOrigen '*') `
    -Destination $assetsDestino -Recurse -Force

# 7. Preparar configuracion arcade
$plantilla = Join-Path $proyecto 'config\config.ini'
$configDestino = Join-Path $bin 'config.ini'

if (!(Test-Path $configDestino)) {
    if (!(Test-Path $plantilla)) {
        throw 'No se encontro config/config.ini'
    }
    Copy-Item $plantilla $configDestino
}

# Desactivar atajos de guardar/cargar estado
$contenido = Get-Content $configDestino -Raw
$contenido = $contenido -replace `
    '(?m)^(\s*(save_state|load_state)\s*=\s*).*$', '${1}none'

Set-Content -Path $configDestino -Value $contenido -Encoding ascii

# 8. No borrar ROM ni records anteriores
Write-Host ''
Write-Host 'Instalacion portable preparada.' -ForegroundColor Green
Write-Host "Ubicacion: $destino" -ForegroundColor Green
Write-Host ''
Write-Host 'ROM y records existentes conservados.'
Write-Host 'Abre ArcadeBros1983Recomp.exe desde la carpeta principal.'
Write-Host ''
Write-Host 'IMPORTANTE: No publicar dist/ ni ejecutables derivados de la ROM.'
