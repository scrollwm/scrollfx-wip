# ScrollFX Implementation

This directory contains the complete, consolidated implementation of ScrollFX - a fork of the Scroll window manager with SceneFX visual effects integration.

## Overview

**ScrollFX** = Scroll WM + SceneFX visual effects

This implementation adds the following visual effects to Scroll while preserving all of Scroll's unique features:
- Backdrop blur
- Window shadows
- Corner radius
- Window dimming
- Layer surface effects

## Directory Structure

```
scrollfx-implementation/
├── include/sway/          # Header files
│   ├── tree/              # Tree/container headers
│   ├── commands.h         # Command declarations
│   ├── config.h           # Configuration structures
│   ├── layer_criteria.h   # Layer matching system
│   ├── layers.h           # Layer shell structures
│   └── output.h           # Output structures with blur
│
├── sway/                  # Source files
│   ├── commands/          # 24 SceneFX command implementations
│   ├── desktop/           # Desktop integration (output, layer_shell, etc.)
│   ├── input/             # Input handling (visual feedback)
│   ├── tree/              # Tree/container management
│   │   └── scene/         # Scene graph helpers
│   ├── commands.c         # Command registration
│   ├── config.c           # Configuration parsing
│   └── layer_criteria.c   # Layer matching implementation
│
└── docs/                  # Documentation
    ├── agent-reports/     # Detailed implementation reports by component
    └── integration/       # Integration guides and scripts
```

## Implementation Components

This implementation was developed in 5 logical components:

### Component 1: Configuration & Commands
**Files:** 28 files
**Purpose:** Configuration system and 24 SceneFX commands
- Command registration and parsing
- Configuration structures for all effects
- Layer criteria matching system
- Commands: blur, shadows, corner_radius, dim_inactive, opacity, etc.

### Component 2: Container & Tree Management
**Files:** 8 files
**Purpose:** Container properties and tree management
- Effect properties added to containers
- Shadow node allocation
- Helper functions for effect queries
- Container lifecycle management

### Component 3: Scene & Output Rendering
**Files:** 4 files
**Purpose:** Rendering pipeline and scene configuration
- Blur layer management per output
- Scene graph traversal and effect application
- Output structure modifications
- Transaction shadow/blur management

### Component 4: Layer Shell & Desktop
**Files:** 6 files
**Purpose:** Layer surface effects and desktop integration
- Layer criteria system for layer surfaces
- Effect application to bars/panels
- Desktop shell integration
- Scratchpad minimize effects

### Component 5: Input & Visual Feedback
**Files:** 3 files
**Purpose:** Visual feedback during user interaction
- Corner radius for scene rects
- Backdrop blur for move indicators
- Enhanced visual feedback during tiling operations

## Quick Start

### Option 1: Use the Integration Script

```bash
# From this directory
cd docs/integration
./integrate-scrollfx.sh <scroll-repo-path> <scrollfx-wip-path>

# Follow the manual steps it prints for:
# 1. meson.build modifications
# 2. transaction.c integration
```

### Option 2: Manual Integration

1. **Copy Files**
   ```bash
   # Copy all files to scroll repo
   cp -r include/sway/* <scroll-repo>/include/sway/
   cp -r sway/* <scroll-repo>/sway/
   ```

2. **Update Build System**
   - Edit `meson.build` - Add SceneFX dependency
   - Edit `sway/meson.build` - Add new source files
   - See `docs/integration/meson-build-guide.md`

3. **Integrate Transaction Code**
   - Add 3 code blocks to `sway/desktop/transaction.c`
   - See `docs/integration/TRANSACTION-INTEGRATION-GUIDE.md`
   - **DO NOT replace the entire file**

4. **Build**
   ```bash
   cd <scroll-repo>
   meson setup build
   ninja -C build
   ```

## Documentation

