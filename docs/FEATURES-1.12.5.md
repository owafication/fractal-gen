# Mandelbrot Live Wallpaper 1.12.5

## Targeted multi-scale Fractal Scout

Fractal Scout can now rank candidate locations for four different visual goals:

- **Balanced detail** — the original general-purpose mix of boundary, variance, edges, detail and light symmetry weighting.
- **Boundary structures** — prioritises mixed interior/exterior regions and strong mathematical boundaries.
- **Fine filaments** — prioritises edge density, iteration variation and escaped-orbit detail.
- **Symmetry and rings** — gives substantial weight to horizontal or rotational similarity while retaining boundary content.

The candidate pool now spans up to three logarithmically spaced depth bands around each deterministic golden-angle position. This lets one search compare nearby wide, central and deeper views instead of forcing every result to the same scale.

After scoring, Scout greedily suppresses candidates that are too close in combined position-and-scale space. If the initial threshold would return too few results, it is relaxed deterministically until the bounded result count can be filled. The dialog reports the number of near-duplicate candidates suppressed during the initial selection pass.

Search remains cancellable and retains all existing candidate, sample-grid, iteration, thumbnail and memory ceilings. Results remain temporary until explicitly applied or saved as a new preset.
