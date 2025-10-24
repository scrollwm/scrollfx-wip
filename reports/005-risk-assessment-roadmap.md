# Risk Assessment and Implementation Roadmap
## ScrollFX Migration: Go/No-Go Analysis and Execution Plan

**Report Date**: 2025-10-24
**Decision**: **GO** (with conditions)
**Confidence**: High (85%)
**Timeline**: 10-14 working days

---

## Executive Risk Assessment

### Overall Risk Score: **6.5/10** (Medium-High)

**Risk Breakdown**:
- **Technical Feasibility**: ✅ 8/10 (High) - APIs compatible, changes well-documented
- **Implementation Complexity**: ⚠️ 7/10 (High) - Large scope, many touch points
- **Integration Risk**: ⚠️ 6/10 (Medium-High) - Transaction system interaction
- **Testing Burden**: ⚠️ 7/10 (High) - Many combinations to test
- **Maintenance Risk**: ✅ 5/10 (Medium) - Clean API, should track well

### Go/No-Go Recommendation: **GO**

**Reasoning**:
1. ✅ **No fundamental blockers**: All API changes are mechanical translations
2. ✅ **Well-documented changes**: Scroll's commits clearly show what changed
3. ✅ **Compatible architectures**: SceneFX's approach works with Scroll's scene fork
4. ⚠️ **High effort but bounded**: 57-82 hours is significant but manageable
5. ✅ **Clear migration path**: Step-by-step guide created
6. ⚠️ **Testing critical**: Must validate with Scroll's unique features

**Conditions for Success**:
1. Dedicated focus period (no context switching)
2. Access to multi-monitor setup for testing
3. Methodical approach (follow phases strictly)
4. Comprehensive testing at each phase
5. Ability to iterate if issues found

---

## Part 1: Critical Risks (Must Mitigate)

### Risk 1: Type Conversion Errors
**Likelihood**: High | **Impact**: High | **Overall**: ⚠️ **CRITICAL**

**Description**:
Missing int↔double conversions causing:
- Truncation errors
- Precision loss
- Incorrect positioning
- Broken effect calculations

**Mitigation**:
1. Use compiler warnings: `-Wconversion`, `-Wfloat-conversion`
2. Global search for all position/size variables
3. Code review all arithmetic operations
4. Unit test critical calculations
5. Visual inspection at all scales

**Contingency**:
- If errors found late, may need additional 8-16 hours for fixes
- Keep detailed log of all type changes made

---

### Risk 2: Transaction Integrity Failure
**Likelihood**: Medium | **Impact**: Critical | **Overall**: ⚠️ **HIGH**

**Description**:
Effects flicker/reset during Scroll's animations because:
- Effect properties not saved in transaction
- Shadow nodes not properly linked
- Blur layers not updated during animations
- Dim rects lost during commits

**Mitigation**:
1. **Phase 3 dedicated to transaction handling** (see roadmap)
2. Study commit e50b16a6 pattern thoroughly
3. Add all effect properties to save/restore code
4. Test EVERY animation type early
5. Use git bisect if regressions occur

**Contingency**:
- If major issues found, may need 12-24 hours additional for transaction debugging
- May need to simplify some effects temporarily

---

### Risk 3: Opacity Conflict
**Likelihood**: Medium | **Impact**: High | **Overall**: ⚠️ **HIGH**

**Description**:
SceneFX's dim_inactive uses buffer opacity. Scroll also uses opacity. Conflict scenarios:
- SceneFX overwrites Scroll's opacity
- Scroll overwrites SceneFX's dim
- Multiplicative opacity not handled correctly

**Mitigation**:
1. **Always read current opacity before setting**
2. Use multiplicative approach: `new = current * dim_factor`
3. Track which component set opacity (flags/comments)
4. Test opacity interactions thoroughly
5. Consider separate dim mechanism if needed

**Contingency**:
- May need to implement separate dim as overlay rect instead of opacity
- Additional 4-8 hours if redesign needed

---

### Risk 4: wlr_box Integer Conversion
**Likelihood**: Medium | **Impact**: Medium | **Overall**: ⚠️ **MEDIUM-HIGH**

**Description**:
Scroll uses double for positions/sizes internally, but `wlr_box` (used for clipping) still uses int. Conversions required at boundaries.

