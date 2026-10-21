/*
 * Arcade Bros 1983 Recomp - records permanentes (experimental)
 * Codigo original independiente de la ROM.
 *
 * El launcher portable arranca Bin/ArcadeBros1983Recomp.exe con Datos/
 * como directorio de trabajo. Por ello, el archivo Records/record.dat
 * queda dentro de la carpeta portable, sin escribir en AppData.
 *
 * Basado en el mapa RAM de Mario Bros. NES:
 * 0x0091..0x0093: record; 0x0095..0x0097: P1; 0x0099..0x009B: P2.
 * Hipotesis a validar en juego: cada byte representa dos digitos BCD.
 */
#define _CRT_SECURE_NO_WARNINGS
#include "cyc_host_extras.h"
#include "cyc_mod.h"
#include "recomp_runtime_ui.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/stat.h>
#endif

#define RUTA_RECORD "Records/record.dat"
#define RUTA_TEMP "Records/record.tmp"
#define FORMATO "ARCADE_BROS_RECORD_V1"

static uint32_t mejor_record = 0;
static unsigned fotogramas = 0;
static bool inicializado = false;

/* Devuelve false si aparece algun nibble que no sea un digito BCD. */
static bool leer_puntuacion(uint16_t direccion, uint32_t *salida) {
    uint32_t total = 0;
    for (unsigned i = 0; i < 3; ++i) {
        uint8_t dato = cyc_mod_peek((uint16_t)(direccion + i));
        uint8_t decenas = (uint8_t)(dato >> 4);
        uint8_t unidades = (uint8_t)(dato & 0x0F);
        if (decenas > 9 || unidades > 9) return false;
        total = total * 100u + decenas * 10u + unidades;
    }
    *salida = total;
    return true;
}

static void cargar_record(void) {
    FILE *archivo = fopen(RUTA_RECORD, "rb");
    char marca[64];
    unsigned numero = 0;
    if (!archivo) return;
    if (fgets(marca, sizeof marca, archivo) &&
        strcmp(marca, FORMATO "\n") == 0 &&
        fscanf(archivo, "%u", &numero) == 1 && numero <= 999999u) {
        mejor_record = numero;
    }
    fclose(archivo);
}

static void guardar_record(void) {
#ifdef _WIN32
    CreateDirectoryA("Records", NULL); /* Ya existe = no es error. */
#else
    mkdir("Records", 0755);
#endif
    FILE *archivo = fopen(RUTA_TEMP, "wb");
    if (!archivo) {
        fprintf(stderr, "[records] No se pudo abrir %s\n", RUTA_TEMP);
        return;
    }
    bool correcto = fprintf(archivo, "%s\n%u\n", FORMATO,
                             (unsigned)mejor_record) > 0;
    if (fclose(archivo) != 0) correcto = false;
    if (!correcto) {
        remove(RUTA_TEMP);
        return;
    }
#ifdef _WIN32
    if (!MoveFileExA(RUTA_TEMP, RUTA_RECORD,
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        fprintf(stderr, "[records] No se pudo reemplazar %s\n", RUTA_RECORD);
    }
#else
    if (rename(RUTA_TEMP, RUTA_RECORD) != 0)
        fprintf(stderr, "[records] No se pudo reemplazar %s\n", RUTA_RECORD);
#endif
}

/* Escribe solo los bytes BCD del record, no toca puntos, vidas ni fase. */
static void restaurar_record(uint32_t puntuacion) {
    uint8_t bcd[3];
    for (int i = 2; i >= 0; --i) {
        uint32_t par = puntuacion % 100u;
        bcd[i] = (uint8_t)(((par / 10u) << 4) | (par % 10u));
        puntuacion /= 100u;
    }
    for (unsigned i = 0; i < 3; ++i) {
        if (!cyc_mod_poke((uint16_t)(0x0091u + i), bcd[i])) {
            fprintf(stderr, "[records] No se pudo escribir el record en RAM\n");
            return;
        }
    }
}

/*
 * La pantalla del titulo puede dibujar el TOP antes de los 120 primeros
 * fotogramas. Por eso NO esperamos para restaurar el record guardado.
 * Esperamos 120 fotogramas unicamente antes de aceptar puntuaciones nuevas,
 * para ignorar basura de RAM durante el arranque.
 */
static void restaurar_si_falta(void) {
    if (!inicializado || mejor_record == 0u) return;
    uint32_t top = 0;
    if (!leer_puntuacion(0x0091u, &top) || top < mejor_record) {
        restaurar_record(mejor_record);
    }
}

static void al_encender(void *ctx) {
    (void)ctx;
    fotogramas = 0;
    mejor_record = 0;
    cargar_record();
    inicializado = true;
    /* Recuperamos el TOP antes del primer fotograma del juego. */
    restaurar_si_falta();
}

static void al_empezar_fotograma(void *ctx) {
    (void)ctx;
    restaurar_si_falta();
}

static void al_terminar_fotograma(void *ctx) {
    (void)ctx;
    ++fotogramas;

    /* El juego pudo haber limpiado el TOP durante su inicio. */
    restaurar_si_falta();

    /* No confundir el contenido inicial de RAM con un record legitimo. */
    if (fotogramas < 120u) return;

    uint32_t top = 0, uno = 0, dos = 0;
    bool top_valido = leer_puntuacion(0x0091u, &top);
    bool uno_valido = leer_puntuacion(0x0095u, &uno);
    bool dos_valido = leer_puntuacion(0x0099u, &dos);
    if (!top_valido) return;

    uint32_t maximo = top;
    if (uno_valido && uno > maximo) maximo = uno;
    if (dos_valido && dos > maximo) maximo = dos;
    if (maximo > mejor_record) {
        mejor_record = maximo;
        guardar_record();
    }
    restaurar_si_falta();
}

/* Creditos personalizados del port. */

static const RecompRuntimeUiItem opciones_creditos[] = {
    {
        .key = "arcade.creditos",
        .section = "Créditos",
        .label = "Port desarrollado por JojitaGamerHD",
        .description = "Ver los creditos completos del proyecto.",
        .type = RECOMP_RUNTIME_UI_ACTION
    }
};

static int mostrar_creditos(
    void *context,
    const RecompRuntimeUiItem *item
) {
    (void)context;
    (void)item;

#ifdef _WIN32
    MessageBoxA(
        NULL,
        "ARCADE BROS 1983 RECOMP\n\n"
        "Port desarrollado por JojitaGamerHD\n"
        "https://github.com/JojitaGamerHD\n\n"
        "Juego original: Mario Bros. (1983) - Nintendo\n\n"
        "NESRecomp: Matthew Stanley (mstan)\n"
        "https://github.com/mstan/nesrecomp\n\n"
        "recomp-ui: RetroPortingToolKit\n"
        "https://github.com/RetroPortingToolKit/recomp-ui",
        "Creditos - Arcade Bros 1983 Recomp",
        MB_OK | MB_ICONINFORMATION
    );
#else
    fprintf(stderr, "Port desarrollado por JojitaGamerHD\n");
#endif

    return 1;
}

static const RecompRuntimeUiCallbacks callbacks_creditos = {
    .run_action = mostrar_creditos
};
static const CycHostExtras extras = {
    .power_on = al_encender,
    .frame_begin = al_empezar_fotograma,
    .frame_end = al_terminar_fotograma,

    .menu_items = opciones_creditos,
    .menu_item_count =
        sizeof(opciones_creditos) / sizeof(opciones_creditos[0]),
    .menu_callbacks = &callbacks_creditos,
};

const CycHostExtras *cyc_host_extras(void) {
    return &extras;
}
