# Motor genérico e importación local

## Crear el ZIP de distribucion

`crear_publico.ps1 -Python C:\ruta\python.exe` construye una carpeta nueva de
distribucion sin leer una ROM ni reutilizar los experimentos de desarrollo.
Prepara el SDK desde las cabeceras genericas y puede obtener LLVM 23.1.4
mediante `obtener_llvm.ps1` y su extractor verificado. Si LLVM ya esta preparado,
se puede indicar su directorio con `-Compilador`.

El resultado contiene ZIP, SHA256, manifiesto de archivos y una copia extraida
limpia. El script comprueba la lista de archivos permitidos y todos los hashes
dentro del ZIP y después de extraerlo. La carpeta `rom` se entrega vacía.
Ejecuta `Bin/ArcadeNativeHost.exe` de la copia extraída para probar el menú.

Las siguientes secciones conservan las pruebas progresivas que validan la
separacion del motor, la importacion y la interfaz.

Experimento de desarrollo para separar el motor de las instrucciones del juego.
El motor se compila primero sin leer la ROM ni enlazar sus fuentes generadas.
Despues se genera una DLL privada a partir de la ROM local compatible, se carga
en el motor y se compara con una compilacion de referencia.

La primera prueba utiliza Visual Studio, CMake y Python del desarrollador.
Las siguientes etapas preparan el importador y el menú integrado.

## Importacion autonoma

`probar_importacion.ps1 -Prueba build/m100-... -Compilador build/p100-...`
prepara un kit generico con `ArcadePrepare.exe`, el motor, NESRecomp y Clang/LLD.
Visual Studio y CMake solo se usan para construir esas herramientas. Despues,
ArcadePrepare verifica la ROM local con SHA256, genera el C privado, compila y
enlaza la DLL sin Python, PowerShell, CMake ni Visual Studio. Los ejecutables
propios usan el runtime C estatico; Clang/LLD solo importan DLLs de Windows.

El kit no recibe la ROM ni el modulo: ambos se guardan en una cache nueva fuera
del kit, con registros y `complete.json`. El prototipo rechaza una ROM distinta,
una ROM ausente o una cache existente. Las rutas usan UTF-8 en Windows 10 1903 o
posterior. La prueba usa una cache con espacios y un caracter acentuado.

La prueba compara estado, audio y ciclos nativos de 960 fotogramas con la
referencia, comprueba las dependencias y confirma que el kit no cambia.

## Menú integrado

`probar_launcher.ps1 -Prueba build/m100-... -Importacion build/i100-... -Compilador build/p100-...`
construye el motor genérico con SDL, el menú principal, récord persistente,
música opcional y suavizado experimental. Genera una carpeta
`build/l102-.../portable` para pruebas locales.

`Bin/ArcadeNativeHost.exe` abre el menú principal sin ROM y conserva la misma
ventana al comenzar una partida o al volver desde la pausa. Coloca tu ROM
compatible en `rom/game.nes`, junto a la carpeta `Bin` del portable, y elige
**Cargar ROM**. El menú indica si falta el archivo. La preparación muestra el
progreso y permite cancelar el importador. En aperturas posteriores se
reutiliza el módulo si el kit, la DLL y la ROM coinciden con el recibo local.
`Bin/kit-id.txt` identifica la versión del kit.

`Bin` contiene el motor y las herramientas genericas. `Datos/Cache` contiene
la ROM y la DLL privadas; `Datos/config.ini` y `Datos/Records` guardan ajustes
y records. La musica aportada por el usuario se busca en `Datos/Music/stage1.m4a`;
se conserva la ruta anterior junto al ejecutable como alternativa compatible.
El script de prueba copia la pista local a Datos y activa su opcion si existe.

La prueba verifica el menú principal y Opciones antes de importar la ROM.
Comprueba después la primera importación, la reutilización de la caché, el
rechazo de otra ROM, la equivalencia de 960 fotogramas y audio, las dependencias
y la integridad de `Bin`. Captura el menú principal mediante `LNG_SCRIPT`, y
el juego y su pausa mediante el servidor TCP local de pruebas. La apertura
normal deja ese puerto desactivado.

Para comprobar únicamente el arranque y Opciones, ejecuta desde PowerShell con
una copia extraída limpia del ZIP:

```powershell
$portable = 'C:\ruta\a\la\copia\extraida'
.\tools\native_module\verificar_ventana.ps1 -SoloMenu `
    -Ejecutable (Join-Path $portable 'Bin/ArcadeNativeHost.exe') `
    -Salida (Join-Path $PWD 'build/menu-sin-rom')
```

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\native_module\probar.ps1 -Python "C:\ruta\a\python.exe"
```

La prueba no abre una ventana. Comprueba:

- Que el EXE del motor conserva su SHA256 al generar la DLL privada.
- Que estado y audio NES de 960 fotogramas coinciden con la referencia.
- Que se usan los mismos ciclos recompilados, sin sustituirlos silenciosamente
  por una ejecucion exclusivamente interpretada. Se conserva el fallback que
  ya utiliza el motor original para instrucciones no cubiertas.
- Que se rechazan una ABI incompatible y un modulo ausente.

El protocolo y la lista de exportaciones son provisionales y especificos de
Windows x64 y la revision soportada de Mario Bros. El SDK se prepara copiando
cabeceras genericas del motor en build/ y marcando sus datos importados. El
modulo generado usa esas cabeceras y no necesita inicializacion del runtime C.
Las comprobaciones del cargador validan compatibilidad; no aislan codigo nativo
arbitrario. El prototipo solo carga la DLL local generada por esta prueba.

Cada ejecucion conserva sus resultados en una carpeta nueva de build/.
La ROM, la DLL generada, el ejecutable de referencia y los hashes/audio locales
no se deben incorporar a Git ni a una descarga publica. Esta carpeta de fuentes
solo contiene el protocolo, el cargador y las herramientas genericas originales.
