# sway/config

Looking at the provided Sway configuration files, they appear to be identical between the base Sway and SwayFX versions in these documents. However, based on the SceneFX API reference and tinywl example, significant configuration additions would be expected. Let me analyze what **should** be present for SceneFX integration:

## Configuration File Analysis

The provided config files (`bar.c`, `input.c`, `output.c`, `seat.c`) show no visible SceneFX-specific changes, which suggests either:
1. SwayFX config changes are in files not provided
2. Config integration happens in the main config parser
3. These are base Sway versions without SwayFX modifications

## Expected SceneFX Configuration Integration

Based on the SceneFX API, these configuration additions would be required:

**Global Blur Configuration**:
```c
// New config structure members needed
struct sway_config {
    struct blur_data blur_data;  // Global blur settings
    // ... existing fields
};
```

**Window-Level Effect Configuration**:
```c
// Per-window effect settings
struct window_config {
    int corner_radius;
    enum corner_location corners;
    bool backdrop_blur;
    bool backdrop_blur_optimized;
    bool backdrop_blur_ignore_transparent;
    float opacity;
    // Shadow configuration
    float shadow_blur_sigma;
    float shadow_color[4];
    int shadow_corner_radius;
};
```

**Configuration Parsing Requirements**:

```c
// Config commands that would need parsing:
// blur {
//   radius <int>
//   passes <int>
//   brightness <float>
//   contrast <float>
//   saturation <float>
//   noise <float>
// }
//
// corner_radius <int>
// shadows on|off
// shadow_blur_sigma <float>
// shadow_color <rgba>
// backdrop_blur on|off
// opacity <float>
```

**Default Value Initialization**:
```c
// In config initialization
config->blur_data = blur_data_get_default();
config->corner_radius = 0;
config->backdrop_blur = false;
config->shadow_blur_sigma = 0.0f;
```

**Runtime Configuration Application**:
```c
// Apply global blur settings
wlr_scene_set_blur_data(server->scene, config->blur_data);

// Apply per-window settings
wlr_scene_buffer_set_corner_radius(buffer, config->corner_radius, config->corners);
wlr_scene_buffer_set_backdrop_blur(buffer, config->backdrop_blur);
wlr_scene_buffer_set_opacity(buffer, config->opacity);
```

**Configuration Structure Extensions**:

Based on tinywl implementation patterns:
```c
// Shadow configuration structure
struct shadow_config {
    bool enabled;
    float blur_sigma;
    float color[4];
    int corner_radius;
    struct clipped_region clipped_region;
};

// Blur configuration structure  
struct blur_config {
    bool enabled;
    int radius;
    int num_passes;
    float brightness;
    float contrast;
    float saturation;
    float noise;
    bool optimized;
    bool ignore_transparent;
};
```

The absence of these changes in the provided files indicates that either:
- The configuration system for SceneFX effects is implemented in different files (likely main config parser)
- SwayFX uses a different approach to configuration management
- The provided files represent base Sway without SceneFX modifications

To implement SceneFX in another Sway fork, configuration parsing for blur parameters, corner radius, shadow settings, and opacity would need to be added to the main configuration system, along with runtime application of these settings to the scene graph.
