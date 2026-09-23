# Mandelbrot Live Wallpaper 1.12.0

## Tricorn and Multicorn templates

The equation editor now presents clearer family templates for:

- Tricorn / Mandelbar, power 2
- Multicorn, power 3
- Multicorn, power 4

They use the existing configurable equation contract rather than a separate renderer, so presets remain editable through the normal equation controls.

## Expanded colour mapping

The Palette Editor now includes:

- Palette frequency from 0.05 to 256
- Palette gamma from 0.05 to 8
- Linear or smooth phase mapping
- Optional stripe-average orbit texture
- Stripe density, phase, strength, and start iteration

New values are persisted in presets and applied by CPU still rendering, OpenGL, and Direct3D 11.

## New built-in style

Added:

- `Cyan Fire Ring` palette
- `Tricorn Cyan Fire Ring` scene preset

The scene combines a power-2 Tricorn equation, high palette repetition, smooth phase mapping, stripe texture, black interior, increased iteration depth, glow, and 4× anti-aliasing as a starting point for cyan/blue scenes with narrow white/yellow/orange boundaries.

## Compatibility

Existing presets keep their previous behaviour through defaults:

- Frequency: 8
- Gamma: 1
- Mapping: Linear
- Stripe-average texture: Disabled
