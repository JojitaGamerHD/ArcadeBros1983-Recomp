# v1.0.1 — rama de pruebas, sin release

Preparada a partir del tag `v1.0.0` (`c8c6c76d28b279477fb7effab84f15ed0cb591c6`).
Rama de pruebas: `desarrollo-v1.0.1`, compartida con autorización del autor para
probarla en otro PC. No se modifica `main` ni se crea tag, release o PR.
La guía breve de descarga y compilación está en [PROBAR-v1.0.1-WINDOWS](PROBAR-v1.0.1-WINDOWS.md).

## Controles

La captura del menú utilizaba el almacén nativo de recomp-ui, mientras que las
etiquetas, la conversión del launcher y el juego utilizaban las asignaciones
del host en `config.ini`. Por eso una tecla parecía no cambiar y no llegaba a
la partida. Ahora una ruta común procesa los eventos SDL directamente contra
las mismas preferencias que lee el motor, sin generar un `keybinds.ini` paralelo.

- Teclado, botones, cruceta y sentidos de ejes/gatillos de mandos SDL GameController.
- Se escucha al mando seleccionado para ese jugador; se espera a soltar sus
  controles para evitar capturar el gesto que abrió la edición.
  Se actualiza la asociación al reconectar, incluido el identificador 0 válido
  de SDL2. Dos mandos con el mismo GUID se distinguen por orden de conexión;
  no se promete conservar su identidad física si se intercambia ese orden.
- Esc cancela, Retroceso borra; las repeticiones de teclado se ignoran.
- F2 se reserva porque conmuta el modo de ejecución del motor.
- Las teclas duplicadas entre jugadores y los solapamientos con atajos se
  rechazan con un mensaje. Un mando puede usar la misma distribución que otro.
  Un botón incluido en un atajo combinado también se considera conflicto.
- Auto Map se detiene ante un conflicto. Para intercambiar dos asignaciones,
  borra primero una de ellas. Las asignaciones ya capturadas se conservan.
- Restaurar valores por defecto conserva las otras preferencias. Si introduce
  un conflicto con otro jugador o atajo personalizado, se rechaza la restauración
  completa de ese grupo y se indica que hay que borrar el conflicto primero.
- Los atajos ocultos de versiones anteriores dejan de consumir entradas.
  El stick izquierdo conserva su navegación automática solo en los sentidos
  que no estén asignados explícitamente a una acción o atajo.

El archivo compartido sigue siendo `Datos/config.ini` en el portable.
La configuración de controles se hace en el launcher. Desde pausa → Sistema →
Volver al launcher se termina la partida actual y se pueden cambiar controles;
no se añadió un segundo editor de controles dentro de la pausa.

## Colores y presets

Pausa → **Colores** ofrece seis campos `#RRGGBB`: tres índices de color de
sprite para Mario y tres para Luigi. No corresponden siempre a prendas fijas:
son los índices 1, 2 y 3 del patrón NES. Activa **Colores experimentales**,
edita con teclado y confirma con Enter. Esc abandona la edición.

Hay ocho ranuras numeradas compartidas por ambos personajes. **Guardar preset**
reemplaza los seis colores de la ranura elegida. **Cargar preset** los restaura
y activa la personalización; una ranura vacía se rechaza con un mensaje.
**Restaurar colores originales** desactiva el efecto sin borrar los presets.
Los valores RGB inválidos se rechazan sin modificar el color anterior.
Durante la edición, Enter confirma, Esc cancela y Retroceso borra texto sin
activar la navegación del menú ni los atajos de pausa o pantalla completa.

Se guardan en `[Game]`: `ColorsEnabled`, `ColorsSlot`, `ColorsActive` y
`ColorsPreset1`…`ColorsPreset8`. Cada lista contiene seis RGB de seis dígitos
separados por comas. Las ediciones y las acciones aceptadas se guardan mediante
el escritor temporal y reemplazo de `config.ini` que ya usa el host.
No hay un archivo de presets separado ni recursos gráficos de la ROM incluidos.

### Implementación y límites

La sustitución global de RGB o de la paleta NES recolorearía también fondo,
enemigos o texto que compartan colores. El renderizado normal de TriCNES
(`PPU_Render_CalculatePixel` → `DrawToScreen`) resuelve la paleta y retrasa el
color varios ciclos antes de escribir el framebuffer: allí ya no conserva la
identidad del personaje. Cambiar esa ruta exigiría propagar metadatos de OAM a
través de la tubería y ampliar la validación de compatibilidad.

Se reutiliza la reconstrucción de sprites de `smooth_host.c`/`smooth_video.cpp`.
El mapa de entidades existente distingue Mario (grupo 1), Luigi (grupo 2) y
los demás objetos. Antes de permitir la personalización se reconstruye el frame
sin colores personalizados y se compara cada píxel con la PPU. Si no coincide,
se devuelve la imagen nativa. Solo después se sustituyen los tres colores de
los grupos 1 y 2, conservando transparencia, prioridad OAM, oclusión por fondo,
recorte, volteos y límite de ocho sprites por línea.

No se escriben RAM del juego, memoria de paleta, OAM ni instrucciones recompiladas.
No cambia el protocolo del módulo privado ni sus exportaciones. La función
presentadora admite el efecto a 30/60 FPS y en imágenes suavizadas a 120/180.

