# Arcade Bros 1983 Recomp — v0.7.0

Estado: desarrollo experimental. Fecha de esta revisión: 29 de octubre de 2026.

## Cambios

### Configuración compartida

- Pantalla: modo ventana, sin bordes o exclusiva; escala de ventana de 1× a 8×.
- Gráficos: escalado entero, filtro lineal, Mostrar FPS, Límite de FPS y VSync.
- Audio: activar/desactivar audio y volumen.
- El launcher y la pausa utilizan las mismas preferencias del motor. Restaurar los valores predeterminados en el launcher también incluye los nuevos ajustes de FPS.
- VSync solo se aplica con el límite de 60 FPS. Con otro límite se deshabilita el control y se conserva la preferencia para volver a 60 FPS.
- Los modos 120/180 FPS repiten imágenes de presentación. No hay interpolación ni nuevas animaciones; la simulación NES conserva su velocidad original.

### Menú de pausa

- Menú fijo y centrado, sin arrastre ni redimensionado manual.
- Tamaño y tipografía adaptados al área disponible.
- Descripciones que se ajustan al ancho y controles numéricos separados del texto.
- Navegación por secciones adaptada al ancho, con desplazamiento cuando el contenido lo necesita.
- Se conserva la estética NES y la tipografía Pixelify Sans.

### Tamaño de la ventana del juego

Cambiar FPS, audio, volumen, filtro o escalado entero ya no vuelve a imponer el tamaño de ventana configurado. El motor conserva el tamaño manual o el estado maximizado cuando esos ajustes no modifican la geometría. Cambiar explícitamente la escala de ventana o el modo de pantalla sigue disponible.

## Validación realizada

- Compilación Release para Windows x64 completada en el equipo de desarrollo; arranque del juego y carga de Pixelify Sans observados en el registro.
- Revisión visual manual del menú final confirmada en ese equipo.
- Pruebas aisladas con ImGui y la tipografía real en 320×240, 640×480, 768×720, 1280×720, 1920×1080 y 3840×2160: centrado, límites, menú inmóvil, botón Continuar y adaptación a cambios del área disponible.
- Pruebas aisladas del ajuste del texto y de los botones −/+ en áreas estrechas.
- Reconstrucción mediante los parches acumulativos y comparación de las 16 fuentes afectadas, normalizando únicamente los finales de línea.
- El preparador reconoce los parches v0.7.0 ya aplicados sobre las revisiones fijadas.

Estas comprobaciones no equivalen a una prueba completa de todas las fases, mandos, resoluciones o equipos. La última revisión del menú se compiló de forma incremental; no se atribuye a esta versión una reconstrucción final completa desde cero.

## Dependencias fijadas

| Componente | Revisión original | Parche acumulativo |
| --- | --- | --- |
| NESRecomp | `5a2eddeecc646e9325bef53f3c711e119423f99c` | `patches/nesrecomp/arcade-v0.7.patch` |
| recomp-ui | `7c635c6b366a6e0d5d3c913a7fce6ec04dd9f256` | `patches/recomp-ui/arcade-v0.7.patch` |

Los parches v0.7 incluyen las adaptaciones anteriores. Se aplican sobre las revisiones originales, no encima de v0.6. El submódulo permanece fijado a la revisión original y las adaptaciones se mantienen en el parche del proyecto.

## Pendientes

- Validar `crear_portable.ps1` y el portable personal de esta versión.
- Ampliar las pruebas de cambios de pantalla y persistencia de preferencias entre sesiones.
- Probar otros equipos, mandos y configuraciones de Windows.
- Realizar una reconstrucción final completa desde un entorno limpio antes de atribuirle esa validación.

## Distribución

Se mantienen la distribución de fuentes y parches y la necesidad de que cada usuario aporte una ROM compatible. No se incluyen ROMs, código generado desde ellas, `dist` ni ejecutables derivados. Las dependencias y la tipografía conservan sus respectivas licencias.
