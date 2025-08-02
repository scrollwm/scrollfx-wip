#!/usr/bin/env python3
"""
extract_scroll_scene.py - Extract actual Scroll scene implementation and convert to sway_scene

This script:
1. Copies scene implementation from /references/scroll/sway/tree/scene/
2. Replaces wlr_scene with sway_scene throughout
3. Updates headers and includes
4. Generates comprehensive report
"""

import os
import re
import shutil
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Tuple

class ScrollSceneExtractor:
    def __init__(self, repo_root: str):
        self.repo_root = Path(repo_root)
        self.source_dir = self.repo_root / "references/scroll/sway/tree/scene"
        self.dest_dir = self.repo_root / "scene-scroll"
        self.report_lines = []
        self.files_processed = []
        self.replacements_made = {}
        
        # Replacement patterns
        self.replacements = [
            # Basic type replacements
            (r'\bwlr_scene\b', 'sway_scene'),
            (r'\bWLR_SCENE\b', 'SWAY_SCENE'),
            (r'\bwlr_scene_(\w+)\b', r'sway_scene_\1'),
            (r'\bWLR_SCENE_(\w+)\b', r'SWAY_SCENE_\1'),
            
            # Include replacements
            (r'#include\s+<wlr/types/wlr_scene\.h>', '#include "scene-scroll/scene.h"'),
            (r'#include\s+"wlr_scene\.h"', '#include "scene-scroll/scene.h"'),
            
            # Special cases from Scroll's modifications
            (r'struct wlr_scene_tree', 'struct sway_scene_tree'),
            (r'struct wlr_scene_node', 'struct sway_scene_node'),
            (r'struct wlr_scene_buffer', 'struct sway_scene_buffer'),
            (r'struct wlr_scene_rect', 'struct sway_scene_rect'),
            (r'struct wlr_scene_surface', 'struct sway_scene_surface'),
            (r'struct wlr_scene_output', 'struct sway_scene_output'),
            (r'struct wlr_scene', 'struct sway_scene'),
            
            # Function pointer types
            (r'wlr_scene_buffer_point_accepts_input_func_t', 'sway_scene_buffer_point_accepts_input_func_t'),
            (r'wlr_scene_buffer_iterator_func_t', 'sway_scene_buffer_iterator_func_t'),
            
            # Enum replacements
            (r'enum wlr_scene_node_type', 'enum sway_scene_node_type'),
            (r'enum wlr_scene_debug_damage_option', 'enum sway_scene_debug_damage_option'),
        ]
        
    def log(self, message: str, level: str = "INFO"):
        """Add message to report"""
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        self.report_lines.append(f"[{timestamp}] [{level}] {message}")
        print(f"[{level}] {message}")
        
    def analyze_source(self) -> Dict[str, List[str]]:
        """Analyze source directory structure"""
        self.log(f"Analyzing source directory: {self.source_dir}")
        
        structure = {
            'c_files': [],
            'h_files': [],
            'other_files': []
        }
        
        if not self.source_dir.exists():
            self.log(f"ERROR: Source directory does not exist: {self.source_dir}", "ERROR")
            return structure
            
        for file_path in self.source_dir.rglob('*'):
            if file_path.is_file():
                rel_path = file_path.relative_to(self.source_dir)
                if file_path.suffix == '.c':
                    structure['c_files'].append(str(rel_path))
                elif file_path.suffix == '.h':
                    structure['h_files'].append(str(rel_path))
                else:
                    structure['other_files'].append(str(rel_path))
                    
        self.log(f"Found {len(structure['c_files'])} .c files")
        self.log(f"Found {len(structure['h_files'])} .h files")
        self.log(f"Found {len(structure['other_files'])} other files")
        
        return structure
        
    def check_existing_stubs(self) -> List[str]:
        """Check what stub files exist in destination"""
        self.log(f"Checking existing files in: {self.dest_dir}")
        
        existing = []
        src_dir = self.dest_dir / "src"
        if src_dir.exists():
            for file_path in src_dir.glob('*.c'):
                existing.append(file_path.name)
                # Check if it's a stub
                content = file_path.read_text()
                if "// Implementation needed" in content or "return NULL;" in content:
                    self.log(f"Found stub file: {file_path.name}")
                    
        return existing
        
    def apply_replacements(self, content: str, file_path: str) -> Tuple[str, Dict[str, int]]:
        """Apply all replacements to content"""
        modified_content = content
        counts = {}
        
        for pattern, replacement in self.replacements:
            matches = re.findall(pattern, modified_content)
            if matches:
                count = len(matches)
                modified_content = re.sub(pattern, replacement, modified_content)
                counts[f"{pattern} -> {replacement}"] = count
                
        return modified_content, counts
        
    def process_c_file(self, source_file: Path, dest_file: Path):
        """Process a single C file"""
        self.log(f"Processing: {source_file.name}")
        
        # Read source content
        try:
            content = source_file.read_text()
        except Exception as e:
            self.log(f"ERROR reading {source_file}: {e}", "ERROR")
            return
            
        # Check file size and complexity
        lines = content.split('\n')
        self.log(f"  - Size: {len(lines)} lines, {len(content)} chars")
        
        # Apply replacements
        modified_content, counts = self.apply_replacements(content, str(source_file))
        
        # Store replacement counts
        self.replacements_made[source_file.name] = counts
        
        # Log replacements
        if counts:
            self.log(f"  - Replacements made:")
            for replacement, count in counts.items():
                self.log(f"    * {replacement}: {count} occurrences")
        
        # Special handling for scene.c
        if source_file.name == "scene.c":
            # Update includes for scene-scroll structure
            modified_content = re.sub(
                r'#include "sway/tree/scene\.h"',
                '#include "scene-scroll/scene.h"',
                modified_content
            )
            # Remove Scroll-specific includes that won't exist in standalone
            scroll_specific = [
                r'#include "sway/.*"',
                r'#include "log\.h"',
                r'#include "util\.h"'
            ]
            for pattern in scroll_specific:
                if re.search(pattern, modified_content):
                    self.log(f"  - Commenting out Scroll-specific include: {pattern}")
                    modified_content = re.sub(pattern, f'// {pattern} // Commented for standalone', modified_content)
        
        # Write to destination
        dest_file.parent.mkdir(parents=True, exist_ok=True)
        try:
            dest_file.write_text(modified_content)
            self.log(f"  - Written to: {dest_file}")
            self.files_processed.append(str(dest_file))
        except Exception as e:
            self.log(f"ERROR writing {dest_file}: {e}", "ERROR")
            
    def process_h_file(self, source_file: Path, dest_file: Path):
        """Process a header file"""
        self.log(f"Processing header: {source_file.name}")
        
        try:
            content = source_file.read_text()
        except Exception as e:
            self.log(f"ERROR reading {source_file}: {e}", "ERROR")
            return
            
        # Apply replacements
        modified_content, counts = self.apply_replacements(content, str(source_file))
        
        # Store replacement counts
        self.replacements_made[source_file.name] = counts
        
        # Update include guards
        modified_content = re.sub(r'WLR_SCENE', 'SWAY_SCENE', modified_content)
        modified_content = re.sub(r'_WLR_TYPES_SCENE_H', '_SCENE_SCROLL_H', modified_content)
        
        # Write to destination
        dest_file.parent.mkdir(parents=True, exist_ok=True)
        try:
            dest_file.write_text(modified_content)
            self.log(f"  - Written to: {dest_file}")
            self.files_processed.append(str(dest_file))
        except Exception as e:
            self.log(f"ERROR writing {dest_file}: {e}", "ERROR")
            
    def update_meson_build(self):
        """Update meson.build to include new source files"""
        meson_file = self.dest_dir / "meson.build"
        self.log(f"Updating meson.build")
        
        # Get list of C files in src/
        src_dir = self.dest_dir / "src"
        c_files = sorted([f.name for f in src_dir.glob("*.c")])
        
        # Generate sources list
        sources_content = "scene_scroll_sources = files(\n"
        for c_file in c_files:
            sources_content += f"  'src/{c_file}',\n"
        sources_content += ")\n"
        
        # Read existing meson.build
        if meson_file.exists():
            content = meson_file.read_text()
            # Replace sources section
            content = re.sub(
                r'scene_scroll_sources = files\([^)]+\)',
                sources_content.strip(),
                content,
                flags=re.DOTALL
            )
        else:
            # Create new meson.build
            content = f"""project('scene-scroll', 'c',
  version: '0.1.0',
  default_options: ['c_std=c11', 'warning_level=2']
)

{sources_content}

scene_scroll_inc = include_directories('include')

scene_scroll_lib = static_library(
  'scene-scroll',
  scene_scroll_sources,
  include_directories: scene_scroll_inc,
  dependencies: [
    dependency('wayland-server'),
    dependency('wlroots'),
    dependency('pixman-1'),
  ]
)

scene_scroll_dep = declare_dependency(
  link_with: scene_scroll_lib,
  include_directories: scene_scroll_inc,
)
"""
        
        meson_file.write_text(content)
        self.log(f"  - Updated meson.build with {len(c_files)} source files")
        
    def run(self):
        """Execute the extraction and conversion"""
        self.log("=== Scroll Scene Extraction Started ===")
        self.log(f"Repository root: {self.repo_root}")
        
        # Analyze source
        structure = self.analyze_source()
        if not structure['c_files'] and not structure['h_files']:
            self.log("ERROR: No source files found!", "ERROR")
            self.generate_report()
            return
            
        # Check existing stubs
        existing_stubs = self.check_existing_stubs()
        self.log(f"Found {len(existing_stubs)} existing stub files")
        
        # Process C files
        self.log("\n=== Processing C Files ===")
        for c_file in structure['c_files']:
            source_file = self.source_dir / c_file
            dest_file = self.dest_dir / "src" / Path(c_file).name
            self.process_c_file(source_file, dest_file)
            
        # Process header files  
        self.log("\n=== Processing Header Files ===")
        for h_file in structure['h_files']:
            source_file = self.source_dir / h_file
            # Headers go to include/scene-scroll/
            dest_file = self.dest_dir / "include/scene-scroll" / Path(h_file).name
            self.process_h_file(source_file, dest_file)
            
        # Update meson.build
        self.log("\n=== Updating Build System ===")
        self.update_meson_build()
        
        # Generate report
        self.generate_report()
        
    def generate_report(self):
        """Generate comprehensive markdown report"""
        report_path = self.repo_root / "report-scene-extraction.md"
        
        with open(report_path, 'w') as f:
            f.write("# Scroll Scene Extraction Report\n\n")
            f.write(f"**Generated**: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n\n")
            
            f.write("## Summary\n\n")
            f.write(f"- **Files Processed**: {len(self.files_processed)}\n")
            f.write(f"- **Source Directory**: `{self.source_dir.relative_to(self.repo_root)}`\n")
            f.write(f"- **Destination Directory**: `{self.dest_dir.relative_to(self.repo_root)}`\n\n")
            
            f.write("## Files Extracted\n\n")
            if self.files_processed:
                for file_path in sorted(self.files_processed):
                    f.write(f"- `{file_path}`\n")
            else:
                f.write("*No files were processed*\n")
                
            f.write("\n## Replacements Made\n\n")
            if self.replacements_made:
                for file_name, counts in self.replacements_made.items():
                    if counts:
                        f.write(f"### {file_name}\n\n")
                        for replacement, count in counts.items():
                            f.write(f"- `{replacement}`: {count} occurrences\n")
                        f.write("\n")
            else:
                f.write("*No replacements were made*\n")
                
            f.write("\n## Build System Updates\n\n")
            f.write("- Updated `meson.build` with extracted source files\n")
            f.write("- Configured include directories for standalone build\n")
            
            f.write("\n## Required Manual Steps\n\n")
            f.write("1. Review and fix any Scroll-specific dependencies in extracted files\n")
            f.write("2. Implement any missing helper functions that were part of Scroll\n")
            f.write("3. Test compilation of scene-scroll as standalone library\n")
            f.write("4. Verify all sway_scene API functions are properly exported\n")
            
            f.write("\n## Detailed Log\n\n")
            f.write("```\n")
            for line in self.report_lines:
                f.write(f"{line}\n")
            f.write("```\n")
            
        self.log(f"\nReport generated: {report_path}")

if __name__ == "__main__":
    # Get repository root (current directory when run from repo root)
    repo_root = os.getcwd()
    
    # Create and run extractor
    extractor = ScrollSceneExtractor(repo_root)
    extractor.run()
