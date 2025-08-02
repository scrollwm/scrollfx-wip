# FX Renderer Replacement Report

**Generated**: 2025-07-31 10:43:32

## Summary

- **Files Scanned**: 10
- **Files Modified**: 7
- **Total Replacements**: 19
- **Target Directory**: `scroll-standalone`

## Renderer Usage Found

Files containing wlr_renderer references:

### `include/sway/config.h`
- `#include <wlr/render/color.h>`

### `include/sway/ext_image_capture_source_v1.h`
- `struct wlr_allocator *allocator, struct wlr_renderer *renderer);`

### `include/sway/server.h`
- `struct wlr_renderer *renderer;`

### `include/sway/tree/root.h`
- `#include <wlr/render/wlr_texture.h>`

### `include/sway/tree/scene.h`
- `#include <wlr/render/wlr_renderer.h>`
- `struct wlr_render_timer *render_timer;`
- `#include <wlr/render/wlr_renderer.h>`

### `sway/commands/output/color_profile.c`
- `#include <wlr/render/color.h>`

### `sway/config/output.c`
- `#include <wlr/render/allocator.h>`

### `sway/desktop/output.c`
- `#include <wlr/render/wlr_renderer.h>`
- `#include <wlr/render/swapchain.h>`
- `#include <wlr/render/wlr_renderer.h>`

### `sway/server.c`
- `#include <wlr/render/wlr_renderer.h>`
- `struct wlr_renderer *renderer = wlr_renderer_autocreate(server->backend);`
- `wlr_renderer_destroy(renderer);`
- ... and 2 more occurrences

### `sway/tree/view.c`
- `#include <wlr/render/wlr_renderer.h>`
- `#include <wlr/render/wlr_renderer.h>`

## Files Modified

### include/sway/
- `ext_image_capture_source_v1.h` (2 replacements)
- `server.h` (2 replacements)

### include/sway/tree/
- `scene.h` (2 replacements)

### sway/
- `server.c` (8 replacements)

### sway/config/
- `output.c` (1 replacements)

### sway/desktop/
- `output.c` (2 replacements)

### sway/tree/
- `view.c` (2 replacements)

## Replacement Details

### include/sway/ext_image_capture_source_v1.h

- `Added SceneFX includes`: 1 occurrences
- `\bstruct wlr_renderer\b -> struct fx_renderer`: 1 occurrences

### include/sway/server.h

- `Added SceneFX includes`: 1 occurrences
- `\bstruct wlr_renderer\b -> struct fx_renderer`: 1 occurrences

### include/sway/tree/scene.h

- `Added SceneFX includes`: 1 occurrences
- `\bwlr_renderer\b(?!\w) -> fx_renderer`: 1 occurrences

### sway/config/output.c

- `#include\s+<wlr/render/allocator\.h> -> #include <wlr/render/allocator.h>
#in...`: 1 occurrences

### sway/desktop/output.c

- `Added SceneFX includes`: 1 occurrences
- `\bwlr_renderer\b(?!\w) -> fx_renderer`: 1 occurrences

### sway/server.c

- `#include\s+<wlr/render/allocator\.h> -> #include <wlr/render/allocator.h>
#in...`: 1 occurrences
- `\bstruct wlr_renderer\b -> struct fx_renderer`: 2 occurrences
- `\bwlr_renderer\b(?!\w) -> fx_renderer`: 1 occurrences
- `wlr_renderer_autocreate -> fx_renderer_create`: 2 occurrences
- `wlr_renderer_destroy -> fx_renderer_destroy`: 2 occurrences

### sway/tree/view.c

- `Added SceneFX includes`: 1 occurrences
- `\bwlr_renderer\b(?!\w) -> fx_renderer`: 1 occurrences

## Key Files Status

Status of critical renderer files:

| File | Status | Replacements |
|------|--------|-------------|
| `sway/desktop/output.c` | ✅ Modified | 2 |
| `sway/desktop/render.c` | ❌ Not Modified | 0 |
| `sway/desktop/transaction.c` | ❌ Not Modified | 0 |
| `sway/server.c` | ✅ Modified | 8 |
| `sway/main.c` | ❌ Not Modified | 0 |
| `sway/tree/container.c` | ❌ Not Modified | 0 |
| `sway/tree/view.c` | ✅ Modified | 2 |

## Build System Updates

- ⚠️ meson.build not updated (may need manual configuration)

## Required Manual Steps

1. **Verify SceneFX dependency**: Ensure scenefx is available in your build environment
2. **Effect framebuffers**: Check if effect framebuffers initialization is needed
3. **Renderer capabilities**: Verify FX renderer supports all required features
4. **Shader compilation**: Ensure SceneFX shaders are compiled and available
5. **Performance testing**: Test rendering performance with FX renderer

## Potential Issues

- **API differences**: FX renderer may have different function signatures
- **Feature parity**: Some wlr_renderer features might not have FX equivalents
- **Initialization order**: FX renderer might require different initialization
- **Resource management**: Different cleanup requirements for FX renderer

