#!/bin/bash
# ScrollFX Integration Script - Direct Replacement Approach
# This script performs direct file replacement from agent outputs to Scroll

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Check arguments
if [ $# -ne 2 ]; then
    echo "Usage: $0 <scroll-repo-path> <wip-directory>"
    echo "Example: $0 ~/scroll ~/scrollwm-scrollfx-wip"
    exit 1
fi

SCROLL_DIR="$1"
WIP_DIR="$2"

# Validate directories
if [ ! -d "$SCROLL_DIR" ]; then
    echo -e "${RED}Error: Scroll directory not found: $SCROLL_DIR${NC}"
    exit 1
fi

if [ ! -d "$WIP_DIR" ]; then
    echo -e "${RED}Error: WIP directory not found: $WIP_DIR${NC}"
    exit 1
fi

if [ ! -d "$SCROLL_DIR/.git" ]; then
    echo -e "${RED}Error: Not a git repository: $SCROLL_DIR${NC}"
    exit 1
fi

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}ScrollFX Integration - Direct Replacement${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""
echo "Scroll directory: $SCROLL_DIR"
echo "WIP directory:    $WIP_DIR"
echo ""

cd "$SCROLL_DIR"

# Check if integration branch exists
if git rev-parse --verify scrollfx-integration >/dev/null 2>&1; then
    echo -e "${YELLOW}Branch 'scrollfx-integration' already exists${NC}"
    read -p "Switch to it and continue? (y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        git checkout scrollfx-integration
    else
        echo "Aborted"
        exit 1
    fi
else
    echo -e "${GREEN}Creating integration branch...${NC}"
    git checkout -b scrollfx-integration
fi

# Function to copy and report
copy_file() {
    local src="$1"
    local dst="$2"
    if [ -f "$src" ]; then
        cp "$src" "$dst"
        echo -e "  ${GREEN}✓${NC} Copied: $(basename "$dst")"
    else
        echo -e "  ${RED}✗${NC} Missing: $src"
        return 1
    fi
}

# Function to copy directory
copy_dir() {
    local src="$1"
    local dst="$2"
    if [ -d "$src" ]; then
        cp -r "$src"/* "$dst"/ 2>/dev/null || true
        echo -e "  ${GREEN}✓${NC} Copied directory: $src -> $dst"
    else
        echo -e "  ${YELLOW}!${NC} Directory not found: $src"
    fi
}

#################################################
# AGENT 1: Configuration & Commands
#################################################
echo ""
echo -e "${BLUE}=== Agent 1: Configuration & Commands ===${NC}"

# Headers
echo "Copying headers..."
copy_file "$WIP_DIR/agent1/include/sway/config.h" "include/sway/config.h"
copy_file "$WIP_DIR/agent1/include/sway/commands.h" "include/sway/commands.h"
copy_file "$WIP_DIR/agent1/include/sway/layer_criteria.h" "include/sway/layer_criteria.h"

# Core files
echo "Copying core files..."
copy_file "$WIP_DIR/agent1/sway/commands.c" "sway/commands.c"
copy_file "$WIP_DIR/agent1/sway/config.c" "sway/config.c"

# Command files
echo "Copying command files..."
COMMANDS=(
    blur.c blur_brightness.c blur_contrast.c blur_noise.c
    blur_passes.c blur_radius.c blur_saturation.c blur_xray.c
    corner_radius.c smart_corner_radius.c
    default_dim_inactive.c dim_inactive.c dim_inactive_colors.c
    layer_effects.c opacity.c scratchpad_minimize.c
    shadow_blur_radius.c shadow_color.c shadow_inactive_color.c
    shadow_offset.c shadows.c shadows_on_csd.c titlebar_separator.c
)

for cmd in "${COMMANDS[@]}"; do
    copy_file "$WIP_DIR/agent1/sway/commands/$cmd" "sway/commands/$cmd"
done

# Commit
git add include/sway/config.h include/sway/commands.h include/sway/layer_criteria.h \
        sway/commands.c sway/config.c sway/commands/ 2>/dev/null
git commit -m "Agent 1: Configuration and commands system

- Add SceneFX configuration structures
- Implement 26 commands for blur, shadows, corner radius, dimming
- Add layer_criteria system
- Initialize defaults" 2>/dev/null || echo -e "${YELLOW}No changes to commit${NC}"

#################################################
# AGENT 2: Container & Tree Management
#################################################
echo ""
echo -e "${BLUE}=== Agent 2: Container & Tree Management ===${NC}"

# Headers
echo "Copying headers..."
copy_file "$WIP_DIR/agent2/include/sway/tree/container.h" "include/sway/tree/container.h"
copy_file "$WIP_DIR/agent2/include/sway/tree/node.h" "include/sway/tree/node.h"
copy_file "$WIP_DIR/agent2/include/sway/tree/root.h" "include/sway/tree/root.h"

# Source files
echo "Copying source files..."
copy_file "$WIP_DIR/agent2/sway/tree/container.c" "sway/tree/container.c"
copy_file "$WIP_DIR/agent2/sway/tree/node.c" "sway/tree/node.c"
copy_file "$WIP_DIR/agent2/sway/tree/arrange.c" "sway/tree/arrange.c"
copy_file "$WIP_DIR/agent2/sway/tree/root.c" "sway/tree/root.c"
copy_file "$WIP_DIR/agent2/sway/tree/view.c" "sway/tree/view.c"

# Commit
git add include/sway/tree/ sway/tree/ 2>/dev/null
git commit -m "Agent 2: Container and tree management

- Add SceneFX effect properties to containers
- Implement shadow allocation and management
- Add helper functions for effect eligibility
- Integrate with container lifecycle" 2>/dev/null || echo -e "${YELLOW}No changes to commit${NC}"

#################################################
# AGENT 3: Scene Graph & Output Rendering
#################################################
echo ""
echo -e "${BLUE}=== Agent 3: Scene Graph & Output Rendering ===${NC}"

# Headers
echo "Copying headers..."
copy_file "$WIP_DIR/agent3/include/sway/output.h" "include/sway/output.h"

# Source files
echo "Copying source files..."
copy_file "$WIP_DIR/agent3/sway/desktop/output.c" "sway/desktop/output.c"
copy_file "$WIP_DIR/agent3/sway/tree/output.c" "sway/tree/output.c"

# Note about transaction.c
echo ""
echo -e "${YELLOW}⚠️  IMPORTANT: transaction.c NOT copied${NC}"
echo -e "${YELLOW}   Scroll's transaction.c must NOT be replaced${NC}"
echo -e "${YELLOW}   You will add 3 code blocks manually${NC}"
echo -e "${YELLOW}   See: TRANSACTION-INTEGRATION-GUIDE.md${NC}"
echo ""

# Commit
git add include/sway/output.h sway/desktop/output.c sway/tree/output.c 2>/dev/null
git commit -m "Agent 3: Scene graph and output rendering

- Add blur layer to output structure
- Implement comprehensive scene configuration
- Integrate effect application pipeline

NOTE: Transaction shadow management pending manual integration" 2>/dev/null || echo -e "${YELLOW}No changes to commit${NC}"

#################################################
# AGENT 4: Layer Shell & Desktop Integration
#################################################
echo ""
echo -e "${BLUE}=== Agent 4: Layer Shell & Desktop Integration ===${NC}"

# Headers
echo "Copying headers..."
copy_file "$WIP_DIR/include/sway/layers.h" "include/sway/layers.h"

# Source files
echo "Copying source files..."
copy_file "$WIP_DIR/sway/layer_criteria.c" "sway/layer_criteria.c"
copy_file "$WIP_DIR/sway/desktop/layer_shell.c" "sway/desktop/layer_shell.c"
copy_file "$WIP_DIR/sway/desktop/xdg_shell.c" "sway/desktop/xdg_shell.c"
copy_file "$WIP_DIR/sway/desktop/xwayland.c" "sway/desktop/xwayland.c"

# Commit
git add include/sway/layers.h sway/layer_criteria.c sway/desktop/ 2>/dev/null
git commit -m "Agent 4: Layer shell and desktop integration

- Implement layer_criteria system for namespace-based effects
- Add shadow support for layer surfaces
- Implement scratchpad minimize for XDG and Xwayland
- Integrate effects with layer shell lifecycle" 2>/dev/null || echo -e "${YELLOW}No changes to commit${NC}"

#################################################
# AGENT 5: Input & Visual Feedback
#################################################
echo ""
echo -e "${BLUE}=== Agent 5: Input & Visual Feedback ===${NC}"

# Headers
echo "Copying headers..."
copy_file "$WIP_DIR/agent5/include/sway/tree/scene.h" "include/sway/tree/scene.h"

# Create scene directory and copy scene.c (NEW FILE)
echo "Creating sway/tree/scene/ directory..."
mkdir -p sway/tree/scene

if [ -f "$WIP_DIR/agent5/sway/tree/scene/scene.c" ]; then
    copy_file "$WIP_DIR/agent5/sway/tree/scene/scene.c" "sway/tree/scene/scene.c"
elif [ -f "$WIP_DIR/agent5/sway/tree/scene.c" ]; then
    copy_file "$WIP_DIR/agent5/sway/tree/scene.c" "sway/tree/scene/scene.c"
else
    echo -e "${RED}Error: scene.c not found in agent5 output${NC}"
fi

# Input modifications
echo "Copying input files..."
copy_file "$WIP_DIR/agent5/sway/input/seatop_move_tiling.c" "sway/input/seatop_move_tiling.c"

# Commit
git add include/sway/tree/scene.h sway/tree/scene/ sway/input/seatop_move_tiling.c 2>/dev/null
git commit -m "Agent 5: Input and visual feedback

- Add corner radius support to scene rects
- Add backdrop blur to move indicators
- Enhance visual feedback during window operations" 2>/dev/null || echo -e "${YELLOW}No changes to commit${NC}"

#################################################
# Build System Integration
#################################################
echo ""
echo -e "${BLUE}=== Build System Integration ===${NC}"

# Create subproject wrapper
echo "Creating SceneFX subproject..."
mkdir -p subprojects

cat > subprojects/scenefx.wrap << 'EOF'
[wrap-git]
url = https://github.com/wlrfx/scenefx.git
revision = main
depth = 1

[provide]
scenefx = scenefx
EOF

git add subprojects/scenefx.wrap 2>/dev/null
git commit -m "Add SceneFX as meson subproject" 2>/dev/null || echo -e "${YELLOW}No changes to commit${NC}"

echo ""
echo -e "${YELLOW}========================================${NC}"
echo -e "${YELLOW}MANUAL STEPS REQUIRED${NC}"
echo -e "${YELLOW}========================================${NC}"
echo ""
echo "The following tasks need manual completion:"
echo ""
echo "1. ${BLUE}meson.build${NC} (root)"
echo "   - Add SceneFX dependency declaration after wlroots"
echo "   - Add 'scenefx' to sway_deps array"
echo ""
echo "2. ${BLUE}sway/meson.build${NC}"
echo "   - Add all new command files to sway_sources"
echo "   - Add 'layer_criteria.c' to sway_sources"
echo "   - Add 'tree/scene/scene.c' to sway_sources"
echo ""
echo "3. ${BLUE}sway/desktop/transaction.c${NC} ⚠️ CRITICAL"
echo "   - Add 3 code blocks to existing functions"
echo "   - DO NOT replace entire file!"
echo "   - See TRANSACTION-INTEGRATION-GUIDE.md"
echo ""
echo "See ${GREEN}SIMPLE-INTEGRATION-GUIDE.md${NC} for detailed instructions"
echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}File Integration Complete!${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo "Current branch: $(git branch --show-current)"
echo "Commits added: $(git log --oneline main..HEAD 2>/dev/null | wc -l)"
echo ""
echo "Next steps:"
echo "  1. Edit meson.build files (see manual steps above)"
echo "  2. Integrate transaction code"
echo "  3. Run: meson setup build"
echo "  4. Run: ninja -C build"
echo ""
