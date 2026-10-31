param(
    [string]$Compilador,
    [string]$Python,
    [string]$Version='1.0.0'
)
$ErrorActionPreference='Stop'
# Windows PowerShell targets an older .NET compatibility profile by default.
[AppContext]::SetSwitch('Switch.System.IO.Compression.ZipFile.UseBackslash',$false)
$raiz=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '../..')).Path
if ($Version -notmatch '^\d+\.\d+\.\d+(?:-[0-9A-Za-z]+(?:[.-][0-9A-Za-z]+)*)?(?:\+[0-9A-Za-z]+(?:[.-][0-9A-Za-z]+)*)?$') { throw 'La version debe tener formato SemVer, por ejemplo 1.0.0.' }
$engine=Join-Path $raiz 'deps/nesrecomp-arcade'
$publico=Join-Path $PSScriptRoot 'public'
foreach ($p in @(
    (Join-Path $engine 'runner/cyc/cyc.cmake'),(Join-Path $engine 'runner/cyc/cyc_recomp.h'),
    (Join-Path $engine 'runner/external/SDL2/cmake/sdl2-config.cmake'),
    (Join-Path $raiz 'recomp-ui/recomp_ui.cmake'),(Join-Path $raiz 'recomp-ui/src/recomp_runtime_ui.h'),
    (Join-Path $raiz 'tools/preparar_dependencias.ps1'),(Join-Path $raiz 'game.toml'),
    (Join-Path $PSScriptRoot 'prepare_sdk.py'),(Join-Path $publico 'LEEME.txt'),(Join-Path $publico 'SOURCES.txt'),
    (Join-Path $publico 'licenses/LLVM-LICENSE.txt'))) {
    if (!(Test-Path -LiteralPath $p -PathType Leaf)) { throw ('Falta una dependencia o fuente preparada: '+$p) }
}
$preparador=[IO.File]::ReadAllText((Join-Path $raiz 'tools/preparar_dependencias.ps1'))
foreach ($pin in @('5a2eddeecc646e9325bef53f3c711e119423f99c','7c635c6b366a6e0d5d3c913a7fce6ec04dd9f256')) {
    if (!$preparador.Contains($pin)) { throw 'El preparador de dependencias no declara las revisiones esperadas.' }
}
if (!$Python) { $Python=(Get-Command python.exe -ErrorAction Stop).Source }
if (!(Test-Path -LiteralPath $Python -PathType Leaf) -or $Python -like '*WindowsApps*') { throw 'Indica -Python con una instalacion real de Python 3.11 o posterior.' }
$Python=(Resolve-Path -LiteralPath $Python).Path
$cmake=(Get-Command cmake.exe -ErrorAction Stop).Source
$buildRaiz=Join-Path $raiz 'build'
New-Item -ItemType Directory -Path $buildRaiz -Force | Out-Null
$build=Join-Path $buildRaiz ('r100-'+[guid]::NewGuid().ToString('N').Substring(0,8))
New-Item -ItemType Directory -Path $build | Out-Null
$dev=Join-Path $build 'dev'
$sdk=Join-Path $build 'sdk'
$nombre='ArcadeBros1983-Recomp-v'+$Version+'-Windows-x64'
$stage=Join-Path (Join-Path $build 'stage') $nombre
$kit=Join-Path $stage 'Bin'
$system32=Join-Path $env:SystemRoot 'System32'
$utf8=[Text.UTF8Encoding]::new($false)

