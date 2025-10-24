# ScrollFX Integration - Simple Direct Replacement Guide

## Understanding the Approach

Each agent:
1. Started with ScrollWM's original code
2. Applied SceneFX modifications following SwayFX patterns  
3. Generated **COMPLETE modified files** (not patches)

Therefore: **Just replace files directly. No merging needed.**

## One Critical Manual Task

### Transaction.c - Add Code Blocks (Don't Replace) ⚠️

**Problem**: Agent 3 needs to add shadow management and blur control to Scroll's `sway/desktop/transaction.c`.

**Important**: **Do NOT replace the entire file** - only add specific code blocks.

**Why**: Scroll has unique animation and workspace scrolling logic that must be preserved. The agent3 transaction.c is SwayFX-based and lacks Scroll's features.

**Solution**: Add 3 small code blocks to existing Scroll functions:
- `arrange_container()` - shadow management (before view_reconfigure())
- `arrange_output()` - blur layer toggle (with other layer toggles)
- `arrange_root()` - blur tree management (layer toggle + reparenting)

**Action**: See [TRANSACTION-INTEGRATION-GUIDE.md](TRANSACTION-INTEGRATION-GUIDE.md) for exact code and precise locations.

**Time**: 30 minutes to 1 hour

---

## Integration Steps

### Step 0: Setup
```bash
# Clone Scroll
git clone https://github.com/dawsers/scroll.git
cd scroll
git checkout -b scrollfx-integration

# Set WIP path
WIP="/path/to/scrollwm-scrollfx-wip"
```

### Step 1: Agent 1 - Configuration & Commands
**Simply replace everything**

```bash
# Headers - Direct replacement
cp $WIP/agent1/include/sway/config.h include/sway/config.h
cp $WIP/agent1/include/sway/commands.h include/sway/commands.h
cp $WIP/agent1/include/sway/layer_criteria.h include/sway/layer_criteria.h

# Core files - Direct replacement
cp $WIP/agent1/sway/commands.c sway/commands.c
cp $WIP/agent1/sway/config.c sway/config.c

# Command files - Direct copy (24 new files)
cp $WIP/agent1/sway/commands/*.c sway/commands/

git add include/sway/config.h include/sway/commands.h include/sway/layer_criteria.h
git add sway/commands.c sway/config.c sway/commands/
git commit -m "Agent 1: Configuration and commands system

- Add SceneFX configuration structures
- Implement 26 commands for blur, shadows, corner radius, dimming
- Add layer_criteria system
- Initialize defaults"
```

**Note**: opacity.c is a REPLACEMENT of Scroll's existing file, not a conflict to merge.

### Step 2: Agent 2 - Container & Tree Management  
**Direct replacement**

```bash
# Headers
cp $WIP/agent2/include/sway/tree/container.h include/sway/tree/container.h
cp $WIP/agent2/include/sway/tree/node.h include/sway/tree/node.h
cp $WIP/agent2/include/sway/tree/root.h include/sway/tree/root.h

# Source files
cp $WIP/agent2/sway/tree/container.c sway/tree/container.c
cp $WIP/agent2/sway/tree/node.c sway/tree/node.c
cp $WIP/agent2/sway/tree/arrange.c sway/tree/arrange.c
cp $WIP/agent2/sway/tree/root.c sway/tree/root.c
cp $WIP/agent2/sway/tree/view.c sway/tree/view.c

git add include/sway/tree/ sway/tree/
git commit -m "Agent 2: Container and tree management

- Add SceneFX effect properties to containers
- Implement shadow allocation and management
- Add helper functions for effect eligibility
- Integrate with container lifecycle"
```

### Step 3: Agent 3 - Scene Graph & Output Rendering
**Direct replacement (except transaction.c)**

```bash
# Headers
cp $WIP/agent3/include/sway/output.h include/sway/output.h

# Source files
cp $WIP/agent3/sway/desktop/output.c sway/desktop/output.c
cp $WIP/agent3/sway/tree/output.c sway/tree/output.c

git add include/sway/output.h sway/desktop/output.c sway/tree/output.c
git commit -m "Agent 3: Scene graph and output rendering

- Add blur layer to output structure
- Implement comprehensive scene configuration
- Integrate effect application pipeline

NOTE: Transaction shadow management pending manual integration"
```

