# Crear el paquete Windows x64

El jugador extrae el ZIP, abre `ArcadeBrosLauncher.exe` y selecciona su ROM
compatible. La preparación inicial se realiza con las herramientas incluidas
y se conserva en `Datos`. No necesita instalar un entorno de desarrollo.

El proceso de construcción del ZIP está separado de esa preparación local.
El empaquetador compila el motor genérico y las herramientas sin consultar una
ROM. La ROM, el C traducido y la DLL del juego se generan únicamente después,
en el equipo del jugador.

## Herramientas del desarrollador

- Windows x64 y Git.
- Visual Studio 2022 o Build Tools 2022 con C++ y Windows SDK.
- CMake 3.21 o posterior.
- Python 3.11 o posterior para preparar las cabeceras genéricas.
- Python 3.14 o posterior si el script también debe extraer el archivo LLVM
  comprimido en Zstandard. El archivo oficial requiere una ventana de
  descompresión de 1 GiB; el extractor trabaja en flujo y extrae solo el
  subconjunto necesario.

El paquete utiliza LLVM/Clang/LLD 23.1.4 para Windows x64. Su archivo oficial
de 276.6 MB se verifica mediante este SHA256 antes de ejecutarse:

```text
1a3f2f998e230b5791cc4e2bfe11a1f7c07fe42c96ff44f227c9db0bedfaf091
```

## Desde un clon

Inicializa los submódulos y ejecuta el preparador del proyecto:

```powershell
git submodule update --init --recursive
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\preparar_dependencias.ps1
```

Conserva los cambios locales de las dependencias: son los parches acumulativos
del proyecto sobre sus revisiones fijadas. El preparador verifica los parches
y se detiene ante cambios incompatibles.

Para construir el ZIP, indica la ruta real de Python en tu equipo:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
    -File .\tools\native_module\crear_publico.ps1 `
    -Python "C:\ruta\a\python.exe"
```

El script puede descargar y verificar LLVM. Si ya tienes el subconjunto de
LLVM 23.1.4 preparado, utiliza `-Compilador` con su carpeta que contiene
`bin/clang.exe`, `bin/lld-link.exe` y `lib/clang/23/include`. En este caso basta
Python 3.11 para la preparación del SDK genérico.

La construcción no depende de los experimentos `m100`, `p100`, `i100` o `l100`
ni requiere una ROM en el repositorio. Usa el proyecto de CMake de
`tools/native_module` con `ARCADE_WINDOWED_HOST=ON` y
`ARCADE_PREPARE_MODULE=OFF`.

## Resultado y comprobación

Cada ejecución crea una carpeta nueva `build/r100-...` y conserva sus registros.
Dentro aparecen:

- `ArcadeBros1983-Recomp-v1.0.0-Windows-x64.zip`.
- El archivo `.zip.sha256` con su huella.
- `resultado.json` con las rutas del artefacto y la copia limpia.
- `prueba-limpia`, extraída desde el ZIP para comprobar su primera apertura.

El empaquetador monta una lista explícita de archivos, añade los avisos,
calcula `kit-id.txt` y un manifiesto SHA256, comprueba las entradas y hashes
dentro del ZIP y vuelve a verificarlos después de extraerlo. El resultado
contiene ejecutables genéricos, cabeceras, recursos y avisos. `Datos`, las ROMs,
las pistas del usuario y los módulos generados quedan fuera.

Abre el launcher de la copia limpia, selecciona una ROM compatible y comprueba
la preparación inicial, una partida, el menú Esc y una segunda apertura. Esta
copia pasa a tener datos privados; el ZIP original permanece separado de ella.

El SHA256 final del ZIP se genera en cada construcción. El proceso conserva
fuentes, versiones y manifiestos para revisión; no se garantiza una huella
binaria idéntica entre distintos equipos o versiones de las herramientas.

Publica el ZIP original y su `.sha256` desde la etiqueta y el commit revisados
de la release. Los componentes y avisos se describen en `SOURCES.txt` y
`Bin/licenses`. El repositorio conserva los parches sobre NESRecomp y recomp-ui.
