param(
    [Parameter(Mandatory=$true)][string]$Prueba,
    [Parameter(Mandatory=$true)][string]$Importacion,
    [Parameter(Mandatory=$true)][string]$Compilador
)
$ErrorActionPreference='Stop'
$raiz=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '../..')).Path
$buildRaiz=[IO.Path]::GetFullPath((Join-Path $raiz 'build'))
function Resolver-Prueba([string]$Nombre) {
    if (![IO.Path]::IsPathRooted($Nombre)) { $Nombre=Join-Path $raiz $Nombre }
    $p=(Resolve-Path -LiteralPath $Nombre).Path
    if (!$p.StartsWith($buildRaiz+'\',[StringComparison]::OrdinalIgnoreCase)) { throw 'Las pruebas anteriores deben pertenecer a build de este proyecto.' }
    return $p
}
$anterior=Resolver-Prueba $Prueba
$importada=Resolver-Prueba $Importacion
$compilador=Resolver-Prueba $Compilador
$referencia=Join-Path $anterior 'Release/ArcadeNativeReference.exe'
$kitAnterior=Join-Path $importada 'kit'
$inspector=Join-Path $compilador 'compiler/bin/llvm-readobj.exe'
$rom=Join-Path $raiz 'roms/Mario Bros..nes'
$entrada=Join-Path $PSScriptRoot 'input.txt'
$helper=Join-Path $PSScriptRoot 'verificar_ventana.ps1'
$cmake=(Get-Command cmake.exe -ErrorAction Stop).Source
foreach ($p in @($referencia,$inspector,$rom,$entrada,$helper,
    (Join-Path $importada 'resultado.json'),(Join-Path $compilador 'resultado.json'),
    (Join-Path $kitAnterior 'compiler/bin/clang.exe'),(Join-Path $kitAnterior 'compiler/bin/lld-link.exe'),
    (Join-Path $kitAnterior 'sdk/runner/cyc/cyc_recomp.h'),(Join-Path $kitAnterior 'licenses/LLVM-LICENSE.txt'))) {
    if (!(Test-Path -LiteralPath $p -PathType Leaf)) { throw ('Falta un archivo: '+$p) }
}
$previa=Get-Content -LiteralPath (Join-Path $importada 'resultado.json') -Raw | ConvertFrom-Json
$llvm=Get-Content -LiteralPath (Join-Path $compilador 'resultado.json') -Raw | ConvertFrom-Json
if (!$previa.ImportacionAutonoma -or !$previa.EstadoYAudioIdenticos -or !$previa.KitSinCambios -or
    !$llvm.EstadoYAudioIdenticos -or $llvm.Version -ne '23.1.4') { throw 'Las pruebas anteriores no terminaron correctamente.' }
if ((Get-FileHash -LiteralPath $rom -Algorithm SHA256).Hash -ne
    'A2A0AA437A735CD36ED29A8E7C5B3DFA10590C6203E893F1668C7995BAD2B309') { throw 'La ROM no es la revision compatible.' }
$id=[guid]::NewGuid().ToString('N').Substring(0,8)
$build=Join-Path $buildRaiz ('l100-'+$id)
$dev=Join-Path $build 'dev'
$portable=Join-Path $build 'portable'
$kit=Join-Path $portable 'Bin'
$datos=Join-Path $portable 'Datos'
New-Item -ItemType Directory -Path $build | Out-Null
$system32=Join-Path $env:SystemRoot 'System32'
function Ejecutar {
    param([string]$Programa,[string[]]$Argumentos,[string]$Registro,[int[]]$Codigos=@(0),[switch]$Aislar,[string]$Directorio=$build)
    foreach ($a in $Argumentos) { if ($a.Contains('"') -or $a.EndsWith('\')) { throw 'Argumento inesperado.' } }
    $pi=New-Object Diagnostics.ProcessStartInfo
    $pi.FileName=$Programa
    $pi.Arguments=($Argumentos | ForEach-Object { '"'+$_+'"' }) -join ' '
    $pi.WorkingDirectory=$Directorio
    $pi.UseShellExecute=$false
    $pi.CreateNoWindow=$true
    $pi.WindowStyle=[Diagnostics.ProcessWindowStyle]::Hidden
    $pi.RedirectStandardOutput=$true
    $pi.RedirectStandardError=$true
    if ($Aislar) {
        foreach ($nombre in @('INCLUDE','LIB','LIBPATH','CL','_CL_','LINK','_LINK_',
            'VCINSTALLDIR','VCToolsInstallDir','VSINSTALLDIR','VSCMD_VER','WindowsSdkDir','WindowsSDKVersion',
            'UniversalCRTSdkDir','UCRTVersion','CPATH','C_INCLUDE_PATH','CPLUS_INCLUDE_PATH',
            'COMPILER_PATH','LIBRARY_PATH','PYTHONPATH','PYTHONHOME','CMAKE_PREFIX_PATH','NESRECOMP_NO_LAUNCHER')) {
            $pi.EnvironmentVariables.Remove($nombre)
        }
        $pi.EnvironmentVariables['PATH']=(Join-Path $kit 'compiler/bin')+';'+$system32
    }
    $p=New-Object Diagnostics.Process
    $p.StartInfo=$pi
    try {
        [void]$p.Start()
        $out=$p.StandardOutput.ReadToEndAsync()
        $err=$p.StandardError.ReadToEndAsync()
        $p.WaitForExit()
        $texto=$out.Result+$err.Result
        $codigo=$p.ExitCode
    } finally { $p.Dispose() }
    $log=Join-Path $build ($Registro+'.log')
    [IO.File]::WriteAllText($log,$texto,[Text.UTF8Encoding]::new($false))
    if ($Codigos -notcontains $codigo) {
        $final=($texto -split "\r?\n" | Select-Object -Last 16) -join "`n"
        throw "Fallo $Registro (codigo $codigo). Registro: $log`n$final"
    }
    [pscustomobject]@{Texto=$texto;Codigo=$codigo}
}
function Huellas([string]$Carpeta) {
    $nombres=[string[]]@(Get-ChildItem -LiteralPath $Carpeta -File -Recurse | ForEach-Object { $_.FullName.Substring($Carpeta.Length+1).Replace('\','/') })
    [Array]::Sort($nombres,[StringComparer]::Ordinal)
    $lineas=@($nombres | ForEach-Object { $_+':'+(Get-FileHash -LiteralPath (Join-Path $Carpeta $_) -Algorithm SHA256).Hash.ToLowerInvariant() })
    return ($lineas -join "`n")+"`n"
}
function Importaciones([string]$Programa,[string]$Nombre) {
    $r=Ejecutar $inspector @('--coff-imports',$Programa) ('imports-'+$Nombre) -Aislar
    return @([regex]::Matches($r.Texto,'(?m)^\s*Name:\s*([^\r\n]+)') | ForEach-Object {$_.Groups[1].Value.Trim()} | Sort-Object -Unique)
}

Write-Host '1/6 Compilando el launcher y el motor generico con ventana, sin generar codigo de la ROM...'
[void](Ejecutar $cmake @('-S',$PSScriptRoot,'-B',$dev,'-G','Visual Studio 17 2022','-A','x64',
    ('-DARCADE_ROOT='+$raiz),'-DARCADE_WINDOWED_HOST=ON','-DARCADE_PREPARE_MODULE=OFF') 'configurar')
[void](Ejecutar $cmake @('--build',$dev,'--config','Release','--target','ArcadeNativeHost',
    'ArcadeBrosLauncher','ArcadePrepare','NESRecomp','--parallel','1') 'compilar-herramientas')

Write-Host '2/6 Preparando el portable generico y verificando sus dependencias...'
New-Item -ItemType Directory -Path $portable | Out-Null
Copy-Item -LiteralPath $kitAnterior -Destination $kit -Recurse
$launcher=Join-Path $portable 'ArcadeBrosLauncher.exe'
Copy-Item -LiteralPath (Join-Path $dev 'Release/ArcadeBrosLauncher.exe') -Destination $launcher
foreach ($nombre in @('ArcadeNativeHost.exe','ArcadeNativeHost.lib','ArcadePrepare.exe','NESRecomp.exe','SDL2.dll')) {
    Copy-Item -LiteralPath (Join-Path $dev ('Release/'+$nombre)) -Destination (Join-Path $kit $nombre) -Force
}
Copy-Item -LiteralPath (Join-Path $dev 'Release/assets') -Destination (Join-Path $kit 'assets') -Recurse
if (@(Get-ChildItem -LiteralPath $kit -File -Recurse | Where-Object {
    $_.Extension -in @('.nes','.mp3','.m4a','.wav') -or $_.Name -like 'game_cyc*.c' -or $_.Name -eq 'ArcadeLocalModule.dll'
}).Count) { throw 'Bin contiene una ROM, musica o un modulo privado.' }
foreach ($nombre in @('ArcadeNativeHost','ArcadePrepare','NESRecomp','SDL2','ArcadeBrosLauncher')) {
    $programa=Join-Path $kit ($nombre+'.exe')
    if ($nombre -eq 'SDL2') { $programa=Join-Path $kit 'SDL2.dll' }
    if ($nombre -eq 'ArcadeBrosLauncher') { $programa=$launcher }
    $imports=@(Importaciones $programa $nombre)
    $runtime=@($imports | Where-Object {$_ -match '^(VCRUNTIME|MSVCP|MSVCR\d|ucrtbase\.dll$|api-ms-win-crt-)'})
    if ($runtime.Count) { throw ($nombre+' requiere runtime externo de Visual Studio: '+($runtime -join ', ')) }
}
$sha=[Security.Cryptography.SHA256]::Create()
try { $kitId=([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes((Huellas $kit))))).Replace('-','').ToLowerInvariant() }
finally { $sha.Dispose() }
[IO.File]::WriteAllText((Join-Path $kit 'kit-id.txt'),$kitId+"`n",[Text.UTF8Encoding]::new($false))
$huellasKit=Huellas $kit

Write-Host '3/6 Importando la ROM desde el launcher y comprobando la reutilizacion de cache...'
[void](Ejecutar $launcher @('--prepare-only','--rom',$rom,'--data',$datos) 'preparar-launcher' -Aislar)
$cacheRaiz=Join-Path $datos 'Cache'
$puntero=Join-Path $cacheRaiz ('current-'+$kitId.Substring(0,16)+'.txt')
$leaf=([IO.File]::ReadAllText($puntero)).Trim()
if (!$leaf -or $leaf -in @('.','..') -or $leaf -match '[\\/:\x00-\x1f]' -or $leaf.EndsWith('.') -or $leaf.EndsWith(' ')) { throw 'El launcher guardo un nombre de cache inseguro.' }
$cache=[IO.Path]::GetFullPath((Join-Path $cacheRaiz $leaf))
if (!$cache.StartsWith([IO.Path]::GetFullPath($cacheRaiz)+'\',[StringComparison]::OrdinalIgnoreCase)) { throw 'La cache no pertenece a Datos/Cache.' }
$modulo=Join-Path $cache 'ArcadeLocalModule.dll'
$romLocal=Join-Path $cache 'game.nes'
foreach ($p in @($modulo,$romLocal,(Join-Path $cache 'complete.json'))) {
    if (!(Test-Path -LiteralPath $p -PathType Leaf)) { throw ('La preparacion no genero: '+$p) }
}
$hashModulo=(Get-FileHash -LiteralPath $modulo -Algorithm SHA256).Hash
$fechaModulo=(Get-Item -LiteralPath $modulo).LastWriteTimeUtc.Ticks
$cuentaCache=@(Get-ChildItem -LiteralPath $cacheRaiz -Directory).Count
[void](Ejecutar $launcher @('--prepare-only','--rom',$rom,'--data',$datos) 'reutilizar-cache' -Aislar)
if ($hashModulo -ne (Get-FileHash -LiteralPath $modulo -Algorithm SHA256).Hash -or
    $fechaModulo -ne (Get-Item -LiteralPath $modulo).LastWriteTimeUtc.Ticks -or
    $cuentaCache -ne @(Get-ChildItem -LiteralPath $cacheRaiz -Directory).Count -or
    $leaf -ne ([IO.File]::ReadAllText($puntero)).Trim()) { throw 'La segunda preparacion no reutilizo la cache intacta.' }
$falsa=Join-Path $build 'rom-invalida.nes'
[IO.File]::WriteAllText($falsa,'Original test input, not a NES ROM.',[Text.UTF8Encoding]::new($false))
$datosRechazo=Join-Path $build 'datos-rechazo'
[void](Ejecutar $launcher @('--prepare-only','--rom',$falsa,'--data',$datosRechazo) 'rechazo-invalida' @(2) -Aislar)
if ((Test-Path -LiteralPath $datosRechazo) -and @(Get-ChildItem -LiteralPath $datosRechazo -File -Recurse -Filter 'ArcadeLocalModule.dll').Count) { throw 'Una ROM rechazada genero un modulo privado.' }
$imports=@(Importaciones $modulo 'modulo')
if ($imports.Count -ne 1 -or $imports[0] -ine 'ArcadeNativeHost.exe') { throw 'La DLL privada tiene dependencias inesperadas.' }

Write-Host '4/6 Comparando 960 fotogramas, audio NES y ciclos con la referencia...'
$resultados=@{}
foreach ($caso in @('referencia','launcher')) {
    $argumentos=@($rom,'--frames','960','--input',$entrada,'--hash-out',(Join-Path $build ($caso+'.hash')),
        '--wav-out',(Join-Path $build ($caso+'.wav')),'--no-save')
    $programa=$referencia
    if ($caso -eq 'launcher') { $programa=Join-Path $kit 'ArcadeNativeHost.exe'; $argumentos=@($modulo,$romLocal)+$argumentos[1..($argumentos.Length-1)] }
    $r=Ejecutar $programa $argumentos $caso -Aislar -Directorio $datos
    if ($r.Texto -notmatch 'mode=native frames=960 cycles=(\d+) native_cycles=(\d+)') { throw ('Falta resumen nativo: '+$caso) }
    $resultados[$caso]=[pscustomobject]@{Ciclos=[uint64]$Matches[1];Nativos=[uint64]$Matches[2]}
}
foreach ($tipo in @('hash','wav')) {
    $a=(Get-FileHash -LiteralPath (Join-Path $build ('referencia.'+$tipo)) -Algorithm SHA256).Hash
    $b=(Get-FileHash -LiteralPath (Join-Path $build ('launcher.'+$tipo)) -Algorithm SHA256).Hash
    if ($a -ne $b) { throw ('Los resultados no coinciden: '+$tipo) }
}
if (!$resultados['launcher'].Nativos -or $resultados['launcher'].Nativos -ne $resultados['referencia'].Nativos -or
    $resultados['launcher'].Ciclos -ne $resultados['referencia'].Ciclos) { throw 'Los ciclos recompilados no coinciden.' }

Write-Host '5/6 Comprobando el launcher y los menus mediante ventanas ocultas...'
if ($huellasKit -ne (Huellas $kit)) { throw 'Bin cambio durante la importacion o la prueba del motor.' }
$musica=Join-Path $raiz 'assets/music/stage1.m4a'
if (Test-Path -LiteralPath $musica -PathType Leaf) {
    New-Item -ItemType Directory -Path (Join-Path $datos 'Music') -Force | Out-Null
    Copy-Item -LiteralPath $musica -Destination (Join-Path $datos 'Music/stage1.m4a')
    if (!(Test-Path -LiteralPath (Join-Path $datos 'config.ini'))) {
        [IO.File]::WriteAllText((Join-Path $datos 'config.ini'),"[Audio]`nMusicEnabled=1`nMusicVolume=35`n",[Text.UTF8Encoding]::new($false))
    }
}
& $helper -Ejecutable (Join-Path $kit 'ArcadeNativeHost.exe') -Modulo $modulo -Rom $romLocal -Datos $datos -Salida (Join-Path $build 'ventana')

Write-Host '6/6 Verificando que el kit generico sigue intacto...'
if ($huellasKit -ne (Huellas $kit)) { throw 'Bin cambio al ejecutar el launcher o los menus.' }
$resumen=[ordered]@{Launcher=$launcher;Kit=$kit;Datos=$datos;CachePrivada=$cache;ModuloPrivado=$modulo;
    EstadoYAudioIdenticos=$true;CacheReutilizada=$true;KitSinCambios=$true;CiclosNativos=$resultados['launcher'].Nativos}
[IO.File]::WriteAllText((Join-Path $build 'resultado.json'),($resumen|ConvertTo-Json),[Text.UTF8Encoding]::new($false))
Write-Host 'PRUEBA DE LAUNCHER PORTATIL COMPLETADA' -ForegroundColor Green
Write-Host ('Estado y audio identicos; ciclos nativos: '+$resultados['launcher'].Nativos)
Write-Host ('Launcher preparado: '+$launcher)
Write-Host ('Resultados y registros: '+$build)
Write-Host 'Es una prueba local sin ZIP publico; la ROM y la musica permanecen en Datos.'