**Manual task remaining**: Add transaction.c code blocks (see [TRANSACTION-INTEGRATION-GUIDE.md](TRANSACTION-INTEGRATION-GUIDE.md)).

### Step 4: Agent 4 - Layer Shell & Desktop Integration
**Direct replacement**

```bash
# Headers
cp $WIP/include/sway/layers.h include/sway/layers.h

# Source files
cp $WIP/sway/layer_criteria.c sway/layer_criteria.c
cp $WIP/sway/desktop/layer_shell.c sway/desktop/layer_shell.c
cp $WIP/sway/desktop/xdg_shell.c sway/desktop/xdg_shell.c
cp $WIP/sway/desktop/xwayland.c sway/desktop/xwayland.c

git add include/sway/layers.h sway/layer_criteria.c sway/desktop/
git commit -m "Agent 4: Layer shell and desktop integration

- Implement layer_criteria system for namespace-based effects
- Add shadow support for layer surfaces
- Implement scratchpad minimize for XDG and Xwayland
- Integrate effects with layer shell lifecycle"
```

### Step 5: Agent 5 - Input & Visual Feedback
**Direct replacement + one new file**

```bash
# Headers
cp $WIP/agent5/include/sway/tree/scene.h include/sway/tree/scene.h

# Source - NEW file to create
mkdir -p sway/tree/scene
cp $WIP/agent5/sway/tree/scene/scene.c sway/tree/scene/scene.c

# Input modifications
cp $WIP/agent5/sway/input/seatop_move_tiling.c sway/input/seatop_move_tiling.c

git add include/sway/tree/scene.h sway/tree/scene/ sway/input/seatop_move_tiling.c
git commit -m "Agent 5: Input and visual feedback

- Add corner radius support to scene rects
- Add backdrop blur to move indicators
- Enhance visual feedback during window operations"
```

### Step 6: Build System Integration

#### 6.1 Add SceneFX Subproject
```bash
mkdir -p subprojects

cat > subprojects/scenefx.wrap << 'EOF'
[wrap-git]
url = https://github.com/wlrfx/scenefx.git
revision = main
depth = 1

[provide]
scenefx = scenefx
EOF

git add subprojects/scenefx.wrap
git commit -m "Add SceneFX as meson subproject"
```

#### 6.2 Modify Root meson.build

**Find this section** (after wlroots):
```meson
wlroots_proj = subproject(
    'wlroots',
    default_options: wlroots_features,
    required: false,
    version: wlroots_version,
)
```

**Add this right after**:
```meson
# SceneFX dependency
scenefx_version = ['>=0.1.0']
scenefx_proj = subproject(
    'scenefx',
    default_options: ['default_library=static'],
    required: true,
    version: scenefx_version,
)
scenefx = scenefx_proj.get_variable('scenefx')
```

**Find the sway_deps array**:
```meson
sway_deps = [
    cairo,
    gdk_pixbuf,
    jsonc,
    math,
    pango,
    pangocairo,
    pcre2,
    server_protos,
    wayland_server,
    wlroots,
    xkbcommon,
]
```

**Add scenefx**:
```meson
sway_deps = [
    cairo,
    gdk_pixbuf,
    jsonc,
    math,
    pango,
    pangocairo,
    pcre2,
    server_protos,
    wayland_server,
    wlroots,
    scenefx,  # ADD THIS LINE
    xkbcommon,
]
```

#### 6.3 Modify sway/meson.build