**Mitigation**:
1. Establish consistent rounding policy (round, floor, ceil?)
2. Use helper functions for conversions
3. Document every wlr_box conversion site
4. Test clipping at fractional positions

**Example Helper**:
```c
static inline struct wlr_box double_box_to_wlr_box(
    double x, double y, double width, double height
) {
    return (struct wlr_box){
        .x = (int)round(x),
        .y = (int)round(y),
        .width = (int)round(width),
        .height = (int)round(height),
    };
}
```

**Contingency**:
- May see 1-pixel clipping errors at fractional scales
- Acceptable for v1, can refine later

---

## Part 2: Medium Risks (Monitor Closely)

### Risk 5: Color Management Conflicts
**Likelihood**: Low | **Impact**: High | **Overall**: 🟡 **MEDIUM**

**Description**:
Scroll added color management. SceneFX effects use hardcoded RGBA. May render incorrectly on HDR outputs.

**Mitigation**:
1. Test on HDR output if available
2. Research if effect shaders need color space awareness
3. Start with sRGB assumption
4. Add TODO for HDR support

**Contingency**:
- HDR support can be deferred to v2
- Document as known limitation

---

### Risk 6: Performance Regression
**Likelihood**: Medium | **Impact**: Medium | **Overall**: 🟡 **MEDIUM**

**Description**:
Double precision math slightly slower than int/float. Many calculations per frame.

**Mitigation**:
1. Profile before/after
2. Use `float` for effect shaders (GPU side)
3. Only use `double` for CPU-side positioning
4. Optimize hot paths if needed

**Contingency**:
- Unlikely to be significant
- Modern CPUs handle double well
- Can optimize specific functions if profiling shows issues

---

### Risk 7: Scale Combination Edge Cases
**Likelihood**: High | **Impact**: Low | **Overall**: 🟡 **MEDIUM**

**Description**:
Scroll has 3 types of scaling: output, workspace, content. Some combinations may reveal edge cases in effect rendering.

**Mitigation**:
1. Comprehensive test matrix (see Part 7)
2. Test extreme combinations early
3. Document which combinations are tested/supported
4. Add integration tests

**Contingency**:
- Some exotic combinations may have minor issues
- Can document as known limitations for v1

---

## Part 3: Low Risks (Accept)

### Risk 8: SceneFX Node Type Enum Conflicts
**Likelihood**: Low | **Impact**: High | **Overall**: 🟢 **LOW**

**Description**:
SceneFX adds custom node types (SHADOW, OPTIMIZED_BLUR) to scene node enum. If Scroll added types, values might conflict.

**Analysis**:
✅ Scroll didn't add new node types (only TREE, RECT, BUFFER)
✅ Safe to add SceneFX types

**No mitigation needed** - not a real risk

---

### Risk 9: Custom Scene Functions
**Likelihood**: Low | **Impact**: Low | **Overall**: 🟢 **LOW**

**Description**:
SceneFX adds custom scene functions. Names might conflict with Scroll additions.

**Mitigation**:
- Use unique prefixes: `scenefx_*` or `sway_scene_fx_*`
- Check for naming conflicts during integration

**Contingency**:
- Rename if needed (trivial)

---

## Part 4: Implementation Phases (Roadmap)

### Phase 1: Foundation (Days 1-3) - 24 hours
**Goal**: Get SceneFX scene code migrated and compiling

**Tasks**:
1. ✅ **Mechanical replacements** (8 hours)
   - `wlr_scene_*` → `sway_scene_*`
   - `int x, y, w, h` → `double`
   - `float scale` → `double scale`
   - `node->scale` → `node->info.scale`
   - `node->wlr_output` → `node->info.wlr_output`

2. ✅ **Function signature updates** (6 hours)
   - All position/size APIs
   - Scene node creation
   - Effect setters

3. ✅ **Structure field updates** (4 hours)
   - Add SceneFX fields to Scroll structures
   - Update container, config, etc.

4. ✅ **Compilation fixes** (6 hours)
   - Fix all type errors
   - Fix all undefined references
   - Get clean compile

**Success Criteria**:
- ✅ Code compiles without errors
- ✅ All SceneFX scene types recognized
- ✅ Basic scene graph intact (no effects yet)

