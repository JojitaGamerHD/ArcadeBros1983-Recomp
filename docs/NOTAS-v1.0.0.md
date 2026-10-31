# Arcade Bros 1983 Recomp — v1.0.0

Versión Windows x64 preparada para distribución con ejecutable y ROM aportada
por el jugador. Revisión: 30 de octubre de 2026.

## Descarga y primera apertura

El paquete contiene `ArcadeBrosLauncher.exe`, un motor genérico y las
herramientas necesarias para preparar el juego. El jugador selecciona su ROM
compatible; el importador verifica su SHA256, genera el C traducido y compila
una DLL privada con NESRecomp y Clang/LLD. La preparación muestra progreso
y admite cancelar sus procesos propios.

El jugador puede usar el paquete sin instalar Visual Studio, CMake ni Python.
Estas herramientas siguen siendo requisitos del desarrollador que construye
el ZIP. El empaquetador funciona sin consultar una ROM y prepara el SDK
directamente desde las cabeceras genéricas del motor.

Se conserva la recompilación estática de las instrucciones cubiertas y el
fallback original del motor. La comparación reproducible no sustituyó el
código traducido por una ejecución exclusivamente interpretada.

## Caché y datos

- `Bin` contiene el motor y las herramientas genéricas.
- `Datos/Cache` conserva la ROM, fuentes traducidas y DLL privadas.
- Un recibo verifica ROM, módulo y versión del kit antes de reutilizar la caché.
- Una versión diferente del kit recibe una preparación nueva; las anteriores
  se conservan.
- `Datos/config.ini`, `Datos/Records` y `Datos/Logs` guardan ajustes, récords y registros.

La primera importación y la reutilización posterior quedaron comprobadas.
También se verificó el rechazo de ROM incompatible, archivo ausente, módulo
ausente, ABI incompatible y caché existente en las pruebas correspondientes.

## Juego e interfaz

Se mantienen el launcher, portada, controles configurables, récord permanente,
regreso al launcher y ajustes compartidos con el menú Esc. La pausa sigue
fija, centrada y adaptada al espacio disponible. Cambiar FPS o audio conserva
el tamaño de ventana.

Los encabezados de la interfaz ahora mantienen juntos los bytes de cada
carácter UTF-8 al aplicar separación entre letras. Esto corrige títulos como
«GRÁFICOS» sin cambiar sus opciones.

60 FPS sigue siendo el valor inicial y recomendado. Los modos 120/180
conservan su suavizado visual experimental; la lógica y el audio de NES
mantienen su ritmo original. El contador de presentación incluye imágenes
repetidas y no constituye una medición del refresco físico del monitor.

## Música opcional

Se recomienda la ruta `Datos/Music/stage1.m4a`. También se conserva
`Bin/assets/music/stage1.m4a` como alternativa compatible junto al ejecutable.
Se busca primero la ruta de Datos.

La pista se activa desde Audio y mantiene su volumen independiente. Comienza
después de la introducción de cada fase, continúa durante la pérdida de una
vida y la espera de reaparición, se pausa con el menú y se corta en Game Over.

El usuario comprobó la música en la copia extraída del ZIP añadiéndola a
`Bin/assets/music`. La adición quedó fuera del ZIP original. La pista sigue
siendo privada y no se incluye como asset de la release.

## Empaquetado

`tools/native_module/crear_publico.ps1` construye una carpeta nueva mediante
una lista explícita de archivos. Añade los avisos de componentes, calcula
`kit-id.txt`, escribe `MANIFIESTO-SHA256.txt` y genera el ZIP y su `.sha256`.

Comprueba rutas, duplicados, enlaces, lista de archivos y hashes dentro del
ZIP; después lo extrae en una carpeta limpia y vuelve a verificar su contenido.
La compatibilidad del formato ZIP se comprobó con Windows PowerShell 5.1 y
PowerShell 7. La distribución conserva los avisos del motor, SDL, interfaz,
fuentes, LLVM y las dependencias estáticas documentadas de LLVM.

Para publicar se usan el ZIP original y su `.sha256`, conservados antes de
crear los datos privados de las partidas de prueba.

## Validación realizada

- Construcción Release x64 del motor genérico con SDL y la interfaz existente,
  importador, launcher y recompilador; runtime C estático en los ejecutables propios.
- Motor compilado sin enlazar fuentes generadas desde la ROM; verificación
  de que la preparación mantiene intacto el kit genérico.
- Comparación de 960 fotogramas con una compilación de referencia: hashes de
  estado y audio NES idénticos y 24.223.872 ciclos nativos en ambas ejecuciones.
- Comprobación del módulo local compilado por Clang/LLD con entorno aislado
  de las cabeceras y bibliotecas del entorno de desarrollo.
- Importación autónoma desde la ROM, uso de rutas con espacios y acentos,
  entradas rechazadas y reutilización de caché sin recompilar el módulo.
- Capturas ocultas del launcher y sus opciones; comprobación del juego y la
  pausa, cierre del menú y continuación en el equipo del usuario.
- Verificación de las entradas del ZIP, su manifiesto y una extracción limpia.
- Preparación desde esa extracción con datos externos y nueva comparación
  de estado y audio contra la referencia, conservando intactos los archivos del paquete.
- El usuario confirmó la primera apertura desde la copia limpia, importación,
  juego, nuevas aperturas y funcionamiento de la música opcional.

El ZIP validado contiene 462 archivos y ocupa 80.272.919 bytes. El manifiesto
cubre 461 archivos, todos salvo el propio manifiesto. Su SHA256 es:

```text
5e9248d320fe1c7a699eb563f2f1e5ec6bb10e8cc408e8259e01be164babc9e7
```

Estas medidas corresponden al artefacto validado en el equipo de desarrollo.
Otra construcción puede producir una huella diferente por las herramientas,
compresión o metadatos. El hash que acompaña al archivo identifica el ZIP
concreto de la release.

## Dependencias fijadas

| Componente | Revisión o versión | Parche activo |
| --- | --- | --- |
| NESRecomp | `5a2eddeecc646e9325bef53f3c711e119423f99c` | `patches/nesrecomp/arcade-v0.9.patch` |
| recomp-ui | `7c635c6b366a6e0d5d3c913a7fce6ec04dd9f256` | `patches/recomp-ui/arcade-v1.0.patch` |
| LLVM/Clang/LLD | `23.1.4`, Windows x64 | archivo oficial verificado mediante SHA256 |

Los parches se conservan como cambios acumulativos sobre las revisiones
originales. El gitlink de recomp-ui permanece en su revisión fijada.

## Alcance y pendientes

Windows 10 1903 o posterior, o Windows 11, x64, con componentes multimedia
y OpenGL 3.3 para el launcher. Windows N necesita Media Feature Pack.

Las pruebas se realizaron en el entorno de desarrollo. No equivalen a una
certificación en todos los equipos ni a una prueba independiente en un Windows
sin herramientas de desarrollo instaladas. Las dependencias e importaciones
se inspeccionaron y la preparación se ejecutó con un entorno de herramientas aislado.

Queda ampliar las pruebas de más fases, dos jugadores, mandos y otros equipos;
mejorar y medir el suavizado experimental; y estudiar Android después del
paquete Windows. El ZIP, los datos privados y los binarios de prueba permanecen
fuera de Git; la release publica el paquete genérico como archivo adjunto.
