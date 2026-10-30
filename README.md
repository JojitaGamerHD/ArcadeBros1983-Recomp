# Arcade Bros 1983 Recomp

**Port no oficial de Mario Bros. (NES) para Windows x64 mediante recompilación estática con NESRecomp.**

[![Estado](https://img.shields.io/badge/estado-%20Jugable-green)](https://github.com/JojitaGamerHD/ArcadeBros1983-Recomp)
[![Plataforma](https://img.shields.io/badge/plataforma-Windows%20x64-blue)](https://github.com/JojitaGamerHD/ArcadeBros1983-Recomp)

**Versión en desarrollo: v0.8.0** · Rama `desarrollo-v0.8.0`

Arcade Bros 1983 Recomp es un proyecto independiente de aprendizaje y experimentación técnica. Su objetivo es ejecutar la versión de NES de **Mario Bros.** como un programa nativo de Windows, conservando el comportamiento del juego y añadiendo funciones propias orientadas a una experiencia arcade y portable.

> **Importante:** este repositorio **no contiene la ROM**, código C generado a partir de ella ni ejecutables que incorporen contenido derivado del juego. Cada usuario debe proporcionar una copia compatible de la ROM conforme a la legislación aplicable. El proyecto no está afiliado a Nintendo.

## ¿Cómo funciona?

[NESRecomp](https://github.com/mstan/nesrecomp) traduce instrucciones de la CPU 6502 de una ROM de NES a código C. Después, el compilador de Visual Studio genera un ejecutable nativo para Windows x64.

El resultado sigue utilizando el *runtime* de NESRecomp para reproducir los componentes de la consola, incluidos los gráficos, el sonido y la entrada. **No es una reescritura manual del juego ni una eliminación completa de la emulación del hardware.**

Para la interfaz se integra [recomp-ui](https://github.com/RetroPortingToolKit/recomp-ui), adaptado al español mediante un parche mantenido en este repositorio.

## Características del proyecto

- **Launcher gráfico** para iniciar el juego y acceder a su configuración.
- **Menús en español**, incluidas las principales opciones del launcher y el menú de pausa.
- **Volver al launcher** desde una partida sin cerrar el ejecutable y comenzar otra sesión.
- **Versión portable para Windows**: importación de una ROM compatible en la primera ejecución y almacenamiento local de los datos.
- **Récord permanente**: la puntuación máxima se guarda entre ejecuciones mediante un módulo propio, sin alterar la ROM.
- **Compatibilidad comprobada con mando Xbox** y controles de teclado.
- **Opciones de visualización** proporcionadas por el runtime, como pantalla completa, escalado y filtrado.
- **Enfoque arcade**: desactivación de los atajos habituales de guardar/cargar estado y de avance rápido; las opciones de estados se ocultan del menú de pausa.

Las funciones anteriores se han probado manualmente en el entorno de desarrollo. **v0.8.0 todavía está en desarrollo** y no implica una certificación de compatibilidad completa.

### Novedades de v0.8.0

- **Música de fondo opcional:** reproduce una pista local durante las fases, después de que termine la introducción original del juego.
- **Inicio y pausa coordinados:** no suena en la portada, las demostraciones ni Game Over; al abrir el menú se pausa y al continuar retoma su posición.
- **Reinicio por fase:** la misma pista vuelve al inicio en cada fase y se repite en bucle mientras se juega. Esta versión no incluye una lista de canciones.
- **Ajustes compartidos:** Música de fondo y Volumen de música están disponibles en Audio tanto en el launcher como en el menú de pausa.
- **Audio original conservado:** los efectos de NES siguen activos; el interruptor y el volumen generales se aplican también a la música.
- **Archivo privado:** cada usuario aporta `assets/music/stage1.m4a`. La música empieza desactivada y su volumen independiente predeterminado es 35 %.

Consulta la preparación de la pista más abajo y el alcance de las pruebas en [las notas de v0.8.0](docs/NOTAS-v0.8.0.md).

### Novedades de v0.7.0

- **Configuración consistente:** Pantalla, Gráficos y Audio comparten los ajustes entre el launcher y el menú de pausa.
- **Launcher:** incorpora Mostrar FPS, Límite de FPS y el interruptor de Audio.
- **Escala de ventana:** los dos menús ofrecen de 1× a 8×; el control se habilita en modo ventana.
- **VSync:** disponible en ambos menús con el límite de 60 FPS; a otras frecuencias se conserva la preferencia, pero no se aplica.
- **Menú de pausa fijo:** centrado y adaptado al espacio disponible, con texto que se ajusta al ancho. No se arrastra ni se redimensiona manualmente.
- **Ventana del juego:** cambiar FPS, audio o filtro ya no vuelve a imponer su tamaño configurado. Los cambios explícitos de escala o pantalla completa siguen disponibles.

Consulta los detalles y el alcance de las pruebas en [las notas de v0.7.0](docs/NOTAS-v0.7.0.md).

### Funciones de v0.5.0 y v0.6.0

- **FPS configurables:** 30, 60, 120 y 180 FPS.
- **Contador de FPS:** simulación NES y presentación.
- **HUD de FPS v0.6.0:** indicador discreto, de tamaño reducido y sin recuadro.
- **Portada v0.6.0:** imagen pixel art personalizada integrada al launcher.
- **Menú NES:** diseño compacto con fuente Pixelify Sans.
- **Gráficos:** ajustes de FPS en una sola sección.
- **Controles:** configuración conservada en el launcher.
- **Reproducibilidad:** parches acumulativos comprobados sobre las revisiones fijadas.

**Importante:** los modos superiores a 60 FPS repiten
fotogramas de presentación. No existe interpolación real.
### Limitaciones y trabajo pendiente

- Ampliar las pruebas de todas las fases, del modo de dos jugadores y de distintos mandos y equipos.
- Verificar y restringir cualquier vía alternativa de uso de save states que permanezca disponible en el runtime.
- Mejorar el proceso de compilación reproducible y las pruebas automatizadas.
- Investigar interpolación real de fotogramas sin alterar la velocidad del juego.
- Validar el empaquetador portable con los recursos opcionales de música y ampliar las pruebas de transiciones durante partidas reales.
- Incorporar en el futuro varias pistas o una selección de música por fase.

## Requisitos para compilar

- Windows 10 u 11 de **64 bits**.
- Componentes multimedia de Windows (Media Foundation) para la música opcional. En las ediciones Windows N puede ser necesario instalar Media Feature Pack.
- Git y PowerShell.
- CMake **3.20 o posterior**.
- Python **3.11 o posterior**, disponible en `PATH`.
- Visual Studio 2022 o Build Tools 2022 con las herramientas de **Desarrollo de escritorio con C++**.
- Las dependencias utilizadas por NESRecomp, incluidas las que prepara el proceso de compilación.
- Una **ROM compatible de Mario Bros. para NES**, proporcionada por el usuario. La configuración espera el nombre exacto `Mario Bros..nes` (con dos puntos antes de `nes`).

El proyecto se ha desarrollado y probado principalmente en Windows 11 con el generador `Visual Studio 17 2022` para arquitectura x64.

## Compilación desde el código fuente

### 1. Clonar el proyecto

```powershell
git clone --recurse-submodules https://github.com/JojitaGamerHD/ArcadeBros1983-Recomp.git
cd ArcadeBros1983-Recomp
git switch desarrollo-v0.8.0
git submodule update --init --recursive
```

Es importante inicializar los submódulos **después** de seleccionar `desarrollo-v0.8.0`, para preparar la revisión que utiliza esta rama. Si ya clonaste el proyecto sin sus submódulos, en una **copia limpia**, ejecuta `git submodule update --init --recursive` antes de continuar. No uses ese comando para restablecer un submódulo que tenga cambios locales sin respaldarlos.

### 2. Preparar la ROM

Crea la carpeta `roms` si es necesario y coloca tu ROM compatible en:

```text
roms/Mario Bros..nes
```

**No añadas la ROM a Git y no la subas a GitHub.** Una revisión distinta puede no ser compatible con la recompilación configurada o con la comprobación de la versión portable.

### 3. Preparar las dependencias

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\preparar_dependencias.ps1
```

El script prepara una copia local de NESRecomp en `deps/nesrecomp-arcade`, verifica revisiones fijadas, aplica los parches de este proyecto a NESRecomp y `recomp-ui` y compila la herramienta `NESRecomp.exe` si es necesario.

Revisiones utilizadas para v0.8.0:

| Componente | Revisión |
| --- | --- |
| NESRecomp | `5a2eddeecc646e9325bef53f3c711e119423f99c` |
| recomp-ui | `7c635c6b366a6e0d5d3c913a7fce6ec04dd9f256` |

Los parches se encuentran en `patches/nesrecomp/arcade-v0.8.patch` y `patches/recomp-ui/arcade-v0.8.patch`. Son acumulativos sobre las revisiones originales: no deben aplicarse encima de los parches de una versión anterior. El script detecta si ya están aplicados; no debe utilizarse para sobrescribir cambios manuales incompatibles.

### 4. Preparar la música opcional

Antes de configurar CMake, coloca una pista propia en:

```text
assets/music/stage1.m4a
```

Debe ser un archivo de audio M4A/MP4 válido compatible con Media Foundation. **Cambiar la extensión de un MP3 a `.m4a` no convierte el audio.** Si tu pista está realmente en MP3, conviértela primero a M4A con una herramienta de audio.

La carpeta `assets/music/` está excluida de Git. La pista no se incluye en el repositorio ni se incorpora dentro del ejecutable. Durante la compilación, si estaba presente al configurar CMake, se copia a `assets/music/stage1.m4a` junto al juego. Si la añades después, vuelve a ejecutar la configuración de CMake y la compilación del paso siguiente.

Si no aportas música o no puede decodificarse, el juego conserva su audio NES. Para escucharla, activa una vez **Audio → Música de fondo** en el launcher o en la pausa y ajusta **Volumen de música**. Los ajustes se guardan entre sesiones.

### 5. Compilar

```powershell
cmake -S . -B build/arcade-test -G "Visual Studio 17 2022" -A x64
cmake --build build/arcade-test --config Release --parallel 4
```

### 6. Ejecutar

```powershell
.\build\arcade-test\Release\ArcadeBros1983Recomp.exe
```

La compilación genera archivos a partir de la ROM local; **no publiques la carpeta `build/`, `generated/` ni sus ejecutables**.

### Pruebas aisladas de música

Las pruebas se configuran por separado y no necesitan recompilar el juego:

```powershell
cmake -S tests -B build/tests-v080 -G "Visual Studio 17 2022" -A x64
cmake --build build/tests-v080 --config Release --parallel 1
ctest --test-dir build/tests-v080 -C Release --output-on-failure
```

Comprueban las preferencias, la mezcla y la coordinación con los estados del juego. La prueba de decodificación con la pista real se omite si no existe el archivo local; las demás se ejecutan normalmente. Estas pruebas no sustituyen la escucha y las partidas de comprobación.

## Crear una versión portable personal

**Estado v0.8.0:** compilación Release y prueba manual de la música confirmadas en el equipo de desarrollo. La generación y las pruebas específicas del portable mediante `crear_portable.ps1`, incluida la música opcional, siguen pendientes para esta versión.

Con el proyecto y sus dependencias preparados:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\crear_portable.ps1
```

El script genera una instalación **para uso personal** dentro de:

```text
dist/
└── Arcade Bros 1983 Recomp/
    ├── ArcadeBros1983Recomp.exe     # Lanzador portable
    ├── Bin/
    │   ├── ArcadeBros1983Recomp.exe # Juego recompilado
    │   ├── assets/                  # Recursos de recomp-ui
    │   ├── config.ini
    │   └── ...                      # DLL y otros recursos
    └── Datos/
        ├── ROM/                     # ROM importada por el usuario
        └── Records/                 # Récords permanentes
```

Abre el ejecutable situado en la **carpeta principal** de la instalación portable. El lanzador solicita una ROM compatible cuando todavía no existe una copia válida en `Datos/ROM`. En las siguientes ejecuciones la reutiliza.

La carpeta portable puede moverse a otra ubicación o unidad de almacenamiento, conservando los datos que contenga. Al regenerarla, el script está diseñado para respetar la ROM importada y los récords existentes.

> **No distribuyas `dist/`:** contiene ejecutables derivados de la ROM y puede contener una copia privada del juego y de la música. La publicación pública de esta versión se limita a código fuente, scripts y parches; cada usuario aporta su ROM y su pista.

## Organización del repositorio

```text
ArcadeBros1983-Recomp/
├── CMakeLists.txt
├── game.toml
├── .gitmodules
├── assets/music/               # Música privada opcional: ignorada por Git
├── config/                     # Plantillas de configuración
├── docs/                       # Documentación técnica
├── patches/
│   ├── nesrecomp/              # Adaptaciones del motor
│   └── recomp-ui/              # Traducciones y ajustes de interfaz
├── src/
│   ├── launcher.cpp            # Lanzador portable propio
│   ├── records.c               # Récords y conexión con la música
│   ├── music.cpp               # Decodificación y mezcla de la pista local
│   ├── music.h                 # Interfaz de música
│   └── music_gate.h            # Coordinación con las fases del juego
├── tests/                      # Pruebas aisladas de música y preferencias
├── tools/
│   ├── preparar_dependencias.ps1
│   └── crear_portable.ps1
├── recomp-ui/                  # Submódulo Git
├── deps/                       # NESRecomp local: ignorado por Git
├── roms/                       # ROM privada: ignorada por Git
├── build/                      # Compilación: ignorada por Git
├── generated/                  # Código derivado: ignorado por Git
└── dist/                       # Instalación privada: ignorada por Git
```

Las carpetas ignoradas pueden no existir inmediatamente después de clonar el repositorio.

## Historial de versiones

### v0.8.0 — Música opcional durante las fases (en desarrollo)

- Pista local en bucle, coordinada con la introducción, la pausa y los cambios de fase.
- Controles compartidos de activación y volumen de música en launcher y pausa.
- Decodificación con Media Foundation y mezcla con los efectos originales de NES.
- Pruebas aisladas de preferencias, mezcla y estados del juego; revisión manual de la música en el equipo de desarrollo.
- Parches acumulativos v0.8.0 con reconstrucción comprobada de las fuentes afectadas.
- Pendiente validar el portable y ampliar las partidas de prueba en otros equipos.

### v0.7.0 — Configuración compartida y menú de pausa adaptable (en desarrollo)

- Mismas opciones de pantalla, gráficos y audio en launcher y pausa.
- Contador y límite de FPS configurables también desde el launcher.
- Menú fijo, centrado y proporcional, con descripciones que se ajustan al ancho.
- Corrección del redimensionado involuntario al cambiar opciones ajenas al tamaño de ventana.
- Parches acumulativos v0.7.0 y comprobación de reconstrucción de las fuentes.
- Pendiente ampliar pruebas en otros equipos y validar el empaquetador portable.

### v0.6.0 — Launcher, HUD y reconstrucción limpia (en desarrollo)

- Nueva portada pixel art personalizada para el launcher.
- Contador de FPS compacto, sin recuadro y menos invasivo.
- Parches de NESRecomp y recomp-ui verificados sobre las revisiones originales.
- Compilación limpia del juego y del launcher en Release x64.
- Recursos visuales y tipografía integrados correctamente.
- Instalación portable de prueba con 8 de 8 comprobaciones funcionales satisfactorias.
- Pendiente: validar nuevamente crear_portable.ps1 y probar en otros equipos.

### v0.5.0 — FPS configurables y menú NES (en desarrollo)

- Presentación configurable a 30, 60, 120 y 180 FPS.
- Contador de FPS NES y FPS de pantalla.
- Menú de pausa con estética NES de 8 bits.
- Tipografía Pixelify Sans compatible con español.
- Controles configurables desde el launcher.
- Parches reproducibles y reconstrucción limpia verificada.
### v0.4.0 — Interfaz en español y regreso al launcher (en desarrollo)

- Integración de `recomp-ui` y traducción de las opciones principales.
- Nuevo menú de pausa en español y acción para regresar al launcher.
- Empaquetado portable actualizado con sus recursos gráficos.
- Motor NESRecomp aislado en `deps/` con parche reproducible.
- Script para preparar y verificar las dependencias.

### v0.3.0 — Edición arcade y récords

- Sistema de puntuación máxima persistente.
- Ajustes para evitar el uso habitual de save states desde atajos y menú.
- Mejoras en el lanzador y la instalación portable.

### v0.1.0 — Primera compilación funcional

- Traducción inicial de **1.235 instrucciones** de CPU 6502 a código C, según las pruebas del proyecto.
- Ejecutable para Windows x64 integrado con el runtime de NESRecomp.
- Prueba manual hasta la fase 10 y reconocimiento de un mando Xbox.

## Créditos

- **Juego original:** [Nintendo](https://www.nintendo.com/) — creadora y titular de los derechos correspondientes sobre Mario Bros. y sus elementos originales.
- **Port y modificaciones de Arcade Bros 1983 Recomp:** [JojitaGamerHD](https://github.com/JojitaGamerHD).
- **Herramienta de recompilación y runtime:** [NESRecomp](https://github.com/mstan/nesrecomp), de [Matthew Stanley (@mstan)](https://github.com/mstan), y colaboradores.
- **Interfaz gráfica:** [recomp-ui](https://github.com/RetroPortingToolKit/recomp-ui), de [RetroPortingToolKit](https://github.com/RetroPortingToolKit) y colaboradores.

El reconocimiento de estos proyectos no implica afiliación ni aprobación oficial de sus autores.

## Aviso legal y licencias

**Arcade Bros 1983 Recomp es un proyecto no oficial, experimental y sin fines comerciales.** No está afiliado, patrocinado ni respaldado por Nintendo.

Este repositorio no proporciona archivos ROM, recursos extraídos del juego ni código compilado o generado a partir de ellos. El usuario es responsable de obtener y utilizar su copia del juego conforme a la legislación aplicable.

NESRecomp y recomp-ui son proyectos de terceros sujetos a sus respectivas licencias. Consulta sus repositorios y archivos de licencia antes de utilizar, modificar o redistribuir sus componentes. Las condiciones aplicables al código original de este proyecto deben comprobarse en el archivo de licencia correspondiente, si se incorpora uno; **no se presume que una licencia propia cubra derechos de terceros o de Nintendo**.

---

*Arcade Bros 1983 Recomp — Un experimento de recompilación nativa y preservación técnica de videojuegos clásicos.*
