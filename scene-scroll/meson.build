project(
	'scene-scroll',
	'c',
	version: '1.0.0',
	license: 'MIT',
	meson_version: '>=1.3',
	default_options: [
		'c_std=c11',
		'warning_level=2',
		'werror=false',
		'wrap_mode=nodownload',
	],
)

add_project_arguments(
	[
		'-DWLR_USE_UNSTABLE',
		'-D_POSIX_C_SOURCE=200809L',
		'-Wno-unused-parameter',
		'-Wno-unused-result',
		'-Wno-missing-braces',
		'-Wno-format-zero-length',
		'-Wundef',
		'-Wvla',
	],
	language: 'c',
)

cc = meson.get_compiler('c')

# Dependencies required for scene-scroll
wlroots_version = ['>=0.20.0', '<0.21.0']
wlroots = dependency('wlroots-0.20', version: wlroots_version)
wayland_server = dependency('wayland-server', version: '>=1.21.0')
pixman = dependency('pixman-1')
math = cc.find_library('m')

# Check wlroots features
wlroots_features = {
	'xwayland': false,
}
foreach name, _ : wlroots_features
	var_name = 'have_' + name.underscorify()
	have = wlroots.get_variable(pkgconfig: var_name, internal: var_name) == 'true'
	wlroots_features += { name: have }
endforeach

# Source files for scene-scroll library
scene_scroll_sources = [
	'src/scene.c',
	'src/color.c',
	'src/debug.c',
	'src/drag_icon.c',
	'src/ext_image_capture_source_v1.c',
	'src/layer_shell_v1.c',
	'src/output.c',
	'src/output_layout.c',
	'src/subsurface_tree.c',
	'src/surface.c',
	'src/xdg_shell.c',
]

# Include directories
scene_scroll_inc = include_directories('include')

# Create the scene-scroll library
scene_scroll_lib = library(
	'scene-scroll',
	scene_scroll_sources,
	include_directories: scene_scroll_inc,
	dependencies: [
		wlroots,
		wayland_server,
		pixman,
		math,
	],
	version: meson.project_version(),
	install: true,
)

# Install headers
install_headers(
	'include/scene-scroll/scene.h',
	subdir: 'scene-scroll',
)

# Generate pkg-config file
pkg = import('pkgconfig')
pkg.generate(
	scene_scroll_lib,
	description: 'Scroll\'s modified scene graph library',
	url: 'https://github.com/scrollwm/scrollfx-wip',
)

# Make the library findable for other projects
scene_scroll_dep = declare_dependency(
	link_with: scene_scroll_lib,
	include_directories: scene_scroll_inc,
	dependencies: [
		wlroots,
		wayland_server,
		pixman,
		math,
	],
)