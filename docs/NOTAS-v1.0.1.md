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

Pausa → **Colores** ofrece un botón **Mario** y otro **Luigi**. Activa
**Personalizar personajes** y abre el personaje con el ratón o Enter.
Dentro del editor se reúnen su vista previa y las tres muestras de tono:
selecciona una muestra y elige su color en el disco, sin triángulo ni códigos.
El ángulo elige el tono; la distancia al centro ajusta la saturación. Una barra
**Brillo** separada permite aclarar u oscurecer el color.

La vista previa cambia mientras editas cualquiera de los tres tonos.
**Listo** conserva los tres colores del personaje; **Cancelar** o Esc restaura
los tres valores que tenía al abrir el editor. No modifica al otro personaje.
La edición no activa los atajos de pausa ni pantalla completa.

La referencia se extrae de los tiles y OAM de la ROM que estás ejecutando.
No se incluyen imágenes de Mario/Luigi en las fuentes. Conserva la última pose
válida de cada personaje; para obtenerla, entra en una partida con ese personaje
antes de abrir su editor. Los tonos son los índices 1, 2 y 3 del patrón NES,
no nombres de prendas que podrían variar según la animación.

Hay ocho ranuras numeradas compartidas por ambos personajes. **Guardar preset**
reemplaza los seis colores de la ranura elegida. **Cargar preset** los restaura
y activa la personalización; una ranura vacía se rechaza con un mensaje.
**Restaurar colores originales** desactiva el efecto sin borrar los presets.

Se conserva el formato de la primera prueba: `[Game]`, `ColorsEnabled`,
`ColorsSlot`, `ColorsActive` y `ColorsPreset1`…`ColorsPreset8`. Cada lista contiene
seis RGB de seis dígitos separados por comas. Tus presets anteriores siguen
siendo compatibles. El editor convierte automáticamente a ese formato al guardar.

### Corrección de animaciones y muerte

Los colores ya no dependen de un par de fotogramas válido para interpolación.
El motor captura información adicional de presentación: el personaje que posee
cada entrada OAM y el índice de color del sprite que realmente ganó cada píxel,
respetando la tubería de la PPU y la prioridad del fondo. La imagen nativa se
recolorea mediante esa procedencia, sin sustituir todos los píxeles del mismo RGB.
Cambiar de animación o entrar en estado de muerte no desactiva por sí mismo los
colores. Cuando el suavizado no puede interpolar, se utiliza la imagen nativa
con sus colores personalizados.

La identificación se observa al completar las escrituras OAM, incluidas las de
DMA, cotejando los rangos de las entidades Mario y Luigi y su shadow OAM. No se
escriben RAM del juego, paleta, instrucciones recompiladas ni reglas de colisión.
Los datos de procedencia son auxiliares de presentación, separados del estado
lógico y de la interfaz del módulo privado.

El mapa de entidades sigue siendo específico de la ROM compatible. Los sprites
no identificados se conservan originales; no se promete personalizar personajes
dibujados fuera de ese mapa en todas las pantallas de título o bonus. Aquí no se
dispone de una ROM privada para comprobar visualmente cada fase: las pruebas
sintéticas cubren cambios de animación, muerte, prioridad y aislamiento de fondo
y enemigos, y queda repetir la prueba con tu copia real.

## Validación realizada en Windows x64

- Compilados con MSVC 19.44 / Visual Studio Build Tools 2022: NESRecomp,
  ArcadeNativeHost, ArcadeBrosLauncher y ArcadePrepare.
- Nueve pruebas pasan; una se omite porque requiere una pista musical privada.
  Captura SDL real de la ruta usada por ImGui, conflictos/cancelación/borrado,
  selección de mando y ejes, aplicación de entradas, guardado/carga, reinicio
  en un proceso nuevo, presets, RGB inválidos y aislamiento de colores por
  personaje; también geometría y pruebas existentes de música.
- El selector visual se prueba con clic de ratón, vista previa inmediata,
  confirmación/cancelación y ventanas de 800×600 y 640×480. El editor de texto
  genérico anterior conserva sus pruebas de compatibilidad.
- La PPU LLE real se prueba con una máquina sintética: carga OAM, estados de
  muerte, reasignación de slots, retraso de tres dots, oclusión por fondo y
  recorte. El hash lógico de PPU y sus índices originales no cambian al activar
  la captura auxiliar.
- El preparador se comprueba en copias limpias, en actualizaciones desde v1.0.0
  desde la primera candidata v1.0.1 y desde la prueba con rueda/triángulo,
  incluyendo su idempotencia.
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
4. Activa los colores en pausa, abre Mario o Luigi y cambia sus tres tonos; comprueba
   la vista previa, **Listo**, **Cancelar** y Esc. Usa tonos distintos para Mario
   y Luigi y comprueba que fondo y enemigos conservan su aspecto al solaparse.
5. Guarda en ranuras 1 y 8, cambia los colores, cierra todo, reinicia y carga cada
   preset. Prueba un RGB inválido, una ranura vacía y restaurar originales.
6. Repite colores a 60, 120 y 180 FPS; prueba caminar, saltar, girarse y morir.
   Los cambios de animación no deben perder el color al fallar la interpolación.
   Prueba cambios de fase, bonus y título; los sprites fuera del mapa reconocido
   conservan sus colores originales.
   Comprueba que la velocidad, colisiones, audio y puntuación se mantienen.

## Aplicar la entrega como parche

El archivo `ArcadeBros1983-v1.0.1.patch` contiene todos los cambios del proyecto,
incluidos los nuevos parches acumulativos para las dependencias. En un clon
limpio situado en `v1.0.0`, aplica `git apply --check` y después `git apply` con
ese archivo; luego ejecuta el preparador. La actualización reconoce los parches
anteriores; si encuentra conflictos, conserva los cambios locales y se detiene.
No cambies el gitlink de recomp-ui al publicar estos cambios: las adaptaciones
de esta dependencia siguen entregándose en `patches/recomp-ui/arcade-v1.0.1-editor.patch`.