**Find sway_sources = files(**

**Add these lines** (in alphabetical order with existing files):
```meson
sway_sources = files(
    # ... existing files ...
    
    # SceneFX command files
    'commands/blur.c',
    'commands/blur_brightness.c',
    'commands/blur_contrast.c',
    'commands/blur_noise.c',
    'commands/blur_passes.c',
    'commands/blur_radius.c',
    'commands/blur_saturation.c',
    'commands/blur_xray.c',
    'commands/corner_radius.c',
    'commands/default_dim_inactive.c',
    'commands/dim_inactive.c',
    'commands/dim_inactive_colors.c',
    'commands/layer_effects.c',
    'commands/scratchpad_minimize.c',
    'commands/shadow_blur_radius.c',
    'commands/shadow_color.c',
    'commands/shadow_inactive_color.c',
    'commands/shadow_offset.c',
    'commands/shadows.c',
    'commands/shadows_on_csd.c',
    'commands/smart_corner_radius.c',
    'commands/titlebar_separator.c',
    
    # SceneFX support files
    'layer_criteria.c',
    'tree/scene/scene.c',
    
    # ... rest of existing files ...
)
```

**Commit build changes**:
```bash
git add meson.build sway/meson.build
git commit -m "Update build system for SceneFX integration

- Add SceneFX as required subproject dependency
- Add 24 new command files to build
- Add layer_criteria.c and tree/scene/scene.c"
```

### Step 7: Build and Test

```bash
# Clean any old build
rm -rf build

# Configure
meson setup build

# Build
ninja -C build

# If successful, test
./build/sway/sway --version
```

---

## What Could Go Wrong

### Build Error: "SceneFX not found"
```bash
# Update subprojects
meson subprojects update

# Or download manually
cd subprojects
meson wrap install scenefx
```

### Build Error: "undefined reference to wlr_scene_shadow_create"
**Cause**: SceneFX not in dependency list
**Fix**: Verify scenefx in sway_deps array

### Build Error: Missing container.c functions
**Cause**: File replacement didn't work correctly
**Fix**: 
```bash
# Verify file copied
ls -la sway/tree/container.c

# Check it has SceneFX modifications
grep "shadow" sway/tree/container.c
```

### Build Error: scene.c not found
**Cause**: Directory not created
**Fix**:
```bash
mkdir -p sway/tree/scene
cp $WIP/agent5/sway/tree/scene/scene.c sway/tree/scene/
```

---

## After Successful Build

### Transaction Code Integration (Manual)

Now we need to add the shadow management code that Agent 3 documented but didn't output as a file:

1. **Find where arrange functions live in Scroll**:
   ```bash
   grep -rn "arrange_container" sway/
   grep -rn "arrange_output" sway/
   grep -rn "arrange_root" sway/
   ```

2. **Extract code from Agent 3 report**:
   - See TRANSACTION-EXTRACTION-GUIDE.md
   - File: `agent3/reports/agent3-scene-rendering.md`
   - Search for "Shadow Management" sections

3. **Apply code to appropriate Scroll files**:
   - Likely `sway/tree/arrange.c` or `sway/desktop/animation.c`
   - Insert shadow management code where documented

4. **Rebuild**:
   ```bash
   ninja -C build
   ```

### Test Visual Effects

Create a test config:
```bash
cat > /tmp/scrollfx-test.conf << 'EOF'
# Test ScrollFX effects
blur enable
blur_radius 5
shadows enable
shadow_blur_radius 20
corner_radius 10

# Test per-window rules
for_window [app_id="firefox"] opacity 0.95
for_window [app_id="kitty"] blur enable

# Test layer effects  
layer_effects waybar "blur enable; shadows enable"
EOF

# Test config loads
./build/sway/sway -C -c /tmp/scrollfx-test.conf
```

### Runtime Testing

```bash
# In a running Scroll session with your build

# Test commands
scrollmsg blur enable
scrollmsg blur_radius 7
scrollmsg shadows enable
scrollmsg corner_radius 15
scrollmsg opacity 0.9

# Move windows - check for:
# - Rounded corners
# - Shadows that follow
# - Blur effects
# - Animations still smooth
```

---

## Quick Verification Checklist

After integration, verify:

- [ ] All agent files copied (run `git status` - should see modified files)
- [ ] SceneFX subproject configured
- [ ] meson.build updated with dependency
- [ ] sway/meson.build updated with new files
- [ ] Build completes: `ninja -C build`
- [ ] Binary runs: `./build/sway/sway --version`
- [ ] Config loads without errors
- [ ] Visual effects render correctly
- [ ] Scroll's animations still work
- [ ] Transaction code applied (manual step)

---

## Summary

**Simple approach**: Direct file replacement, no merging needed.

**Only manual work**:
1. Build system updates (meson.build files)
2. Transaction code integration from Agent 3's report

**Time estimate**: 30-60 minutes for basic integration, plus 1-2 hours for transaction code.

**Support**: If you hit build errors, check the error logs and the specific agent's report for context on what that file does.
