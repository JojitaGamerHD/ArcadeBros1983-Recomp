# Componentes y avisos del paquete Windows

Los textos originales de licencia y copyright de esta carpeta se conservan
con sus componentes. Los encabezados internos de Clang tambien conservan sus
avisos originales dentro de `Bin/compiler/lib/clang/23/include`.

| Componente | Procedencia / revision | Archivos de aviso |
| --- | --- | --- |
| NESRecomp, motor y recompilador | https://github.com/mstan/nesrecomp/tree/5a2eddeecc646e9325bef53f3c711e119423f99c | NESRecomp-LICENSE.txt; PolyForm Noncommercial 1.0.0 |
| TriCNES, base del motor de ciclo | Copia incluida por NESRecomp | TriCNES-LICENSE.txt |
| emu2413 | Copia incluida por NESRecomp, runner/cyc/vendor/emu2413 | emu2413-MIT.txt |
| recomp-ui | https://github.com/RetroPortingToolKit/recomp-ui/tree/7c635c6b366a6e0d5d3c913a7fce6ec04dd9f256 | recomp-ui-MIT.txt |
| Dear ImGui y backends | Copia incluida por recomp-ui | Dear-ImGui-MIT.txt |
| stb_image, stb_image_write, stb_truetype | Copias incluidas por recomp-ui | stb-LICENSE.txt |
| tinyfiledialogs | Copia 3.21.3 incluida por recomp-ui | tinyfiledialogs-zlib.txt |
| imgl3w y declaraciones Khronos | Backend OpenGL de Dear ImGui | imgl3w-Public-Domain.txt; Khronos-OpenGL-NOTICE.txt; Khronos-platform-MIT.txt |
| SDL2 | SDK Windows 2.30.0 incluido por NESRecomp | SDL2-zlib.txt; SDL2-hidapi-BSD.txt |
| LLVM/Clang/LLD | https://github.com/llvm/llvm-project/releases/tag/llvmorg-23.1.4 | LLVM-LICENSE.txt; LLVM-Support-NOTICE.txt |
| xxHash, expresiones regulares, MD5, BLAKE3 | Componentes de LLVM Support en llvmorg-23.1.4 | LLVM-xxHash-BSD.txt; LLVM-regex-BSD.txt; LLVM-MD5-NOTICE.txt; LLVM-BLAKE3-LICENSE.txt |
| zlib 1.3.2, Zstandard 1.5.7, libxml2 2.9.12 | Dependencias estaticas declaradas por el script oficial Windows de LLVM 23.1.4 | zlib-1.3.2-LICENSE.txt; zstd-1.5.7-BSD.txt; libxml2-2.9.12-Copyright.txt; libxml2-*-NOTICE.txt |
| Pixelify Sans | Fuente del proyecto Arcade Bros | PixelifySans-OFL.txt |
| LatoLatin Regular y Bold | Subconjunto Latin de Lato 2.015 distribuido por recomp-ui | LatoLatin-OFL.txt; recomp-ui-FONTS-NOTICE.md |
| Noto Sans Symbols 2 | Fuente distribuida por recomp-ui | NotoSansSymbols2-OFL.txt; recomp-ui-FONTS-NOTICE.md |
| OpenMoji Black | Fuente distribuida por recomp-ui; OpenMoji, HfG Schwabisch Gmund | OpenMoji-CC-BY-SA-4.0.txt; recomp-ui-FONTS-NOTICE.md |
| Banderas flags.png | Renderizadas desde Noto Color Emoji, segun el aviso de recomp-ui | NotoColorEmoji-Flags-OFL.txt; recomp-ui-IMAGES-NOTICE.md |

`FONT-METADATA-NOTICE.txt` conserva los avisos de copyright y licencia leidos
directamente de los cinco archivos TTF de este paquete, sin modificarlos.
Los demas recursos de interfaz de recomp-ui se conservan junto con su licencia
y avisos. La portada `arcade_bros_1983.png` pertenece a los recursos existentes
del proyecto Arcade Bros 1983 Recomp.

NESRecomp y recomp-ui incluyen adaptaciones de Arcade Bros. Las fuentes y los
parches acumulativos del proyecto se publican en:
https://github.com/JojitaGamerHD/ArcadeBros1983-Recomp

## Procedencia de los textos adicionales

- Lato: https://github.com/hakger/latolatin-webfont/blob/4a7162e18a6a01f2ce87dec05c4e4605d8167eb6/OFL.txt
- Noto Symbols: https://github.com/notofonts/symbols/blob/main/OFL.txt
- Noto Emoji: https://github.com/googlefonts/noto-emoji/blob/main/LICENSE
- OpenMoji: https://github.com/hfg-gmuend/openmoji/blob/master/LICENSE.txt
- SDL hidapi: https://github.com/libsdl-org/SDL/blob/release-2.30.0/src/hidapi/LICENSE-bsd.txt
- Khronos platform: https://github.com/KhronosGroup/EGL-Registry/blob/main/api/KHR/khrplatform.h
- LLVM build Windows: https://github.com/llvm/llvm-project/blob/llvmorg-23.1.4/llvm/utils/release/build_llvm_release.bat
- LLVM xxHash: https://github.com/llvm/llvm-project/blob/llvmorg-23.1.4/llvm/lib/Support/xxhash.cpp
- LLVM regex: https://github.com/llvm/llvm-project/blob/llvmorg-23.1.4/llvm/lib/Support/regex_impl.h
- LLVM MD5: https://github.com/llvm/llvm-project/blob/llvmorg-23.1.4/llvm/lib/Support/MD5.cpp
- LLVM BLAKE3: https://github.com/llvm/llvm-project/blob/llvmorg-23.1.4/llvm/lib/Support/BLAKE3/LICENSE
- zlib: https://github.com/madler/zlib/blob/v1.3.2/LICENSE
- Zstandard: https://github.com/meta/zstd/blob/v1.5.7/LICENSE
- libxml2: https://github.com/GNOME/libxml2/blob/v2.9.12/Copyright
- libxml2 hash/list/trio: avisos iniciales de hash.c, list.c y trio.c del mismo tag.

La ROM, las fuentes generadas a partir de ella, la DLL local del juego y la
musica aportada por el usuario se guardan en Datos y se excluyen del paquete
publico. Esta carpeta de avisos solo documenta los componentes genericos.