**Risks**:
- May find unexpected API mismatches (+4 hours)
- Struct size changes may affect serialization (+2 hours)

**Deliverable**: Compiling but non-functional ScrollFX

---

### Phase 2: Core Effects (Days 4-6) - 24 hours
**Goal**: Get basic effects working (no animations yet)

**Tasks**:
1. ✅ **Blur implementation** (6 hours)
   - Port blur data structures
   - Port blur shaders
   - Integrate optimized blur layer
   - Test basic blur

2. ✅ **Shadow implementation** (6 hours)
   - Port shadow node creation
   - Port shadow management
   - Hook into container lifecycle
   - Test basic shadows

3. ✅ **Corner radius** (4 hours)
   - Port corner radius logic
   - Buffer and rect corner radius
   - Smart corner radius

4. ✅ **Dim inactive** (4 hours)
   - Port dim rect creation
   - **Implement opacity multiplication** (critical!)
   - Color switching (focused/unfocused/urgent)

5. ✅ **Basic testing** (4 hours)
   - Each effect individually
   - Effect combinations
   - Visual inspection

**Success Criteria**:
- ✅ Blur visible and correct
- ✅ Shadows appear with correct offset/color
- ✅ Corner radius renders properly
- ✅ Dim works without opacity conflicts
- ✅ No crashes or visual corruption

**Risks**:
- Blur shader incompatibilities (+4 hours)
- Shadow clipping math errors (+4 hours)
- Opacity conflicts requiring redesign (+8 hours)

**Deliverable**: Static effects working (without animations)

---

### Phase 3: Transaction Integration (Days 7-8) - 16 hours
**Goal**: Effects survive Scroll's animations

**Tasks**:
1. ✅ **Save/restore implementation** (6 hours)
   - Study commit e50b16a6 pattern
   - Add all effect properties to save
   - Implement in view_save_buffer_iterator
   - Implement restore logic

2. ✅ **Shadow persistence** (3 hours)
   - Ensure shadows linked to containers
   - Shadow updates during transactions
   - Shadow clipping updates

3. ✅ **Blur layer persistence** (3 hours)
   - Blur layer resize during transactions
   - Optimized blur dirty marking
   - Blur region updates

4. ✅ **Animation testing** (4 hours)
   - Test window movement
   - Test window resizing
   - Test workspace switching
   - Test focus changes
   - Fix any flickering

**Success Criteria**:
- ✅ No effect flickering during animations
- ✅ Shadows move with windows
- ✅ Blur regions resize correctly
- ✅ Dim persists through focus changes

**Risks**:
- Transaction save/restore is complex (+8 hours if major issues)
- Animation timing issues (+4 hours)
- State synchronization bugs (+6 hours)

**Deliverable**: Effects persist through all animations

---

### Phase 4: Scroll Integration (Days 9-10) - 16 hours
**Goal**: Effects work with Scroll's unique features

**Tasks**:
1. ✅ **Workspace scaling** (4 hours)
   - Effects scale with workspace
   - Overview mode testing
   - Scaled workspace interaction

2. ✅ **Content scaling** (4 hours)
   - Effects match scaled content
   - Shadow sizes correct
   - Blur regions correct

3. ✅ **Layer shell effects** (3 hours)
   - Layer criteria application
   - Background layer handling
   - Panel/notification effects

4. ✅ **Scroll-specific commands** (3 hours)
   - Commands parse correctly
   - IPC integration
   - Config file support

5. ✅ **Testing** (2 hours)
   - All Scroll features
   - Scale combinations
   - Layout modes

**Success Criteria**:
- ✅ Effects scale correctly with workspace
- ✅ Effects work in overview mode
- ✅ Content scaling doesn't break effects
- ✅ Layer shell effects work
- ✅ Commands functional

**Risks**:
- Workspace scale math errors (+4 hours)
- Content scale conflicts (+4 hours)
- Command parsing issues (+2 hours)

**Deliverable**: Full ScrollFX feature set working

---

### Phase 5: Testing & Polish (Days 11-14) - 32 hours
**Goal**: Production-ready quality

**Tasks**:
1. ✅ **Comprehensive scale testing** (8 hours)
   - Output: 1.0, 1.25, 1.5, 1.75, 2.0
   - Workspace: 0.5, 0.75, 1.0, 1.25, 1.5
   - Content: 0.5, 1.0, 1.5, 2.0
   - Combined scenarios

