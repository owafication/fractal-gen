# Mandelbrot Live Wallpaper 1.13.1

## Windows build correction

The Journey Settings dialog now validates waypoint text through a public, read-only `AnimationController::HasValidJourneyScriptTargets` contract. The underlying parser and its private `JourneyPoint` representation remain private to the animation controller.

This resolves the MSVC C2248 build failure caused by the dialog calling the private `ParseJourneyScript` function directly. No journey syntax or runtime route behaviour changed.
