# Arcade Bros 1983 Recomp — v0.9.0

Estado: desarrollo experimental. Revisión: 30 de octubre de 2026.

## Música continua durante la pérdida de vidas

- Una vez iniciada la música de la fase, perder una vida no pausa ni reinicia la pista.
- Continúa durante la plataforma de reaparición, incluso mientras el jugador espera sin moverse, junto al sonido original de reaparición.
- Game Over corta la pista al comenzar. La introducción inicial, la pausa y el cambio de fase conservan su coordinación anterior.
- El ajuste permite el jingle de reaparición; los demás jingles mantienen la política previa. El audio original de NES y los controles de volumen se conservan.
- La pista sigue siendo opcional, privada y aportada por el usuario en `assets/music/stage1.m4a`.

La política está en `src/music_gate.h`. `src/records.c` observa también el índice del jingle actual; no modifica estas variables del juego.

## Suavizado visual experimental

Los modos 120/180 pueden dibujar posiciones intermedias de sprites reconocidos durante las fases normales. La lógica, las físicas, los controles y el audio de NES conservan su ritmo original, aproximadamente 60 pasos por segundo.

`src/smooth_host.c` captura el fondo y los sprites del PPU y comprueba su correspondencia con el dibujo siguiente preparado por el juego. `src/smooth_video.cpp` compone posiciones intermedias a escala interna 6×, conservando grupos de varios tiles, prioridad, recorte y el límite de ocho sprites por línea de origen. La imagen lógica, el HUD y los menús mantienen sus dimensiones.

Se usa la imagen original en bonus, menús, introducciones, Game Over y transiciones no validadas. También se conserva cuando falla la continuidad de grupos o la reconstrucción no coincide con la imagen del PPU. Las animaciones mantienen sus fotogramas originales.

**60 FPS es la opción recomendada y el valor inicial en configuraciones nuevas.** No se modifican las preferencias ya guardadas. El contador `NES` mide simulación; `FPS` cuenta presentaciones, incluidas imágenes repetidas. No equivale a medir directamente imágenes únicas mostradas por el monitor.

Los modos 120/180 siguen siendo experimentales. La presentación permanece ligada al bucle NES y VSync solo se aplica con el objetivo de 60 FPS. Falta mejorar la cadencia, la sincronización y la cobertura del suavizado. No se garantiza una presentación estable de 180 FPS ni una mejora perceptible uniforme.

## Validación realizada

- Compilación Release x64 del juego y el launcher con el prototipo integrado.
- Cuatro pruebas automatizadas aprobadas en el equipo del usuario: geometría de sprites, decodificación/mezcla de música, coordinación musical y preferencias.
- Casos de geometría: mitades y tercios distintos, sprites compuestos, cruce horizontal de bordes, prioridad, recorte, inversión de tiles y respaldo ante discontinuidades.
- Secuencia reproducible de caminar y saltar: estado del motor y audio NES idénticos con captura/interpolación activadas y desactivadas.
- En 430 muestras de esa secuencia, 393 tuvieron tres imágenes distintas para las fracciones 0, 1/3 y 2/3; cero diferencias en la reconstrucción comprobada contra el PPU. Una escena quieta no necesita producir posiciones diferentes.
- Comprobación adicional de imágenes intermedias distintas después del escalado en una ventana SDL oculta.
- La ejecución con ventana oculta mantuvo la simulación cerca de 60 Hz y alcanzó presentaciones cercanas a 180 FPS, con caídas puntuales. No es una medición del barrido físico del monitor.
- El usuario comprobó el funcionamiento general y aprobó la continuidad musical al perder una vida. No se confirmó una mejora de fluidez perceptible frente a 60 FPS; por ello se conserva la etiqueta experimental.
- Parche acumulativo del motor reconstruido sobre su revisión fijada y nueve fuentes comparadas; ocho fuentes de la interfaz reconstruidas y verificadas contra su parche v0.8, que no cambió.

Estas comprobaciones no cubren todas las fases, el modo de dos jugadores completo, todos los mandos ni otros equipos. No se atribuye a esta versión una validación del portable ni una reconstrucción final desde un entorno limpio del usuario.

## Dependencias fijadas

| Componente | Revisión original | Parche activo |
| --- | --- | --- |
| NESRecomp | `5a2eddeecc646e9325bef53f3c711e119423f99c` | `patches/nesrecomp/arcade-v0.9.patch` |
| recomp-ui | `7c635c6b366a6e0d5d3c913a7fce6ec04dd9f256` | `patches/recomp-ui/arcade-v0.8.patch` |

Los parches se aplican sobre las revisiones originales. El preparador identifica los cambios ya aplicados y conserva las revisiones fijadas. El gitlink de `recomp-ui` permanece en su revisión original.

## Pendientes y distribución

- Mejorar y medir la sincronización y la cadencia del suavizado; ampliar la comparación visual a más escenas y equipos.
- Ampliar la validación de bonus, transiciones y dos jugadores.
- Validar nuevamente el portable personal y la copia opcional de música.
- Investigar para una versión futura la preparación local automática desde la ROM del usuario. El ZIP público con ejecutable sin compilación manual todavía no está implementado.

La publicación de v0.9.0 contiene fuentes, scripts y parches. Cada usuario aporta su ROM y su música y compila siguiendo el README. No se publican ROMs, pistas, código generado desde la ROM, ejecutables derivados ni `dist/`. Se conservan las licencias y avisos de las dependencias y recursos.
