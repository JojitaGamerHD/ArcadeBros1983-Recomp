param([Parameter(Mandatory=$true)][string]$Root,
      [Parameter(Mandatory=$true)][string]$Scratch)
$ErrorActionPreference = 'Stop'
$ast = [Management.Automation.Language.Parser]::ParseFile(
    (Join-Path $Root 'tools/preparar_dependencias.ps1'), [ref]$null, [ref]$null)
foreach ($name in @('Verificar-Comando', 'Aplicar-Parche')) {
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
    Write-Output ("PASS $($case[0]): limpio, idempotente y migracion desde $($previous.Count) parches anteriores.")
}
