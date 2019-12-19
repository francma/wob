project(
  'wob',
  'c',
  version: '0.4',
  license: 'ISC',
  default_options: ['c_std=c99']
  )

cc = meson.get_compiler('c')

wlr_protocol_dir = meson.source_root() + '/wlr-protocols'
wayland_protos = dependency('wayland-protocols', version: '>=1.13')
wl_protocol_dir = wayland_protos.get_pkgconfig_variable('pkgdatadir')
wayland_scanner = find_program('wayland-scanner')
wayland_client = dependency('wayland-client')
rt = cc.find_library('rt')

wob_version = '"@0@"'.format(meson.project_version())
add_project_arguments('-DWOB_VERSION=@0@'.format(wob_version), language: 'c')

if get_option('buildtype').startswith('debug')
	add_project_arguments('-DDEBUG', language: 'c')
endif

xdg_shell_h = custom_target(
	'xdg-shell-client-protocol.h',
	output: 'xdg-shell-client-protocol.h',
	command: [wayland_scanner, 'client-header', wl_protocol_dir + '/stable/xdg-shell/xdg-shell.xml', '@OUTPUT@'],
)

xdg_shell_c = custom_target(
	'xdg-shell-client-protocol.c',
	output: 'xdg-shell-client-protocol.c',
	command: [wayland_scanner, 'private-code', wl_protocol_dir + '/stable/xdg-shell/xdg-shell.xml', '@OUTPUT@'],
)

wlr_layer_shell_unstable_v1_h = custom_target(
	'wlr-layer-shell-unstable-v1.h',
	output: 'wlr-layer-shell-unstable-v1.h',
	command: [wayland_scanner, 'client-header', wlr_protocol_dir + '/unstable/wlr-layer-shell-unstable-v1.xml', '@OUTPUT@'],
)

wlr_layer_shell_unstable_v1_c = custom_target(
	'wlr-layer-shell-unstable-v1.c',
	output: 'wlr-layer-shell-unstable-v1.c',
	command: [wayland_scanner, 'private-code', wlr_protocol_dir + '/unstable/wlr-layer-shell-unstable-v1.xml', '@OUTPUT@'],
)

xdg_shell_lib = static_library('xdg_shell_lib', [xdg_shell_c, xdg_shell_h], dependencies: [wayland_client])
xdg_shell = declare_dependency(link_with: xdg_shell_lib)

wlr_layer_shell_unstable_v1_lib = static_library('wlr_layer_shell_unstable_v1_lib', [wlr_layer_shell_unstable_v1_c, wlr_layer_shell_unstable_v1_h], dependencies: [wayland_client])
wlr_layer_shell_unstable_v1 = declare_dependency(link_with: wlr_layer_shell_unstable_v1_lib)

executable(
	'wob',
	'wob.c',
	wlr_layer_shell_unstable_v1_h,
	dependencies: [wayland_client, wlr_layer_shell_unstable_v1, xdg_shell, rt],
	install: true
)

e = executable(
	'wob-test', 
	'test.c',
	wlr_layer_shell_unstable_v1_h,
	dependencies: [wayland_client, wlr_layer_shell_unstable_v1, xdg_shell, rt],
	c_args: ['-Wno-unused-function']
)
test('wob-test', e)
