# Sistema de récords permanentes

## Descripción

Arcade Bros 1983 Recomp permite conservar la puntuación
máxima de Mario Bros. (1983) entre distintas sesiones.

Esta funcionalidad no existe de forma permanente en
el juego original de NES.

## Funcionamiento

1. El jugador comienza una partida desde cero.
2. El juego calcula la puntuación normalmente.
3. El módulo detecta cuando se supera el récord.
4. El nuevo récord se guarda automáticamente.
5. Al reiniciar el juego, se recupera la puntuación máxima.

## Implementación técnica

Archivo principal: src/records.c

Se utiliza la interfaz de modificaciones de NESRecomp:

- cyc_mod_peek(): lectura de memoria.
- cyc_mod_poke(): escritura controlada de memoria.
- frame_begin / frame_end: integración con el ciclo del juego.

El módulo trabaja con las direcciones de memoria del
récord original de Mario Bros. y conserva el resultado
en un archivo local.

## Almacenamiento

Datos/Records/record.dat

El archivo permanece dentro de la instalación portable.

## Modo arcade

Los atajos F8 y F9 para guardar y cargar estados están
desactivados de forma predeterminada.

Esto no implica que las funciones internas de save states
hayan sido eliminadas completamente del runtime.

## Pruebas realizadas

- Detección de nuevas puntuaciones: correcta.
- Guardado del récord: correcto.
- Recuperación tras cerrar el juego: correcta.
- Funcionamiento del launcher portable: correcto.
- Atajos F8 y F9 desactivados: comprobado.

Las pruebas fueron manuales en Windows 11.

## Créditos

NESRecomp fue creado por Matthew Stanley (@mstan).

https://github.com/mstan/nesrecomp