2. ✅ **Effect combination testing** (6 hours)
   - All effect combinations
   - Edge cases
   - Stress testing

3. ✅ **Multi-output testing** (4 hours)
   - Different scales per output
   - Output hotplug
   - Mixed fractional scales

4. ✅ **Performance profiling** (4 hours)
   - FPS measurement
   - CPU usage
   - GPU usage
   - Memory leaks

5. ✅ **Bug fixes** (8 hours)
   - Fix issues found in testing
   - Edge case handling
   - Polish visual glitches

6. ✅ **Documentation** (2 hours)
   - User documentation
   - Configuration examples
   - Known limitations

**Success Criteria**:
- ✅ All test cases pass
- ✅ Performance acceptable (>= 60 FPS typical)
- ✅ No visual glitches
- ✅ No memory leaks
- ✅ Multi-output stable
- ✅ Documentation complete

**Risks**:
- Hidden bugs require deep debugging (+16 hours)
- Performance issues need optimization (+8 hours)
- Edge cases require design changes (+12 hours)

**Deliverable**: Production-ready ScrollFX

---

## Part 5: Resource Requirements

### Human Resources:
- **1 senior developer** with:
  - Strong C programming
  - Wayland/wlroots knowledge
  - Scene graph experience
  - Shader programming (for blur)
  - Git/debugging skills

### Hardware Requirements:
- Linux workstation
- Multiple monitors (different scales)
- HDR monitor (nice to have)
- GPU with Wayland support

### Software Requirements:
- Scroll development environment
- SceneFX source code
- wlroots 0.20dev
- Debug tools (gdb, valgrind)
- Profiling tools (perf, flamegraph)

---

## Part 6: Critical Success Factors

### Must Have:
1. ✅ **Methodical approach**: Follow phases strictly, no shortcuts
2. ✅ **Transaction integrity**: Save/restore must be perfect
3. ✅ **Type discipline**: All int/float→double conversions correct
4. ✅ **Testing rigor**: Every scale combination tested
5. ✅ **Code review**: Second pair of eyes on critical sections

### Nice to Have:
1. 💡 **Automated tests**: Reduce regression risk
2. 💡 **CI/CD pipeline**: Catch issues early
3. 💡 **Performance benchmarks**: Track performance over time
4. 💡 **HDR validation**: Future-proof the implementation

---

## Part 7: Test Matrix

### Scale Combinations (Priority: Critical):

| Output Scale | Workspace Scale | Content Scale | Test Status |
|-------------|----------------|---------------|-------------|
| 1.0 | 1.0 | 1.0 | ⬜ Required |
| 1.5 | 1.0 | 1.0 | ⬜ Required |
| 2.0 | 1.0 | 1.0 | ⬜ Required |
| 1.0 | 0.5 | 1.0 | ⬜ Required |
| 1.0 | 1.5 | 1.0 | ⬜ Required |
| 1.0 | 1.0 | 0.5 | ⬜ Required |
| 1.0 | 1.0 | 2.0 | ⬜ Required |
| 1.5 | 0.75 | 1.5 | ⬜ Required |
| 2.0 | 0.5 | 2.0 | ⬜ Edge case |

### Animation Types (Priority: Critical):

| Animation | Effect Persistence | Test Status |
|-----------|-------------------|-------------|
| Window move | Shadows move | ⬜ Required |
| Window resize | Effects resize | ⬜ Required |
| Focus change | Dim updates | ⬜ Required |
| Workspace scale | Effects scale | ⬜ Required |
| Overview mode | Effects visible | ⬜ Required |
| Workspace switch | No flicker | ⬜ Required |

### Effect Combinations (Priority: High):

| Combination | Expected Result | Test Status |
|------------|----------------|-------------|
| Blur + Shadow | Both render | ⬜ Required |
| Blur + Corner | Rounded blur | ⬜ Required |
| Shadow + Corner | Rounded shadow | ⬜ Required |
| Dim + Blur + Shadow | All 3 work | ⬜ Required |
| Opacity + Corner | Correct alpha | ⬜ Required |

### Multi-Output (Priority: Medium):

