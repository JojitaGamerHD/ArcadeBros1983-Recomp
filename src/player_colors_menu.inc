/* Three NES pixel indices per character, independent RGB values. */
{.key="arcade.color.enabled",.section="Colores",.label="Personalizar personajes",.description="Usa los colores elegidos para Mario y Luigi en partida.",.type=RECOMP_RUNTIME_UI_BOOL,.minimum=0,.maximum=1,.step=1},
{.key="arcade.color.mario",.section="Colores",.label="Mario",.description="Abre el personaje y elige sus tres tonos.",.type=RECOMP_RUNTIME_UI_PALETTE},
{.key="arcade.color.luigi",.section="Colores",.label="Luigi",.description="Abre el personaje y elige sus tres tonos.",.type=RECOMP_RUNTIME_UI_PALETTE},
{.key="arcade.color.slot",.section="Colores",.label="Preset (1-8)",.description="Selecciona una ranura antes de guardar o cargar.",.type=RECOMP_RUNTIME_UI_INT,.minimum=1,.maximum=8,.step=1},
{.key="arcade.color.store",.section="Colores",.label="Guardar preset",.description="Reemplaza los seis colores de la ranura seleccionada.",.type=RECOMP_RUNTIME_UI_ACTION},
{.key="arcade.color.load",.section="Colores",.label="Cargar preset",.description="Carga los seis colores y activa la personalizacion. Una ranura vacia se rechaza.",.type=RECOMP_RUNTIME_UI_ACTION},
{.key="arcade.color.reset",.section="Colores",.label="Restaurar colores originales",.description="Desactiva la personalizacion y conserva tus presets.",.type=RECOMP_RUNTIME_UI_ACTION},
