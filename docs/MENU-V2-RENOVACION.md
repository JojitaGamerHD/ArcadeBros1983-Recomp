# Renovación del menú — propuesta V2

> Estado: **diseño preliminar**. Este documento no modifica todavía el menú ejecutable. El prototipo HTML independiente se revisa antes de integrar cambios de C++.

## Estado de la versión v1.0.3

- Interfaz nativa con ImGui y `recomp-ui` en el launcher, integrada mediante parches acumulativos.
- El menú usa `ArcadeTitleState`, `ArcadeTitleCanvas`, `draw_arcade_title_screen`, `draw_arcade_options_categories` y `draw_arcade_options_detail`, incorporados en `patches/recomp-ui/arcade-v1.0.2-menu.patch`.
- `patches/recomp-ui/arcade-v1.0.3-grid.patch` amplía las opciones gráficas.
- `tools/preparar_dependencias.ps1` aplica las capas de parches sobre revisiones fijadas del submódulo; no se deben editar las fuentes del submódulo en `main` sin conservar el cambio reproducible.

## Dirección visual propuesta

**Arcade 1983 renovado**: conservar el espíritu de máquina recreativa con jerarquía más moderna y legible.

- Título grande «ARCADE BROS.», subtítulo «1983 RECOMP», créditos discretos a JojitaGamerHD.
- Paleta azul noche, cian, dorado y rojo cálido con bordes pixelados; suficiente contraste.
- Lista de acciones a la izquierda y un panel visual a la derecha.
- Estado de ROM visible, mensaje contextual, pistas de teclado y mando.
- Menús de un jugador/dos jugadores, modo Estándar A o Arcade B, ROM, opciones, créditos y salir.
- Opciones separadas en Vídeo, Gráficos, Audio y Controles.
- Respeto por proporciones, escalado entero y textos en pantallas pequeñas; sin recortar controles.

## Integración técnica prevista

1. Preparar un parche incremental de launcher en `patches/recomp-ui/`, sobre la capa v1.0.3, sin cambiar la revisión fijada de la dependencia.
2. Separar dibujo y navegación del menú para reutilizar botones, foco, retorno y mensajes.
3. Conservar intactos los callbacks `arcade_menu_action(-1)` (ROM) y `arcade_menu_action(0..3)` (cuatro modos), y las rutas de pausa, controles y configuración.
4. Mantener la compatibilidad de teclado y mando. Evaluar soporte opcional de ratón, actualmente desactivado en el menú arcade.
5. Incorporar la capa nueva al script de preparación y a las pruebas correspondientes.
6. Compilar en Windows x64 y comprobar carga de ROM, retorno del juego al menú, opciones, pausa, filtros, audio y altas tasas de presentación.

## Criterios de aceptación

- [ ] El menú nuevo arranca antes de importar una ROM.
- [ ] Los seis destinos del menú principal funcionan y preservan la selección al volver.
- [ ] Los cuatro modos existentes siguen iniciando la partida.
- [ ] Vídeo, Gráficos, Audio y Controles mantienen los ajustes persistidos.
- [ ] El aviso de ROM faltante o incompatible es claro y no bloquea la interfaz.
- [ ] Navegación completa por teclado y mando, sin conflictos entre confirmación y cancelación.
- [ ] No hay pérdida de foco ni clipping con cambios de escala/DPI.
- [ ] La partida y el menú de pausa existentes siguen funcionando.
- [ ] La preparación de dependencias es reproducible desde una clonación limpia.
- [ ] Pruebas y compilación Windows x64 superadas antes de fusionar a `main`.

## Referencia de diseño

Hay un **prototipo HTML externo** de la pantalla principal y de los submenús; su aspecto es orientativo y su selector de ROM solo muestra el nombre del archivo, no lo valida ni lo ejecuta. La implementación definitiva será en C++/ImGui, no HTML.
