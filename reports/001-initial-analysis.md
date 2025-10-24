# SceneFX to Scroll Migration Analysis - Initial Report
## wlroots 0.19 (Sway 1.11) → wlroots 0.20dev Migration

**Analysis Date**: 2025-10-24
**Scroll Repository**: github.com/dawsers/scroll
**Baseline Commit**: d038e365 (build: prepare version 1.11)
**Target Commit**: e00ca868 (xwayland: Avoid configure is view is being destroyed)
**Commits Analyzed**: 227 commits (0/227 analyzed so far)
**Analysis Start Time**: 2025-10-24

---

## Executive Summary

### Overview

This report documents the analysis of Scroll window manager's evolution from Sway 1.11 (based on wlroots 0.19) to its current state (based on wlroots 0.20dev). The goal is to understand what changes would affect integrating SceneFX (from SwayFX) into Scroll to create ScrollFX.

Scroll is a fork of Sway that implements a PaperWM-like scrolling layout with additional features like animations, content scaling, workspace scaling, and Lua scripting. The integration must preserve all these Scroll-specific features while adding SceneFX's cosmetic enhancements (blur, shadows, rounded corners, dimming).

### Key Challenge

Scroll uses `sway_scene_*` APIs (extracted and modified from wlroots) instead of `wlr_scene_*`. SceneFX (from SwayFX) adds custom scene node types and effects that directly modify the wlroots scene graph. We need to understand:

1. What scene graph API changes occurred in wlroots 0.19 → 0.20dev
2. How Scroll's `sway_scene_*` fork evolved during this period
3. Which changes affect SceneFX's integration points

### Initial Metrics

- **Total commits to analyze**: 227
- **Commit range**: d038e365..e00ca868
- **Time period**: ~1 year (Sway 1.11 release to present)
- **Status**: Initial scan in progress

### Quick Scan Observations

From an initial review of the first 50 commits, I can already see several critical areas:

1. **Scene updates to track wlroots changes**: Multiple commits like "scene: update to latest wlroots changes [SHA]"
2. **Rendering improvements**: Buffer rendering optimizations
3. **Scale handling**: New fractional scale features
4. **XDG activation changes**: Protocol updates
5. **Animation system**: Scroll-specific feature that needs preservation

---

## Analysis Strategy

### Phase 1: Commit Categorization (In Progress)

I will categorize all 227 commits into:

1. **Scene Graph commits**: Direct modifications to scene graph code
2. **Renderer commits**: Changes to rendering system
3. **Scroll-specific features**: PaperWM layout, animations, scaling
4. **Upstream merges**: Sway/wlroots updates
5. **Bug fixes**: Non-breaking fixes
6. **API changes**: Breaking changes to APIs

### Phase 2: SceneFX Impact Assessment

For each relevant commit, assess:
- Does it touch SceneFX integration points?
- Does it change APIs that SceneFX uses?
- Does it conflict with SceneFX's custom node types?
- Does it affect rendering pipeline?

### Phase 3: Risk Assessment

Identify:
- Critical blockers (must fix before compilation)
- Runtime issues (compiles but won't work)
- Integration conflicts (SceneFX vs Scroll features)

### Phase 4: Implementation Roadmap

Create detailed migration plan with:
- File-by-file changes needed
- API translation guide
- Testing strategy
- Effort estimates

---

## Next Steps

1. Analyze commits in batches of 25-50
2. Create detailed reports for significant changes
3. Build comprehensive API migration guide
4. Generate final roadmap and recommendation

---

**Status**: Analysis beginning
**Next Report**: Scene graph commits analysis (commits 1-50)
