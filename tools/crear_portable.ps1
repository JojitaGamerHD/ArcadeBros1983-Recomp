# Ejecutar desde cualquier carpeta: powershell -ExecutionPolicy Bypass -File .\tools\crear_portable.ps1
# Genera una instalacion portable PERSONAL en dist/. NO subas la instalacion a GitHub.
$ErrorActionPreference = 'Stop'
$proyecto = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
Set-Location $proyecto

Write-Host 'Configurando el proyecto...' -ForegroundColor Cyan
cmake -S . -B build -G 'Visual Studio 17 2022' -A x64
if ($LASTEXITCODE -ne 0) { throw 'Fallo la configuracion de CMake.' }

Write-Host 'Compilando juego y launcher...' -ForegroundColor Cyan
cmake --build build --config Release --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'Fallo la compilacion.' }

$carpetaBuild = Join-Path $proyecto 'build\Release'
$destino = Join-Path $proyecto 'dist\Arcade Bros 1983 Recomp'
$bin = Join-Path $destino 'Bin'
$rom = Join-Path $destino 'Datos\ROM'
$exeLauncher = Join-Path $carpetaBuild 'ArcadeBrosLauncher.exe'
$exeCore = Join-Path $carpetaBuild 'ArcadeBros1983Recomp.exe'
if (!(Test-Path $exeLauncher)) { throw 'No se encontro ArcadeBrosLauncher.exe. Verifica que agregaste el target a CMakeLists.txt.' }
if (!(Test-Path $exeCore)) { throw 'No se encontro el ejecutable recompilado.' }

New-Item -ItemType Directory -Force $bin, $rom | Out-Null
Copy-Item $exeCore (Join-Path $bin 'ArcadeBros1983Recomp.exe') -Force
Copy-Item $exeLauncher (Join-Path $destino 'ArcadeBros1983Recomp.exe') -Force
Get-ChildItem -Path $carpetaBuild -Filter '*.dll' -File | ForEach-Object {
    Copy-Item $_.FullName (Join-Path $bin $_.Name) -Force
}
Write-Host ''
Write-Host 'Instalacion portable preparada en:' -ForegroundColor Green
Write-Host $destino
Write-Host 'Abre ArcadeBros1983Recomp.exe. La ROM se solicita solo la primera vez.'
Write-Host 'No publiques la carpeta dist/: contiene el ejecutable derivado de la ROM.'
