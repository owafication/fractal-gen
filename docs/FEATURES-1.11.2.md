# Mandelbrot Live Wallpaper 1.11.2

## GPU-tiled huge still rendering

The default OpenGL high-resolution exporter no longer creates one GPU surface for the entire requested image. It now:

- queries the GPU texture and viewport limit;
- renders bounded tiles up to 2048 pixels per side, reduced automatically for smaller device limits;
- uses one-pixel overlap around internal tile edges so the glow post-process has neighbouring pixels and does not introduce tile seams;
- assembles only a bounded output band, then streams completed rows directly into the Windows image encoder;
- retains only the bounded preview, current band and current GPU tile rather than a full-resolution frame in memory.

This allows output dimensions far beyond a single OpenGL texture, subject to Windows encoder, address-space, memory, disk-space and file-format limits.

## Resolution-aware detail depth

High-resolution still rendering now resolves an automatic quality budget from the camera scale and output pixel height. The selected preset iteration count remains the minimum. When the requested pixel span resolves detail beyond a normal 1080p full-view reference, the renderer raises the iteration floor by 32 iterations per additional detail stop, capped at the existing 4096 safety limit.

The same resolved quality budget is used by both GPU and CPU still exporters. The completion status reports the actual iteration count, anti-aliasing level and tile dimensions used.

## Deterministic tile phase

Animated equation coefficients are frozen to one captured render time for the complete still. Every GPU tile therefore uses the same coefficient phase instead of changing as tiles are processed.
