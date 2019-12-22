project(
  'wob',
  'c',
  version: '0.4',
  license: 'ISC',
  default_options: ['c_std=c99']
  )

cc = meson.get_compiler('c')

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

wayland_scanner_code = generator(
  wayland_scanner,
  output: '@BASENAME@-protocol.c',
  arguments: ['private-code', '@INPUT@', '@OUTPUT@'],
)

wayland_scanner_client = generator(
  wayland_scanner,
  output: '@BASENAME@-client-protocol.h',
  arguments: ['client-header', '@INPUT@', '@OUTPUT@'],
)

client_protocols = [
  [wl_protocol_dir + '/stable/xdg-shell', 'xdg-shell.xml'],
  [meson.source_root(), 'wlr-layer-shell-unstable-v1.xml'],
]

foreach p : client_protocols
  xml = join_paths(p)
  src = wayland_scanner_code.process(xml)
  header = wayland_scanner_client.process(xml)

  name = p[1].split('.')[0].underscorify()

  lib = static_library(
    name,
    [src, header],
    dependencies: [wayland_client],
  )

  dep = declare_dependency(
    link_with: lib,
    sources: header,
  )

  set_variable(name, dep)
endforeach

executable(
  'wob',
  'wob.c',
  dependencies: [wayland_client, wlr_layer_shell_unstable_v1, xdg_shell, rt],
  install: true
)

e = executable(
  'wob-test', 
  'test.c',
  dependencies: [wayland_client, wlr_layer_shell_unstable_v1, xdg_shell, rt],
  c_args: ['-Wno-unused-function']
)
test('wob-test', e)

scdoc = dependency('scdoc', version: '>=1.9.2', native: true, required: get_option('man-page'))
if scdoc.found()
  scdoc = find_program(scdoc.get_pkgconfig_variable('scdoc'), native: true)
  sh = find_program('sh', native: true)
  mandir = get_option('mandir')
  scdfile = 'wob.1.scd'
  manfile = scdfile.split('.scd')[0]

  custom_target(
    manfile,
    input: scdfile,
    output: manfile,
    command: [
      sh, '-c', '@0@ < @INPUT@ > @1@'.format(scdoc.path(), manfile)
    ],
    install: true,
    install_dir: join_paths(mandir, 'man.1')
  )
endif