### Integration Guides
- **[SIMPLE-INTEGRATION-GUIDE.md](docs/integration/SIMPLE-INTEGRATION-GUIDE.md)** - Step-by-step integration
- **[TRANSACTION-INTEGRATION-GUIDE.md](docs/integration/TRANSACTION-INTEGRATION-GUIDE.md)** - Critical transaction.c integration
- **[meson-build-guide.md](docs/integration/meson-build-guide.md)** - Build system setup

### Implementation Reports
- **[agent1-configuration-commands.md](docs/agent-reports/agent1-configuration-commands.md)** - Configuration and commands
- **[agent2-container-tree.md](docs/agent-reports/agent2-container-tree.md)** - Container and tree management
- **[agent3-scene-rendering.md](docs/agent-reports/agent3-scene-rendering.md)** - Scene and output rendering
- **[agent4-layer-desktop.md](docs/agent-reports/agent4-layer-desktop.md)** - Layer shell and desktop
- **[agent5-input-feedback.md](docs/agent-reports/agent5-input-feedback.md)** - Input and visual feedback

## File Manifest

All 60 files in this implementation:

### Headers (9 files)
- `include/sway/commands.h` - Command declarations
- `include/sway/config.h` - Config structures with SceneFX additions
- `include/sway/layer_criteria.h` - Layer matching
- `include/sway/layers.h` - Layer shell structures
- `include/sway/output.h` - Output with blur layer
- `include/sway/tree/container.h` - Container with effects
- `include/sway/tree/node.h` - Node structures
- `include/sway/tree/root.h` - Root with blur tree
- `include/sway/tree/scene.h` - Scene helpers

### Core Sources (3 files)
- `sway/commands.c` - Command registration
- `sway/config.c` - Configuration parsing
- `sway/layer_criteria.c` - Layer matching

### Commands (24 files)
- `sway/commands/blur.c` - Blur enable/disable
- `sway/commands/blur_*.c` - 7 blur parameter commands
- `sway/commands/corner_radius.c` - Corner radius
- `sway/commands/smart_corner_radius.c` - Smart corner logic
- `sway/commands/dim_*.c` - 3 dimming commands
- `sway/commands/layer_effects.c` - Layer surface effects
- `sway/commands/opacity.c` - Window opacity
- `sway/commands/scratchpad_minimize.c` - Scratchpad effects
- `sway/commands/shadow_*.c` - 4 shadow parameter commands
- `sway/commands/shadows.c` - Shadow enable/disable
- `sway/commands/shadows_on_csd.c` - CSD shadow control
- `sway/commands/titlebar_separator.c` - Titlebar separator

### Desktop (5 files)
- `sway/desktop/output.c` - Output rendering with scene config
- `sway/desktop/transaction.c` - Transaction with shadow/blur management
- `sway/desktop/layer_shell.c` - Layer shell with effects
- `sway/desktop/xdg_shell.c` - XDG shell integration
- `sway/desktop/xwayland.c` - XWayland integration

### Tree (6 files)
- `sway/tree/arrange.c` - Arrangement with effects
- `sway/tree/container.c` - Container lifecycle with effects
- `sway/tree/node.c` - Node management
- `sway/tree/output.c` - Output creation with blur layer
- `sway/tree/root.c` - Root with blur tree
- `sway/tree/view.c` - View management

### Scene (1 file)
- `sway/tree/scene/scene.c` - Scene graph helpers

### Input (1 file)
- `sway/input/seatop_move_tiling.c` - Move operation with visual feedback

## Integration Time Estimate

- **Automated script**: 5 minutes
- **Manual build system edits**: 15 minutes
- **Transaction.c integration**: 30-60 minutes
- **Build and test**: 30 minutes
- **Total**: 1.5-2 hours

## Success Criteria

After integration, you should have:
- ✅ ScrollFX builds successfully
- ✅ `scrollmsg blur enable` works
- ✅ `scrollmsg shadows enable` works
- ✅ Windows have rounded corners
- ✅ Shadows follow window movements
- ✅ Scroll's animations remain smooth
- ✅ Workspace scrolling works with effects

