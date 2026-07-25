# Mandelbrot Live Wallpaper 1.11.0

## Expanded equation library

Version 1.11.0 expands the bounded equation editor from a small example list to 45 built-in equation presets. The library includes Mandelbrot and Multibrot powers, Burning Ship and Tricorn variants, Julia constants, Newton basins, rational maps, orbit traps, distance-estimated colouring, coefficient animation, transcendental transforms and complex coefficient variants.

The recurrence model now supports an independent integer exponent for the parameter term:

```text
A · T(z)^p + B · T(z) + C · c^r + D + E · iteration + λ / T(z)^q
```

`p`, `r`, and `q` are bounded to 1–12 where applicable. The `c` exponent is serialized in settings and preset JSON, shown in the equation summary, evaluated by the CPU renderer, and uploaded to both OpenGL shader paths. Powered-`c` maps are excluded from perturbation precision modes that assume a linear parameter term.

## Supplied reference equations

The eight formulas from the supplied comparison artwork are represented as built-in equations and complete scene presets:

1. `z² + c`
2. `z² + 1.2c`
3. `z² + c + 0.5`
4. `1.2z² + c`
5. `z² + 0.5z + c`
6. `z + c²`
7. component-absolute `z`, squared, plus `c`
8. `z² - c`

The powered-parameter addition makes `z + c²` an exact recurrence rather than an approximation using a linear `c` coefficient.

## Expanded colour library

The palette editor now contains 30 reusable built-in colour presets. Eight palette families are matched to the supplied reference artwork:

- Electric Blue and Gold
- Cyan Aurora
- Magenta Nebula
- Golden Halo
- Deep Cyan
- Crimson Web
- Ice Lightning
- Toxic Green

Additional palettes cover fire, ocean, amethyst, neon, matrix green, copper, glacier, monochrome, pastel, high-contrast, teal/coral, vaporwave, solar, emerald, blackbody, arctic, forest, retro-rainbow and peacock themes.

Built-in palettes are read-only. Loading one applies its colour stops to the current preview; saving creates or updates only a user palette entry. Existing user palette JSON remains separate from the built-in library.

## Complete scene presets

The main preset library now contains 36 built-in scenes. Eight combine the supplied equations with their corresponding reference palettes. The remaining scenes combine equation and palette presets for Multibrot, Julia, Newton, rational, orbit-trap, distance and animated-coefficient examples.

## Preset save reliability

`Save Preset As` now:

- prompts for a name from every entry point;
- validates the new preset before persistence;
- reports persistence errors to the user;
- rolls the in-memory addition back if the atomic settings save fails;
- confirms success after the preset is stored and selected;
- keeps built-in presets read-only and creates editable custom copies.

The custom scene library retains its existing 256-entry safety bound.

## Hover menu

The preview hover menu retains the requested actions:

- Start/Stop Zoom
- Start/Stop Colours
- Set Live
- Static Desktop
- Slideshow Desktop
- Stop Desktop
- Jump to Coordinates
- Quick Controls
- Save Preset As
- Copy Coordinates
- Save Image
- Render Hi-Res
