param(
    [Parameter(Mandatory=$true)][string]$Prueba,
    [Parameter(Mandatory=$true)][string]$Compilador
)
$ErrorActionPreference = 'Stop'
$raiz = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '../..')).Path
$buildRaiz = [IO.Path]::GetFullPath((Join-Path $raiz 'build'))
function Resolver-Prueba([string]$Nombre) {
    if (![IO.Path]::IsPathRooted($Nombre)) { $Nombre = Join-Path $raiz $Nombre }
    $p = (Resolve-Path -LiteralPath $Nombre).Path
    if (!$p.StartsWith($buildRaiz+'\',[StringComparison]::OrdinalIgnoreCase)) {
        throw 'Las pruebas anteriores deben pertenecer a build de este proyecto.'
    }
    return $p
}
$anterior = Resolver-Prueba $Prueba
$portatil = Resolver-Prueba $Compilador
$referencia = Join-Path $anterior 'Release/ArcadeNativeReference.exe'
$sdk = Join-Path $anterior 'generic-sdk'
$llvm = Join-Path $portatil 'compiler'
$inspector = Join-Path $llvm 'bin/llvm-readobj.exe'
$rom = Join-Path $raiz 'roms/Mario Bros..nes'
$entrada = Join-Path $PSScriptRoot 'input.txt'
$cmake = (Get-Command cmake.exe -ErrorAction Stop).Source
foreach ($p in @($referencia,$inspector,$rom,$entrada,(Join-Path $sdk 'runner/cyc/cyc_recomp.h'),
    (Join-Path $llvm 'bin/clang.exe'),(Join-Path $llvm 'bin/lld-link.exe'),
    (Join-Path $portatil 'resultado.json'),(Join-Path $PSScriptRoot 'licenses/LLVM-LICENSE.txt'))) {
    if (!(Test-Path -LiteralPath $p -PathType Leaf)) { throw ('Falta un archivo: ' + $p) }
}
$previo = Get-Content -LiteralPath (Join-Path $portatil 'resultado.json') -Raw | ConvertFrom-Json
if (!$previo.EstadoYAudioIdenticos -or $previo.Version -ne '23.1.4') {
    throw 'La prueba anterior del compilador no termino correctamente.'
}
if ((Get-FileHash -LiteralPath $rom -Algorithm SHA256).Hash -ne
    'A2A0AA437A735CD36ED29A8E7C5B3DFA10590C6203E893F1668C7995BAD2B309') { throw 'La ROM no es la revision compatible.' }
$id = [guid]::NewGuid().ToString('N').Substring(0,8)
$build = Join-Path $buildRaiz ('i100-'+$id)
$dev = Join-Path $build 'dev'
$kit = Join-Path $build 'kit'
$cache = Join-Path $build ('private '+[char]0x00e1)
New-Item -ItemType Directory -Path $build | Out-Null
$system32 = Join-Path $env:SystemRoot 'System32'

function Ejecutar {
    param([string]$Programa,[string[]]$Argumentos,[string]$Registro,[int[]]$Codigos=@(0),[switch]$Aislar)
    foreach ($a in $Argumentos) { if ($a.Contains('"') -or $a.EndsWith('\')) { throw 'Argumento inesperado.' } }
    $pi = New-Object Diagnostics.ProcessStartInfo
    $pi.FileName=$Programa
    $pi.Arguments=($Argumentos | ForEach-Object { '"'+$_+'"' }) -join ' '
    $pi.WorkingDirectory=$build
    $pi.UseShellExecute=$false
    $pi.CreateNoWindow=$true
    $pi.WindowStyle=[Diagnostics.ProcessWindowStyle]::Hidden
    $pi.RedirectStandardOutput=$true
    $pi.RedirectStandardError=$true
    if ($Aislar) {
        foreach ($nombre in @('INCLUDE','LIB','LIBPATH','CL','_CL_','LINK','_LINK_',
            'VCINSTALLDIR','VCToolsInstallDir','VSINSTALLDIR','WindowsSdkDir','WindowsSDKVersion',
            'UniversalCRTSdkDir','UCRTVersion','CPATH','C_INCLUDE_PATH','CPLUS_INCLUDE_PATH',
            'COMPILER_PATH','LIBRARY_PATH','PYTHONPATH','PYTHONHOME','CMAKE_PREFIX_PATH')) {
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
    $datos=@(Get-ChildItem -LiteralPath $Carpeta -File -Recurse | Sort-Object FullName | ForEach-Object {
        $_.FullName.Substring($Carpeta.Length) + ':' + (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
    })
    return ($datos -join "`n")
}
function Importaciones([string]$Programa,[string]$Nombre) {
    $r=Ejecutar $inspector @('--coff-imports',$Programa) ('imports-'+$Nombre) -Aislar
    return @([regex]::Matches($r.Texto,'(?m)^\s*Name:\s*([^\r\n]+)') | ForEach-Object {$_.Groups[1].Value.Trim()})
}

Write-Host '1/6 Compilando las herramientas genericas, sin generar codigo de la ROM...'
[void](Ejecutar $cmake @('-S',$PSScriptRoot,'-B',$dev,'-G','Visual Studio 17 2022','-A','x64',
    ('-DARCADE_ROOT='+$raiz),'-DARCADE_PREPARE_MODULE=OFF') 'configurar')
[void](Ejecutar $cmake @('--build',$dev,'--config','Release','--target',
    'ArcadeNativeHost','ArcadePrepare','NESRecomp','--parallel','1') 'compilar-herramientas')

Write-Host '2/6 Preparando el kit sin ROM ni modulo privado...'
New-Item -ItemType Directory -Path $kit,(Join-Path $kit 'compiler/bin') | Out-Null
foreach ($nombre in @('ArcadeNativeHost.exe','ArcadeNativeHost.lib','ArcadePrepare.exe','NESRecomp.exe')) {
    Copy-Item -LiteralPath (Join-Path $dev ('Release/'+$nombre)) -Destination (Join-Path $kit $nombre)
}
foreach ($nombre in @('module_entry.c','protocol.h')) {
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot $nombre) -Destination (Join-Path $kit $nombre)
}
Copy-Item -LiteralPath (Join-Path $raiz 'game.toml') -Destination (Join-Path $kit 'game.toml')
Copy-Item -LiteralPath $sdk -Destination (Join-Path $kit 'sdk') -Recurse
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'licenses') -Destination (Join-Path $kit 'licenses') -Recurse
foreach ($nombre in @('clang.exe','lld-link.exe')) {
    Copy-Item -LiteralPath (Join-Path $llvm ('bin/'+$nombre)) -Destination (Join-Path $kit ('compiler/bin/'+$nombre))
}
New-Item -ItemType Directory -Path (Join-Path $kit 'compiler/lib') | Out-Null
Copy-Item -LiteralPath (Join-Path $llvm 'lib/clang') -Destination (Join-Path $kit 'compiler/lib/clang') -Recurse
if (@(Get-ChildItem -LiteralPath $kit -File -Recurse | Where-Object {
    $_.Extension -in @('.nes','.dll','.mp3','.m4a','.wav') -or $_.Name -like 'game_cyc*.c'
}).Count) { throw 'El kit contiene un archivo privado inesperado.' }
foreach ($nombre in @('ArcadeNativeHost','ArcadePrepare','NESRecomp')) {
    $imports=@(Importaciones (Join-Path $kit ($nombre+'.exe')) $nombre)
    if (@($imports | Where-Object {$_ -match 'VCRUNTIME|MSVCP|^ucrtbase\.dll$|^api-ms-win-crt-'}).Count) {
        throw ('La herramienta sigue dependiendo del runtime de Visual Studio: '+$nombre)
    }
}
$huellasKit=Huellas $kit

Write-Host '3/6 Importando directamente la ROM con el kit y un entorno aislado...'
$preparador=Join-Path $kit 'ArcadePrepare.exe'
[void](Ejecutar $preparador @('--kit',$kit,'--rom',$rom,'--cache',$cache) 'importar' -Aislar)
$modulo=Join-Path $cache 'ArcadeLocalModule.dll'
if (!(Test-Path -LiteralPath (Join-Path $cache 'complete.json'))) { throw 'Falta el manifiesto de importacion.' }
$imports=@(Importaciones $modulo 'modulo')
if ($imports.Count -ne 1 -or $imports[0] -ine 'ArcadeNativeHost.exe') { throw 'La DLL tiene dependencias inesperadas.' }

Write-Host '4/6 Comparando 960 fotogramas, audio NES y ciclos recompilados...'
$resultados=@{}
foreach ($caso in @('referencia','importado')) {
    $argumentos=@($rom,'--frames','960','--input',$entrada,'--hash-out',(Join-Path $build ($caso+'.hash')),
        '--wav-out',(Join-Path $build ($caso+'.wav')),'--no-save')
    $programa=$referencia
    if ($caso -eq 'importado') {
        $programa=Join-Path $kit 'ArcadeNativeHost.exe'
        $argumentos=@($modulo,(Join-Path $cache 'game.nes'))+$argumentos[1..($argumentos.Length-1)]
    }
    $r=Ejecutar $programa $argumentos $caso -Aislar
    if ($r.Texto -notmatch 'mode=native frames=960 cycles=(\d+) native_cycles=(\d+)') { throw ('Falta resumen nativo: '+$caso) }
    $resultados[$caso]=[pscustomobject]@{Ciclos=[uint64]$Matches[1];Nativos=[uint64]$Matches[2]}
}
foreach ($tipo in @('hash','wav')) {
    $a=(Get-FileHash -LiteralPath (Join-Path $build ('referencia.'+$tipo)) -Algorithm SHA256).Hash
    $b=(Get-FileHash -LiteralPath (Join-Path $build ('importado.'+$tipo)) -Algorithm SHA256).Hash
    if ($a -ne $b) { throw ('Los resultados no coinciden: '+$tipo) }
}
if (!$resultados['importado'].Nativos -or
    $resultados['importado'].Nativos -ne $resultados['referencia'].Nativos -or
    $resultados['importado'].Ciclos -ne $resultados['referencia'].Ciclos) { throw 'Los ciclos recompilados no coinciden.' }

Write-Host '5/6 Comprobando ROM incompatible, archivo ausente y cache ya existente...'
$falsa=Join-Path $build 'rom-invalida.nes'
[IO.File]::WriteAllText($falsa,'Original test input, not a NES ROM.',[Text.UTF8Encoding]::new($false))
foreach ($caso in @('invalida','ausente')) {
    $archivo=$falsa
    if ($caso -eq 'ausente') { $archivo=Join-Path $build 'no-existe.nes' }
    $rechazada=Join-Path $build ('rechazo-'+$caso)
    [void](Ejecutar $preparador @('--kit',$kit,'--rom',$archivo,'--cache',$rechazada) ('rechazo-'+$caso) @(2) -Aislar)
    if (Test-Path -LiteralPath $rechazada) { throw 'Una ROM rechazada creo una cache.' }
}
$antes=Huellas $cache
[void](Ejecutar $preparador @('--kit',$kit,'--rom',$rom,'--cache',$cache) 'rechazo-existente' @(2) -Aislar)
if ($antes -ne (Huellas $cache)) { throw 'Se modifico una cache existente.' }

Write-Host '6/6 Verificando que el kit generico sigue intacto...'
if ($huellasKit -ne (Huellas $kit)) { throw 'El kit cambio al importar la ROM.' }
$resumen=[ordered]@{Kit=$kit;CachePrivada=$cache;ModuloPrivado=$modulo;ImportacionAutonoma=$true;
    EstadoYAudioIdenticos=$true;CiclosNativos=$resultados['importado'].Nativos;KitSinCambios=$true}
[IO.File]::WriteAllText((Join-Path $build 'resultado.json'),($resumen|ConvertTo-Json),[Text.UTF8Encoding]::new($false))
Write-Host 'PRUEBA DE IMPORTACION AUTONOMA COMPLETADA' -ForegroundColor Green
Write-Host ('Estado y audio identicos; ciclos nativos: '+$resultados['importado'].Nativos)
Write-Host ('Resultados y registros: '+$build)
Write-Host 'La importacion usa solo los ejecutables del kit; CMake y Visual Studio se usan para prepararlo.'
Write-Host 'Falta integrar el importador al launcher y al juego con ventana; aun no es el ZIP publico.'
