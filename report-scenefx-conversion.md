# SceneFX Conversion Report

## Status: BLOCKED

### Issue Summary
Cannot proceed with SceneFX to sway_scene API conversion due to missing source code.

### Current State

The following reference directories are **empty**:
- `/references/scenefx/` - Should contain SceneFX source code
- `/references/scroll/` - Should contain Scroll window manager with sway_scene implementation
- `/references/swayfx/` - Should contain SwayFX (Sway with SceneFX integration)

Only `/references/sway/` contains content (the original Sway window manager).

### Investigation Findings

The investigation files in `/investigation/` directory contain detailed documentation about SceneFX integration patterns:

1. **investigation-include.md** - Shows SceneFX header includes and API usage:
   - `#include <scenefx/types/fx/blur_data.h>`
   - References to `wlr_scene_shadow`, `wlr_scene_optimized_blur`
   - SceneFX provides blur, shadow, corner radius, and dimming effects

2. **investigation-sway-*.md** files - Document how SwayFX integrates SceneFX throughout the codebase

### Required Actions

To proceed with this task, the following repositories need to be cloned or provided:

1. **SceneFX** - The effects library that needs to be converted
2. **Scroll** - The window manager containing the sway_scene implementation
3. **SwayFX** (optional but helpful) - To see complete SceneFX integration

### Recommendations

1. Check if these are meant to be git submodules that need initialization
2. Clone the required repositories into the references directory
3. Verify that the Scroll repository contains the sway_scene implementation in `sway/tree/scene/`

### Next Steps

Once the source code is available, the conversion process would involve:
1. Analyzing Scroll's sway_scene API structure
2. Creating a comprehensive API mapping
3. Converting all wlr_scene references to sway_scene equivalents
4. Testing the converted code for compatibility

## Conclusion

The task cannot be completed without access to the SceneFX and Scroll source code. The empty reference directories need to be populated before any conversion work can begin.