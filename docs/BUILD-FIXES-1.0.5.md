# Colour cycling control 1.0.5

## Implemented

- Added a dedicated **Pause Colours / Play Colours** button beside the colour-speed control.
- The switch pauses only palette offset animation; camera zoom and journey animation continue.
- The current colour offset is retained while paused and resumes without a jump.
- The state applies to preview, mirror, span, and independent monitor animation controllers.
- The state is stored in the versioned local settings JSON. Older settings files default to colour cycling enabled.
- Added core tests for persistence and pause/resume behaviour.
