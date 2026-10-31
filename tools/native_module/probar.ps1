param([string]$Python)

$ErrorActionPreference = 'Stop'
$raiz = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '../..')).Path
$cmake = (Get-Command cmake.exe -ErrorAction Stop).Source
if (!$Python) {
    $Python = (Get-Command python.exe -ErrorAction Stop).Source
}
if (!(Test-Path -LiteralPath $Python -PathType Leaf) -or $Python -like '*WindowsApps*') {
    throw 'Indica la ruta de Python real mediante -Python. El alias de Microsoft Store no sirve para esta prueba.'
}
$Python = (Resolve-Path -LiteralPath $Python).Path
$rom = Join-Path $raiz 'roms/Mario Bros..nes'
if (!(Test-Path -LiteralPath $rom -PathType Leaf)) { throw 'Falta la ROM privada compatible en roms.' }
$huellaRom = (Get-FileHash -LiteralPath $rom -Algorithm SHA256).Hash
if ($huellaRom -ne 'A2A0AA437A735CD36ED29A8E7C5B3DFA10590C6203E893F1668C7995BAD2B309') {
    throw 'La ROM no coincide con la revision compatible. No se genero ningun modulo.'
}

# Cada prueba usa una carpeta nueva: los resultados anteriores se conservan.
$identificador = [guid]::NewGuid().ToString('N').Substring(0,8)
$build = Join-Path $raiz ('build/m100-' + $identificador)
if (Test-Path -LiteralPath $build) { throw 'La carpeta de prueba ya existe. Ejecuta otra vez para generar una nueva.' }
New-Item -ItemType Directory -Path $build | Out-Null

