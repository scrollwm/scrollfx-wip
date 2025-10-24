# Agent 6: Integration & Testing

## Your Mission
Verify that all agent implementations work together correctly. You are the integration specialist who ensures ScrollFX builds, runs, and preserves all Scroll functionality while adding SceneFX effects.

## Domain Scope
- Build system integration
- Cross-agent interface validation
- Scroll feature preservation testing
- Performance validation
- Bug fixing and integration issues
- Multi-output testing

## Your Workflow

### Phase 1: Wait for Agents 1-5 (REQUIRED)
**DO NOT START until all agents 1-5 have completed their PRs**

Review each agent's report:
- `/reports/agent1-configuration-commands.md`
- `/reports/agent2-container-tree.md`
- `/reports/agent3-scene-rendering.md`
- `/reports/agent4-layer-desktop.md`
- `/reports/agent5-input-feedback.md`

### Phase 2: Build System (HIGH)
1. Verify meson.build includes all new files
2. Add SceneFX dependency to build
3. Ensure all headers findable
4. Resolve compilation errors
5. Test clean build from scratch

### Phase 3: Interface Validation (HIGH)
1. Verify config structures accessible by all subsystems
2. Test container properties read by rendering
3. Validate scene graph structure
4. Check memory management across boundaries
5. Verify no circular dependencies

### Phase 4: Scroll Feature Testing (CRITICAL)
1. **Workspace Scrolling:**
   - Effects persist during scroll
   - Shadows position correctly
   - Blur doesn't break scrolling
   - Corner radius renders during scroll

2. **Animations:**
   - Effects survive transaction system
   - Shadows animate with windows
   - Blur maintains during Bezier animations
   - No visual artifacts

3. **Workspace Scaling (Overview):**
   - Effects scale with workspace
   - Shadow sizing adjusts
   - Blur optimizes for zoomed view
   - Corner radius scales properly

4. **Content Scaling:**
   - Per-window scaling works with effects
   - Effects scale with content
   - No rendering issues

### Phase 5: Core SceneFX Testing (HIGH)
1. **Blur Effects:**
   - Global blur works
   - Per-container blur toggles
   - Xray optimization functional
   - Blur parameters apply correctly
   - Layer surface blur works

2. **Shadow Effects:**
   - Shadows render on containers
   - Shadow colors (active/inactive) work
   - Shadow offset configurable
   - Shadow blur sigma applies
   - Shadows on layer surfaces work
   - CSD shadow control functional

3. **Corner Radius:**
   - Global corner radius applies
   - Per-container radius works
   - Smart corner radius logic correct
   - Titlebar corners in tabbed/stacked
   - Layer surface corners work

4. **Dimming:**
   - Dim inactive works
   - Urgent color switching works
   - Dim values apply correctly

### Phase 6: Multi-Output Testing (MEDIUM)
1. Test effects on multiple monitors
2. Verify blur layer per output
3. Test output hotplug with effects
4. Validate shadow rendering across outputs

### Phase 7: Performance Testing (MEDIUM)
1. Measure FPS with effects enabled
2. Test blur performance during scrolling
3. Validate shadow rendering cost
4. Check memory usage
5. Profile effect application overhead

## Investigation Files to Reference

**Primary References:**
- `reports/000-executive-summary.md` - Overall strategy
- `reports/005-risk-assessment-roadmap.md` - Known risks
- All agent investigation files for cross-reference

## Critical Integration Points

**Build System:**
```meson
# In meson.build
scenefx = dependency('scenefx', version: '>=0.1.0')

executable('scroll',
  # ... all source files from agents ...
  dependencies: [
    scenefx,
    # ... other deps ...
  ],
)
```

**Header Dependencies:**
```c
// Verify this chain works:
config.h → commands.h → container.h → output.h → layer_shell.c
         → SceneFX headers
```

**Scroll Transaction Testing:**
```c
// Test that effects persist through:
1. container_update_representation()
2. transaction_apply()
3. arrange_windows()
4. Animation completion
```

## Testing Checklist

### Build Testing:
- [ ] Clean build succeeds
- [ ] No compiler warnings
- [ ] All new files compiled
- [ ] SceneFX headers found
- [ ] No linking errors

### Configuration Testing:
- [ ] Config file loads with all new options
- [ ] Commands parse correctly
- [ ] Invalid values rejected
- [ ] Colors parse properly

### Container Testing:
- [ ] Containers have shadow nodes
- [ ] Effect properties accessible
- [ ] Helper functions work
- [ ] Memory properly freed

### Rendering Testing:
- [ ] output_configure_scene() applies effects
- [ ] Scene traversal works
- [ ] Buffer effects apply
- [ ] Blur layer functional

### Scroll Feature Testing:
- [ ] Workspace scrolling works
- [ ] Animations smooth
- [ ] Workspace scaling works
- [ ] Content scaling works
- [ ] Effects persist through all above

### SceneFX Testing:
- [ ] Blur global/per-container works
- [ ] Shadows render correctly
- [ ] Corner radius applies
- [ ] Dimming functional
- [ ] Layer effects work

### Integration Testing:
- [ ] Multi-output stable
- [ ] No memory leaks
- [ ] Performance acceptable
- [ ] No visual artifacts

## Bug Fixing Priorities

1. **Build errors** - Highest priority
2. **Crashes** - Highest priority
3. **Scroll feature regressions** - High priority
4. **Effect rendering issues** - Medium priority
5. **Visual polish** - Low priority

## Deliverables
1. Working ScrollFX build
2. All Scroll features functional
3. All SceneFX effects working
4. Integration test results
5. Performance metrics
6. Bug fixes for integration issues
7. PR with comprehensive report at `/reports/agent6-integration-testing.md`

## Success Criteria
- ScrollFX compiles successfully
- All Scroll features work (scrolling, animations, scaling)
- All SceneFX effects functional (blur, shadows, corners, dim)
- No memory leaks
- Performance acceptable (within 5% of Scroll baseline)
- Multi-output stable
- No visual artifacts
- All tests passing

## Notes
- You may need to request changes from agents 1-5
- Document all integration issues found
- Be thorough - this is the quality gate
- Scroll functionality preservation is paramount
