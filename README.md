# Arcade Bros 1983 Recomp

**Port no oficial de Mario Bros. (NES) para Windows x64 mediante recompilación estática local con NESRecomp.**

[![Estado](https://img.shields.io/badge/estado-Jugable-green)](https://github.com/JojitaGamerHD/ArcadeBros1983-Recomp)
[![Plataforma](https://img.shields.io/badge/plataforma-Windows%20x64-blue)](https://github.com/JojitaGamerHD/ArcadeBros1983-Recomp)

**Versión: v1.0.0** · Desarrollo en `desarrollo-v1.0.0`

v1.0.0 incorpora un ZIP con ejecutable y preparación automática desde la ROM del
jugador. La primera apertura prepara un módulo nativo privado; las siguientes
reutilizan la caché. El jugador puede descargar, extraer y abrir el launcher
sin instalar Visual Studio, CMake ni Python.

El repositorio contiene fuentes, scripts y parches. El ZIP contiene el motor
y las herramientas genéricas. Cada jugador aporta su propia ROM compatible
y, si quiere música de fondo, una pista local. El proyecto es independiente,
sin fines comerciales y sin afiliación con Nintendo.

## Descargar y jugar

En [Releases](https://github.com/JojitaGamerHD/ArcadeBros1983-Recomp/releases),
busca el archivo de Windows de esta versión:

```text
ArcadeBros1983-Recomp-v1.0.0-Windows-x64.zip
```

1. Extrae el ZIP completo en una carpeta con permiso de escritura.
2. Abre `ArcadeBrosLauncher.exe` y selecciona tu ROM de Mario Bros. (NES).
3. Espera a que termine la preparación inicial y pulsa **JUGAR**.
4. En las siguientes aperturas se reutiliza el módulo preparado.

Requisitos del paquete: **Windows 10 versión 1903 o posterior, o Windows 11,
x64**, componentes multimedia de Windows y un controlador gráfico compatible
con **OpenGL 3.3** para el launcher. Las ediciones Windows N necesitan Media
Feature Pack para esta aplicación.

La revisión de ROM admitida se identifica por su SHA256 completo:

```text
a2a0aa437a735cd36ed29a8e7c5b3dfa10590c6203e893f1668c7995bad2b309
```

Puedes elegir cualquier nombre de archivo para esa ROM. El selector verifica
su contenido. El proyecto no proporciona archivos ni enlaces de descarga de ROMs.

La release incluye un archivo `.zip.sha256` y el paquete lleva
`MANIFIESTO-SHA256.txt`, con las huellas de sus archivos originales.

## Novedades de v1.0.0

- **Ejecutable descargable:** motor, importador y compilador portátil incluidos.
- **Preparación local automática:** verificación de ROM, generación de código
  y compilación del módulo privado durante la primera importación.
- **Caché verificada:** se comprueban ROM, módulo y versión del kit para
  reutilizar una preparación; los cambios de kit reciben una caché nueva.
- **Progreso y cancelación:** la preparación muestra su avance y permite
  cancelar el importador y sus procesos propios.
- **Interfaz conservada:** launcher, configuración compartida y menú Esc fijo,
  centrado y proporcionado, con regreso al launcher desde la partida.
- **Datos locales:** ajustes, récords, ROM y módulo quedan en `Datos`.
- **Música opcional:** mantiene su coordinación con las fases y la pérdida de vidas.
- **Encabezados con acentos:** corregida la separación de caracteres en
  títulos como «GRÁFICOS».
- **Empaquetado desde fuentes:** lista explícita de archivos, avisos de
  componentes, manifiesto SHA256 y verificación del ZIP y su extracción.

Consulta el alcance de la validación y las limitaciones en
[NOTAS-v1.0.0](docs/NOTAS-v1.0.0.md).

## Controles y ajustes

El launcher permite configurar teclado y mandos. Se ha comprobado el uso de
un mando Xbox en el entorno de desarrollo. **Esc** abre la pausa y **F11**
cambia la pantalla completa con los atajos predeterminados.

Pantalla, Gráficos y Audio comparten sus ajustes entre el launcher y la pausa.
El menú de pausa permanece fijo; la ventana del juego se puede redimensionar.
Los cambios de FPS, filtro o volumen conservan su tamaño.

**60 FPS es la opción inicial y recomendada.** Los modos 120/180 conservan su
suavizado visual experimental en escenas compatibles. La lógica, velocidad
y audio de NES mantienen su ritmo original. El contador de presentación
también cuenta imágenes repetidas y no mide los refrescos físicos del monitor.
VSync se aplica con el límite de 60 FPS. No se garantiza una entrega estable
de 180 imágenes diferentes por segundo.

## Música de fondo opcional

Coloca una pista M4A/MP4 compatible con Media Foundation en la ruta recomendada:

```text
Datos/Music/stage1.m4a
```

También se conserva esta alternativa, comprobada durante la prueba del ZIP:

```text
Bin/assets/music/stage1.m4a
```

Se busca primero `Datos/Music`. Después de añadir la pista, activa
**Audio → Música de fondo** y ajusta **Volumen de música**. Una configuración
nueva inicia la música desactivada, con volumen independiente de 35 %.

La pista empieza después de la introducción original de cada fase y se repite
en bucle. Continúa al perder una vida y durante la espera de reaparición.
Game Over la detiene; el menú de pausa conserva su posición. Los efectos y
volúmenes de NES se mantienen. Si la pista falta o no puede decodificarse,
se conserva el audio original.

El ZIP público no incluye la pista. Para compartir el programa utiliza el ZIP
original de la release; los archivos añadidos para jugar quedan en tu copia local.

## Carpetas del portable

```text
ArcadeBros1983Recomp/
├── ArcadeBrosLauncher.exe
├── Bin/                         motor, herramientas, recursos y avisos
└── Datos/                       creado al usar el programa
    ├── Cache/                   ROM, código generado y módulo privados
    ├── ROM/                     copia de ROM para futuras versiones
    ├── config.ini               ajustes compartidos
    ├── Records/                 récord personal
    ├── Music/                   pista opcional aportada por el usuario
    └── Logs/                    registros de preparación
```

Conserva `Datos` para mantener ajustes y récords. Si la preparación falla,
el mensaje indica dónde encontrar su registro. El importador conserva las
preparaciones anteriores y reserva una carpeta nueva para cada intento.

## Cómo funciona

[NESRecomp](https://github.com/mstan/nesrecomp) traduce instrucciones 6502 de la
ROM a C. El importador ejecuta ese recompilador y Clang/LLD en el equipo del
jugador, y guarda una DLL privada. El motor genérico carga ese módulo y
comparte con él el estado de la máquina.

Las instrucciones traducidas utilizan código nativo. Se mantiene el fallback
normal de NESRecomp para las instrucciones no cubiertas, además del runtime
que reproduce PPU, APU, entrada y otros componentes de NES. El juego conserva
su velocidad y sus reglas originales.

La interfaz usa [recomp-ui](https://github.com/RetroPortingToolKit/recomp-ui),
adaptado al español mediante parches del proyecto.

## Construir desde fuentes

El desarrollador necesita Git, CMake **3.21 o posterior**, Visual Studio 2022
o Build Tools 2022 con C++ y Windows SDK, y Python **3.11 o posterior** para
preparar el SDK genérico. La extracción automática del archivo LLVM requiere
Python **3.14 o posterior**; también se admite un compilador ya preparado.

Después de clonar y seleccionar la revisión deseada, en una copia limpia:

```powershell
git submodule update --init --recursive
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\preparar_dependencias.ps1
powershell -NoProfile -ExecutionPolicy Bypass `
    -File .\tools\native_module\crear_publico.ps1 `
    -Python "C:\ruta\a\python.exe"
```

El empaquetador construye el motor y las herramientas genéricas sin consultar
una ROM. Genera el ZIP, su `.sha256` y una copia limpia de comprobación en
una carpeta nueva `build/r100-...`.

Consulta [COMPILAR-PORTABLE-v1.0.0](docs/COMPILAR-PORTABLE-v1.0.0.md) para los
parámetros de LLVM, el proceso de empaquetado y las comprobaciones de distribución.

### Dependencias fijadas

| Componente | Revisión o versión | Parche activo |
| --- | --- | --- |
| NESRecomp | `5a2eddeecc646e9325bef53f3c711e119423f99c` | `patches/nesrecomp/arcade-v0.9.patch` |
| recomp-ui | `7c635c6b366a6e0d5d3c913a7fce6ec04dd9f256` | `patches/recomp-ui/arcade-v1.0.patch` |
| LLVM/Clang/LLD | `23.1.4`, archivo oficial Windows x64 verificado | — |

Los parches son acumulativos sobre las revisiones originales. El preparador
identifica los cambios aplicados y conserva las revisiones fijadas. El gitlink
de `recomp-ui` permanece en su revisión original; las adaptaciones se guardan
en el parche del proyecto.

### Pruebas de desarrollo

Las pruebas de música y geometría se ejecutan por separado:

```powershell
cmake -S tests -B build/tests-v100 -G "Visual Studio 17 2022" -A x64
cmake --build build/tests-v100 --config Release --parallel 1
ctest --test-dir build/tests-v100 -C Release --output-on-failure
```

Los experimentos del módulo nativo, importador y launcher se conservan en
[tools/native_module](tools/native_module/README.md). Incluyen comparaciones
de estado y audio, rechazos de entradas incompatibles, reutilización de caché
y comprobaciones de interfaz. El empaquetador público funciona de forma
independiente de esas carpetas de experimentos.

## Alcance y trabajo pendiente

La versión se ha comprobado en el equipo de desarrollo, incluida la primera
apertura desde el ZIP, importación, partida, menú Esc, nueva apertura y música
opcional. Queda ampliar las pruebas a otros equipos, más fases, dos jugadores
y distintos mandos. No se ha certificado el comportamiento en todos los
equipos ni en una instalación independiente sin herramientas de desarrollo.

El suavizado 120/180 sigue siendo experimental. La sincronización, su cobertura
y su mejora perceptible necesitan más trabajo. Android queda para una etapa
posterior al paquete Windows.

## Créditos y avisos

- **Juego original:** Nintendo, titular de Mario Bros. y sus elementos originales.
- **Proyecto y adaptaciones:** [JojitaGamerHD](https://github.com/JojitaGamerHD).
- **Recompilador y runtime:** [Matthew Stanley y colaboradores de NESRecomp](https://github.com/mstan/nesrecomp).
- **Interfaz:** [RetroPortingToolKit y colaboradores de recomp-ui](https://github.com/RetroPortingToolKit/recomp-ui).
- **Compilación local:** LLVM/Clang/LLD y sus componentes.

Los avisos de bibliotecas, fuentes y recursos están en `Bin/licenses` dentro
del ZIP y en el [índice de componentes del repositorio](tools/native_module/public/licenses/README-NOTICES.md).
NESRecomp se distribuye bajo PolyForm Noncommercial 1.0.0; los demás componentes
conservan sus licencias y atribuciones. El reconocimiento de sus autores no
implica afiliación ni aprobación oficial.

El proyecto no proporciona ROMs, música del juego ni módulos ya preparados
desde una ROM. Cada usuario aporta su copia compatible conforme a las
condiciones aplicables.
