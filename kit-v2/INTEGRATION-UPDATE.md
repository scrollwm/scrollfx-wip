# Integration Documentation - Updated Version

## What Changed

A new review identified a critical clarification about transaction.c integration:

### ❌ OLD (Incorrect) Approach
- Extract code from Agent 3's report
- Find where functions might be in Scroll
- Try to replace or merge entire transaction.c file

### ✅ NEW (Correct) Approach  
- **Add 3 specific code blocks** to Scroll's existing transaction.c
- **Do NOT replace** the entire file
- Preserves Scroll's unique animation and scrolling logic

## Key Insight

The agent3 output contains a **SwayFX-based transaction.c** that:
- Has the shadow/blur code we need ✅
- BUT lacks Scroll's animation system ❌
- AND lacks Scroll's workspace scrolling ❌
- Would break Scroll if used as replacement ❌

**Solution**: Only add the SceneFX-specific code blocks, leave Scroll's code intact.

## Updated Documentation

### New Primary Guide
**[TRANSACTION-INTEGRATION-GUIDE.md](TRANSACTION-INTEGRATION-GUIDE.md)**
- Shows exact code to add
- Shows exact locations in functions
- Includes before/after examples
- Explains why each code block is needed
- Takes 30 min to 1 hour (not 2+ hours)

### Replaced Guide
**TRANSACTION-EXTRACTION-GUIDE-OLD.md** (backed up)
- Old version was less precise
- New version is surgical and clear

### Updated Files
All documentation now points to the new guide:
- ✅ README.md
- ✅ SIMPLE-INTEGRATION-GUIDE.md
- ✅ integrate-scrollfx.sh

## The 3 Code Blocks to Add

Quick reference (see full guide for complete code):

### 1. In `arrange_container()`
Location: Right before `view_reconfigure(con->view);`
Purpose: Shadow management - size, position, color, corner radius

### 2. In `arrange_output()`
Location: With other layer enable/disable calls
Purpose: Disable blur during workspace fullscreen
```c
wlr_scene_node_set_enabled(&output->layers.blur_layer->node, !fs);
```

### 3. In `arrange_root()`
Location: Two parts:
- Part A: Blur tree toggle with other layers
- Part B: Blur layer reparenting loop

## Why This Matters

**Before this update**: You might have tried to:
- Replace Scroll's transaction.c entirely (breaks Scroll)
- Search for functions in wrong files
- Spend hours trying to merge two different files

**After this update**: You will:
- Open Scroll's existing transaction.c
- Add 3 clearly defined code blocks
- Keep all Scroll functionality working
- Finish in 30 minutes

## Integration Workflow (Updated)

### Step 1: Run Automated Script (5 minutes)
```bash
./integrate-scrollfx.sh ~/scroll ~/scrollwm-scrollfx-wip
```
Handles Agents 1-5 file copying.

### Step 2: Edit meson.build Files (15 minutes)
Add SceneFX dependency and new source files.

### Step 3: Edit transaction.c (30 minutes) ⭐ **UPDATED**
Open `sway/desktop/transaction.c` in Scroll.
Add the 3 code blocks from TRANSACTION-INTEGRATION-GUIDE.md.

### Step 4: Build & Test (30 minutes)
```bash
meson setup build
ninja -C build
./build/sway/sway --version
```

**Total time: 1-2 hours** (down from 4-6 hours)

## Verification

After integration, you should have:

```bash
# Scroll's transaction.c with additions
grep "Shadow management" sway/desktop/transaction.c
grep "blur_layer" sway/desktop/transaction.c
grep "Reparent output blur layers" sway/desktop/transaction.c
```

All three should return matches.

## What This Preserves in Scroll

By NOT replacing transaction.c, we keep:
- ✅ Scroll's animation transaction system
- ✅ Scroll's workspace scrolling logic  
- ✅ Scroll's gesture handling
- ✅ Scroll's custom timing and scheduling
- ✅ Any other Scroll-specific enhancements

We only ADD:
- ➕ Shadow management during arrangements
- ➕ Blur layer control during fullscreen
- ➕ Blur tree management at root level

## Files in This Integration Kit

**Primary Guides** (Use these):
- README.md - Overview and index
- SIMPLE-INTEGRATION-GUIDE.md - Step-by-step integration
- TRANSACTION-INTEGRATION-GUIDE.md - ⭐ New precise guide
- integrate-scrollfx.sh - Automated script

**Reference Guides**:
- meson-build-guide.md - Build system details
- TRANSACTION-EXTRACTION-GUIDE-OLD.md - Old version (kept for reference)

**Deprecated** (ignore):
- integration-script.sh - Old version
- merge-plan.md - Based on incorrect assumptions
- critical-issues.md - Solved by new guide

## Quick Start (Updated)

```bash
# 1. Get the integration kit
tar xzf scrollfx-integration-kit.tar.gz

# 2. Read the overview
cat README.md

# 3. Run automated integration
./integrate-scrollfx.sh ~/scroll ~/scrollwm-scrollfx-wip

# 4. Follow manual steps (script will tell you)
#    - Edit 2 meson.build files
#    - Add 3 code blocks to transaction.c

# 5. Build
cd ~/scroll
meson setup build
ninja -C build

# 6. Test
./build/sway/sway --version
```

## Questions?

The new [TRANSACTION-INTEGRATION-GUIDE.md](TRANSACTION-INTEGRATION-GUIDE.md) includes:
- Prerequisites checklist
- Exact code with comments
- Before/after examples
- Troubleshooting section
- Validation steps
- Runtime testing

**If you hit issues**, the guide has solutions for:
- Missing fields compile errors
- NULL pointer crashes
- Functions in different files
- Blur not working during fullscreen

---

**Bottom Line**: Don't replace transaction.c, just add 3 code blocks. It's surgical, not wholesale replacement. This makes integration much simpler and faster.
