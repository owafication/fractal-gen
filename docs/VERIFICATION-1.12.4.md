# Verification — 1.12.4

## Scope

Completed Phase 5 of `docs/FRACTAL-STYLE-BUILD-PLAN.md` by adding the bounded, deterministic and cancellable Fractal Scout core and Win32 dialog integration.

## Automated coverage

Core tests cover:

- bounded request values preserved by the limit resolver;
- hard ceilings for excessive candidate, thumbnail and iteration requests;
- complete candidate-pool evaluation;
- descending score ordering;
- normalised score range;
- complete bounded thumbnail buffers;
- unsaved candidate identity;
- deterministic cameras, scores and thumbnail pixels across repeated searches;
- cancellation without partial candidate exposure.

Source verification checks:

- core Scout model and scoring implementation;
- cancellable worker-thread UI;
- Fast, Balanced and Detailed budgets;
- refinement, temporary preview application and explicit Save as New handoff;
- AppWindow command and layout wiring;
- Phase 5 completion status and 1.12.4 metadata.

## Runtime limits

Native MSVC compilation and interactive Win32 testing are unavailable in this environment. The dialog source is statically inspected, while mathematical, scoring, thumbnail and cancellation behaviour is covered by the portable core tests.
