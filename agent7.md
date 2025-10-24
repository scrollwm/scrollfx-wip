# Agent 7: Documentation & Final Polish

## Your Mission
Create comprehensive documentation for ScrollFX, ensure code consistency and quality, and deliver the final polished product. You are the last agent before user review.

## Domain Scope
- User-facing documentation
- Developer documentation
- Code consistency and style
- Final polish and cleanup
- Migration guide creation
- Configuration examples

## Your Workflow

### Phase 1: Wait for Agent 6 (REQUIRED)
**DO NOT START until Agent 6 has completed integration and testing**

Review:
- `/reports/agent6-integration-testing.md`
- All integration test results
- Build status and known issues

### Phase 2: User Documentation (HIGH)
1. Create `README-SCROLLFX.md` with:
   - What is ScrollFX
   - Differences from Scroll
   - Feature list
   - Installation instructions
   - Basic usage examples

2. Create `CONFIG-REFERENCE.md` with:
   - All new configuration options
   - Command syntax and examples
   - Default values
   - Best practices

3. Create example config files:
   - `examples/scrollfx.conf` - Full featured
   - `examples/minimal-effects.conf` - Minimal setup
   - `examples/performance.conf` - Performance optimized

### Phase 3: Developer Documentation (HIGH)
1. Create `ARCHITECTURE.md`:
   - How SceneFX integrates with Scroll
   - Scene graph structure
   - Effect application pipeline
   - Transaction system interaction

2. Create `PORTING-GUIDE.md`:
   - How we ported SwayFX patterns to Scroll
   - Challenges and solutions
   - Scroll-specific considerations
   - Lessons learned

3. Update existing docs:
   - Build instructions
   - Contributing guidelines
   - Testing procedures

### Phase 4: Code Consistency (MEDIUM)
1. Style consistency check:
   - Naming conventions match Scroll
   - Indentation consistent
   - Comment style uniform
   - Header guards consistent

2. Code quality review:
   - Remove debug code
   - Clean up commented code
   - Add missing comments
   - Improve error messages

3. Documentation strings:
   - Document all new functions
   - Document all new structures
   - Add usage examples where helpful

### Phase 5: Configuration Polish (MEDIUM)
1. Default value review:
   - Sane defaults for all options
   - Conservative effect defaults
   - Performance-first defaults

2. Config validation:
   - Range checking consistent
   - Error messages helpful
   - Invalid configs rejected gracefully

3. Example configs tested:
   - All examples actually work
   - Examples demonstrate features well
   - Performance configs actually perform

### Phase 6: Final Polish (LOW)
1. Visual polish:
   - Default shadow colors pleasant
   - Default corner radius reasonable
   - Default blur subtle

2. Error handling:
   - Graceful degradation when effects fail
   - Helpful error messages
   - No silent failures

3. Performance optimization:
   - Review hot paths
   - Consider effect caching
   - Profile if needed

### Phase 7: Release Preparation (LOW)
1. Create `CHANGELOG.md`:
   - All new features
   - Breaking changes (if any)
   - Known issues
   - Contributors

2. Create `KNOWN-ISSUES.md`:
   - Document any remaining issues
   - Workarounds
   - Future work

3. Final validation:
   - All docs render properly
   - Links work
   - Examples correct

## Investigation Files to Reference

**Primary References:**
- All agent reports for comprehensive understanding
- `reports/000-executive-summary.md` - High-level overview
- `reports/004-scenefx-api-migration-guide.md` - Technical details

## Documentation Templates

**README-SCROLLFX.md Structure:**
```markdown
# ScrollFX

Scroll window manager with SceneFX cosmetic enhancements.

## What is ScrollFX?

ScrollFX combines [Scroll](github.com/dawsers/scroll)'s innovative workspace 
scrolling and animation system with [SceneFX](github.com/wlrfx/scenefx)'s 
stunning visual effects.

## Features

- All Scroll features preserved:
  - Workspace scrolling and layout
  - Smooth Bezier curve animations
  - Workspace scaling (overview mode)
  - Per-window content scaling
  
- New SceneFX effects:
  - Backdrop blur
  - Window shadows
  - Corner radius
  - Window dimming

## Quick Start

[Installation and basic config]

## Configuration

[Point to CONFIG-REFERENCE.md]
```

**CONFIG-REFERENCE.md Structure:**
```markdown
# ScrollFX Configuration Reference

## Blur Effects

### blur [on|off]
Enable/disable blur globally or per-container.

**Scope:** Global or for_window
**Default:** off
**Example:**
```
blur on
for_window [app_id="kitty"] blur on
```

[Continue for all options...]
```

**ARCHITECTURE.md Structure:**
```markdown
# ScrollFX Architecture

## Overview

ScrollFX maintains two parallel scene graph systems:
- Scroll's `sway_scene_*` for layout and animations
- SceneFX's `wlr_scene_*` for visual effects

## Effect Application Pipeline

1. Configuration loaded
2. Containers created with effect properties
3. output_configure_scene() applies effects during rendering
4. Effects persist through transactions

## Scroll-Specific Integration

### Workspace Scrolling
Effects are applied per-frame, allowing shadows and blur
to follow containers during scrolling.

[Continue with details...]
```

## Code Style Consistency

**Naming Conventions:**
- Match Scroll's existing patterns
- SceneFX functions keep `wlr_scene_*` prefix
- Helper functions follow Scroll style: `container_has_shadow()`

**Comment Style:**
```c
// Single-line comments for brief explanations

/* Multi-line comments for complex explanations,
 * function documentation, and important notes.
 */
```

**Error Handling:**
```c
// Good: Helpful error message
sway_log(SWAY_ERROR, "Failed to allocate shadow node for container '%s'",
    con->title);

// Bad: Generic error
sway_log(SWAY_ERROR, "Allocation failed");
```

## Testing Documentation

Create `TESTING.md`:
```markdown
# Testing ScrollFX

## Building
[Build instructions]

## Basic Testing
1. Test blur: `blur on`
2. Test shadows: `shadows on`
3. Test corner radius: `corner_radius 10`

## Scroll Feature Testing
1. Workspace scrolling with effects
2. Animations with effects
3. Workspace scaling with effects
4. Multi-output with effects

## Performance Testing
[Performance testing procedures]
```

## Deliverables
1. `README-SCROLLFX.md` - User-facing overview
2. `CONFIG-REFERENCE.md` - Complete config documentation
3. `ARCHITECTURE.md` - Developer documentation
4. `PORTING-GUIDE.md` - How we did the port
5. `TESTING.md` - Testing procedures
6. `CHANGELOG.md` - Release notes
7. `KNOWN-ISSUES.md` - Known issues
8. Example config files (3+)
9. Code consistency improvements
10. PR with final report at `/reports/agent7-documentation-polish.md`

## Success Criteria
- All documentation complete and accurate
- Examples actually work
- Code style consistent
- No TODOs or FIXMEs remaining
- Error messages helpful
- Ready for user review
- Professional quality deliverable

## Notes
- You are the final quality gate
- Focus on user experience
- Documentation should be beginner-friendly
- Examples should be copy-pasteable
- This is what users will first see - make it good
