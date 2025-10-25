# Meson Build System Integration for SceneFX

## Overview
This guide shows how to modify ScrollWM's meson.build files to link against SceneFX, following the SwayFX pattern.

## Step 1: Root meson.build Modifications

### Add SceneFX Dependency Declaration

In the root `meson.build`, after the wlroots dependency section:

```meson
# Find wlroots
wlroots_version = ['>=0.18.0', '<0.19.0']
wlroots_proj = subproject(
    'wlroots',
    default_options: wlroots_features,
    required: false,
    version: wlroots_version,
)

# ADD THIS: Find SceneFX
scenefx_version = ['>=0.1.0']  # Adjust version as needed
scenefx_proj = subproject(
    'scenefx',
    default_options: ['default_library=static'],  # Static linking
    required: true,  # Required dependency
    version: scenefx_version,
)
scenefx = scenefx_proj.get_variable('scenefx')
```

### Update sway_deps

Add scenefx to the dependencies list:

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
    scenefx,  # ADD THIS
    xkbcommon,
]
```

## Step 2: sway/meson.build Modifications

### Add New Command Files

In `sway/meson.build`, add the new command files to the sources list:

```meson
sway_sources = files(
    # ... existing files ...
    
    # ADD THESE: New command files
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
    'commands/opacity.c',
    'commands/scratchpad_minimize.c',
    'commands/shadow_blur_radius.c',
    'commands/shadow_color.c',
    'commands/shadow_inactive_color.c',
    'commands/shadow_offset.c',
    'commands/shadows.c',
    'commands/shadows_on_csd.c',
    'commands/smart_corner_radius.c',
    'commands/titlebar_separator.c',
    
    # ADD THIS: Layer criteria implementation
    'layer_criteria.c',
)
```

## Step 3: Subproject Setup

### Option A: As a Meson Subproject (Recommended)

Create `subprojects/scenefx.wrap`:

```ini
[wrap-git]
url = https://github.com/wlrfx/scenefx.git
revision = main
depth = 1

[provide]
scenefx = scenefx
```

### Option B: System Dependency

If SceneFX is installed system-wide:

```meson
# In root meson.build, replace subproject with:
scenefx = dependency('scenefx', version: scenefx_version)
```

## Step 4: Include Directories

Ensure SceneFX headers are in the include path. This is usually automatic with the dependency, but verify:

```meson
# In root meson.build
sway_inc = include_directories('include')
```

SceneFX headers will be available as:
```c
#include <scenefx/types/wlr_scene.h>
#include <scenefx/types/fx/blur_data.h>
#include <scenefx/types/fx/corner_location.h>
```

## Step 5: Build Configuration Options

Optionally add a feature flag to disable SceneFX (for compatibility):

```meson
# In meson_options.txt
option('scenefx', type: 'feature', value: 'enabled',
    description: 'Enable SceneFX visual effects')
```

Then in meson.build:

```meson
scenefx_enabled = get_option('scenefx')

if scenefx_enabled.enabled()
    scenefx = scenefx_proj.get_variable('scenefx')
    sway_deps += scenefx
    add_project_arguments('-DHAVE_SCENEFX', language: 'c')
endif
```

## Step 6: Testing the Build

```bash
# Clean any existing build
rm -rf build/

# Configure with meson
meson setup build/

# Build
ninja -C build/

# Check for SceneFX linkage
ldd build/sway/sway | grep scenefx
```

## Expected Output

Successful build should show:
```
Found SceneFX: YES
Configuring sway executable...
Building targets...
[1/150] Compiling C object sway/commands/blur.c.o
[2/150] Compiling C object sway/commands/shadows.c.o
...
[150/150] Linking target sway/sway
```

## Common Issues

### Issue: SceneFX not found
**Solution**: Ensure scenefx.wrap exists in subprojects/ or SceneFX is installed system-wide

### Issue: Undefined references to wlr_scene_*
**Solution**: Verify scenefx is in sway_deps list

### Issue: Header not found
**Solution**: Check include paths and SceneFX installation

### Issue: Duplicate symbols
**Solution**: Ensure SceneFX is only linked once (check sway_deps)

## Verification Checklist

- [ ] scenefx subproject configured correctly
- [ ] scenefx in sway_deps
- [ ] All 24 new command files added to sway_sources
- [ ] layer_criteria.c added to sway_sources
- [ ] Build completes without errors
- [ ] ldd shows scenefx linkage
- [ ] Sway binary runs: `./build/sway/sway --version`

## Next Steps After Successful Build

1. Test config file parsing: `./build/sway/sway -c test-config -d`
2. Run in nested mode: `./build/sway/sway -c ~/.config/sway/config`
3. Test individual commands via swaymsg
4. Verify visual effects render correctly
