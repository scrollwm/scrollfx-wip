# Transaction.c Integration Guide for Scroll WM
## Agent 3 Shadow and Blur Management

## Overview
This guide shows **exactly what code to add** to Scroll's existing `sway/desktop/transaction.c` file. You do NOT need to replace the entire file - just add these specific code blocks.

---

## ⚠️ IMPORTANT: Prerequisites

Before modifying transaction.c, ensure these are complete:

### From Agent 1 (Container structures):
- `struct sway_container::shadow` field exists
- `struct sway_container::corner_radius` field exists  
- `struct sway_container::blur_enabled` field exists
- Helper functions: `container_has_shadow()`, `container_has_corner_radius()`

### From Agent 2 (Configuration):
- `config->shadow_blur_sigma`
- `config->shadow_offset_x`, `config->shadow_offset_y`
- `config->shadow_color`, `config->shadow_inactive_color`
- `config->blur_xray`

### From Output structures:
- `output->layers.blur_layer` field exists
- `root->layers.blur_tree` field exists

**If any of these are missing, this integration will not compile.**

---

## Required Includes

Add these includes to the top of `sway/desktop/transaction.c`:

```c
#include <scenefx/types/wlr_scene.h>
#include <scenefx/types/fx/shadow.h>
#include <scenefx/types/fx/corner_location.h>
```

---

## Modification 1: Shadow Management in `arrange_container()`

### Location
Find the function `static void arrange_container(...)` in your Scroll transaction.c file.

### What to Find
Look for this section near the end of the `if (con->view)` block:

```c
if (con->view) {
    // ... lots of border and view configuration code ...
    
    view_reconfigure(con->view);  // ← FIND THIS LINE
}
```

### What to Add
Add this code **RIGHT BEFORE** the `view_reconfigure(con->view);` line:

```c
        // Shadow management
        if (container_has_shadow(con)) {
            bool has_corner_radius = container_has_corner_radius(con);
            int corner_radius = has_corner_radius ?
                con->corner_radius + con->current.border_thickness : 0;

            wlr_scene_shadow_set_size(con->shadow,
                width + config->shadow_blur_sigma * 2,
                height + config->shadow_blur_sigma * 2);

            wlr_scene_node_set_position(&con->shadow->node,
                con->current.x - config->shadow_blur_sigma + config->shadow_offset_x,
                con->current.y - config->shadow_blur_sigma + config->shadow_offset_y);

            wlr_scene_shadow_set_clipped_region(con->shadow, (struct clipped_region) {
                .corner_radius = corner_radius,
                .corners = CORNER_LOCATION_ALL,
                .area = {
                    .x = config->shadow_blur_sigma - config->shadow_offset_x,
                    .y = config->shadow_blur_sigma - config->shadow_offset_y,
                    .width = width,
                    .height = height,
                },
            });

            float *color = con->current.focused || con->current.urgent ?
                config->shadow_color : config->shadow_inactive_color;
            wlr_scene_shadow_set_color(con->shadow, color);
            wlr_scene_shadow_set_blur_sigma(con->shadow, config->shadow_blur_sigma);
            wlr_scene_shadow_set_corner_radius(con->shadow, corner_radius);
        }

        view_reconfigure(con->view);
```

### Why This Code
- Updates shadow size to match container dimensions plus blur padding
- Positions shadow with configured offsets
- Applies corner radius to match container shape
- Sets shadow color based on focus state
- Runs on every container arrangement (moves, resizes, focus changes)

---

## Modification 2: Blur Layer Control in `arrange_output()`

### Location
Find the function `static void arrange_output(...)` in transaction.c.

### What to Find
Look for this section that handles fullscreen:

```c
if (activated) {
    struct sway_container *fs = child->current.fullscreen;

    sway_scene_node_set_enabled(&output->layers.shell_background->node, !fs);
    sway_scene_node_set_enabled(&output->layers.shell_bottom->node, !fs);
    // ... possibly more layer toggles ...
```

### What to Add
Add this line immediately after the existing layer enable/disable calls:

```c
    sway_scene_node_set_enabled(&output->layers.shell_background->node, !fs);
    sway_scene_node_set_enabled(&output->layers.shell_bottom->node, !fs);
    // ADD THIS LINE:
    wlr_scene_node_set_enabled(&output->layers.blur_layer->node, !fs);
```

**Note:** Use `wlr_scene_node_set_enabled` (not `sway_scene_node_set_enabled`) because `blur_layer` is a `wlr_scene_optimized_blur*` not a `sway_scene_tree*`.

### Why This Code
- Disables blur rendering when a workspace enters fullscreen mode
- Improves performance by skipping unnecessary blur calculations
- Re-enables blur when exiting fullscreen

---

## Modification 3: Blur Tree Management in `arrange_root()`

### Location
Find the function `static void arrange_root(...)` in transaction.c.

### What to Find
Look for this section at the beginning of the function:

```c
static void arrange_root(struct sway_root *root) {
    struct sway_container *fs = root->fullscreen_global;

    sway_scene_node_set_enabled(&root->layers.shell_background->node, !fs);
    sway_scene_node_set_enabled(&root->layers.shell_bottom->node, !fs);
    // ... more layer toggles ...
```

### What to Add

**Part A - Add blur tree disable:**
Add this line with the other layer toggles:

