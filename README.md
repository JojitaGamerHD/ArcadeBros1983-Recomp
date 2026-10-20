# Arcade Bros 1983 Recomp

**Recompilación no oficial de Mario Bros. (1983) para Windows, realizada con NESRecomp.**

Arcade Bros 1983 Recomp es un proyecto de aprendizaje y preservación técnica que busca ejecutar el código del juego original de Nintendo Entertainment System (NES) como código nativo de Windows de 64 bits, conservando su comportamiento y permitiendo añadir mejoras propias en el futuro.

> **Estado del proyecto:** versión inicial **v0.1.0**, compilada y probada manualmente hasta la fase 10. Es una primera versión funcional, no una certificación de compatibilidad completa.

## ¿Cómo funciona?

El proyecto utiliza **[NESRecomp](https://github.com/mstan/nesrecomp)** para traducir instrucciones de la CPU 6502 de la ROM original a código C. Luego, Visual Studio compila ese código para Windows x64.

El juego sigue dependiendo del entorno de ejecución (*runtime*) de NESRecomp para reproducir el funcionamiento de los componentes de la NES, como gráficos, sonido y controles. La recompilación del código de CPU no significa que se haya reescrito el juego original a mano ni que se haya eliminado toda la reproducción de hardware.

## Estado y características

### Verificado en la versión v0.1.0

- [x] Generación de código C a partir de la ROM de NES.
- [x] Compilación de un ejecutable nativo para Windows x64.
- [x] Inicio y ejecución del juego.
- [x] Detección y funcionamiento de un mando de Xbox.
- [x] Prueba manual de jugabilidad hasta la fase 10.
- [x] Textos, controles y comportamiento de enemigos sin fallos visibles durante esa prueba.
- [x] Jugabilidad fluida y sin retraso de entrada perceptible durante la prueba.

### Pendiente de comprobar o desarrollar

- [ ] Pruebas más extensas de todas las fases y situaciones del juego.
- [ ] Pruebas específicas del modo de dos jugadores y del audio.
- [ ] Configuración propia de controles, pantalla y otras opciones.
- [ ] Mejoras en la presentación gráfica y el escalado.
- [ ] Investigación de renderizado a 120, 144 y 180 FPS mediante interpolación, **sin acelerar la lógica original del juego**.
- [ ] Pruebas automatizadas y compilaciones reproducibles.

**Nota:** la fluidez observada no equivale a una medición instrumental de FPS o latencia. El modo de 120–180 FPS todavía **no está implementado**.

## Requisitos

Para compilar el proyecto en Windows:

- Windows 10 u 11 de 64 bits.
- Git.
- CMake 3.20 o posterior.
- Visual Studio 2022 Build Tools con herramientas de desarrollo de C/C++ para escritorio.
- Python 3.11 o posterior.
- [NESRecomp](https://github.com/mstan/nesrecomp) y sus dependencias de ejecución, incluida SDL2.
- Una copia de la ROM de NES de **Mario Bros. (1983)** compatible con la configuración del proyecto.

El entorno utilizado durante la primera compilación fue Windows 11, Git 2.56, CMake 4.5.0-rc1, Python 3.15 y Visual Studio Build Tools 2022. Estas son las versiones **probadas**, no necesariamente requisitos mínimos.

## Estructura del proyecto

```text
Projects/
├── nesrecomp/                  # Herramienta de terceros (repositorio independiente)
└── ArcadeBros1983-Recomp/
    ├── docs/                   # Documentación del proyecto
    ├── roms/                   # ROM local del usuario; NO se publica
    ├── src/                    # Espacio para modificaciones propias
    ├── generated/              # Código generado desde la ROM; NO se publica
    ├── build/                  # Archivos de compilación; NO se publican
    ├── .gitignore
    ├── CMakeLists.txt
    ├── game.toml
    └── README.md
```

Las carpetas ignoradas por Git pueden no aparecer en una descarga limpia del repositorio. Se crean localmente cuando corresponde.

## Compilación paso a paso

Estas instrucciones suponen que **NESRecomp** y **ArcadeBros1983-Recomp** están en carpetas hermanas, tal como lo espera la configuración actual de CMake.

### 1. Descargar y compilar NESRecomp

Desde PowerShell:

```powershell
New-Item -ItemType Directory -Force -Path "$env:USERPROFILE\Projects"
cd "$env:USERPROFILE\Projects"
git clone https://github.com/mstan/nesrecomp.git
cd nesrecomp
cmake -S recompiler -B build/recompiler -G "Visual Studio 17 2022" -A x64
cmake --build build/recompiler --config Release
```

### 2. Preparar Arcade Bros 1983 Recomp

Obtén los archivos de este proyecto y guárdalos en:

```text
C:\Users\TU_USUARIO\Projects\ArcadeBros1983-Recomp\
```

Cuando este repositorio esté publicado, también se podrá clonar con Git. Asegúrate de conservar la ubicación de carpetas indicada en la sección anterior.

Crea la carpeta local `roms` si todavía no existe y coloca ahí tu ROM, con este nombre exacto:

```text
roms/Mario Bros..nes
```

**No se incluye ninguna ROM en el repositorio.** La compilación está configurada para una ROM concreta; otras revisiones pueden requerir ajustes.

### 3. Generar el código C

Entra en la carpeta de Arcade Bros 1983 Recomp y ejecuta:

```powershell
cd "$env:USERPROFILE\Projects\ArcadeBros1983-Recomp"
& "$env:USERPROFILE\Projects\nesrecomp\build\recompiler\Release\NESRecomp.exe" ".\roms\Mario Bros..nes" --game ".\game.toml"
```

NESRecomp escribirá los archivos C en `generated/`. Esos archivos se crean a partir de la ROM y no deben modificarse manualmente.

### 4. Compilar el juego

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel 4
```

Si la compilación termina correctamente, el ejecutable debería estar en:

```text
build/Release/ArcadeBros1983Recomp.exe
```

### 5. Ejecutar el juego

```powershell
& ".\build\Release\ArcadeBros1983Recomp.exe" ".\roms\Mario Bros..nes"
```

El ejecutable se inicia utilizando la ROM local del usuario. No compartas esa ROM cuando publiques el proyecto.

## Código y futuras modificaciones

El código recompilado está en `generated/` y procede de las instrucciones originales del juego. Puede ser difícil de leer o mantener y se regenera automáticamente.

Las modificaciones propias se desarrollarán preferentemente en `src/` o mediante las interfaces de extensión compatibles con NESRecomp. Esto permitirá incorporar cambios sin sobrescribir los archivos generados.

La meta es mantener el proyecto preparado para actualizaciones, controles configurables, mejoras visuales y futuras investigaciones sobre renderizado a mayor frecuencia. No se garantiza todavía la compatibilidad de esas mejoras con todas las situaciones del juego.

## Historial de versiones

### v0.1.0 — Primera compilación funcional

- Traducción inicial de **1.235 instrucciones** del procesador 6502 a código C, distribuido en cuatro archivos de banco y un despachador principal.
- Integración con el runtime de precisión por ciclos de NESRecomp.
- Generación correcta del ejecutable para Windows x64.
- Prueba manual hasta la fase 10 sin errores visuales o de jugabilidad observados.
- Mando de Xbox reconocido correctamente.

## Créditos y agradecimientos

### Matthew Stanley (@mstan) — Creador de NESRecomp

Un agradecimiento especial a **Matthew Stanley**, conocido en GitHub como **[@mstan](https://github.com/mstan)**, creador de **[NESRecomp](https://github.com/mstan/nesrecomp)**.

NESRecomp proporciona las herramientas de recompilación estática de código 6502 a C y el entorno de ejecución que hacen posible este experimento técnico. Agradecemos también las contribuciones de quienes colaboran con NESRecomp.

- **Proyecto original:** https://github.com/mstan/nesrecomp
- **Autor:** https://github.com/mstan
- **Licencia de NESRecomp:** [PolyForm Noncommercial 1.0.0](https://github.com/mstan/nesrecomp/blob/master/LICENSE).

### Nintendo

**Mario Bros. (1983)**, sus personajes, marcas, gráficos, sonidos y otros elementos originales pertenecen a Nintendo o a sus respectivos titulares. Este proyecto no está afiliado, patrocinado ni respaldado por Nintendo.

## Aviso legal y distribución

Este es un proyecto **independiente, experimental y sin fines comerciales**.

El repositorio no distribuye la ROM, imágenes, audio ni código generado derivado de la ROM. Cada usuario debe obtener y utilizar su propia copia conforme a la legislación aplicable.

NESRecomp es software de terceros sujeto a su licencia **PolyForm Noncommercial 1.0.0**. Sus condiciones deben respetarse al utilizarlo, modificarlo o redistribuir sus componentes. La licencia de los archivos originales de este repositorio se definirá por separado antes de su publicación; no se debe asumir que el código de terceros o el contenido de Nintendo queda cubierto por ella.

---

**Arcade Bros 1983 Recomp** — Una exploración técnica de la recompilación nativa de juegos clásicos.