function Ejecutar {
    param([string]$Programa, [string[]]$Argumentos, [string]$Registro, [int[]]$Codigos = @(0))
    foreach ($a in $Argumentos) {
        if ($a.Contains('"') -or $a.EndsWith('\')) { throw 'Argumento inesperado para la herramienta.' }
    }
    $inicio = New-Object Diagnostics.ProcessStartInfo
    $inicio.FileName = $Programa
    $inicio.Arguments = ($Argumentos | ForEach-Object { '"' + $_ + '"' }) -join ' '
    $inicio.WorkingDirectory = $build
    $inicio.UseShellExecute = $false
    $inicio.CreateNoWindow = $true
    $inicio.WindowStyle = [Diagnostics.ProcessWindowStyle]::Hidden
    $inicio.RedirectStandardOutput = $true
    $inicio.RedirectStandardError = $true
    $proceso = New-Object Diagnostics.Process
    $proceso.StartInfo = $inicio
    try {
        [void]$proceso.Start()
        $salida = $proceso.StandardOutput.ReadToEndAsync()
        $errores = $proceso.StandardError.ReadToEndAsync()
        $proceso.WaitForExit()
        $texto = $salida.Result + $errores.Result
        $codigo = $proceso.ExitCode
    } finally { $proceso.Dispose() }
    $rutaLog = Join-Path $build ($Registro + '.log')
    [IO.File]::WriteAllText($rutaLog,$texto,[Text.UTF8Encoding]::new($false))
    if ($Codigos -notcontains $codigo) {
        $final = ($texto -split "\r?\n" | Select-Object -Last 16) -join "`n"
        throw "Fallo $Registro (codigo $codigo). Registro: $rutaLog`n$final"
    }
    return [pscustomobject]@{ Codigo=$codigo; Texto=$texto }
}

Write-Host '1/5 Compilando el motor generico sin generar codigo de la ROM...'
[void](Ejecutar $cmake @('-S',$PSScriptRoot,'-B',$build,'-G','Visual Studio 17 2022','-A','x64',('-DARCADE_ROOT='+$raiz),'-DARCADE_PREPARE_MODULE=OFF') 'configurar-motor')
[void](Ejecutar $cmake @('--build',$build,'--config','Release','--target','ArcadeNativeHost','--parallel','1') 'compilar-motor')
$motor = Join-Path $build 'Release/ArcadeNativeHost.exe'
$huellaMotor = (Get-FileHash -LiteralPath $motor -Algorithm SHA256).Hash

Write-Host '2/5 Generando y compilando el modulo privado desde tu ROM...'
[void](Ejecutar $cmake @('-S',$PSScriptRoot,'-B',$build,'-DARCADE_PREPARE_MODULE=ON',('-DPython3_EXECUTABLE='+$Python)) 'configurar-modulo')
[void](Ejecutar $cmake @('--build',$build,'--config','Release','--target','ArcadeLocalModule','ArcadeNativeReference','ArcadeBadModule','--parallel','1') 'compilar-modulo')
if ($huellaMotor -ne (Get-FileHash -LiteralPath $motor -Algorithm SHA256).Hash) {
    throw 'El ejecutable del motor cambio al generar el modulo. Comparte el resultado.'
}
Write-Host 'Motor generico: SHA256 sin cambios.'

Write-Host '3/5 Comparando 960 fotogramas y el audio NES...'
$entrada = Join-Path $PSScriptRoot 'input.txt'
$resultados = @{}
foreach ($caso in @('referencia','modulo')) {
    $argumentos = @($rom,'--frames','960','--input',$entrada,'--hash-out',(Join-Path $build ($caso+'.hash')),
        '--wav-out',(Join-Path $build ($caso+'.wav')),'--no-save')
    $programa = Join-Path $build 'Release/ArcadeNativeReference.exe'
    if ($caso -eq 'modulo') {
        $programa = $motor
        $argumentos = @((Join-Path $build 'Release/ArcadeLocalModule.dll')) + $argumentos
    }
    $resultado = Ejecutar $programa $argumentos $caso
    if ($resultado.Texto -notmatch 'mode=native frames=960 cycles=(\d+) native_cycles=(\d+)') {
        throw "No se encontro el resumen de ejecucion nativa: $caso"
    }
    $resultados[$caso] = [pscustomobject]@{ Ciclos=[uint64]$Matches[1]; Nativos=[uint64]$Matches[2] }
}
foreach ($tipo in @('hash','wav')) {
    $a = (Get-FileHash -LiteralPath (Join-Path $build ('referencia.'+$tipo)) -Algorithm SHA256).Hash
    $b = (Get-FileHash -LiteralPath (Join-Path $build ('modulo.'+$tipo)) -Algorithm SHA256).Hash
    if ($a -ne $b) { throw "La comparacion no coincide: $tipo" }
}
if (!$resultados['modulo'].Nativos -or
    $resultados['modulo'].Nativos -ne $resultados['referencia'].Nativos -or
    $resultados['modulo'].Ciclos -ne $resultados['referencia'].Ciclos) {
    throw 'Los ciclos recompilados no coinciden con la referencia.'
}
Write-Host ('Estado y audio identicos; ciclos nativos: ' + $resultados['modulo'].Nativos)

Write-Host '4/5 Comprobando el rechazo de un modulo incompatible...'
$r = Ejecutar $motor @((Join-Path $build 'Release/ArcadeBadModule.dll'),$rom,'--frames','1','--no-save') 'rechazo-abi' @(2)
if ($r.Texto -notmatch 'ABI mismatch') { throw 'El modulo incompatible no fue rechazado por ABI.' }

Write-Host '5/5 Comprobando el rechazo de un modulo ausente...'
$r = Ejecutar $motor @((Join-Path $build 'Release/no-existe.dll'),$rom,'--frames','1','--no-save') 'rechazo-ausente' @(2)
if ($r.Texto -notmatch 'not an accessible file') { throw 'La ausencia del modulo no se manejo como se esperaba.' }

Write-Host 'PRUEBA DE MODULO NATIVO COMPLETADA' -ForegroundColor Green
Write-Host ('Resultados y registros: ' + $build)
Write-Host 'Es una prueba tecnica sin ventana; todavia no es el ZIP publico de v1.0.0.'
Write-Host 'La DLL local, el ejecutable de referencia y los datos de esta prueba permanecen en build/.'
