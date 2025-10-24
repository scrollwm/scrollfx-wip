# ScrollFX Integration - Documentation Index

This directory contains all the guides and tools needed to integrate your ScrollFX work (agents 1-5) into the Scroll window manager.

## 🚀 Quick Start

**You want this file**: [`SIMPLE-INTEGRATION-GUIDE.md`](SIMPLE-INTEGRATION-GUIDE.md)

**You want this script**: [`integrate-scrollfx.sh`](integrate-scrollfx.sh)

### Running the Integration

```bash
# 1. Use the automated script for file copying
./integrate-scrollfx.sh ~/scroll ~/scrollwm-scrollfx-wip

# 2. Follow the manual steps it prints out for:
#    - meson.build modifications
#    - transaction code integration

# 3. Build
cd ~/scroll
meson setup build
ninja -C build
```

## 📚 Document Guide

### Primary Documents (Start Here)

1. **[SIMPLE-INTEGRATION-GUIDE.md](SIMPLE-INTEGRATION-GUIDE.md)** ⭐ **START HERE**
   - Simplified, correct approach
   - Direct file replacement (no merging)
   - Step-by-step instructions
   - Build system integration
   - Testing procedures

2. **[integrate-scrollfx.sh](integrate-scrollfx.sh)** 🤖 **AUTOMATION**
   - Automated script for file copying
   - Handles all 5 agents
   - Creates git commits
   - Prints manual steps at end

3. **[TRANSACTION-INTEGRATION-GUIDE.md](TRANSACTION-INTEGRATION-GUIDE.md)** ⚠️ **CRITICAL MANUAL STEP**
   - Shows exactly what to add to Scroll's transaction.c
   - Three specific code blocks with precise locations
   - Required after automated integration
   - **DO NOT replace entire file - only add these blocks**

### Reference Documents

4. **[INTEGRATION-ACTION-PLAN.md](INTEGRATION-ACTION-PLAN.md)**
   - Earlier, more complex version
   - Kept for reference
   - Includes detailed issue analysis

5. **[meson-build-guide.md](meson-build-guide.md)**
   - Detailed meson.build modification guide
   - SceneFX dependency setup
   - Build troubleshooting

6. **[critical-issues.md](critical-issues.md)**
   - Initial problem analysis
   - Still useful for understanding issues

### Deprecated Documents

7. **integration-script.sh** - Older version, use `integrate-scrollfx.sh` instead
8. **merge-plan.md** - Based on incorrect merging assumption

## 🎯 Integration Workflow

### Phase 1: Automated File Copy (5 minutes)
```bash
./integrate-scrollfx.sh <scroll-dir> <wip-dir>
```

This handles:
- ✅ Agent 1: Config and commands (26 files)
- ✅ Agent 2: Container and tree (8 files)
- ✅ Agent 3: Scene and output (3 files)
- ✅ Agent 4: Layer shell (5 files)
- ✅ Agent 5: Input feedback (3 files)
- ✅ SceneFX subproject setup

### Phase 2: Manual Build System (15 minutes)

Edit two files:

1. **Root `meson.build`**:
   - Add SceneFX dependency
   - Add to sway_deps array

2. **`sway/meson.build`**:
   - Add 24 command files
   - Add layer_criteria.c
   - Add tree/scene/scene.c

See [SIMPLE-INTEGRATION-GUIDE.md](SIMPLE-INTEGRATION-GUIDE.md) Section 6 for exact code.

### Phase 3: Transaction Code (30 minutes - 1 hour)

Add shadow and blur management to Scroll's transaction.c:
- See [TRANSACTION-INTEGRATION-GUIDE.md](TRANSACTION-INTEGRATION-GUIDE.md)
- Add 3 specific code blocks to existing functions
- **Do NOT replace the entire file**
- Add to: `arrange_container()`, `arrange_output()`, `arrange_root()`

### Phase 4: Build & Test (30 minutes)