## Detailed Log

```
[2025-07-31 10:43:32] [INFO] === FX Renderer Replacement Started ===
[2025-07-31 10:43:32] [INFO] Target directory: /var/home/tom/scrollwm/scrollfx-wip/scroll-standalone
[2025-07-31 10:43:32] [INFO] Scanning for wlr_renderer usage...
[2025-07-31 10:43:32] [INFO] Found renderer usage in 10 files
[2025-07-31 10:43:32] [INFO] 
=== Processing Files ===
[2025-07-31 10:43:32] [INFO] Processing: sway/server.c
[2025-07-31 10:43:32] [INFO]   - Found patterns: {'renderer_creation': 2, 'render_begin_end': 0, 'texture_rendering': 0, 'render_pass': 0, 'renderer_structs': 2, 'renderer_includes': 2}
[2025-07-31 10:43:32] [INFO]   - Applying server.c specific fixes
[2025-07-31 10:43:32] [INFO]   - Modified with 8 replacements
[2025-07-31 10:43:32] [INFO] Processing: sway/config/output.c
[2025-07-31 10:43:32] [INFO]   - Found patterns: {'renderer_creation': 0, 'render_begin_end': 0, 'texture_rendering': 0, 'render_pass': 0, 'renderer_structs': 0, 'renderer_includes': 1}
[2025-07-31 10:43:32] [INFO]   - Applying output.c specific fixes
[2025-07-31 10:43:32] [INFO]   - Modified with 1 replacements
[2025-07-31 10:43:32] [INFO] Processing: sway/desktop/output.c
[2025-07-31 10:43:32] [INFO]   - Found patterns: {'renderer_creation': 0, 'render_begin_end': 0, 'texture_rendering': 0, 'render_pass': 0, 'renderer_structs': 0, 'renderer_includes': 2}
[2025-07-31 10:43:32] [INFO]   - Applying output.c specific fixes
[2025-07-31 10:43:32] [INFO]   - Modified with 2 replacements
[2025-07-31 10:43:32] [INFO] Processing: sway/tree/view.c
[2025-07-31 10:43:32] [INFO]   - Found patterns: {'renderer_creation': 0, 'render_begin_end': 0, 'texture_rendering': 0, 'render_pass': 0, 'renderer_structs': 0, 'renderer_includes': 1}
[2025-07-31 10:43:32] [INFO]   - Modified with 2 replacements
[2025-07-31 10:43:32] [INFO] Processing: sway/commands/output/color_profile.c
[2025-07-31 10:43:32] [INFO]   - Found patterns: {'renderer_creation': 0, 'render_begin_end': 0, 'texture_rendering': 0, 'render_pass': 0, 'renderer_structs': 0, 'renderer_includes': 1}
[2025-07-31 10:43:32] [INFO]   - No modifications needed
[2025-07-31 10:43:32] [INFO] Processing: include/sway/config.h
[2025-07-31 10:43:32] [INFO]   - Found patterns: {'renderer_creation': 0, 'render_begin_end': 0, 'texture_rendering': 0, 'render_pass': 0, 'renderer_structs': 0, 'renderer_includes': 1}
[2025-07-31 10:43:32] [INFO]   - No modifications needed
[2025-07-31 10:43:32] [INFO] Processing: include/sway/server.h
[2025-07-31 10:43:32] [INFO]   - Found patterns: {'renderer_creation': 0, 'render_begin_end': 0, 'texture_rendering': 0, 'render_pass': 0, 'renderer_structs': 1, 'renderer_includes': 0}
[2025-07-31 10:43:32] [INFO]   - Modified with 2 replacements
[2025-07-31 10:43:32] [INFO] Processing: include/sway/ext_image_capture_source_v1.h
[2025-07-31 10:43:32] [INFO]   - Found patterns: {'renderer_creation': 0, 'render_begin_end': 0, 'texture_rendering': 0, 'render_pass': 0, 'renderer_structs': 1, 'renderer_includes': 0}
[2025-07-31 10:43:32] [INFO]   - Modified with 2 replacements
[2025-07-31 10:43:32] [INFO] Processing: include/sway/tree/scene.h
[2025-07-31 10:43:32] [INFO]   - Found patterns: {'renderer_creation': 0, 'render_begin_end': 0, 'texture_rendering': 0, 'render_pass': 0, 'renderer_structs': 0, 'renderer_includes': 1}
[2025-07-31 10:43:32] [INFO]   - Modified with 2 replacements
[2025-07-31 10:43:32] [INFO] Processing: include/sway/tree/root.h
[2025-07-31 10:43:32] [INFO]   - Found patterns: {'renderer_creation': 0, 'render_begin_end': 0, 'texture_rendering': 0, 'render_pass': 0, 'renderer_structs': 0, 'renderer_includes': 1}
[2025-07-31 10:43:32] [INFO]   - No modifications needed
[2025-07-31 10:43:32] [INFO] 
Updating build configuration...
```
