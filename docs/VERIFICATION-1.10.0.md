# Verification — 1.10.0

## Automated checks available in the packaging environment

- Source structure and offline-policy verification.
- Static regression checks requiring responsive layout integration in every custom dialog.
- Static checks for per-monitor DPI handling, scroll support, keyboard dialog processing, remembered placement, scaled owner-drawn rows, and equation tooltips.
- GCC C++20 core build with warnings treated as errors.
- Clang C++20 core build with warnings treated as errors.
- Core test suite under GCC and Clang.
- AddressSanitizer and UndefinedBehaviorSanitizer core build and tests.
- Source-only archive audit and ZIP integrity test.

## Windows verification still required

The packaging environment cannot compile or interact with Win32 controls. A Windows build should verify:

1. Open every dialog at 100%, 125%, 150%, 200%, and mixed-monitor DPI.
2. Resize each dialog to its minimum and maximum useful size.
3. Confirm both scroll bars expose clipped controls on a small display.
4. Tab through every interactive control and confirm focused controls scroll into view.
5. Confirm Enter activates focused/default buttons and Escape cancels or hides the Quick Controller.
6. Enable Windows High Contrast and confirm labels, list selections, focus rectangles, and buttons remain readable.
7. Run Narrator or another MSAA/UIA client and confirm list names, labels, values, and button text are announced.
8. Move and resize dialogs, restart the app, and confirm their placements restore within the active monitor work area.
9. Move an open dialog between monitors with different DPI and confirm fonts and controls rescale without clipping.
