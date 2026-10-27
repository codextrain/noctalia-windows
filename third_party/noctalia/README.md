# Noctalia theme code (MIT)

Copied unchanged from https://github.com/noctalia-dev/noctalia-shell:

- `theme/` — palette generation from an image (Material Design 3 schemes through
  material_color_utilities, plus Noctalia's custom vibrant / faithful / soft / muted schemes)
  and the expansion of fixed palettes into the full color-token set.
- `render/core/color.*` — the `Color` type those files use.

`ui/palette.h` is a small stand-in: it keeps only the `Palette` struct from Noctalia's
`src/ui/palette.h`, which pulls in the whole UI toolkit otherwise.

## Local change to material_color_utilities

`(Vec3){...}` / `(ViewingConditions){...}` compound literals were rewritten as `Vec3{...}` /
`ViewingConditions{...}` (same meaning; MSVC rejects the C-style form).
