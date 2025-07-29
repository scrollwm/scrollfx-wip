This file provides guidance to Claude Code when working with the ScrollFX project.

## Project Overview

ScrollFX is a window manager that combines Scroll's PaperWM-like scrolling functionality with SceneFX's cosmetic enhancements (blur, shadows, rounded corners, dimming).

## Project Structure

```
scrollfx-wip/
├── /references/          # Source repositories (READ-ONLY)
│   ├── scroll/          # Original Scroll window manager
│   ├── sway/            # Original Sway
│   ├── swayfx/          # SwayFX with SceneFX integration
│   └── scenefx/         # SceneFX renderer
├── investigation/       # where the investigation files mentioned below exist
├── scenefx-scroll/      # SceneFX adapted for Scroll (Task 1 output)
├── scene-scroll/        # Extracted Scroll scene graph (Task 2 output)
├── scroll-standalone/   # Scroll without embedded scene (Task 2 output)
└── scrollfx/            # Final merged window manager (Task 4+ output)
```

## Key Technical Context

1. **Scene Graph Divergence**: Scroll uses `sway_scene_*` (extracted and modified from wlroots) instead of `wlr_scene_*`. This is located in `sway/tree/scene/` in Scroll.

2. **Renderer**: SceneFX provides an FX renderer that replaces wlroots renderer with shader-based effects.

3. **API Mapping**: When working on scenefx-scroll, all `wlr_scene_*` calls must be converted to `sway_scene_*` equivalents.

## Important Rules

1. **Always generate report-{taskname}.md**: Document findings, decisions, and implementation details.

2. **Reference analysis documents**: Use the investigation MD files for understanding SwayFX's SceneFX integration patterns.

3. **Preserve Scroll functionality**: Content/workspace scaling must continue to work with all effects.

4. **Use specific paths**: When analyzing code, look in the exact directories specified in the GitHub issue.

## Available Analysis Documents

- `investigation-include.md` - Header file changes for SceneFX
- `investigation-sway-commands.md` - New commands added by SwayFX
- `investigation-sway-config.md` - Configuration integration patterns
- `investigation-sway-desktop.md` - Desktop component modifications
- `investigation-sway-input.md` - Input system changes (minimal)
- `investigation-sway-tree.md` - Tree/container modifications

## Code Generation Guidelines

1. Don't suggest specific implementation details in prompts
2. Let the coding agent determine the best approach
3. Focus prompts on what needs to be done, not how
4. Include specific file paths and directories to analyze
5. Reference the investigation documents for patterns