**Es experimental y está desactivado por defecto.** Solo actúa en las escenas
que el observador existente reconoce y verifica. Título, demostración, bonus,
transiciones, cambios de actividad de entidades y cualquier reconstrucción
rechazada mantienen los colores originales. Puede haber cambios visibles entre
color personalizado y original. No promete una recoloración continua durante
toda la partida. La comparación de píxeles es una protección de presentación,
no sustituye la validación del mapa de entidades con una ROM real.

## Validación realizada en Windows x64

- Compilados con MSVC 19.44 / Visual Studio Build Tools 2022: NESRecomp,
  ArcadeNativeHost, ArcadeBrosLauncher y ArcadePrepare.
- Siete pruebas pasan; una se omite porque requiere una pista musical privada.
  Captura SDL real de la ruta usada por ImGui, conflictos/cancelación/borrado,
  selección de mando y ejes, aplicación de entradas, guardado/carga, reinicio
  en un proceso nuevo, presets, RGB inválidos y aislamiento de colores por
  personaje; también geometría y pruebas existentes de música.
- El widget RGB real de ImGui se prueba sin ventana ni ROM: escritura, borrado,
  flechas, Enter y Esc, manteniendo los atajos suspendidos hasta soltar las teclas.
- La reproducción aislada de la llamada usada en v1.0.0 confirma que escribe
  otro almacén y deja la asignación del host en Z. Código de salida esperado: 42.
- Revisión de cambios y comprobación de parches acumulativos sobre las
  revisiones fijadas. Los gitlinks y las revisiones de dependencias se conservan.

No se aportó una ROM compatible ni pista musical a esta copia. No se ejecutó
una partida real, no se probaron mandos físicos, ni se validó visualmente la
interfaz en partida. No se generó el ZIP portable completo: falta una instalación
real de Python y el kit LLVM portátil. Los binarios del motor sí compilan y las
pruebas no necesitan ROM. La candidata requiere las comprobaciones manuales
siguientes antes de publicar.

## Cómo compilar y probar en Windows

En un clon del repositorio con estos cambios:

```powershell
git submodule update --init --recursive
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\preparar_dependencias.ps1
cmake -S tests -B build/tests-v101 -G "Visual Studio 17 2022" -A x64
cmake --build build/tests-v101 --config Release --parallel 1
ctest --test-dir build/tests-v101 -C Release --output-on-failure
```

Reproducir la ruta defectuosa anterior sin abrir el juego:

```powershell
& .\build\tests-v101\Release\controls_capture_test.exe .\build\tests-v101 --reproduce-v100
# Mensaje REPRODUCED y salida 42 son el resultado esperado de esta comprobación.
```

Compilar el motor genérico, launcher e importador, sin ROM:

```powershell
cmake -S tools/native_module -B build/v101-host -G "Visual Studio 17 2022" -A x64 `
  -DARCADE_PREPARE_MODULE=OFF -DARCADE_WINDOWED_HOST=ON "-DARCADE_RELEASE_VERSION=1.0.1"
cmake --build build/v101-host --config Release `
  --target ArcadeNativeHost ArcadeBrosLauncher ArcadePrepare --parallel 1
```

Para preparar un portable completo de prueba con el compilador LLVM verificado,
sigue `docs/COMPILAR-PORTABLE-v1.0.0.md`, usando ahora `-Version 1.0.1` y las
herramientas actualizadas. El script crea el paquete local, no publica nada:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\native_module\crear_publico.ps1 `
  -Version 1.0.1 -Python "C:\ruta\real\python.exe" -Compilador "C:\ruta\al\LLVM"
```

Prueba manual en una copia nueva del portable y con tu ROM compatible:

1. Conserva una copia de `Datos` de v1.0.0. Configura A con C y movimiento con
   teclas libres. Confirma en partida que funcionan y que Z ya no activa A.
2. Cierra completamente el launcher, vuelve a abrir y comprueba las asignaciones.
   Repite con dos jugadores y dos mandos, cruceta, botones, gatillos y reconexión.
3. Intenta duplicar una tecla y asignar el botón de pausa a una acción: debe
   rechazarse. Prueba Esc, Retroceso, Auto Map y restaurar predeterminados.
4. Activa los colores en pausa, usa colores muy distintos para Mario y Luigi,
   y comprueba que fondo y enemigos conservan su aspecto, incluso al solaparse.
5. Guarda en ranuras 1 y 8, cambia los colores, cierra todo, reinicia y carga cada
   preset. Prueba un RGB inválido, una ranura vacía y restaurar originales.
6. Repite colores a 60, 120 y 180 FPS; prueba pérdida de vida, cambios de fase,
   bonus y título. En escenas no verificadas deben verse los colores originales.
   Comprueba que la velocidad, colisiones, audio y puntuación se mantienen.

## Aplicar la entrega como parche

El archivo `ArcadeBros1983-v1.0.1.patch` contiene todos los cambios del proyecto,
incluidos los nuevos parches acumulativos para las dependencias. En un clon
limpio situado en `v1.0.0`, aplica `git apply --check` y después `git apply` con
ese archivo; luego ejecuta el preparador. La actualización reconoce los parches
anteriores; si encuentra conflictos, conserva los cambios locales y se detiene.
No cambies el gitlink de recomp-ui al publicar estos cambios: las adaptaciones
de esta dependencia siguen entregándose en `patches/recomp-ui/arcade-v1.0.1.patch`.
