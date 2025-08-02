#!/bin/bash
# run_mechanical_replacements.sh - Execute both mechanical replacement scripts

set -e  # Exit on error

echo "=========================================="
echo "Running Mechanical Replacements for ScrollFX"
echo "=========================================="
echo ""

# Check if we're in the correct directory
if [ ! -d "references/scroll" ] || [ ! -d "scene-scroll" ]; then
    echo "ERROR: This script must be run from the scrollfx-wip repository root"
    echo "Current directory: $(pwd)"
    exit 1
fi

# Make scripts executable
chmod +x extract_scroll_scene.py 2>/dev/null || true
chmod +x replace_fx_renderer.py 2>/dev/null || true

# Run Scene Extraction (fills scene-scroll stubs)
echo "=== STEP 1: Extracting Scroll Scene Implementation ==="
echo "This will:"
echo "- Copy scene files from references/scroll/sway/tree/scene/"
echo "- Replace wlr_scene with sway_scene throughout"
echo "- Generate report-scene-extraction.md"
echo ""
read -p "Continue with scene extraction? (y/n) " -n 1 -r
echo ""
if [[ $REPLY =~ ^[Yy]$ ]]; then
    python3 extract_scroll_scene.py
    echo ""
    echo "Scene extraction complete! Check report-scene-extraction.md for details."
else
    echo "Skipping scene extraction."
fi

echo ""
echo "=== STEP 2: Replacing wlroots Renderer with FX Renderer ==="
echo "This will:"
echo "- Find all wlr_renderer usage in scroll-standalone/"
echo "- Replace with fx_renderer equivalents"
echo "- Generate report-fx-renderer-replacement.md"
echo ""
read -p "Continue with renderer replacement? (y/n) " -n 1 -r
echo ""
if [[ $REPLY =~ ^[Yy]$ ]]; then
    python3 replace_fx_renderer.py
    echo ""
    echo "Renderer replacement complete! Check report-fx-renderer-replacement.md for details."
else
    echo "Skipping renderer replacement."
fi

echo ""
echo "=========================================="
echo "Mechanical Replacements Complete!"
echo "=========================================="
echo ""
echo "Generated reports:"
echo "- report-scene-extraction.md"
echo "- report-fx-renderer-replacement.md"
echo ""
echo "Next steps:"
echo "1. Review the generated reports"
echo "2. Test compilation of modified components"
echo "3. Fix any remaining issues identified in reports"
echo "4. Commit and push changes"
echo ""
echo "To commit changes:"
echo "  git add scene-scroll/ scroll-standalone/ report-*.md"
echo "  git commit -m 'Apply mechanical replacements: extract scene and replace renderer'"
echo "  git push"
