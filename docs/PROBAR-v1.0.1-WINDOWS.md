# Probar v1.0.1 en otro PC con Windows

Esta es la rama de pruebas `desarrollo-v1.0.1`. Descargarla no cambia tu copia
de v1.0.0 ni publica una release. Los comandos siguientes construyen un juego
de prueba con tu ROM local; no necesitan preparar el ZIP portable ni descargar
el kit LLVM de ese paquete.

## 1. Herramientas que necesita ese PC

- Windows 10/11 de 64 bits, componentes multimedia y gráficos OpenGL 3.3.
- Git, CMake 3.21 o posterior y Visual Studio 2022 o Build Tools 2022 con
  **Desarrollo para el escritorio con C++** y Windows SDK.
- Una instalación real de **Python 3.11 o posterior**, detectable por CMake.
  El acceso directo de Microsoft Store que invita a instalar Python no basta.
- Tu ROM compatible. Ni la ROM ni tus datos personales se descargan de GitHub.

Si ese PC ya se utilizaba para compilar el proyecto, quizá tenga estas
herramientas. La construcción inicial necesita Internet para las dependencias.

## Si ya probaste la primera candidata

Dentro de tu carpeta `ArcadeBros-Prueba-v101`, actualiza y vuelve a compilar:

```powershell
git pull --ff-only origin desarrollo-v1.0.1
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\preparar_dependencias.ps1
cmake -S . -B build/prueba-v101 -G "Visual Studio 17 2022" -A x64 "-DNESRECOMP_GAME_VERSION=1.0.1"
cmake --build build/prueba-v101 --config Release --target ArcadeBros1983Recomp --parallel 1
```

El preparador reconoce los parches de la primera candidata. No borres `deps`,
`recomp-ui`, `config.ini` ni tus presets para actualizar. Si Git o el preparador
informan de un conflicto con cambios tuyos, conserva esos cambios y comparte
el mensaje antes de reemplazar archivos. Después abre el ejecutable como en
el paso 4 de esta guía.

## 2. Descargar una copia separada

Abre PowerShell en la carpeta donde quieras guardar la prueba y ejecuta:

```powershell
git clone --branch desarrollo-v1.0.1 --recurse-submodules https://github.com/JojitaGamerHD/ArcadeBros1983-Recomp.git ArcadeBros-Prueba-v101
Set-Location .\ArcadeBros-Prueba-v101
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\preparar_dependencias.ps1
```

La carpeta `ArcadeBros-Prueba-v101` debe ser nueva. Conserva tu proyecto y tu
portable anteriores para compararlos. No copies sus carpetas `deps`,
`recomp-ui` o `build` encima de esta copia; el preparador descarga las revisiones
fijadas y les aplica los parches de la candidata.

## 3. Añadir tu ROM y compilar

Crea la carpeta `roms` dentro del clon y copia allí tu ROM con este nombre
exacto, incluidos los dos caracteres `..` antes de `nes`:

```text
roms/Mario Bros..nes
```

Puedes comprobar su huella con:

```powershell
Get-FileHash -Algorithm SHA256 -LiteralPath '.\roms\Mario Bros..nes'
```

La revisión compatible tiene esta huella:

```text
a2a0aa437a735cd36ed29a8e7c5b3dfa10590c6203e893f1668c7995bad2b309
```

Desde la carpeta principal del clon:

```powershell
cmake -S . -B build/prueba-v101 -G "Visual Studio 17 2022" -A x64 "-DNESRECOMP_GAME_VERSION=1.0.1"
cmake --build build/prueba-v101 --config Release --target ArcadeBros1983Recomp --parallel 1
```

Continúa solo si ambos comandos terminan correctamente. Si CMake no encuentra
Python, indica su ejecutable real al repetir el primer comando con
`"-DPython3_EXECUTABLE=C:\ruta\real\python.exe"`, sustituyendo esa ruta por la
de tu instalación.

## 4. Abrir el juego de prueba

Desde la carpeta principal del clon, ejecuta este bloque completo:

```powershell
$romPrueba = (Resolve-Path -LiteralPath '.\roms\Mario Bros..nes').Path
Push-Location .\build\prueba-v101\Release
try {
    & .\ArcadeBros1983Recomp.exe $romPrueba
} finally {
    Pop-Location
}
```

El ejecutable está en `build/prueba-v101/Release/ArcadeBros1983Recomp.exe`.
Abre su launcher de configuración antes de iniciar la partida. Para esta
construcción directa, `config.ini` queda junto al ejecutable y los récords
quedan en su carpeta `Records`; no utiliza el `Datos` de tu portable anterior.
Repite el bloque para volver a abrirlo y comprobar la persistencia.

Usa este ejecutable para la prueba. El launcher portable externo de v1.0.0
espera otra distribución de carpetas y no se necesita en este procedimiento.

## 5. Qué probar primero

1. Cambia A de Z a C y prueba en partida. C debe funcionar y Z debe dejar de
   activar A. Cierra todo y vuelve a abrir: la asignación debe conservarse.
2. Repite con mando: botones, cruceta, un gatillo y reconexión. Comprueba
   conflictos, Esc para cancelar, Retroceso para borrar y restaurar valores.
3. En pausa → **Colores**, activa **Personalizar personajes** y abre una muestra
   de Mario. Elige con la rueda y el triángulo de brillo/saturación: el personaje
   de referencia debe cambiar inmediatamente. Prueba **Listo**, **Cancelar** y Esc.
   La referencia aparece después de haber visto al personaje en una partida.
4. Asigna colores distintos a Mario y Luigi. Guarda en las ranuras 1 y 8,
   cambia los colores, reinicia el juego y carga ambos presets.
5. Comprueba caminar, saltar, girarse y morir a 60, 120 y 180 FPS: un cambio
   de animación no debe volver al color original por fallar la interpolación.
   Comprueba también fondo, enemigos, pérdida de vida, cambios de fase y dos
   jugadores. Los sprites no identificados en pantallas fuera del mapa conocido
   conservan su color original.

Anota qué hiciste, qué esperabas y qué ocurrió. Para un problema de mando,
incluye su modelo; para un problema visual, incluye la fase y el límite de FPS.
Con esos resultados se corrige la misma rama antes de fusionar o publicar.

Las pruebas automáticas y la construcción del portable completo se explican
en [NOTAS-v1.0.1](NOTAS-v1.0.1.md). Este procedimiento genera código privado
a partir de tu ROM para uso local; conserva esos archivos en tu equipo.
