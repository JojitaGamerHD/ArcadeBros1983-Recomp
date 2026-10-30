# Arcade Bros 1983 Recomp — v0.8.0

Estado: desarrollo experimental. Fecha de esta revisión: 30 de octubre de 2026.

## Cambios

### Música durante las fases

- Pista externa opcional en `assets/music/stage1.m4a`, aportada por el usuario y excluida de Git.
- La música comienza cuando la fase está en juego y ha terminado la introducción original. No se reproduce en la portada, las demostraciones ni Game Over.
- Se pausa al abrir el menú y continúa desde la misma posición al regresar a la partida.
- Cada nueva fase reinicia la pista. Durante una fase se repite en bucle; se utiliza el mismo archivo en todas las fases, sin lista de reproducción.
- Al perder una vida se reinicia cuando no queda otro jugador vivo. En una partida de dos jugadores se conserva la reproducción si el otro sigue jugando.
- Se mantiene la recompilación estática y el audio original del juego.

### Ajustes de audio

- Launcher y menú de pausa incluyen Música de fondo y Volumen de música en la sección Audio.
- Las preferencias se guardan junto a los ajustes existentes: `MusicEnabled=0` y `MusicVolume=35` son los valores iniciales de `[Audio]`.
- El volumen musical es independiente; el interruptor y el volumen generales de Audio afectan tanto a los efectos NES como a la música.
- Sin pista disponible o si falla la decodificación se conserva el audio NES, con un diagnóstico en la consola.
- Se conserva el menú de pausa fijo, centrado y adaptable de v0.7.0.

### Implementación y preparación de la pista

`src/music.cpp` utiliza Media Foundation de Windows para decodificar el archivo a PCM de 16 bits, mono y 48.000 Hz. La música se mezcla con el audio existente limitando la suma al intervalo permitido para evitar desbordamientos. La coordinación con el juego se mantiene en `src/music_gate.h` y se conecta desde `src/records.c`.

El archivo debe contener audio M4A/MP4 válido compatible con Media Foundation. Cambiar la extensión de un MP3 a `.m4a` no convierte su contenido. La pista utilizada durante el desarrollo tenía nombre `.mp3`, pero su contenido ya era MP4; no debe generalizarse ese procedimiento a otros archivos.

Coloca la pista antes de configurar CMake. Si está presente durante la configuración, la compilación la copia a `assets/music/stage1.m4a` junto al ejecutable; no la incorpora dentro del binario. Si se añade después, hay que volver a configurar y compilar. Activa Música de fondo en Audio para escucharla, ya que comienza desactivada.

La reproducción requiere los componentes multimedia de Windows. En Windows N puede ser necesario instalar Media Feature Pack.

## Validación realizada

- Compilación Release para Windows x64 completada y funcionamiento de la música aprobado manualmente en el equipo de desarrollo.
- Decodificación aislada de la pista utilizada, de aproximadamente 75,02 segundos, y comprobaciones de mezcla, saturación, desactivación, repetición y recarga.
- Pruebas aisladas de valores iniciales, límites de volumen y guardado/carga de preferencias.
- Pruebas de coordinación con portada, introducción, pausa, muerte, transición de fase, Game Over y estado de dos jugadores.
- Traza nativa sin ventana con una ROM privada compatible: la reproducción se habilitó después de terminar la introducción, se reanudó tras la pausa y no se habilitó en los estados prohibidos comprobados.
- Reconstrucción de los parches acumulativos y comparación de las 17 fuentes afectadas de las dependencias —9 del motor y 8 de la interfaz—, normalizando únicamente los finales de línea.
- El preparador reconoce los parches v0.8.0 ya aplicados sobre las revisiones fijadas.

Las pruebas se conservan en `tests/` y pueden ejecutarse mediante CMake y CTest según el README. La prueba de la pista real se omite cuando no existe el archivo privado; las otras pruebas no dependen de él.

Estas comprobaciones no equivalen a una prueba manual de todas las fases, de todas las situaciones de dos jugadores ni de otros equipos y mandos. La compilación confirmada por el usuario fue incremental; no se atribuye a esta revisión una reconstrucción final completa desde cero ni una validación del portable.

## Dependencias fijadas

| Componente | Revisión original | Parche acumulativo |
| --- | --- | --- |
| NESRecomp | `5a2eddeecc646e9325bef53f3c711e119423f99c` | `patches/nesrecomp/arcade-v0.8.patch` |
| recomp-ui | `7c635c6b366a6e0d5d3c913a7fce6ec04dd9f256` | `patches/recomp-ui/arcade-v0.8.patch` |

Los parches v0.8 incluyen las adaptaciones anteriores y se aplican sobre las revisiones originales, no encima de v0.7. El submódulo permanece fijado a su revisión original; las adaptaciones se mantienen en el parche del proyecto.

## Pendientes

- Validar `crear_portable.ps1` y la inclusión opcional de música en el portable personal.
- Ampliar la escucha y las partidas reales, especialmente las transiciones de fase y situaciones de dos jugadores.
- Probar otros equipos, mandos y configuraciones multimedia de Windows.
- Realizar una reconstrucción final completa desde un entorno limpio antes de atribuirle esa validación.
- Considerar varias pistas o selección por fase en una versión posterior.

## Distribución

La publicación pública se limita a fuentes, scripts y parches. Cada usuario aporta por separado su ROM compatible y su música; no se incluyen ROMs, pistas, código generado desde la ROM, `dist/` ni ejecutables derivados. La carpeta `/assets/music/` permanece excluida de Git. Las dependencias y la tipografía conservan sus respectivas licencias.