```c
    sway_scene_node_set_enabled(&root->layers.shell_background->node, !fs);
    sway_scene_node_set_enabled(&root->layers.shell_bottom->node, !fs);
    // ADD THIS LINE:
    wlr_scene_node_set_enabled(&root->layers.blur_tree->node, !fs);
    sway_scene_node_set_enabled(&root->layers.tiling->node, !fs);
    // ... rest of toggles ...
```

**Part B - Add blur layer reparenting:**
Add this code block after the layer toggles but before the fullscreen handling:

```c
    sway_scene_node_set_enabled(&root->layers.fullscreen->node, !fs);

    // ADD THIS BLOCK:
    // Reparent output blur layers to root blur tree
    for (int i = 0; i < root->outputs->length; i++) {
        struct sway_output *output = root->outputs->items[i];
        wlr_scene_node_reparent(&output->layers.blur_layer->node, root->layers.blur_tree);
    }

    // ... rest of the function (scratchpad, fullscreen handling, etc.)
```

### Why This Code
- Part A: Disables blur during global fullscreen for performance
- Part B: Ensures all output blur layers are properly parented to the root blur tree
- Maintains correct scene graph hierarchy for blur rendering

---

## Validation Checklist

After making these changes, verify:

### Compilation Check
```bash
cd /path/to/scroll
meson compile -C build
```

If you get errors like:
- `'blur_layer' undeclared` → Agent 3's output structure changes not applied
- `'container_has_shadow' undeclared` → Agent 1's container changes not applied  
- `'shadow_blur_sigma' undeclared` → Agent 2's config changes not applied
- `'wlr_scene_shadow_set_size' undeclared` → SceneFX not properly linked

### Runtime Test
```bash
# After successful build, test shadow updates
scrollmsg shadows enable
scrollmsg corner_radius 10

# Move a window - shadow should follow smoothly
# Resize a window - shadow should resize
# Focus/unfocus - shadow color should change

# Test blur toggle
scrollmsg blur enable
# Fullscreen a window - blur should disable
# Exit fullscreen - blur should re-enable
```

---

## Key Point: Why NOT Replace the Entire File

Your agent3/sway/desktop/transaction.c file is **SwayFX-based** with many Scroll-specific features already present. You should **NOT** replace Scroll's transaction.c with this file because:

1. **Scroll has unique animation logic** that SwayFX doesn't have
2. **Scroll has workspace scrolling** that SwayFX doesn't have  
3. **Scroll has different transaction timing** for animations
4. **Scroll may have custom gesture handling** in transactions

**Only add the 3 code blocks above** - they integrate cleanly with Scroll's existing architecture without disrupting Scroll-specific features.

---

## Troubleshooting

### "blur_layer field missing"
The output structure wasn't updated by Agent 3. Check:
- `include/sway/output.h` has the `blur_layer` field in `struct sway_output::layers`
- `sway/tree/output.c` creates the blur layer in `output_create()`

### "shadow is NULL" crash
Check that Agent 2 properly creates shadow nodes when containers are created. Add NULL check:

```c
if (con->shadow && container_has_shadow(con)) {
    // shadow code here
}
```

### "Shadows don't follow animations"
This is expected. The shadow code runs during `arrange_container()`, which is called during Scroll's animation frames. Shadows should animate smoothly with containers.

### "Blur still renders during fullscreen"
Check that:
1. The blur layer disable code is actually executing
2. The `!fs` logic is correct (disables when fs is NOT NULL)
3. Root blur tree management runs during root arrangement

---

## Finding the Functions in Scroll

If you can't find these functions in Scroll's transaction.c, they might be in different files:

```bash
# Search for the functions
cd /path/to/scroll
grep -rn "arrange_container" sway/
grep -rn "arrange_output" sway/
grep -rn "arrange_root" sway/
```

Likely locations:
- `sway/desktop/transaction.c` (most likely)
- `sway/tree/arrange.c` (alternative)
- `sway/desktop/animation.c` (Scroll-specific)

Apply the same code patterns wherever you find these functions.

---

## Summary

**DO THIS:**
✅ Add 3 specific code blocks to Scroll's existing transaction.c
✅ Add required includes at the top
✅ Verify prerequisites from Agent 1 and Agent 2 are complete

**DON'T DO THIS:**
❌ Replace entire transaction.c with agent3's version
❌ Copy code that already exists in Scroll
❌ Modify Scroll's animation or gesture code

The agent3 transaction.c file (if provided) is for **reference** showing where the code should go, but **you only need to add the specific shadow and blur management blocks** shown above.

---

## Quick Reference: The 3 Changes

1. **In `arrange_container()`**: Add shadow management before `view_reconfigure()`
2. **In `arrange_output()`**: Add `blur_layer` enable/disable with other layers
3. **In `arrange_root()`**: Add `blur_tree` toggle and blur layer reparenting

That's it! Three surgical additions to existing functions.

---

## Need Help?

If you encounter issues:
1. Check that all Agent 1 and Agent 2 prerequisites are met
2. Verify SceneFX is properly linked in meson.build
3. Look for similar patterns in Scroll's existing transaction.c
4. Consider adding debug logging with `sway_log(SWAY_DEBUG, "Shadow update: %dx%d", width, height);`

The integration should be straightforward if all dependencies are in place!
