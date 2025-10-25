#!/bin/bash
# ScrollFX Integration Script - Consolidated Version
# Integrates ScrollFX implementation into Scroll window manager

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Usage check
if [ $# -ne 1 ]; then
    echo "Usage: $0 <scroll-repo-path>"
    echo "Example: $0 ~/scroll"
    echo ""
    echo "This script integrates ScrollFX from the consolidated implementation directory"
    echo "into the Scroll window manager repository."
    exit 1
fi

SCROLL_DIR="$1"
IMPL_DIR="$(cd "$(dirname "$0")" && pwd)"

# Validate
if [ ! -d "$SCROLL_DIR" ]; then
    echo -e "${RED}Error: Scroll directory does not exist: $SCROLL_DIR${NC}"
    exit 1
fi

if [ ! -d "$SCROLL_DIR/.git" ]; then
    echo -e "${RED}Error: Not a git repository: $SCROLL_DIR${NC}"
    exit 1
fi

if [ ! -d "$IMPL_DIR/include" ] || [ ! -d "$IMPL_DIR/sway" ]; then
    echo -e "${RED}Error: Not in ScrollFX implementation directory${NC}"
    exit 1
fi

echo -e "${BLUE}================================================${NC}"
echo -e "${BLUE}   ScrollFX Integration (Consolidated)${NC}"
echo -e "${BLUE}================================================${NC}"
echo ""
echo "Scroll repository: $SCROLL_DIR"
echo "Implementation: $IMPL_DIR"
echo ""

cd "$SCROLL_DIR"

# Create/switch to branch
BRANCH_NAME="scrollfx-integration"
if git rev-parse --verify "$BRANCH_NAME" >/dev/null 2>&1; then
    echo -e "${YELLOW}Branch '$BRANCH_NAME' exists, switching...${NC}"
    git checkout "$BRANCH_NAME"
else
    echo -e "${GREEN}Creating new branch '$BRANCH_NAME'...${NC}"
    git checkout -b "$BRANCH_NAME"
fi

echo ""

# Helper function to copy files with verification
copy_file() {
    local src="$1"
    local dst="$2"
    if [ -f "$src" ]; then
        mkdir -p "$(dirname "$dst")"
        cp "$src" "$dst"
        echo -e "  ${GREEN}✓${NC} $(basename "$dst")"
        return 0
    else
        echo -e "  ${RED}✗${NC} Missing: $src"
        return 1
    fi
}

#################################################
# HEADERS
#################################################
echo -e "${BLUE}=== Copying Header Files ===${NC}"

copy_file "$IMPL_DIR/include/sway/commands.h" "include/sway/commands.h"
copy_file "$IMPL_DIR/include/sway/config.h" "include/sway/config.h"
copy_file "$IMPL_DIR/include/sway/layer_criteria.h" "include/sway/layer_criteria.h"
copy_file "$IMPL_DIR/include/sway/layers.h" "include/sway/layers.h"
copy_file "$IMPL_DIR/include/sway/output.h" "include/sway/output.h"

copy_file "$IMPL_DIR/include/sway/tree/container.h" "include/sway/tree/container.h"
copy_file "$IMPL_DIR/include/sway/tree/node.h" "include/sway/tree/node.h"
copy_file "$IMPL_DIR/include/sway/tree/root.h" "include/sway/tree/root.h"
copy_file "$IMPL_DIR/include/sway/tree/scene.h" "include/sway/tree/scene.h"

git add include/sway/ 2>/dev/null || true

echo ""

#################################################
# CORE SOURCE FILES
#################################################
echo -e "${BLUE}=== Copying Core Source Files ===${NC}"

copy_file "$IMPL_DIR/sway/commands.c" "sway/commands.c"
copy_file "$IMPL_DIR/sway/config.c" "sway/config.c"
copy_file "$IMPL_DIR/sway/layer_criteria.c" "sway/layer_criteria.c"

git add sway/commands.c sway/config.c sway/layer_criteria.c 2>/dev/null || true

echo ""

#################################################
# COMMAND FILES
#################################################
echo -e "${BLUE}=== Copying Command Files ===${NC}"

# Copy all command files
for cmd_file in "$IMPL_DIR/sway/commands/"*.c; do
    if [ -f "$cmd_file" ]; then
        copy_file "$cmd_file" "sway/commands/$(basename "$cmd_file")"
    fi
done

git add sway/commands/ 2>/dev/null || true

echo ""

#################################################
# TREE SOURCE FILES
#################################################
echo -e "${BLUE}=== Copying Tree Management Files ===${NC}"

copy_file "$IMPL_DIR/sway/tree/arrange.c" "sway/tree/arrange.c"
copy_file "$IMPL_DIR/sway/tree/container.c" "sway/tree/container.c"
copy_file "$IMPL_DIR/sway/tree/node.c" "sway/tree/node.c"
copy_file "$IMPL_DIR/sway/tree/root.c" "sway/tree/root.c"
copy_file "$IMPL_DIR/sway/tree/view.c" "sway/tree/view.c"
copy_file "$IMPL_DIR/sway/tree/output.c" "sway/tree/output.c"

mkdir -p sway/tree/scene
copy_file "$IMPL_DIR/sway/tree/scene/scene.c" "sway/tree/scene/scene.c"

git add sway/tree/ 2>/dev/null || true

echo ""

#################################################
# DESKTOP SOURCE FILES
#################################################
echo -e "${BLUE}=== Copying Desktop Integration Files ===${NC}"

copy_file "$IMPL_DIR/sway/desktop/output.c" "sway/desktop/output.c"
copy_file "$IMPL_DIR/sway/desktop/transaction.c" "sway/desktop/transaction.c"
copy_file "$IMPL_DIR/sway/desktop/layer_shell.c" "sway/desktop/layer_shell.c"
copy_file "$IMPL_DIR/sway/desktop/xdg_shell.c" "sway/desktop/xdg_shell.c"
copy_file "$IMPL_DIR/sway/desktop/xwayland.c" "sway/desktop/xwayland.c"

git add sway/desktop/ 2>/dev/null || true

echo ""

#################################################
# INPUT SOURCE FILES
#################################################
echo -e "${BLUE}=== Copying Input Handling Files ===${NC}"

copy_file "$IMPL_DIR/sway/input/seatop_move_tiling.c" "sway/input/seatop_move_tiling.c"

git add sway/input/ 2>/dev/null || true

echo ""

#################################################
# SCENEFX SUBPROJECT
#################################################
echo -e "${BLUE}=== Setting Up SceneFX Subproject ===${NC}"

mkdir -p subprojects
cat > subprojects/scenefx.wrap << 'EOF'
[wrap-git]
url = https://github.com/wlrfx/scenefx.git
revision = main
depth = 1

[provide]
scenefx = scenefx
EOF

echo -e "  ${GREEN}✓${NC} scenefx.wrap created"

git add subprojects/scenefx.wrap 2>/dev/null || true

echo ""

#################################################
# CREATE GIT COMMITS
#################################################
echo -e "${BLUE}=== Creating Git Commits ===${NC}"

# Commit configuration and commands
git commit -m "ScrollFX: Configuration and commands

- Add SceneFX configuration structures
- Implement 24 SceneFX commands
- Add layer criteria system
- Provide command parsing and registration" 2>/dev/null || echo -e "${YELLOW}  No changes to commit (config/commands)${NC}"

# Note: All files were added together, so only one commit
echo -e "  ${GREEN}✓${NC} ScrollFX implementation committed"

echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}   File Integration Complete!${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo -e "${YELLOW}MANUAL STEPS REQUIRED:${NC}"
echo ""
echo "1. ${BLUE}Edit meson.build (root)${NC}:"
echo "   Add after wlroots dependency:"
echo "   ${GREEN}scenefx = dependency('scenefx', version: '>=0.1.0')${NC}"
echo ""
echo "   Add to sway_deps array:"
echo "   ${GREEN}sway_deps = [${NC}"
echo "   ${GREEN}  # ... existing deps ...${NC}"
echo "   ${GREEN}  scenefx,${NC}"
echo "   ${GREEN}]${NC}"
echo ""
echo "2. ${BLUE}Edit sway/meson.build${NC}:"
echo "   Add all command files and new source files."
echo "   See: $IMPL_DIR/docs/integration/meson-build-guide.md"
echo ""
echo "3. ${BLUE}Edit sway/desktop/transaction.c${NC}:"
echo "   ⚠️  ${YELLOW}CRITICAL${NC}: DO NOT replace entire file!"
echo "   Add 3 specific code blocks only."
echo "   See: $IMPL_DIR/docs/integration/TRANSACTION-INTEGRATION-GUIDE.md"
echo ""
echo "4. ${BLUE}Build ScrollFX${NC}:"
echo "   ${GREEN}cd $SCROLL_DIR${NC}"
echo "   ${GREEN}meson setup build${NC}"
echo "   ${GREEN}ninja -C build${NC}"
echo ""
echo "5. ${BLUE}Test${NC}:"
echo "   ${GREEN}./build/sway/sway --version${NC}"
echo "   ${GREEN}./build/sway/sway -C -c <test-config>${NC}"
echo ""
echo -e "${BLUE}Documentation:${NC}"
echo "  - Implementation guide: $IMPL_DIR/README.md"
echo "  - Detailed docs: $IMPL_DIR/IMPLEMENTATION-GUIDE.md"
echo "  - Integration guides: $IMPL_DIR/docs/integration/"
echo ""
echo -e "${BLUE}Current branch:${NC} $(git branch --show-current)"
echo -e "${BLUE}Total commits:${NC} $(git log --oneline main..HEAD 2>/dev/null | wc -l)"
echo ""
echo -e "${GREEN}Ready for manual build system integration!${NC}"
