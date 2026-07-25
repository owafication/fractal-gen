# Verification — 1.11.6

## Scope

Expanded the Quick Controller so it exposes the remaining hover-menu actions without duplicating controls already present there.

## Completed checks

- Packaged-source verifier updated for 1.11.6 metadata and Quick Controller command wiring.
- Quick Controller source inspection confirms buttons for: Apply Settings Live, Static Desktop, Slideshow Desktop, Stop Desktop, Jump to Coordinates, Preview/Desktop zoom toggles, Preview/Desktop colour toggles, and Render Hi-Res.
- App command routing updated so the new Quick Controller buttons call the same handlers as the hover menu.
- Desktop zoom and desktop colour toggle handlers now refresh the Quick Controller state immediately.

## Build verification

Run and pass the existing platform-independent checks:

- `python3 scripts/verify-source.py`
- `cmake -S . -B build-core -DCMAKE_BUILD_TYPE=Release`
- `cmake --build build-core -- -j2`
- `ctest --test-dir build-core --output-on-failure`
- `cmake -S . -B build-core-clang -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++`
- `cmake --build build-core-clang -- -j2`
- `ctest --test-dir build-core-clang --output-on-failure`

## Limits

This environment does not provide the Windows SDK or a Win32 desktop session, so native MSVC compilation and interactive Quick Controller UI testing remain unverified here.
