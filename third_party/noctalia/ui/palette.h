#pragma once
// Minimal stand-in for noctalia/src/ui/palette.h: only the Palette struct the theme code needs.
#include "render/core/color.h"

// Runtime UI palette: one color per ColorRole, resolved from the generated theme tokens.
struct Palette {
  Color primary;
  Color onPrimary;
  Color primaryContainer;
  Color onPrimaryContainer;
  Color secondary;
  Color onSecondary;
  Color secondaryContainer;
  Color onSecondaryContainer;
  Color tertiary;
  Color onTertiary;
  Color tertiaryContainer;
  Color onTertiaryContainer;
  Color error;
  Color onError;
  Color errorContainer;
  Color onErrorContainer;
  Color surface;
  Color onSurface;
  Color onSurfaceVariant;
  Color surfaceContainerLow;
  Color surfaceContainer;
  Color surfaceContainerHigh;
  Color surfaceContainerHighest;
  Color outlineVariant;
  Color shadow;
  Color scrim;
  Color hover;
  Color onHover;

  bool operator==(const Palette&) const = default;
};

