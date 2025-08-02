#!/usr/bin/env python3
"""
replace_fx_renderer.py - Replace wlroots renderer with SceneFX FX renderer

This script:
1. Finds all wlr_renderer usage in scroll-standalone
2. Replaces with fx_renderer equivalents
3. Updates includes and initialization
4. Generates comprehensive report
"""

import os
import re
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Tuple, Set

class FXRendererReplacer:
    def __init__(self, repo_root: str):
        self.repo_root = Path(repo_root)
        self.target_dir = self.repo_root / "scroll-standalone"
        self.report_lines = []
        self.files_modified = []
        self.replacements_made = {}
        self.renderer_usage = {}
        
        # Key files that typically contain renderer code
        self.key_files = [
            "sway/desktop/output.c",
            "sway/desktop/render.c", 
            "sway/desktop/transaction.c",
            "sway/server.c",
            "sway/main.c",
            "sway/tree/container.c",
            "sway/tree/view.c",
        ]
        
        # Renderer API mappings
        self.api_mappings = [
            # Basic type replacements
            (r'\bstruct wlr_renderer\b', 'struct fx_renderer'),
            (r'\bwlr_renderer\b(?!\w)', 'fx_renderer'),
            
            # Creation/destruction
            (r'wlr_renderer_autocreate', 'fx_renderer_create'),
            (r'wlr_renderer_destroy', 'fx_renderer_destroy'),
            (r'wlr_backend_get_renderer', 'fx_get_renderer'),
            
            # Rendering functions
            (r'wlr_renderer_begin\s*\(([^,]+),\s*([^,]+),\s*([^)]+)\)', 
             r'fx_renderer_begin(\1, \2)'),  # FX renderer has different signature
            (r'wlr_renderer_end', 'fx_renderer_end'),
            (r'wlr_renderer_clear', 'fx_renderer_clear'),
            
            # Texture operations
            (r'wlr_render_texture', 'fx_render_texture'),
            (r'wlr_render_texture_with_matrix', 'fx_render_texture_with_matrix'),
            (r'wlr_render_rect', 'fx_render_rect'),
            (r'wlr_render_quad_with_matrix', 'fx_render_quad_with_matrix'),
            
            # Render pass (if using new wlroots render pass API)
            (r'wlr_renderer_begin_render_pass', 'fx_renderer_begin_render_pass'),
            (r'struct wlr_render_pass', 'struct fx_render_pass'),
            
            # Include replacements
            (r'#include\s+<wlr/render/wlr_renderer\.h>', 
             '#include <scenefx/render/fx_renderer/fx_renderer.h>'),
            (r'#include\s+<wlr/render/allocator\.h>',
             '#include <wlr/render/allocator.h>\n#include <scenefx/render/fx_renderer/fx_renderer.h>'),
        ]
        
        # Additional SceneFX includes that might be needed
        self.scenefx_includes = [
            '#include <scenefx/render/fx_renderer/fx_renderer.h>',
            '#include <scenefx/render/pass.h>',
            '#include <scenefx/types/wlr_scene.h>',
        ]
        
    def log(self, message: str, level: str = "INFO"):
        """Add message to report"""
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        self.report_lines.append(f"[{timestamp}] [{level}] {message}")
        print(f"[{level}] {message}")
        
    def find_renderer_usage(self) -> Dict[str, List[str]]:
        """Find all files using wlr_renderer"""
        self.log("Scanning for wlr_renderer usage...")
        
        usage = {}
        patterns = [
            r'wlr_renderer',
            r'wlr_render_',
            r'WLR_RENDER',
            r'<wlr/render/',
        ]
        
        # Search in all .c and .h files
        for pattern in ['**/*.c', '**/*.h']:
            for file_path in self.target_dir.glob(pattern):
                if file_path.is_file():
                    try:
                        content = file_path.read_text()
                        found_patterns = []
                        
                        for search_pattern in patterns:
                            if re.search(search_pattern, content):
                                matches = re.findall(f'.*{search_pattern}.*', content)
                                found_patterns.extend(matches[:3])  # First 3 matches
                                
                        if found_patterns:
                            rel_path = file_path.relative_to(self.target_dir)
                            usage[str(rel_path)] = found_patterns
                            
                    except Exception as e:
                        self.log(f"Error reading {file_path}: {e}", "ERROR")
                        
        self.renderer_usage = usage
        self.log(f"Found renderer usage in {len(usage)} files")
        return usage
        
    def analyze_renderer_patterns(self, content: str) -> Dict[str, int]:
        """Analyze specific renderer patterns in content"""
        patterns = {
            'renderer_creation': len(re.findall(r'wlr_renderer_autocreate|wlr_backend_get_renderer', content)),
            'render_begin_end': len(re.findall(r'wlr_renderer_begin|wlr_renderer_end', content)),
            'texture_rendering': len(re.findall(r'wlr_render_texture', content)),
            'render_pass': len(re.findall(r'wlr_render_pass|wlr_renderer_begin_render_pass', content)),
            'renderer_structs': len(re.findall(r'struct wlr_renderer', content)),
            'renderer_includes': len(re.findall(r'#include.*wlr/render', content)),
        }
        return patterns
        
    def apply_replacements(self, content: str, file_path: str) -> Tuple[str, Dict[str, int]]:
        """Apply all renderer replacements"""
        modified_content = content
        counts = {}
        
        # First pass: apply API mappings
        for pattern, replacement in self.api_mappings:
            try:
                matches = re.findall(pattern, modified_content)
                if matches:
                    count = len(matches)
                    modified_content = re.sub(pattern, replacement, modified_content)
                    counts[f"{pattern} -> {replacement}"] = count
            except re.error as e:
                self.log(f"Regex error in {pattern}: {e}", "ERROR")
                
        # Second pass: ensure SceneFX includes are present
        if 'fx_renderer' in modified_content and not any(inc in modified_content for inc in self.scenefx_includes):
            # Add includes after the last include
            last_include = None
            for match in re.finditer(r'#include\s+[<"][^>"]+[>"]', modified_content):
                last_include = match
                
            if last_include:
                insert_pos = last_include.end()
                include_text = '\n' + self.scenefx_includes[0]
                modified_content = modified_content[:insert_pos] + include_text + modified_content[insert_pos:]
                counts['Added SceneFX includes'] = 1
                
        return modified_content, counts
        
    def process_file(self, file_path: Path):
        """Process a single file for renderer replacement"""
        rel_path = file_path.relative_to(self.target_dir)
        self.log(f"Processing: {rel_path}")
        
        try:
            content = file_path.read_text()
            original_content = content
        except Exception as e:
            self.log(f"ERROR reading {file_path}: {e}", "ERROR")
            return
            
        # Analyze patterns before replacement
        patterns = self.analyze_renderer_patterns(content)
        if patterns:
            self.log(f"  - Found patterns: {patterns}")
            
        # Apply replacements
        modified_content, counts = self.apply_replacements(content, str(file_path))
        
        # Check if file was actually modified
        if modified_content != original_content:
            # Special handling for specific files
            if 'server.c' in str(file_path):
                # Server initialization needs special handling
                modified_content = self.handle_server_init(modified_content)
            elif 'output.c' in str(file_path):
                # Output rendering needs special handling
                modified_content = self.handle_output_rendering(modified_content)
                
            # Write back
            try:
                file_path.write_text(modified_content)
                self.files_modified.append(str(rel_path))
                self.replacements_made[str(rel_path)] = counts
                self.log(f"  - Modified with {sum(counts.values())} replacements")
            except Exception as e:
                self.log(f"ERROR writing {file_path}: {e}", "ERROR")
        else:
            self.log(f"  - No modifications needed")
            
    def handle_server_init(self, content: str) -> str:
        """Special handling for server.c initialization"""
        self.log("  - Applying server.c specific fixes")
        
        # Fix renderer creation
        content = re.sub(
            r'server->renderer\s*=\s*fx_renderer_create\s*\([^)]*\)',
            'server->renderer = fx_renderer_create(server->backend)',
            content
        )
        
        # Add effect framebuffers initialization if needed
        if 'fx_renderer_create' in content and 'fx_effect_framebuffers_create' not in content:
            # Find where to add effect framebuffers init
            renderer_create = re.search(r'server->renderer\s*=\s*fx_renderer_create[^;]+;', content)
            if renderer_create:
                insert_pos = renderer_create.end()
                fb_init = """
    // Initialize effect framebuffers for SceneFX
    server->effect_fbos = fx_effect_framebuffers_create(server->renderer);
    if (!server->effect_fbos) {
        wlr_log(WLR_ERROR, "Failed to create effect framebuffers");
    }"""
                content = content[:insert_pos] + fb_init + content[insert_pos:]
                
        return content
        
    def handle_output_rendering(self, content: str) -> str:
        """Special handling for output.c rendering"""
        self.log("  - Applying output.c specific fixes")
        
        # Fix render begin signature (FX renderer might have different params)
        content = re.sub(
            r'fx_renderer_begin\s*\(\s*([^,]+),\s*([^,]+),\s*([^)]+)\s*\)',
            r'fx_renderer_begin(\1, \2)',
            content
        )
        
        # Ensure damage tracking is compatible
        if 'wlr_output_damage' in content:
            self.log("    - Output uses damage tracking, may need FX-specific handling")
            
        return content
        
    def update_build_files(self):
        """Update meson.build to link with SceneFX"""
        self.log("\nUpdating build configuration...")
        
        meson_file = self.target_dir / "meson.build"
        if not meson_file.exists():
            self.log("ERROR: meson.build not found", "ERROR")
            return
            
        try:
            content = meson_file.read_text()
            original_content = content
            
            # Add SceneFX dependency if not present
            if 'scenefx' not in content:
                # Find dependencies section
                deps_match = re.search(r'(dependencies\s*:\s*\[)([^\]]+)(\])', content, re.MULTILINE | re.DOTALL)
                if deps_match:
                    deps = deps_match.group(2)
                    if 'scenefx' not in deps:
                        new_deps = deps.rstrip() + ",\n  dependency('scenefx')\n"
                        content = content[:deps_match.start(2)] + new_deps + content[deps_match.end(2):]
                        self.log("  - Added scenefx dependency to meson.build")
                        
            # Write back if modified
            if content != original_content:
                meson_file.write_text(content)
                self.files_modified.append("meson.build")
                
        except Exception as e:
            self.log(f"ERROR updating meson.build: {e}", "ERROR")
            
    def generate_summary(self) -> Dict[str, any]:
        """Generate summary statistics"""
        summary = {
            'total_files_scanned': len(self.renderer_usage),
            'total_files_modified': len(self.files_modified),
            'total_replacements': sum(sum(counts.values()) for counts in self.replacements_made.values()),
            'key_files_modified': [f for f in self.files_modified if any(key in f for key in self.key_files)],
        }
        return summary
        
    def run(self):
        """Execute the renderer replacement"""
        self.log("=== FX Renderer Replacement Started ===")
        self.log(f"Target directory: {self.target_dir}")
        
        if not self.target_dir.exists():
            self.log(f"ERROR: Target directory does not exist: {self.target_dir}", "ERROR")
            self.generate_report()
            return
            
        # Find all renderer usage
        usage = self.find_renderer_usage()
        
        # Process each file with renderer usage
        self.log("\n=== Processing Files ===")
        for file_path_str in usage.keys():
            file_path = self.target_dir / file_path_str
            if file_path.exists():
                self.process_file(file_path)
                
        # Update build files
        self.update_build_files()
        
        # Generate report
        self.generate_report()
        
    def generate_report(self):
        """Generate comprehensive markdown report"""
        report_path = self.repo_root / "report-fx-renderer-replacement.md"
        summary = self.generate_summary()
        
        with open(report_path, 'w') as f:
            f.write("# FX Renderer Replacement Report\n\n")
            f.write(f"**Generated**: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n\n")
            
            f.write("## Summary\n\n")
            f.write(f"- **Files Scanned**: {summary['total_files_scanned']}\n")
            f.write(f"- **Files Modified**: {summary['total_files_modified']}\n")
            f.write(f"- **Total Replacements**: {summary['total_replacements']}\n")
            f.write(f"- **Target Directory**: `{self.target_dir.relative_to(self.repo_root)}`\n\n")
            
            f.write("## Renderer Usage Found\n\n")
            if self.renderer_usage:
                f.write("Files containing wlr_renderer references:\n\n")
                for file_path, patterns in sorted(self.renderer_usage.items()):
                    f.write(f"### `{file_path}`\n")
                    for pattern in patterns[:3]:  # Show first 3 examples
                        f.write(f"- `{pattern.strip()}`\n")
                    if len(patterns) > 3:
                        f.write(f"- ... and {len(patterns) - 3} more occurrences\n")
                    f.write("\n")
                    
            f.write("## Files Modified\n\n")
            if self.files_modified:
                # Group by directory
                dirs = {}
                for file_path in sorted(self.files_modified):
                    dir_name = str(Path(file_path).parent)
                    if dir_name not in dirs:
                        dirs[dir_name] = []
                    dirs[dir_name].append(file_path)
                    
                for dir_name, files in sorted(dirs.items()):
                    f.write(f"### {dir_name}/\n")
                    for file_path in files:
                        if file_path in self.replacements_made:
                            count = sum(self.replacements_made[file_path].values())
                            f.write(f"- `{Path(file_path).name}` ({count} replacements)\n")
                        else:
                            f.write(f"- `{Path(file_path).name}`\n")
                    f.write("\n")
                    
            f.write("## Replacement Details\n\n")
            if self.replacements_made:
                for file_path, counts in sorted(self.replacements_made.items()):
                    if counts:
                        f.write(f"### {file_path}\n\n")
                        for replacement, count in sorted(counts.items()):
                            # Simplify long regex patterns for readability
                            display_replacement = replacement
                            if len(replacement) > 80:
                                display_replacement = replacement[:77] + "..."
                            f.write(f"- `{display_replacement}`: {count} occurrences\n")
                        f.write("\n")
                        
            f.write("## Key Files Status\n\n")
            f.write("Status of critical renderer files:\n\n")
            f.write("| File | Status | Replacements |\n")
            f.write("|------|--------|-------------|\n")
            for key_file in self.key_files:
                status = "✅ Modified" if key_file in self.files_modified else "❌ Not Modified"
                count = sum(self.replacements_made.get(key_file, {}).values())
                found = "Found" if key_file in self.renderer_usage else "Not Found"
                f.write(f"| `{key_file}` | {status} | {count} |\n")
                
            f.write("\n## Build System Updates\n\n")
            if "meson.build" in self.files_modified:
                f.write("- ✅ Added scenefx dependency to meson.build\n")
            else:
                f.write("- ⚠️ meson.build not updated (may need manual configuration)\n")
                
            f.write("\n## Required Manual Steps\n\n")
            f.write("1. **Verify SceneFX dependency**: Ensure scenefx is available in your build environment\n")
            f.write("2. **Effect framebuffers**: Check if effect framebuffers initialization is needed\n")
            f.write("3. **Renderer capabilities**: Verify FX renderer supports all required features\n")
            f.write("4. **Shader compilation**: Ensure SceneFX shaders are compiled and available\n")
            f.write("5. **Performance testing**: Test rendering performance with FX renderer\n")
            
            f.write("\n## Potential Issues\n\n")
            f.write("- **API differences**: FX renderer may have different function signatures\n")
            f.write("- **Feature parity**: Some wlr_renderer features might not have FX equivalents\n")
            f.write("- **Initialization order**: FX renderer might require different initialization\n")
            f.write("- **Resource management**: Different cleanup requirements for FX renderer\n")
            
            f.write("\n## Detailed Log\n\n")
            f.write("```\n")
            for line in self.report_lines:
                f.write(f"{line}\n")
            f.write("```\n")
            
        self.log(f"\nReport generated: {report_path}")

if __name__ == "__main__":
    # Get repository root (current directory when run from repo root)
    repo_root = os.getcwd()
    
    # Create and run replacer
    replacer = FXRendererReplacer(repo_root)
    replacer.run()
