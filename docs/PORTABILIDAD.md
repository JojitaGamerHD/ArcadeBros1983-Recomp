# Lanzador portable — Arcade Bros 1983 Recomp

## Objetivo

El usuario abre `ArcadeBros1983Recomp.exe`. En el primer inicio elige una ROM NES original compatible. El launcher verifica el SHA-256 contra la ROM de la recompilacion v0.1.0 y hace una copia privada en `Datos/ROM/Mario Bros..nes`. En los inicios siguientes reutiliza esa copia, sin pedirla otra vez.

## Estructura de distribucion

```text
Arcade Bros 1983 Recomp/
├── ArcadeBros1983Recomp.exe      # Launcher propio
├── Bin/
│   ├── ArcadeBros1983Recomp.exe  # Juego recompilado
│   └── *.dll                     # Dependencias nativas
└── Datos/
    ├── ROM/
    │   └── Mario Bros..nes       # Aparece tras la importacion (NO distribuir)
    └── ...                       # Preferencias y archivos creados al ejecutar
```

No necesita instalacion ni rutas absolutas guardadas para la ROM. El launcher da al juego `Datos/` como directorio de trabajo, para mantener dentro de la instalacion los archivos que el runtime escriba relativamente al directorio actual. **Falta verificar en prueba real** si alguna funcion del runtime usa rutas del perfil de Windows, rutas derivadas del ejecutable (Bin/) u otras ubicaciones.

## Construccion

1. Copia `src/launcher.cpp` y `tools/crear_portable.ps1` en tu repositorio.
2. Agrega al final del archivo `CMakeLists.txt`:

```cmake
if(WIN32)
    add_executable(ArcadeBrosLauncher WIN32 src/launcher.cpp)
    target_compile_features(ArcadeBrosLauncher PRIVATE cxx_std_17)
    target_link_libraries(ArcadeBrosLauncher PRIVATE comdlg32 bcrypt)
endif()
```

3. Agrega `dist/` al `.gitignore`, con una linea separada.
4. En PowerShell, dentro del proyecto:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\crear_portable.ps1
```

5. Abre `dist/Arcade Bros 1983 Recomp/ArcadeBros1983Recomp.exe` y prueba el selector.
6. Cierra, mueve toda la carpeta `Arcade Bros 1983 Recomp` a otra ruta o pendrive y vuelve a ejecutar el launcher.

## Notas y limites

- Esto no integra aun launcher y motor en UN solo binario. El usuario abre UN `.exe` de entrada, pero el juego real y sus DLLs viven dentro de `Bin/`.
- Por ahora solo se admite el SHA-256 exacto de la ROM utilizada al recompilar. No se modifica la ROM.
- No se alteran `generated/` ni `main`, y no se crea una version publica sin verificar el conjunto.
- No subas `Datos/`, la ROM, `dist/` ni los ejecutables derivados a GitHub. Mantiene el proyecto en `desarrollo` hasta pruebas.
- La seleccion nativa de archivos de Windows puede generar historiales del sistema (p. ej., ultimos archivos). Eso lo gestiona Windows, no el launcher; no se puede prometer cero rastros a nivel de SO.
- Para volver a elegir ROM, elimina solo `Datos/ROM/Mario Bros..nes` (siempre que tengas copia de tu ROM original).