## Critical Notes

### Transaction.c Integration
**DO NOT** replace Scroll's `transaction.c` with the one in this directory. The version here is SwayFX-based and lacks Scroll's unique animation and workspace scrolling logic.

**Instead**: Add only the 3 specific code blocks documented in `docs/integration/TRANSACTION-INTEGRATION-GUIDE.md`

### Scroll Feature Preservation
This integration preserves all Scroll features:
- ✅ Workspace scrolling
- ✅ Bezier curve animations
- ✅ Workspace scaling (overview mode)
- ✅ Per-window content scaling
- ✅ Custom gesture handling

Effects are applied on top of these features, not replacing them.

## Testing Your Build

### Config File Test
```bash
# Create test config
cat > /tmp/test-scrollfx.conf << 'EOF'
blur enable
blur_radius 5
shadows enable
shadow_blur_radius 20
corner_radius 10

for_window [app_id="firefox"] opacity 0.95
layer_effects waybar "blur enable; shadows enable"
EOF

# Test config parsing
./build/sway/sway -C -c /tmp/test-scrollfx.conf
```

### Runtime Test
```bash
# In a running ScrollFX session
scrollmsg blur enable
scrollmsg shadows enable
scrollmsg corner_radius 15

# Move and resize windows to see:
# - Rounded corners
# - Shadows following windows smoothly
# - Blur effects
# - Scroll animations still working
```

## Build System Requirements

### Dependencies
- SceneFX >= 0.1.0
- wlroots 0.19.1 (same as Scroll)
- All existing Scroll dependencies

### Meson Configuration
Add to root `meson.build`:
```meson
scenefx = dependency('scenefx', version: '>=0.1.0')

# Add to sway_deps array
sway_deps = [
  # ... existing deps ...
  scenefx,
]
```

### SceneFX Subproject
Create `subprojects/scenefx.wrap`:
```ini
[wrap-git]
url = https://github.com/wlrfx/scenefx.git
revision = main
depth = 1

[provide]
scenefx = scenefx
```

## Troubleshooting

### Build Fails - SceneFX Not Found
```bash
cd <scroll-repo>
meson subprojects update
```

### Build Fails - Undefined References
Check that `scenefx` is in `sway_deps` array in root `meson.build`

### Build Fails - Missing Files
Verify all files were copied:
```bash
ls sway/commands/blur.c
ls include/sway/tree/container.h
ls sway/tree/scene/scene.c
```

### Effects Don't Work
1. Check transaction code was integrated (3 code blocks added)
2. Verify SceneFX is linked: `ldd build/sway/sway | grep scenefx`
3. Check config file syntax
4. Review logs: `sway -d 2>&1 | grep -i scene`

## Architecture Notes

### Two Parallel Scene Graphs
ScrollFX maintains two scene graph systems:
- **Scroll's `sway_scene_*`**: Layout, animations, workspace management
- **SceneFX's `wlr_scene_*`**: Visual effects (blur, shadows, corners)

Effects are applied during rendering via `output_configure_scene()`, which traverses the Scroll scene graph and applies SceneFX effects to the corresponding wlroots scene nodes.

### Effect Application Pipeline
1. Configuration loaded and parsed
2. Containers created with effect properties
3. Scene graph constructed
4. `output_configure_scene()` traverses and applies effects
5. Effects persist through transactions and animations

## Project Links

- **Scroll**: https://github.com/dawsers/scroll
- **SceneFX**: https://github.com/wlrfx/scenefx
- **SwayFX**: https://github.com/WillPower3309/swayfx (reference)

## License

Same as Scroll window manager.

## Contributors

Implementation by Claude Code agents 1-5, consolidated into this unified structure.

---

**Last Updated**: 2025-10-25
**ScrollFX Version**: Based on Scroll 1.11.7 + SceneFX latest
**wlroots Version**: 0.19.1
