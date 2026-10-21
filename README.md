# Arcade Bros 1983 Recomp

**Port no oficial de Mario Bros. (NES) para Windows x64 mediante recompilación estática con NESRecomp.**

[![Estado](https://img.shields.io/badge/estado-en%20desarrollo-yellow)](https://github.com/JojitaGamerHD/ArcadeBros1983-Recomp)
[![Plataforma](https://img.shields.io/badge/plataforma-Windows%20x64-blue)](https://github.com/JojitaGamerHD/ArcadeBros1983-Recomp)

**Versión en desarrollo: v0.4.0** · Rama `desarrollo`

Arcade Bros 1983 Recomp es un proyecto independiente de aprendizaje y experimentación técnica. Su objetivo es ejecutar la versión de NES de **Mario Bros.** como un programa nativo de Windows, conservando el comportamiento del juego y añadiendo funciones propias orientadas a una experiencia arcade y portable.

> **Importante:** este repositorio **no contiene la ROM**, código C generado a partir de ella ni ejecutables que incorporen contenido derivado del juego. Cada usuario debe proporcionar una copia compatible de la ROM conforme a la legislación aplicable. El proyecto no está afiliado a Nintendo.

## ¿Cómo funciona?

[NESRecomp](https://github.com/mstan/nesrecomp) traduce instrucciones de la CPU 6502 de una ROM de NES a código C. Después, el compilador de Visual Studio genera un ejecutable nativo para Windows x64.

El resultado sigue utilizando el *runtime* de NESRecomp para reproducir los componentes de la consola, incluidos los gráficos, el sonido y la entrada. **No es una reescritura manual del juego ni una eliminación completa de la emulación del hardware.**

Para la interfaz se integra [recomp-ui](https://github.com/RetroPortingToolKit/recomp-ui), adaptado al español mediante un parche mantenido en este repositorio.

## Características de v0.4.0

- **Launcher gráfico** para iniciar el juego y acceder a su configuración.
- **Menús en español**, incluidas las principales opciones del launcher y el menú de pausa.
- **Volver al launcher** desde una partida sin cerrar el ejecutable y comenzar otra sesión.
- **Versión portable para Windows**: importación de una ROM compatible en la primera ejecución y almacenamiento local de los datos.
- **Récord permanente**: la puntuación máxima se guarda entre ejecuciones mediante un módulo propio, sin alterar la ROM.
- **Compatibilidad comprobada con mando Xbox** y controles de teclado.
- **Opciones de visualización** proporcionadas por el runtime, como pantalla completa, escalado y filtrado.
- **Enfoque arcade**: desactivación de los atajos habituales de guardar/cargar estado y de avance rápido; las opciones de estados se ocultan del menú de pausa.

Las funciones anteriores se han probado manualmente en el entorno de desarrollo. **v0.4.0 todavía está en desarrollo** y no implica una certificación de compatibilidad completa.

### Limitaciones y trabajo pendiente

- Ampliar las pruebas de todas las fases, del modo de dos jugadores y de distintos mandos y equipos.
- Verificar y restringir cualquier vía alternativa de uso de save states que permanezca disponible en el runtime.
- Mejorar el proceso de compilación reproducible y las pruebas automatizadas.
- Investigar renderizado interpolado a **120, 144 y 180 FPS** sin acelerar la lógica original. **Esta función todavía no está implementada.**

## Requisitos para compilar

- Windows 10 u 11 de **64 bits**.
- Git y PowerShell.
- CMake **3.20 o posterior**.
- Visual Studio 2022 o Build Tools 2022 con las herramientas de **Desarrollo de escritorio con C++**.
- Las dependencias utilizadas por NESRecomp, incluidas las que prepara el proceso de compilación.
- Una **ROM compatible de Mario Bros. para NES**, proporcionada por el usuario. La configuración espera el nombre exacto `Mario Bros..nes` (con dos puntos antes de `nes`).

El proyecto se ha desarrollado y probado principalmente en Windows 11 con el generador `Visual Studio 17 2022` para arquitectura x64.

## Compilación desde el código fuente

### 1. Clonar el proyecto

```powershell
git clone --recurse-submodules https://github.com/JojitaGamerHD/ArcadeBros1983-Recomp.git
cd ArcadeBros1983-Recomp
git switch desarrollo
git submodule update --init --recursive
```

Es importante inicializar los submódulos **después** de seleccionar `desarrollo`, para preparar la revisión que utiliza esta rama. Si ya clonaste el proyecto sin sus submódulos, en una **copia limpia**, ejecuta `git submodule update --init --recursive` antes de continuar. No uses ese comando para restablecer un submódulo que tenga cambios locales sin respaldarlos.

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

Revisiones utilizadas para v0.4.0:

| Componente | Revisión |
| --- | --- |
| NESRecomp | `5a2eddeecc646e9325bef53f3c711e119423f99c` |
| recomp-ui | `7c635c6b366a6e0d5d3c913a7fce6ec04dd9f256` |

Los parches se encuentran en `patches/nesrecomp/arcade-v0.4.patch` y `patches/recomp-ui/espanol-v0.4.patch`. El script detecta si ya están aplicados; no debe utilizarse para sobrescribir cambios manuales incompatibles.

### 4. Compilar

```powershell
cmake -S . -B build/arcade-test -G "Visual Studio 17 2022" -A x64
cmake --build build/arcade-test --config Release --parallel 4
```

### 5. Ejecutar

```powershell
.\build\arcade-test\Release\ArcadeBros1983Recomp.exe
```

La compilación genera archivos a partir de la ROM local; **no publiques la carpeta `build/`, `generated/` ni sus ejecutables**.

## Crear una versión portable personal

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

> **No distribuyas `dist/`:** contiene ejecutables derivados de la ROM y puede contener una copia privada del juego. El repositorio público distribuye código fuente, scripts y parches, no el juego de Nintendo.

## Organización del repositorio

```text
ArcadeBros1983-Recomp/
├── CMakeLists.txt
├── game.toml
├── .gitmodules
├── config/                     # Plantillas de configuración
├── docs/                       # Documentación técnica
├── patches/
│   ├── nesrecomp/              # Adaptaciones del motor
│   └── recomp-ui/              # Traducciones y ajustes de interfaz
├── src/
│   ├── launcher.cpp            # Lanzador portable propio
│   └── records.c               # Récords y extensiones del juego
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
