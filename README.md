# Arcade Bros 1983 Recomp

**Port no oficial de Mario Bros. (NES) para Windows x64 mediante recompilación estática local con NESRecomp.**

[![Estado](https://img.shields.io/badge/estado-Completo-blue)](https://github.com/JojitaGamerHD/ArcadeBros1983-Recomp)
[![Plataforma](https://img.shields.io/badge/plataforma-Windows%20x64-blue)](https://github.com/JojitaGamerHD/ArcadeBros1983-Recomp)

**Versión estable v1.0.5** · [Descargar para Windows x64](https://github.com/JojitaGamerHD/ArcadeBros1983-Recomp/releases/download/v1.0.5/ArcadeBros1983-Recomp-v1.0.5-Windows-x64.zip)

El menú principal se abre con
`Bin/ArcadeNativeHost.exe` antes de importar la ROM. El menú reúne los
cuatro modos de juego, Opciones, Cargar ROM y Salir. La partida y la
pausa se muestran en esa misma ventana.

El repositorio contiene fuentes, scripts y parches. El ZIP contiene el motor
y las herramientas genéricas. Cada jugador aporta su propia ROM compatible.
El proyecto es independiente,
sin fines comerciales y sin afiliación con Nintendo.

## Descargar y jugar

Descarga `ArcadeBros1983-Recomp-v1.0.5-Windows-x64.zip`. [Release v1.0.5 y archivo SHA-256](https://github.com/JojitaGamerHD/ArcadeBros1983-Recomp/releases/tag/v1.0.5)

1. Extrae el ZIP completo en una carpeta con permiso de escritura.
2. Guarda tu ROM compatible de Mario Bros. (NES) como `rom/game.nes` en la
   carpeta extraída.
3. Abre `Bin/ArcadeNativeHost.exe` y elige **Cargar ROM**.
4. Cuando termine la preparación, elige jugadores y modo.

Requisitos del paquete: **Windows 10 versión 1903 o posterior, o Windows 11,
x64**, componentes multimedia de Windows y un controlador gráfico compatible
con **OpenGL 3.3** para el menú. Las ediciones Windows N necesitan Media
Feature Pack para esta aplicación.

La revisión de ROM admitida se identifica por su SHA256 completo:

```text
a2a0aa437a735cd36ed29a8e7c5b3dfa10590c6203e893f1668c7995bad2b309
```

Al cargar la ROM se verifica su contenido. En aperturas posteriores se puede
reutilizar el módulo preparado. El proyecto no proporciona archivos ni enlaces
de descarga de ROMs.

La release incluye un archivo `.zip.sha256` y el paquete lleva
`MANIFIESTO-SHA256.txt`, con las huellas de sus archivos originales.

Para actualizar, cierra el programa, extrae el ZIP en una carpeta nueva y
copia `Datos` y tu archivo `rom/game.nes` de la instalación anterior.

## Cambios de v1.0.5

- Filtros **Scanlines**, **CRT** y **Nítido** para la partida y la demo, disponibles desde el menú principal y la pausa.
- **Grid**, **Scanlines** y **CRT** guardan intensidades independientes. El nombre del ajuste sigue al filtro activo.
- **Overworld Theme**, de Louswan, viene incluida bajo CC0 1.0. **Música de fondo** permite activarla y una pista personal en `Datos/Music/stage1.m4a` tiene prioridad. [Fuente y licencia de la pista](https://opengameart.org/content/overworld-theme-0)

## Cambios de v1.0.4

- Menús centrados y proporcionales, con texto blanco de tamaño uniforme y una flecha que indica la opción seleccionada.
- El encabezado con el logo de Mario Bros., JojitaGamerHD y Arcade Bros 1983 Recomp se comparte entre el menú principal y sus opciones, y se conserva al volver de una partida.
- La pausa y sus opciones aparecen en un cuadro con borde blanco que deja visible la partida alrededor.
- El marco clásico tiene las mismas dimensiones en el menú y en la partida. Sus cuatro lados quedan visibles con el escalado entero activado o desactivado.

## Cambios de v1.0.3

- Filtro **Grid** para la partida y la demo, con intensidad ajustable y activación desde **Gráficos → Filtro visual**. Empieza desactivado y se muestra a partir de 2× de escala.
- Marco clásico con color personalizable. Se dibuja en el espacio disponible alrededor de la imagen, que conserva su tamaño y proporción original.
- **Ventana / Pantalla completa** se aplica al instante desde Gráficos. Audio y Música de fondo muestran claramente **Sí / No**.
- **Cargar ROM** busca `rom/game.nes` y muestra el aviso en el menú si el archivo falta o es incompatible.

## Cambios de v1.0.2

- Menú retro integrado, pausa y opciones en una sola ventana, con navegación por teclado y mando.
- Modos Estándar (A) y Arcade (B) para uno o dos jugadores.
- Fanfarria y demostración originales desde la ROM local. Al terminar la partida se vuelve al menú integrado.
- Ajustes, controles personalizados, rueda de colores, ocho presets y récords persistentes.

## Cambios de v1.0.1

- Remapeo de teclado, botones y ejes de mando dirigido al mismo almacén que usa el juego.
- Conflictos visibles, borrar con Retroceso, cancelar con Esc y restaurar asignaciones.
- Atajos ocultos dejan de consumir entradas; los ejes personalizados tienen prioridad sobre el stick automático.
- Un botón Mario y otro Luigi: cada editor reúne su vista previa y tres tonos, con disco de color y brillo separado. Ocho presets persistentes.
- Colores independientes del suavizado: se aplican a los píxeles identificados del personaje, incluidas sus animaciones de muerte, el final de fase y el recuento del bonus.
- Motor y launcher compilados en Windows x64; pruebas automáticas de captura, aplicación, persistencia y renderizado.

Consulta [NOTAS-v1.0.1](docs/NOTAS-v1.0.1.md) para compilar, probar y conocer las limitaciones.
Para probar en otro PC, sigue [PROBAR-v1.0.1-WINDOWS](docs/PROBAR-v1.0.1-WINDOWS.md).
La release incluye el ZIP de Windows y su `.zip.sha256`; las fuentes se identifican
con el tag `v1.0.1`. Para actualizar, extrae el ZIP en una carpeta nueva y copia
tu carpeta `Datos` anterior con el programa cerrado.

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

Abre **Opciones → Controles** y elige **J1** o **J2** para configurar teclado
y mandos. **Esc** abre la pausa y **F11** cambia la pantalla completa con los
atajos predeterminados.

Gráficos y Audio comparten sus ajustes entre el menú de inicio y la pausa.
En **Gráficos → Marco** puedes activar el marco clásico y ajustar su color
en **Color marco**. El marco viene desactivado. Sigue la proporción 16:15
de la imagen original de 256 × 240 píxeles y deja espacio para los cuatro
lados dentro de la ventana. Al redimensionar la ventana, la pausa se
mantiene centrada sobre la partida.

En la v1.0.5, **Gráficos → Filtro visual** ofrece **Ninguno**, **Grid**,
**Scanlines**, **CRT** y **Nítido** para la partida y la demo. Grid,
Scanlines y CRT guardan su intensidad por separado. La cuadrícula y las
líneas aparecen a partir de 2× de escala. La máscara de color de CRT
se añade a partir de 3× de escala horizontal. Nítido ajusta los bordes
de los píxeles cuando la escala es fraccionaria.

**60 FPS es la opción inicial y recomendada.** Los modos 120/180 conservan su
suavizado visual experimental en escenas compatibles. La lógica, velocidad
y audio de NES mantienen su ritmo original. El contador de presentación
también cuenta imágenes repetidas y no mide los refrescos físicos del monitor.
VSync se aplica con el límite de 60 FPS. No se garantiza una entrega estable
de 180 imágenes diferentes por segundo.

## Música de fondo opcional

Desde la v1.0.5, activa **Opciones → Audio → Música de fondo** para escuchar
*Overworld Theme* de Louswan. La pista viene incluida bajo CC0 1.0.
[Fuente y licencia de la pista](https://opengameart.org/content/overworld-theme-0)

Para usar tu propia música, guarda un archivo M4A compatible con Media
Foundation con este nombre en la carpeta extraída.

```text
Datos/Music/stage1.m4a
```

El archivo personal tiene prioridad sobre la pista incluida. Esta ruta
también sirve en la v1.0.4. El ajuste está desactivado al instalar y
**Volumen de música** comienza en 35 %.

La pista empieza después de la introducción original de cada fase y se repite
en bucle. Continúa al perder una vida y durante la espera de reaparición.
Game Over la detiene; el menú de pausa conserva su posición. Los efectos y
volúmenes de NES se mantienen. Para volver a la pista incluida, mueve tu
archivo personal fuera de `Datos/Music` y reinicia el juego.

## Carpetas del portable

```text
ArcadeBros1983Recomp/
├── Bin/                         motor, herramientas, recursos y avisos
│   └── ArcadeNativeHost.exe     ejecutable del juego
├── rom/                         carpeta donde el jugador añade game.nes
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

En la rama de desarrollo, el empaquetador construye el motor y las herramientas
genéricas sin consultar una ROM. Genera el ZIP, su `.sha256` y una copia
limpia de comprobación en una carpeta nueva dentro de `build/`.

Consulta [Motor genérico e importación local](tools/native_module/README.md)
para los parámetros de LLVM y las pruebas de distribución.

### Dependencias fijadas

| Componente | Revisión o versión | Parches |
| --- | --- | --- |
| NESRecomp | `5a2eddeecc646e9325bef53f3c711e119423f99c` | [Capas de NESRecomp](patches/nesrecomp) |
| recomp-ui | `7c635c6b366a6e0d5d3c913a7fce6ec04dd9f256` | [Capas de recomp-ui](patches/recomp-ui) |
| LLVM/Clang/LLD | `23.1.4`, archivo oficial Windows x64 verificado | — |

Los parches son acumulativos sobre las revisiones originales. El preparador
identifica los cambios aplicados y conserva las revisiones fijadas. El gitlink
de `recomp-ui` permanece en su revisión original; las adaptaciones se guardan
en los parches del proyecto.

### Pruebas de desarrollo

Las pruebas de música y geometría se ejecutan por separado:

```powershell
cmake -S tests -B build/tests-v101 -G "Visual Studio 17 2022" -A x64
cmake --build build/tests-v101 --config Release --parallel 1
ctest --test-dir build/tests-v101 -C Release --output-on-failure
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
