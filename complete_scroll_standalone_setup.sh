#!/bin/bash
# complete_scroll_standalone_setup.sh - Complete setup of scroll-standalone with FX renderer

set -e  # Exit on error

echo "============================================="
echo "Complete Scroll Standalone Setup with FX Renderer"
echo "============================================="
echo ""

# Check if we're in the correct directory
if [ ! -d "references/scroll" ] || [ ! -d "scene-scroll" ]; then
    echo "ERROR: This script must be run from the scrollfx-wip repository root"
    echo "Current directory: $(pwd)"
    exit 1
fi

# Check if scroll-standalone already exists
if [ -d "scroll-standalone" ]; then
    echo "WARNING: scroll-standalone directory already exists!"
    read -p "Remove existing scroll-standalone and continue? (y/n) " -n 1 -r
    echo ""
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        rm -rf scroll-standalone
    else
        echo "Aborting."
        exit 1
    fi
fi

echo "=== Step 1: Creating scroll-standalone ==="
echo "Copying references/scroll to scroll-standalone..."
cp -r references/scroll scroll-standalone

echo "Removing embedded scene implementation..."
rm -rf scroll-standalone/sway/tree/scene

echo ""
echo "=== Step 2: Updating includes to use external scene-scroll ==="
echo "Replacing internal scene includes with external scene-scroll..."

# Update all C and H files to use external scene-scroll
find scroll-standalone -type f \( -name "*.c" -o -name "*.h" \) -print0 | while IFS= read -r -d '' file; do
    # Create temp file for safe editing
    temp_file=$(mktemp)
    
    # Apply all include replacements
    sed -e 's|#include "sway/tree/scene/|#include <scene-scroll/|g' \
        -e 's|#include "sway/tree/scene\.h"|#include <scene-scroll/scene.h>|g' \
        -e 's|#include <sway/tree/scene/|#include <scene-scroll/|g' \
        -e 's|#include <sway/tree/scene\.h>|#include <scene-scroll/scene.h>|g' \
        "$file" > "$temp_file"
    
    # Only update file if changes were made
    if ! cmp -s "$file" "$temp_file"; then
        mv "$temp_file" "$file"
        echo "  Updated: ${file#scroll-standalone/}"
    else
        rm "$temp_file"
    fi
done

echo ""
echo "=== Step 3: Updating meson.build to use scene-scroll dependency ==="

# Create a Python script to properly update meson.build
cat << 'EOF' > update_meson.py
#!/usr/bin/env python3
import re
import sys

meson_file = "scroll-standalone/meson.build"

try:
    with open(meson_file, 'r') as f:
        content = f.read()
    
    original_content = content
    
    # First, check if scene-scroll dependency already exists
    if 'scene-scroll' in content:
        print("scene-scroll dependency already present in meson.build")
        sys.exit(0)
    
    # Look for subprojects section and add scene-scroll
    subprojects_pattern = r"(subproject\s*\(\s*['\"]wlroots['\"]\s*[^)]*\))"
    if re.search(subprojects_pattern, content):
        # Add scene-scroll subproject after wlroots
        scene_scroll_subproject = """
scene_scroll = subproject(
  'scene-scroll',
  default_options: ['default_library=static']
)
scene_scroll_dep = scene_scroll.get_variable('scene_scroll_dep')"""
        
        content = re.sub(
            subprojects_pattern,
            r"\1" + scene_scroll_subproject,
            content,
            count=1
        )
        print("Added scene-scroll subproject")
    
    # Add scene_scroll_dep to server dependencies
    server_deps_pattern = r"(server_deps\s*=\s*\[)([^\]]+)(\])"
    match = re.search(server_deps_pattern, content, re.MULTILINE | re.DOTALL)
    if match:
        deps_list = match.group(2)
        if 'scene_scroll_dep' not in deps_list:
            # Add scene_scroll_dep to the dependencies
            new_deps = deps_list.rstrip() + ",\n  scene_scroll_dep\n"
            content = content[:match.start(2)] + new_deps + content[match.end(2):]
            print("Added scene_scroll_dep to server_deps")
    
    # Also check for sway_deps if server_deps not found
    if 'scene_scroll_dep' not in content:
        sway_deps_pattern = r"(sway_deps\s*=\s*\[)([^\]]+)(\])"
        match = re.search(sway_deps_pattern, content, re.MULTILINE | re.DOTALL)
        if match:
            deps_list = match.group(2)
            if 'scene_scroll_dep' not in deps_list:
                new_deps = deps_list.rstrip() + ",\n  scene_scroll_dep\n"
                content = content[:match.start(2)] + new_deps + content[match.end(2):]
                print("Added scene_scroll_dep to sway_deps")
    
    # Write back if modified
    if content != original_content:
        with open(meson_file, 'w') as f:
            f.write(content)
        print("meson.build updated successfully")
    else:
        print("WARNING: Could not automatically update meson.build - manual edit required")
        print("Add this to your subprojects section:")
        print("""
scene_scroll = subproject(
  'scene-scroll',
  default_options: ['default_library=static']
)
scene_scroll_dep = scene_scroll.get_variable('scene_scroll_dep')
""")
        print("And add 'scene_scroll_dep' to your dependencies list")
        
except Exception as e:
    print(f"ERROR updating meson.build: {e}")
    sys.exit(1)
EOF

python3 update_meson.py
rm update_meson.py

echo ""
echo "=== Step 4: Creating scene-scroll subproject link ==="
# Create subprojects directory if it doesn't exist
mkdir -p scroll-standalone/subprojects

# Create a wrap file for scene-scroll
cat << 'EOF' > scroll-standalone/subprojects/scene-scroll.wrap
[wrap-file]
directory = scene-scroll

[provide]
scene-scroll = scene_scroll_dep
EOF

# Create symlink to scene-scroll
if [ ! -e "scroll-standalone/subprojects/scene-scroll" ]; then
    ln -s ../../scene-scroll scroll-standalone/subprojects/scene-scroll
    echo "Created symlink: scroll-standalone/subprojects/scene-scroll -> ../../scene-scroll"
fi

echo ""
echo "=== Step 5: Running FX Renderer Replacement ==="
echo "Replacing wlroots renderer with FX renderer..."
python3 replace_fx_renderer.py

echo ""
echo "============================================="
echo "Setup Complete!"
echo "============================================="
echo ""
echo "Summary of changes:"
echo "1. ✅ Created scroll-standalone from references/scroll"
echo "2. ✅ Removed embedded scene implementation"
echo "3. ✅ Updated includes to use external scene-scroll"
echo "4. ✅ Updated meson.build to add scene-scroll dependency"
echo "5. ✅ Created subproject link for scene-scroll"
echo "6. ✅ Ran FX renderer replacement"
echo ""
echo "Next steps:"
echo "1. Review report-fx-renderer-replacement.md"
echo "2. Test compilation:"
echo "   cd scroll-standalone"
echo "   meson setup build"
echo "   ninja -C build"
echo ""
echo "If compilation fails, check:"
echo "- Missing dependencies in meson.build"
echo "- Include path issues"
echo "- Scene-scroll API compatibility"
