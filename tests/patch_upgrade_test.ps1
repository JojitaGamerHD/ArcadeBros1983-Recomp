param([Parameter(Mandatory=$true)][string]$Root,
      [Parameter(Mandatory=$true)][string]$Scratch)
$ErrorActionPreference = 'Stop'
$ast = [Management.Automation.Language.Parser]::ParseFile(
    (Join-Path $Root 'tools/preparar_dependencias.ps1'), [ref]$null, [ref]$null)
foreach ($name in @('Verificar-Comando', 'Probar-ParcheAplicado', 'Aplicar-Parche', 'Aplicar-ParchesMenu')) {
    $node = $ast.Find({ param($n)
        $n -is [Management.Automation.Language.FunctionDefinitionAst] -and $n.Name -eq $name
    }, $true)
    if (!$node) { throw "No se encontro $name" }
    . ([scriptblock]::Create($node.Extent.Text))
}
$fixtureRoot = Join-Path $Scratch ('patch-upgrade-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $fixtureRoot | Out-Null
foreach ($case in @(
    @('ui', 'recomp-ui', 'patches/recomp-ui', 'arcade-v1.0.patch', 'arcade-v1.0.1-editor.patch'),
    @('engine', 'deps/nesrecomp-arcade', 'patches/nesrecomp', 'arcade-v0.9.patch', 'arcade-v1.0.1-estable.patch'))) {
    $source = Join-Path $Root $case[1]
    $fixture = Join-Path $fixtureRoot $case[0]
    $archive = Join-Path $fixtureRoot ($case[0] + '.tar')
    New-Item -ItemType Directory -Path $fixture | Out-Null
    git -C $source archive --format=tar "--output=$archive" HEAD
    Verificar-Comando 'No se pudo crear la copia local de la revision fijada.'
    tar -xf $archive -C $fixture
    Verificar-Comando 'No se pudo extraer la copia de prueba.'
    git -C $fixture init --quiet
    Verificar-Comando 'No se pudo aislar la copia de prueba.'
    $patchRoot = Join-Path $Root $case[2]
    $current = Join-Path $patchRoot $case[4]
    $previous = @('arcade-v1.0.1-editor.patch', 'arcade-v1.0.1-colores.patch',
                  'arcade-v1.0.1.patch', $case[3]) |
        Where-Object { $_ -ne $case[4] } | ForEach-Object { Join-Path $patchRoot $_ }
    $sentinel = Join-Path $fixture 'mis-cambios-locales.txt'
    [IO.File]::WriteAllText($sentinel, 'conservar')

    Aplicar-Parche $fixture $current $case[0] $previous
    Aplicar-Parche $fixture $current $case[0] $previous
    git -C $fixture apply --reverse $current
    Verificar-Comando 'Fallo la aplicacion limpia o la idempotencia.'
    $menu = Join-Path $patchRoot 'arcade-v1.0.2-menu.patch'
    $ajustes = Join-Path $patchRoot 'arcade-v1.0.2-ajustes.patch'
    Aplicar-ParchesMenu $fixture $current $menu $ajustes $case[0] $previous
    Aplicar-ParchesMenu $fixture $current $menu $ajustes $case[0] $previous
    git -C $fixture apply --reverse $ajustes
    Verificar-Comando 'Fallo la aplicacion limpia o la idempotencia de los ajustes.'
    git -C $fixture apply --reverse $menu
    Verificar-Comando 'Los ajustes alteraron la capa del menu publicado.'
    git -C $fixture apply --reverse $current
    Verificar-Comando 'El parche del menu altero la base de v1.0.1.'

    # Reproduce an existing developer checkout of the published menu.
    Aplicar-Parche $fixture $current $case[0]
    Aplicar-Parche $fixture $menu $case[0]
    Aplicar-ParchesMenu $fixture $current $menu $ajustes $case[0] $previous
    git -C $fixture apply --reverse $ajustes
    Verificar-Comando 'No se pudieron actualizar los ajustes sobre el menu publicado.'
    git -C $fixture apply --reverse $menu
    Verificar-Comando 'No se pudo restablecer el menu publicado.'
    git -C $fixture apply --reverse $current
    Verificar-Comando 'El parche del menu altero la base de v1.0.1.'
    foreach ($old in $previous) {
        git -C $fixture apply $old
        Verificar-Comando 'No se pudo preparar la version anterior.'
        Aplicar-Parche $fixture $current $case[0] $previous
        git -C $fixture apply --reverse --check $current
        Verificar-Comando 'La migracion no llego al parche actual.'
        git -C $fixture apply --reverse $current
        Verificar-Comando 'No se pudo restablecer la copia de prueba.'
    }
    if ([IO.File]::ReadAllText($sentinel) -ne 'conservar') { throw 'Se altero un archivo ajeno al parche.' }
    Write-Output ("PASS $($case[0]): limpio, idempotente y actualizacion desde el menu publicado y $($previous.Count) bases anteriores.")
}
