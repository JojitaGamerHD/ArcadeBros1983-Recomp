param(
    [Parameter(Mandatory=$true)][string]$Ejecutable,
    [Parameter(Mandatory=$true)][string]$Modulo,
    [Parameter(Mandatory=$true)][string]$Rom,
    [Parameter(Mandatory=$true)][string]$Datos,
    [Parameter(Mandatory=$true)][string]$Salida
)
$ErrorActionPreference='Stop'
New-Item -ItemType Directory -Path $Salida | Out-Null
$utf8=[Text.UTF8Encoding]::new($false)
$config=Join-Path $Salida 'config.ini'
[IO.File]::WriteAllText($config,"[Display]`nWindowScale=3`nFullscreen=0`nTargetFPS=60`nVSync=0`n[Audio]`nEnabled=1`nMusicEnabled=1`nMusicVolume=35`n",$utf8)

function Iniciar-Ventana([string[]]$Argumentos,[hashtable]$Entorno) {
    $pi=New-Object Diagnostics.ProcessStartInfo
    $pi.FileName=$Ejecutable
    foreach($a in $Argumentos){if($a.Contains('"') -or $a.EndsWith('\')){throw 'Argumento inesperado de ventana.'}}
    $pi.Arguments=($Argumentos|ForEach-Object{'"'+$_+'"'}) -join ' '
    $pi.WorkingDirectory=$Datos
    $pi.UseShellExecute=$false
    $pi.CreateNoWindow=$true
    $pi.WindowStyle=[Diagnostics.ProcessWindowStyle]::Hidden
    $pi.RedirectStandardOutput=$true
    $pi.RedirectStandardError=$true
    foreach($n in @('NESRECOMP_NO_LAUNCHER','LNG_SCRIPT','LNG_TEST_HIDDEN','SDL_VIDEODRIVER','SDL_AUDIODRIVER')){
        $pi.EnvironmentVariables.Remove($n)
    }
    foreach($n in $Entorno.Keys){$pi.EnvironmentVariables[$n]=[string]$Entorno[$n]}
    $p=New-Object Diagnostics.Process
    $p.StartInfo=$pi
    [void]$p.Start()
    [pscustomobject]@{Proceso=$p;Out=$p.StandardOutput.ReadToEndAsync();Err=$p.StandardError.ReadToEndAsync()}
}
function Finalizar-Ventana($Trabajo,[string]$Nombre) {
    $p=$Trabajo.Proceso
    if(!$p.WaitForExit(30000)) {
        # This is the process created by this check, never another user process.
        $p.Kill()
        $p.WaitForExit()
        [IO.File]::WriteAllText((Join-Path $Salida ($Nombre+'.log')),$Trabajo.Out.Result+$Trabajo.Err.Result,$utf8)
        $p.Dispose()
        throw ('La ventana de prueba no termino. Registro: '+$Nombre+'.log')
    }
    $codigo=$p.ExitCode
    $texto=$Trabajo.Out.Result+$Trabajo.Err.Result
    [IO.File]::WriteAllText((Join-Path $Salida ($Nombre+'.log')),$texto,$utf8)
    $p.Dispose()
    if($codigo -ne 0){throw ('Fallo la ventana de prueba '+$Nombre+' (codigo '+$codigo+'). Revisa su registro.')}
    return $texto
}

# Exercise the real launcher renderer without showing or focusing a window.
$dashboard=Join-Path $Salida 'launcher.png'
$settings=Join-Path $Salida 'launcher-opciones.png'
$script='wait:8;shot:'+$dashboard+';view:settings;wait:6;shot:'+$settings+';quit'
$w=Iniciar-Ventana @($Modulo,$Rom,'--config',$config,'--no-save','--hidden') @{
    LNG_SCRIPT=$script;LNG_TEST_HIDDEN='1';SDL_AUDIODRIVER='dummy'
}
[void](Finalizar-Ventana $w 'launcher')
foreach($p in @($dashboard,$settings)){
    if(!(Test-Path -LiteralPath $p) -or (Get-Item -LiteralPath $p).Length -lt 1000){throw 'Falta una captura del launcher.'}
}

# The game and pause menu use SDL's software renderer and the TCP test API.
$listener=[Net.Sockets.TcpListener]::new([Net.IPAddress]::Loopback,0)
$listener.Start()
$port=$listener.LocalEndpoint.Port
$listener.Stop()
$w=Iniciar-Ventana @($Modulo,$Rom,'--config',$config,'--no-save','--hidden','--tcp',[string]$port,'--exit-after','1800') @{
    NESRECOMP_NO_LAUNCHER='1';SDL_VIDEODRIVER='dummy';SDL_AUDIODRIVER='dummy'
}
$client=$null
$reader=$null
$writer=$null
$id=0
$fallo=$null
try {
    $limite=[DateTime]::UtcNow.AddSeconds(20)
    while([DateTime]::UtcNow -lt $limite) {
        if($w.Proceso.HasExited){throw 'El juego termino antes de abrir el servidor de prueba.'}
        $c=[Net.Sockets.TcpClient]::new([Net.Sockets.AddressFamily]::InterNetwork)
        try {$c.Connect([Net.IPAddress]::Loopback,$port);$client=$c;break} catch {
            $c.Dispose()
            if($_.Exception.InnerException -is [Net.Sockets.SocketException] -and
                $_.Exception.InnerException.NativeErrorCode -eq 10013){
                throw 'Windows bloqueo la conexion local de la prueba del menu (socket 10013).'
            }
            Start-Sleep -Milliseconds 100
        }
    }
    if(!$client){throw 'El juego no abrio el servidor de prueba.'}
    $client.ReceiveTimeout=5000
    $client.SendTimeout=5000
    $stream=$client.GetStream()
    $reader=[IO.StreamReader]::new($stream,$utf8,$false,4096,$true)
    $writer=[IO.StreamWriter]::new($stream,$utf8,4096,$true)
    $writer.AutoFlush=$true
    function Enviar([hashtable]$Comando) {
        $script:id++
        $Comando['id']=$script:id
        $writer.WriteLine(($Comando|ConvertTo-Json -Compress))
        $line=$reader.ReadLine()
        if(!$line){throw 'El servidor de prueba cerro la conexion.'}
        $r=$line|ConvertFrom-Json
        if(!$r.ok){throw ('La orden de prueba fallo: '+$r.err)}
        return $r
    }
    $state=Enviar @{cmd='state'}
    if(!$state.native -or !$state.recomp_ui -or $state.dev_ui){throw 'El runtime no tiene la configuracion esperada.'}
    [void](Enviar @{cmd='screenshot';layer='ui';path=(Join-Path $Salida 'juego.png')})
    $menu=Enviar @{cmd='menu';open=$true}
    if(!$menu.open){throw 'El menu de pausa no se abrio.'}
    [void](Enviar @{cmd='screenshot';layer='ui';path=(Join-Path $Salida 'pausa.png')})
    Start-Sleep -Milliseconds 60
    $before=Enviar @{cmd='state'}
    Start-Sleep -Milliseconds 120
    $after=Enviar @{cmd='state'}
    if(!$after.menu_open -or $before.frame -ne $after.frame){throw 'El menu no mantuvo el juego en pausa.'}
    [void](Enviar @{cmd='menu';open=$false})
    Start-Sleep -Milliseconds 100
    $after=Enviar @{cmd='state'}
    if($after.menu_open -or $after.frame -le $before.frame){throw 'El juego no continuo al cerrar el menu.'}
    [IO.File]::WriteAllText((Join-Path $Salida 'estado.json'),($after|ConvertTo-Json -Depth 8),$utf8)
    [void](Enviar @{cmd='quit'})
} catch {
    $fallo=$_
    if($writer -and !$w.Proceso.HasExited){try{[void](Enviar @{cmd='quit'})}catch{}}
} finally {
    if($writer){$writer.Dispose()}
    if($reader){$reader.Dispose()}
    if($client){$client.Dispose()}
    try{$texto=Finalizar-Ventana $w 'juego'}catch{if(!$fallo){$fallo=$_}}
}
if($fallo){throw $fallo}
if(Test-Path -LiteralPath (Join-Path $Datos 'Music/stage1.m4a')){
    if($texto -notmatch '\[arcade music\] pista cargada:'){throw 'No se cargo la pista privada de Datos/Music.'}
}
foreach($p in @('juego.png','pausa.png')){
    if(!(Test-Path -LiteralPath (Join-Path $Salida $p))){throw ('Falta captura: '+$p)}
}
Write-Host 'Launcher, opciones y menu de pausa comprobados en ventanas ocultas.'