```bash
cd scroll
rm -rf build
meson setup build
ninja -C build
./build/sway/sway --version
```

## 🐛 Troubleshooting

### Build fails with "SceneFX not found"
```bash
cd scroll
meson subprojects update
```

### Build fails with undefined references
Check that `scenefx` is in the `sway_deps` array in root `meson.build`

### Build fails with missing files
Verify all agent files were copied:
```bash
ls sway/commands/blur.c
ls sway/tree/scene/scene.c
ls include/sway/tree/container.h
```

### Visual effects don't work
1. Check transaction code was integrated
2. Verify SceneFX is actually linked: `ldd build/sway/sway | grep scenefx`
3. Check config file syntax

## 📋 Integration Checklist

Before starting:
- [ ] Have Scroll repository cloned
- [ ] Have WIP directory with all agent outputs
- [ ] Read SIMPLE-INTEGRATION-GUIDE.md

After automated script:
- [ ] All 5 agent commits created
- [ ] No git errors during copy
- [ ] subprojects/scenefx.wrap exists

After manual edits:
- [ ] Root meson.build has SceneFX dependency
- [ ] sway/meson.build has all new files listed
- [ ] Transaction code identified and applied

After build:
- [ ] `meson setup build` succeeds
- [ ] `ninja -C build` succeeds
- [ ] `./build/sway/sway --version` runs
- [ ] Config file with SceneFX commands parses
- [ ] Visual effects render correctly

## 🎨 Testing Your Build

### Config File Test
Create `/tmp/test-scrollfx.conf`:
```
blur enable
blur_radius 5
shadows enable
shadow_blur_radius 20
corner_radius 10

for_window [app_id="firefox"] opacity 0.95
layer_effects waybar "blur enable; shadows enable"
```

Test: `./build/sway/sway -C -c /tmp/test-scrollfx.conf`

### Runtime Test
In a running session:
```bash
scrollmsg blur enable
scrollmsg shadows enable
scrollmsg corner_radius 15

# Move windows - should see:
# - Rounded corners
# - Shadows following windows
# - Blur on backgrounds
# - Smooth Scroll animations
```

## 📞 Getting Help

If you encounter issues:

1. **Check agent reports**: Each has detailed implementation notes
   - `agent1/reports/agent1-configuration-commands.md`
   - `agent2/reports/agent2-container-tree.md`
   - `agent3/reports/agent3-scene-rendering.md`
   - `agent4/reports/agent4-layer-desktop.md`
   - `agent5/reports/agent5-input-feedback.md`

2. **Check build logs**:
   ```bash
   meson setup build 2>&1 | tee meson.log
   ninja -C build 2>&1 | tee build.log
   ```

3. **Check git history**:
   ```bash
   git log --oneline --graph
   git show <commit-hash>  # See what each agent added
   ```

## 🎉 Success Criteria

You've succeeded when:
- ✅ Build completes without errors
- ✅ Sway binary runs: `./build/sway/sway --version`
- ✅ Config with SceneFX commands loads
- ✅ `blur enable` works
- ✅ `shadows enable` works  
- ✅ `corner_radius 10` works
- ✅ Effects persist during Scroll's workspace animations
- ✅ No crashes during normal usage

## 📝 Notes

- **Direct Replacement**: Agent files are complete replacements, not patches
- **No Merging Needed**: Just copy files over (except transaction.c)
- **Transaction Code**: Only manual step - must extract from Agent 3 report
- **Build System**: Two meson.build files need manual editing
- **Time Estimate**: 2-4 hours total including build and testing

## 🔗 External Resources

- [SceneFX Repository](https://github.com/wlrfx/scenefx)
- [Scroll Window Manager](https://github.com/dawsers/scroll)
- [SwayFX Reference](https://github.com/WillPower3309/swayfx)

---

**Last Updated**: 2025-10-24
**Integration Method**: Direct file replacement
**Automation Level**: ~80% (files), 20% manual (build system + transaction code)
