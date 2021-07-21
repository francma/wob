project(
  'wob',
  'c',
  version: '0.13',
  license: 'ISC',
  default_options: ['c_std=c99']
)

cc = meson.get_compiler('c')

wayland_protos = dependency('wayland-protocols', version: '>=1.13')
wl_protocol_dir = wayland_protos.get_pkgconfig_variable('pkgdatadir')
wayland_scanner = find_program('wayland-scanner')
wayland_client = dependency('wayland-client')
rt = cc.find_library('rt')
seccomp = dependency('libseccomp', required: get_option('seccomp'))
inih = dependency('inih')

wob_version = '"@0@"'.format(meson.project_version())
add_project_arguments('-DWOB_VERSION=@0@'.format(wob_version), language: 'c')

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
  [wl_protocol_dir + '/unstable/xdg-output', 'xdg-output-unstable-v1.xml'],
  [meson.source_root() + '/protocols', 'wlr-layer-shell-unstable-v1.xml'],
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

wob_inc = include_directories('include')

wob_sources = ['main.c', 'parse.c', 'buffer.c', 'log.c', 'color.c', 'config.c']
wob_dependencies = [xdg_output_unstable_v1, wayland_client, wlr_layer_shell_unstable_v1, xdg_shell, rt, inih]
if seccomp.found()
  wob_dependencies += seccomp
  wob_sources += 'pledge_seccomp.c'
else
  wob_sources += 'pledge.c'
endif

executable(
  'wob',
  wob_sources,
  include_directories: [wob_inc],
  dependencies: wob_dependencies,
  install: true
)

test('parse-input', executable(
  'test-parse-input',
  ['tests/wob_parse_input.c', 'parse.c', 'color.c'],
  include_directories: [wob_inc]
))

scdoc = dependency('scdoc', version: '>=1.9.2', native: true, required: get_option('man-pages'))
if scdoc.found()
  scdfiles = ['wob.1.scd', 'wob.ini.5.scd']
  scdoc = find_program(scdoc.get_pkgconfig_variable('scdoc'), native: true)
  sh = find_program('sh', native: true)
  mandir = get_option('mandir')

  foreach scdfile : scdfiles
    manfile = scdfile.split('.scd')[0]
    section = scdfile.split('.')[-2]

    custom_target(
      manfile,
      input: scdfile,
      output: manfile,
      command: [
        sh, '-c', '@0@ < @INPUT@ > @1@'.format(scdoc.path(), manfile)
      ],
      install: true,
      install_dir: 'man@1@'.format(mandir, section)
    )
  endforeach
endif
