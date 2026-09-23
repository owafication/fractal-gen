# Pack Manifest

**Status:** Complete response-generated pack  
**Authority:** Supporting proposal only; canonical repository owners prevail  
**Owner:** Deep-zoom planning and integration  
**Update trigger:** Source-baseline change, accepted decision, ID conflict or phase-scope change

## Purpose

Register the pack, its ownership boundaries and the canonical repository files that must be updated if the proposal is accepted.

## File register

| File | Purpose | Read trigger | Canonical ownership affected |
|---|---|---|---|
| `README.md` | Baseline, claim boundary, frozen ranges and reading sequence | First read | `PROJECT_INDEX.md`, `AGENTS.md` |
| `SOURCE_ASSESSMENT_AND_DIRECTION.md` | Inspected source capability, defects, conflicts and product direction | Planning or audit | Foundation, architecture, validation, maintenance |
| `DEEP_ZOOM_REQUIREMENTS_AND_SCOPE.md` | Scope, requirements, formula support and user outcomes | Product or acceptance change | `PROJECT_FOUNDATION.md`, `TRACEABILITY.md` |
| `DEEP_ZOOM_ARCHITECTURE_AND_CONTRACTS.md` | Exact camera, planner, orbit, perturbation, export and resource contracts | Code or schema work | Architecture and rendering owners |
| `DEEP_ZOOM_BUILD_PLAN.md` | PH-12–PH-15 implementation sequence | Build execution | `IMPLEMENTATION_PLAN.md` |
| `DEEP_ZOOM_VALIDATION_AND_TRACEABILITY.md` | AC/VAL catalogue, fixtures and mappings | Test or completion claim | `TRACEABILITY.md`, `VALIDATION_AND_EVIDENCE.md` |
| `DEEP_ZOOM_RISKS_DECISIONS_AND_MIGRATION.md` | Decisions, risks, compatibility and rollback | Cross-cutting change | Decision, risk and persistence owners |
| `CANONICAL_INTEGRATION_AND_HANDOFF.md` | Exact canonical update map and agent handoff | Repository integration | All affected owners |
| `AUDIT_REPORT.md` | Pack audit, evidence boundary and unproven items | Delivery or compatibility review | `DELIVERY_REPORT.md` |

## Canonical integration targets

Update only after acceptance and direct repository inspection:

```text
project_docs/PROJECT_INDEX.md
project_docs/PROJECT_FOUNDATION.md
project_docs/IMPLEMENTATION_PLAN.md
project_docs/TRACEABILITY.md
project_docs/VALIDATION_AND_EVIDENCE.md
project_docs/SECURITY_PRIVACY_AND_RISK.md
project_docs/DECISIONS_AND_CHANGE_HISTORY.md
project_docs/DATA_AND_PERSISTENCE.md
project_docs/DEBUGGING_AND_MAINTENANCE.md
project_docs/UI_WORKFLOWS_AND_ROUTES.md
project_docs/PROJECT_SETTINGS.md
docs/architecture/PROJECT-STATE-AND-PARAMETERS.md
docs/architecture/RENDERING-AND-EXPORT-CONTRACTS.md
docs/testing/VISUAL-REGRESSION.md
docs/features/ANIMATION-TRACKS-PLAN.md
docs/features/OFFLINE-EXPORT-PLAN.md
docs/features/FRACTAL-SCOUT-STATUS.md
docs/roadmaps/INTEGRATED-CREATIVE-ROADMAP.md
scripts/verify-source.py
```

## Ownership rule

This pack may define proposed IDs and contracts, but it must not become a competing canonical owner. After integration:

- requirement definitions belong in `PROJECT_FOUNDATION.md`;
- phase definitions and status belong in `IMPLEMENTATION_PLAN.md` and `TRACEABILITY.md`;
- rendering/data/UI contracts belong in their current canonical owners;
- decision and risk status belongs in the existing registers;
- this pack remains a supporting design record linked from `PROJECT_INDEX.md`.

## Status by file

| File | Status |
|---|---|
| `README.md` | Complete |
| `PACK_MANIFEST.md` | Complete |
| `SOURCE_ASSESSMENT_AND_DIRECTION.md` | Complete |
| `DEEP_ZOOM_REQUIREMENTS_AND_SCOPE.md` | Complete |
| `DEEP_ZOOM_ARCHITECTURE_AND_CONTRACTS.md` | Complete |
| `DEEP_ZOOM_BUILD_PLAN.md` | Complete |
| `DEEP_ZOOM_VALIDATION_AND_TRACEABILITY.md` | Complete |
| `DEEP_ZOOM_RISKS_DECISIONS_AND_MIGRATION.md` | Complete |
| `CANONICAL_INTEGRATION_AND_HANDOFF.md` | Complete |
| `AUDIT_REPORT.md` | Complete |