function Ejecutar {
    param([string]$Programa,[string[]]$Argumentos,[string]$Registro,[switch]$Aislar)
    foreach ($a in $Argumentos) { if ($a.Contains('"') -or $a.EndsWith('\')) { throw 'Argumento inesperado de herramienta.' } }
    $pi=New-Object Diagnostics.ProcessStartInfo
    $pi.FileName=$Programa
    $pi.Arguments=($Argumentos | ForEach-Object { '"'+$_+'"' }) -join ' '
    $pi.WorkingDirectory=$build
    $pi.UseShellExecute=$false
    $pi.CreateNoWindow=$true
    $pi.WindowStyle=[Diagnostics.ProcessWindowStyle]::Hidden
    $pi.RedirectStandardOutput=$true
    $pi.RedirectStandardError=$true
    if ($Aislar) {
        foreach ($n in @('INCLUDE','LIB','LIBPATH','CL','_CL_','LINK','_LINK_','VCINSTALLDIR','VCToolsInstallDir',
            'VSINSTALLDIR','WindowsSdkDir','WindowsSDKVersion','UniversalCRTSdkDir','UCRTVersion',
            'CPATH','C_INCLUDE_PATH','CPLUS_INCLUDE_PATH','COMPILER_PATH','LIBRARY_PATH')) { $pi.EnvironmentVariables.Remove($n) }
        $pi.EnvironmentVariables['PATH']=(Join-Path $Compilador 'bin')+';'+$system32
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
    [IO.File]::WriteAllText($log,$texto,$utf8)
    if ($codigo -ne 0) {
        $final=($texto -split "\r?\n" | Select-Object -Last 16) -join "`n"
        throw "Fallo $Registro (codigo $codigo). Registro: $log`n$final"
    }
    return $texto
}
function Inventario([string]$Carpeta,[string[]]$Excluir=@()) {
    $paths=[string[]]@(Get-ChildItem -LiteralPath $Carpeta -File -Recurse | ForEach-Object { $_.FullName.Substring($Carpeta.Length+1).Replace('\','/') } | Where-Object { $Excluir -notcontains $_ })
    [Array]::Sort($paths,[StringComparer]::Ordinal)
    foreach ($rel in $paths) {
        $f=Get-Item -LiteralPath (Join-Path $Carpeta $rel)
        [pscustomobject]@{Path=$rel;SHA256=(Get-FileHash -LiteralPath $f.FullName -Algorithm SHA256).Hash.ToLowerInvariant();Bytes=$f.Length}
    }
}
function Validar-Ruta([string]$Ruta) {
    if (!$Ruta -or $Ruta.StartsWith('/') -or $Ruta -match '[\\:\x00-\x1f]') { throw ('Ruta insegura en el paquete: '+$Ruta) }
    foreach ($parte in $Ruta.Split('/')) {
        if (!$parte -or $parte -in @('.','..') -or $parte.EndsWith('.') -or $parte.EndsWith(' ') -or
            $parte -match '[<>"|?*]' -or $parte -match '^(CON|PRN|AUX|NUL|CONIN\$|CONOUT\$|COM[1-9]|LPT[1-9])(?:\.|$)') { throw ('Componente de ruta inseguro: '+$Ruta) }
    }
}
$archivosBin=@('ArcadeNativeHost.exe','ArcadeNativeHost.lib','ArcadePrepare.exe','NESRecomp.exe','SDL2.dll',
    'game.toml','module_entry.c','protocol.h','kit-id.txt','compiler/bin/clang.exe','compiler/bin/lld-link.exe')
$assets=@('assets/fonts/LatoLatin-Bold.ttf','assets/fonts/LatoLatin-Regular.ttf','assets/fonts/NotoSansSymbols2-Regular.ttf',
    'assets/fonts/OpenMoji-black-glyf.ttf','assets/fonts/PixelifySans.ttf','assets/fonts/OFL-PixelifySans.txt',
    'assets/img/arcade_bros_1983.png','assets/img/brand_mark.tga','assets/img/brand_nes.tga','assets/img/flags.png',
    'assets/img/pad_nes.tga','assets/img/verdict_bad.tga','assets/img/verdict_none.tga','assets/img/verdict_ok.tga','assets/img/verdict_warn.tga')
function Permitido([string]$Relativo) {
    if ($Relativo -cin @('ArcadeBrosLauncher.exe','LEEME.txt','SOURCES.txt','MANIFIESTO-SHA256.txt')) { return $true }
    if (!$Relativo.StartsWith('Bin/',[StringComparison]::Ordinal)) { return $false }
    $r=$Relativo.Substring(4)
    if ($r -cin $archivosBin -or $r -cin $assets) { return $true }
    if ($r -cmatch '^sdk/(common|runner/cyc|runner/include)/.+\.h$') { return $true }
    if ($r -cmatch '^licenses/.+\.(txt|md|html)$') { return $true }
    if ($r -cmatch '^compiler/lib/clang/[0-9.]+/include/(.+)$') {
        $extension=[IO.Path]::GetExtension($Matches[1])
        return $extension -cin @('','.h','.inc','.modulemap','.tcc')
    }
    return $false
}

Write-Host '1/6 Preparando el compilador portatil y verificando las herramientas de desarrollo...'
if (!$Compilador) {
    $helper=Join-Path $PSScriptRoot 'obtener_llvm.ps1'
    if (!(Test-Path -LiteralPath $helper -PathType Leaf)) { throw 'Falta tools/native_module/obtener_llvm.ps1.' }
    $resultado=@(& $helper -Python $Python)
    if ($resultado.Count -ne 1 -or !$resultado[0].Compilador) { throw 'obtener_llvm.ps1 debe devolver un unico objeto con la propiedad Compilador.' }
    $Compilador=[string]$resultado[0].Compilador
}
if (![IO.Path]::IsPathRooted($Compilador)) { $Compilador=Join-Path $raiz $Compilador }
$Compilador=(Resolve-Path -LiteralPath $Compilador).Path
$clang=Join-Path $Compilador 'bin/clang.exe'
$linker=Join-Path $Compilador 'bin/lld-link.exe'
foreach ($p in @($clang,$linker)) { if (!(Test-Path -LiteralPath $p -PathType Leaf)) { throw ('Falta herramienta LLVM: '+$p) } }
$v=Ejecutar $clang @('--no-default-config','--version') 'version-clang' -Aislar
if ($v -notmatch 'clang version 23\.1\.4\b') { throw 'Esta version del paquete requiere Clang 23.1.4.' }
[void](Ejecutar $linker @('--version') 'version-lld' -Aislar)
$recurso=(Ejecutar $clang @('--no-default-config','-print-resource-dir') 'recursos-clang' -Aislar).Trim()
$recurso=(Resolve-Path -LiteralPath $recurso).Path
$baseRecursos=[IO.Path]::GetFullPath((Join-Path $Compilador 'lib/clang'))+'\'
if (!$recurso.StartsWith($baseRecursos,[StringComparison]::OrdinalIgnoreCase) -or (Split-Path $recurso -Leaf) -notmatch '^[0-9.]+$') { throw 'Los recursos de Clang no pertenecen al compilador indicado.' }
foreach ($h in @('stdint.h','stdbool.h','stddef.h')) {
    if (!(Test-Path -LiteralPath (Join-Path $recurso ('include/'+$h)) -PathType Leaf)) { throw ('Falta un header interno: '+$h) }
}
[void](Ejecutar $Python @('-I','-c','import sys; assert sys.version_info >= (3,11)') 'version-python')

Write-Host '2/6 Compilando los ejecutables genericos, sin consultar una ROM...'
[void](Ejecutar $cmake @('-S',$PSScriptRoot,'-B',$dev,'-G','Visual Studio 17 2022','-A','x64',
    ('-DARCADE_ROOT='+$raiz),'-DARCADE_WINDOWED_HOST=ON','-DARCADE_PREPARE_MODULE=OFF',('-DARCADE_RELEASE_VERSION='+$Version)) 'configurar')
[void](Ejecutar $cmake @('--build',$dev,'--config','Release','--target','ArcadeNativeHost','ArcadeBrosLauncher',
    'ArcadePrepare','NESRecomp','--parallel','1') 'compilar')
[void](Ejecutar $Python @('-I',(Join-Path $PSScriptRoot 'prepare_sdk.py'),$engine,$sdk) 'preparar-sdk')
if (@(Get-ChildItem -LiteralPath $sdk -File -Recurse | Where-Object {$_.Extension -cne '.h'}).Count) { throw 'El SDK generico contiene archivos distintos de headers.' }

Write-Host '3/6 Montando una carpeta nueva con herramientas, assets y avisos...'
New-Item -ItemType Directory -Path $kit,(Join-Path $kit 'compiler/bin'),(Join-Path $kit 'compiler/lib/clang') | Out-Null
Copy-Item -LiteralPath (Join-Path $dev 'Release/ArcadeBrosLauncher.exe') -Destination (Join-Path $stage 'ArcadeBrosLauncher.exe')
foreach ($f in @('ArcadeNativeHost.exe','ArcadeNativeHost.lib','ArcadePrepare.exe','NESRecomp.exe','SDL2.dll')) {
    Copy-Item -LiteralPath (Join-Path $dev ('Release/'+$f)) -Destination (Join-Path $kit $f)
}
foreach ($f in @('module_entry.c','protocol.h')) { Copy-Item -LiteralPath (Join-Path $PSScriptRoot $f) -Destination (Join-Path $kit $f) }
Copy-Item -LiteralPath (Join-Path $raiz 'game.toml') -Destination (Join-Path $kit 'game.toml')
Copy-Item -LiteralPath $sdk -Destination (Join-Path $kit 'sdk') -Recurse
foreach ($f in @('clang.exe','lld-link.exe')) { Copy-Item -LiteralPath (Join-Path $Compilador ('bin/'+$f)) -Destination (Join-Path $kit ('compiler/bin/'+$f)) }
$destinoRecurso=Join-Path $kit ('compiler/lib/clang/'+(Split-Path $recurso -Leaf))
New-Item -ItemType Directory -Path $destinoRecurso | Out-Null
Copy-Item -LiteralPath (Join-Path $recurso 'include') -Destination (Join-Path $destinoRecurso 'include') -Recurse
Copy-Item -LiteralPath (Join-Path $dev 'Release/assets') -Destination (Join-Path $kit 'assets') -Recurse
Copy-Item -LiteralPath (Join-Path $publico 'licenses') -Destination (Join-Path $kit 'licenses') -Recurse
foreach ($f in @('LEEME.txt','SOURCES.txt')) { Copy-Item -LiteralPath (Join-Path $publico $f) -Destination (Join-Path $stage $f) }
foreach ($f in @(Inventario $stage)) {
    Validar-Ruta $f.Path
    if (!(Permitido $f.Path)) { throw ('Archivo fuera de la lista publica: '+$f.Path) }
}
if (@(Get-ChildItem -LiteralPath $stage -Recurse -Force | Where-Object {
    ($_.Attributes -band [IO.FileAttributes]::ReparsePoint) -or ($_.PSIsContainer -and $_.Name -ieq 'Datos')
}).Count) { throw 'La carpeta publica contiene Datos o enlaces de sistema de archivos.' }
foreach ($r in $archivosBin) { if ($r -ne 'kit-id.txt' -and !(Test-Path -LiteralPath (Join-Path $kit $r) -PathType Leaf)) { throw ('Falta componente del kit: '+$r) } }
foreach ($r in $assets) { if (!(Test-Path -LiteralPath (Join-Path $kit $r) -PathType Leaf)) { throw ('Falta asset: '+$r) } }
$kitLineas=@(Inventario $kit | ForEach-Object {$_.Path+':'+$_.SHA256})
$sha=[Security.Cryptography.SHA256]::Create()
try { $kitId=([BitConverter]::ToString($sha.ComputeHash($utf8.GetBytes(($kitLineas -join "`n")+"`n")))).Replace('-','').ToLowerInvariant() }
finally { $sha.Dispose() }
[IO.File]::WriteAllText((Join-Path $kit 'kit-id.txt'),$kitId+"`n",$utf8)
$archivos=@(Inventario $stage)
$manifest=Join-Path $stage 'MANIFIESTO-SHA256.txt'
$manifestLineas=@($archivos | ForEach-Object {$_.SHA256+'  '+$_.Path})
[IO.File]::WriteAllText($manifest,($manifestLineas -join "`n")+"`n",$utf8)
$esperados=New-Object 'Collections.Generic.Dictionary[string,object]' ([StringComparer]::Ordinal)
foreach ($f in @(Inventario $stage)) { $esperados.Add($f.Path,$f) }

Write-Host '4/6 Creando el ZIP y verificando todas sus entradas y hashes...'
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip=Join-Path $build ($nombre+'.zip')
[IO.Compression.ZipFile]::CreateFromDirectory($stage,$zip,[IO.Compression.CompressionLevel]::Optimal,$true)
$dirs=New-Object 'Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
[void]$dirs.Add($nombre)
foreach ($rel in $esperados.Keys) {
    $parent=$nombre+'/'+$rel
    while ($parent.Contains('/')) { $parent=$parent.Substring(0,$parent.LastIndexOf('/')); [void]$dirs.Add($parent) }
}
$vistos=New-Object 'Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase)
$vistosArchivos=New-Object 'Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
$archivoZip=[IO.Compression.ZipFile]::OpenRead($zip)
try {
    foreach ($e in $archivoZip.Entries) {
        $directorio=$e.FullName.EndsWith('/')
        $ruta=$e.FullName.TrimEnd('/')
        Validar-Ruta $ruta
        if (!$vistos.Add($ruta)) { throw ('Entrada duplicada en el ZIP: '+$ruta) }
        if ((($e.ExternalAttributes -shr 16) -band 61440) -eq 40960) { throw ('El ZIP contiene un enlace: '+$ruta) }
        if ($directorio) {
            if (!$dirs.Contains($ruta) -or $e.Length -ne 0) { throw ('Directorio inesperado en el ZIP: '+$ruta) }
            continue
        }
        if (!$ruta.StartsWith($nombre+'/',[StringComparison]::Ordinal)) { throw ('Entrada fuera de la raiz prevista: '+$ruta) }
        $rel=$ruta.Substring($nombre.Length+1)
        if (!(Permitido $rel) -or !$esperados.ContainsKey($rel)) { throw ('Archivo inesperado dentro del ZIP: '+$rel) }
        if ($e.Length -ne $esperados[$rel].Bytes) { throw ('Tamano incorrecto en el ZIP: '+$rel) }
        $stream=$e.Open()
        $sha=[Security.Cryptography.SHA256]::Create()
        try { $hash=([BitConverter]::ToString($sha.ComputeHash($stream))).Replace('-','').ToLowerInvariant() }
        finally { $sha.Dispose(); $stream.Dispose() }
        if ($hash -cne $esperados[$rel].SHA256) { throw ('SHA256 incorrecto dentro del ZIP: '+$rel) }
        [void]$vistosArchivos.Add($rel)
    }
    if ($vistosArchivos.Count -ne $esperados.Count) { throw 'El ZIP no contiene todos los archivos del manifiesto.' }
} finally { $archivoZip.Dispose() }
$zipSha=(Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash.ToLowerInvariant()
[IO.File]::WriteAllText(($zip+'.sha256'),$zipSha+'  '+[IO.Path]::GetFileName($zip)+"`n",$utf8)

Write-Host '5/6 Extrayendo el ZIP en una carpeta limpia y verificando otra vez el contenido...'
$limpia=Join-Path $build 'prueba-limpia'
New-Item -ItemType Directory -Path $limpia | Out-Null
[IO.Compression.ZipFile]::ExtractToDirectory($zip,$limpia)
$copia=Join-Path $limpia $nombre
$extraidos=@(Inventario $copia)
if ($extraidos.Count -ne $esperados.Count) { throw 'La extraccion cambio la cantidad de archivos.' }
foreach ($f in $extraidos) {
    if (!$esperados.ContainsKey($f.Path) -or $f.SHA256 -cne $esperados[$f.Path].SHA256 -or $f.Bytes -ne $esperados[$f.Path].Bytes) { throw ('Archivo extraido distinto: '+$f.Path) }
}
if (Test-Path -LiteralPath (Join-Path $copia 'Datos')) { throw 'La extraccion publica contiene Datos.' }

Write-Host '6/6 Guardando el resultado de la candidata publica...'
$resumen=[ordered]@{Zip=$zip;Sha256=$zipSha;CarpetaPrueba=$copia;Launcher=(Join-Path $copia 'ArcadeBrosLauncher.exe');
    Kit=(Join-Path $copia 'Bin');Manifest=(Join-Path $copia 'MANIFIESTO-SHA256.txt');Version=$Version;RuntimeGenerico=$true}
[IO.File]::WriteAllText((Join-Path $build 'resultado.json'),($resumen|ConvertTo-Json),$utf8)
Write-Host 'PAQUETE PUBLICO PREPARADO' -ForegroundColor Green
Write-Host ('ZIP: '+$zip)
Write-Host ('SHA256: '+$zipSha)
Write-Host ('Copia limpia para probar: '+$resumen.Launcher)
Write-Host 'La candidata no incluye Datos ni se ha ejecutado con una ROM o publicado en GitHub.'
