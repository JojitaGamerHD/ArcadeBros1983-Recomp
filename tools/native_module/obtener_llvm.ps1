param([string]$Python,[string]$Destino)
$ErrorActionPreference='Stop'
$ProgressPreference='SilentlyContinue'
$raiz=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '../..')).Path
if(!$Python){$Python=(Get-Command python.exe -ErrorAction Stop).Source}
if(!(Test-Path -LiteralPath $Python -PathType Leaf) -or $Python -like '*WindowsApps*'){
    throw 'Indica la ruta de Python 3.14 o posterior con -Python para extraer LLVM.'
}
if(!$Destino){$Destino=Join-Path $raiz ('build/llvm23-'+[guid]::NewGuid().ToString('N').Substring(0,8))}
$Destino=[IO.Path]::GetFullPath($Destino)
$cache=Join-Path $raiz 'build/descargas'
New-Item -ItemType Directory -Force -Path $cache | Out-Null
$archivo=Join-Path $cache 'llvm-23.1.4-win64.tar.zst'
$sha='1A3F2F998E230B5791CC4E2BFE11A1F7C07FE42C96FF44F227C9DB0BEDFAF091'
if(!(Test-Path -LiteralPath $archivo -PathType Leaf)){
    Write-Host 'Descargando LLVM 23.1.4 oficial (276.6 MB)...'
    $temporal=Join-Path $cache ('llvm-'+[guid]::NewGuid().ToString('N')+'.part')
    [Net.ServicePointManager]::SecurityProtocol=[Net.ServicePointManager]::SecurityProtocol -bor [Net.SecurityProtocolType]::Tls12
    Invoke-WebRequest -UseBasicParsing -Uri 'https://github.com/llvm/llvm-project/releases/download/llvmorg-23.1.4/clang%2Bllvm-23.1.4-x86_64-pc-windows-msvc.tar.zst' -OutFile $temporal -TimeoutSec 600
    if((Get-FileHash -LiteralPath $temporal -Algorithm SHA256).Hash -ne $sha){throw ('La descarga no coincide con el SHA256 oficial: '+$temporal)}
    Move-Item -LiteralPath $temporal -Destination $archivo
}
if((Get-FileHash -LiteralPath $archivo -Algorithm SHA256).Hash -ne $sha){throw 'El archivo LLVM en cache no coincide con el SHA256 oficial.'}
$helper=Join-Path $PSScriptRoot 'extraer_llvm.py'
if(!(Test-Path -LiteralPath $helper)){throw 'Falta extraer_llvm.py.'}
foreach($a in @($helper,$archivo,$Destino)){
    if($a.Contains('"') -or $a.EndsWith('\')){throw 'Ruta inesperada para el extractor.'}
}
$pi=New-Object Diagnostics.ProcessStartInfo
$pi.FileName=$Python
$pi.Arguments=(@('-I',$helper,$archivo,$Destino)|ForEach-Object{'"'+$_+'"'}) -join ' '
$pi.UseShellExecute=$false
$pi.CreateNoWindow=$true
$pi.WindowStyle=[Diagnostics.ProcessWindowStyle]::Hidden
$pi.RedirectStandardOutput=$true
$pi.RedirectStandardError=$true
$p=New-Object Diagnostics.Process
$p.StartInfo=$pi
Write-Host 'Extrayendo las herramientas verificadas con Python...'
try{
    [void]$p.Start()
    $out=$p.StandardOutput.ReadToEndAsync()
    $err=$p.StandardError.ReadToEndAsync()
    $p.WaitForExit()
    $texto=$out.Result+$err.Result
    $codigo=$p.ExitCode
}finally{$p.Dispose()}
if($codigo -ne 0){throw ('No se pudo extraer LLVM: '+$texto)}
[pscustomobject]@{Compilador=$Destino;Version='23.1.4';ArchivoSHA256=$sha}
