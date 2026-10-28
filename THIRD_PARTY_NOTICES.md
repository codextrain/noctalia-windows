# Third-party notices

## Noctalia (MIT)
https://github.com/noctalia-dev/noctalia-shell — Copyright (c) 2026 noctalia-dev

Adapted from Noctalia:
- `assets/palettes.json` — the 10 built-in palettes (from `src/theme/builtin_palettes.cpp`)
- `src/core/Theme.cpp` — palette expansion rules (from `src/theme/fixed_palette.cpp`)
- `qml/Commons/Style.qml` — design constants (from `src/ui/style.h`)
- `assets/glyphs.json` — icon alias table (from `src/render/text/glyph_registry.cpp`)
- `assets/fonts/noctalia-tabler.ttf` — Noctalia's build of the Tabler icon font
- `assets/noctalia.svg` — Noctalia logo

## Tabler Icons (MIT)
https://tabler.io/icons — Copyright (c) 2020-2025 Paweł Kuna.
See `assets/fonts/tabler-icons-license.txt`.

## Qt (LGPLv3)
https://www.qt.io — linked dynamically. When you distribute the app, ship the Qt DLLs as
separate files (windeployqt does this) so users can replace them.

## Noctalia theme generator (MIT) — `third_party/noctalia/`
Noctalia's palette generation and fixed-palette expansion, copied unchanged (see the README
in that folder). Copyright (c) 2026 noctalia-dev.

## Material Color Utilities (Apache 2.0) — `third_party/material_color_utilities/`
https://github.com/material-foundation/material-color-utilities — Copyright Google LLC.
See `third_party/material_color_utilities/LICENSE`.

## Vosk / modelo português (Apache 2.0)
https://github.com/alphacep/vosk-api e https://alphacephei.com/vosk/models — usados pelo helper opcional de comandos por voz locais. Preservar as licenças distribuídas com a biblioteca e o modelo.

## python-sounddevice (MIT)
https://github.com/spatialaudio/python-sounddevice — entrada de áudio do helper opcional. PortAudio utiliza sua própria licença MIT; preservar as licenças incluídas nas dependências empacotadas.
