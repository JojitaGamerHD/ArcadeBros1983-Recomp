# Motor generico e importacion local de v1.0.0

## Crear el ZIP de distribucion

`crear_publico.ps1 -Python C:\ruta\python.exe` construye una carpeta nueva de
distribucion sin leer una ROM ni reutilizar los experimentos de desarrollo.
Prepara el SDK desde las cabeceras genericas y puede obtener LLVM 23.1.4
mediante `obtener_llvm.ps1` y su extractor verificado. Si LLVM ya esta preparado,
se puede indicar su directorio con `-Compilador`.

El resultado contiene ZIP, SHA256, manifiesto de archivos y una copia extraida
limpia. El script comprueba la lista de archivos permitidos y todos los hashes
dentro del ZIP y despues de extraerlo. Las instrucciones completas estan en
[COMPILAR-PORTABLE-v1.0.0](../../docs/COMPILAR-PORTABLE-v1.0.0.md).

Las siguientes secciones conservan las pruebas progresivas que validan la
separacion del motor, la importacion y la interfaz.

Experimento de desarrollo para separar el motor de las instrucciones del juego.
El motor se compila primero sin leer la ROM ni enlazar sus fuentes generadas.
Despues se genera una DLL privada a partir de la ROM local compatible, se carga
en el motor y se compara con una compilacion de referencia.

La primera prueba utiliza Visual Studio, CMake y Python del desarrollador.
Las siguientes etapas preparan el importador y el launcher portable.

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

## Launcher portable con ventana

`probar_launcher.ps1 -Prueba build/m100-... -Importacion build/i100-... -Compilador build/p100-...`
construye el motor generico con SDL, el launcher y los menus existentes,
record permanente, musica opcional y suavizado experimental. Genera una
carpeta `build/l100-.../portable` para pruebas locales. No crea una release.

`ArcadeBrosLauncher.exe` prepara la ROM compatible una vez y reutiliza su modulo
en aperturas posteriores. La preparacion muestra una ventana de progreso con
cancelacion del importador y sus hijos. `Bin/kit-id.txt` identifica la version
del kit; una version diferente recibe una cache nueva. El recibo local verifica
el SHA256 de la DLL y la ROM antes de reutilizarlas. Las preparaciones anteriores
se conservan, incluida cualquier preparacion interrumpida.

`Bin` contiene el motor y las herramientas genericas. `Datos/Cache` contiene
la ROM y la DLL privadas; `Datos/config.ini` y `Datos/Records` guardan ajustes
y records. La musica aportada por el usuario se busca en `Datos/Music/stage1.m4a`;
se conserva la ruta anterior junto al ejecutable como alternativa compatible.
El script de prueba copia la pista local a Datos y activa su opcion si existe.

La prueba verifica la primera importacion, la reutilizacion sin recompilar,
el rechazo de otra ROM, equivalencia de 960 fotogramas y audio, las dependencias
y la integridad de Bin. Captura el launcher oculto mediante LNG_SCRIPT y el
juego y su pausa mediante el servidor TCP local de pruebas. Los controles de
verificacion son opcionales del motor; la apertura normal no habilita ese puerto.
Para una distribucion publica se preparara otro paquete excluyendo `Datos`,
los registros y todos los archivos generados desde la ROM.

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