| Scenario | Expected Result | Test Status |
|---------|----------------|-------------|
| 2 outputs, same scale | Consistent | ⬜ Required |
| 2 outputs, different scale | Correct each | ⬜ Required |
| Output hotplug | No crash | ⬜ Required |
| Mixed int/fractional | Correct each | ⬜ Nice to have |

---

## Part 8: Rollback Plan

### If Migration Fails:

**Decision Point 1** (End of Phase 2):
- **Criteria**: Core effects not working after 30 hours
- **Action**: Pause, reassess approach
- **Options**:
  1. Continue with extended timeline (+16 hours)
  2. Simplify scope (drop some effects)
  3. Abort migration

**Decision Point 2** (End of Phase 3):
- **Criteria**: Transactions fundamentally broken after 40 hours
- **Action**: Major reassessment
- **Options**:
  1. Redesign transaction handling (+24 hours)
  2. Disable animations (not acceptable for Scroll)
  3. Abort migration

**Abort Criteria**:
- Technical blocker discovered (fundamental incompatibility)
- Timeline exceeds 120 hours (15 days)
- Performance unacceptable (<30 FPS)
- Instability persists (crashes, corruption)

**Abort Procedure**:
1. Document findings thoroughly
2. Create issue with blockers identified
3. Revert all changes
4. Recommend alternative approaches:
   - Wait for upstream SceneFX update
   - Implement minimal effects in Scroll
   - Fork SwayFX instead of migrating

---

## Part 9: Post-Migration Maintenance

### Expected Ongoing Effort:

**Tracking Scroll updates**:
- **Frequency**: Monthly review of Scroll commits
- **Effort**: 2-4 hours/month
- **Focus**: Scene graph API changes

**Tracking SceneFX updates**:
- **Frequency**: Quarterly review of SwayFX/SceneFX
- **Effort**: 4-8 hours/quarter
- **Focus**: New effects, optimizations

**Bug fixes**:
- **Expected**: 4-8 hours/month initially
- **Stabilizing**: 2-4 hours/month after 3 months

### Long-term Risks:

1. **Scroll diverges further from Sway**: Migration becomes harder
2. **SceneFX major refactor**: May need re-migration
3. **wlroots scene API changes**: Both need updates

### Mitigation:

1. **Automated tests**: Catch regressions early
2. **CI integration**: Test on every commit
3. **Documentation**: Maintainable codebase
4. **Community**: Engage with Scroll/SceneFX developers

---

## Part 10: Alternative Approaches (If GO Fails)

### Option A: Minimal Effects in Scroll
**Effort**: 20-30 hours
**Scope**: Corner radius + shadows only (no blur)
**Pros**: Lower complexity, easier maintenance
**Cons**: Not full SceneFX feature set

### Option B: Fork SwayFX Instead
**Effort**: 40-60 hours
**Scope**: Port Scroll features to SwayFX base
**Pros**: SceneFX already integrated
**Cons**: Lose Scroll's architecture, harder merge

### Option C: Wait for Upstream
**Effort**: 0 hours
**Scope**: Wait for SceneFX to support wlroots 0.20
**Pros**: No effort, proper upstream support
**Cons**: Indefinite timeline

### Option D: Hybrid Approach
**Effort**: 50-70 hours
**Scope**: Basic effects in Scroll + extended via plugins
**Pros**: Flexible, maintainable
**Cons**: Requires plugin system design

---

## Conclusion

### Recommendation: **GO** with migration

**Confidence**: 85%

**Rationale**:
1. ✅ Technical feasibility confirmed
2. ✅ Clear migration path identified
3. ✅ Risks identified and mitigated
4. ✅ Timeline reasonable (10-14 days)
5. ✅ No fundamental blockers found
6. ✅ High value to Scroll users

**Key to Success**:
- **Phase discipline**: Complete each phase before moving on
- **Test early, test often**: Especially animations (Phase 3)
- **Transaction integrity**: Non-negotiable for Scroll
- **Type correctness**: Compiler warnings are your friend

**Expected Outcome**:
Production-ready ScrollFX with full SceneFX effects integrated into Scroll window manager, maintaining all Scroll features (animations, scaling, etc.)

---

**Report Status**: Risk assessment and roadmap complete
**Next Report**: Final executive summary
**Ready to Begin**: Phase 1 implementation can start immediately
