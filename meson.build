project(
  'wob',
  'c',
  version: '0.14.2',
  license: 'ISC',
  default_options: ['c_std=c99'],
  meson_version: '>=0.60.0',
)

cc = meson.get_compiler('c')

wayland_protos = dependency('wayland-protocols', version: '>=1.13')
wl_protocol_dir = wayland_protos.get_variable('pkgdatadir')
wayland_scanner = find_program('wayland-scanner')
wayland_client = dependency('wayland-client')
rt = cc.find_library('rt')
seccomp = dependency('libseccomp', required: get_option('seccomp'))
inih = dependency('inih')
pixman = dependency('pixman-1')

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
  [meson.project_source_root() + '/protocols', 'wlr-layer-shell-unstable-v1.xml'],
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

wob_sources = ['src/main.c', 'src/image.c', 'src/log.c', 'src/color.c', 'src/config.c', 'src/wob.c']
wob_dependencies = [wayland_client, wlr_layer_shell_unstable_v1, xdg_shell, rt, inih, pixman]
if seccomp.found()
  wob_dependencies += seccomp
  wob_sources += 'src/pledge_seccomp.c'
else
  wob_sources += 'src/pledge.c'
endif

executable(
  'wob',
  wob_sources,
  dependencies: wob_dependencies,
  install: true
)

scdoc = dependency('scdoc', version: '>=1.9.2', native: true, required: get_option('man-pages'))
if scdoc.found()
  scdfiles = ['wob.1.scd', 'wob.ini.5.scd']
  mandir = get_option('mandir')

  foreach scdfile : scdfiles
    manfile = scdfile.split('.scd')[0]
    section = scdfile.split('.')[-2]

    custom_target(
      manfile,
      input: scdfile,
      output: manfile,
      command: scdoc.get_variable('scdoc'),
      feed: true,
      capture: true,
      install: true,
      install_dir: '@0@/man@1@'.format(mandir, section)
    )
  endforeach
endif
