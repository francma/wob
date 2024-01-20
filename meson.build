project(
  'wob',
  'c',
  version: '0.15.1',
  license: 'ISC',
  default_options: ['c_std=c99'],
  meson_version: '>=0.60.0',
)

cc = meson.get_compiler('c')

# minimal version with fractional scaling protocol
wayland_protos = dependency('wayland-protocols', version: '>=1.31')
wl_protocol_dir = wayland_protos.get_variable('pkgdatadir')
wayland_scanner = find_program('wayland-scanner')
wayland_client = dependency('wayland-client')
rt = cc.find_library('rt')
libm = cc.find_library('m')
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

wl_proto_xml = [
  wl_protocol_dir / 'stable/xdg-shell/xdg-shell.xml',
  wl_protocol_dir / 'stable/viewporter/viewporter.xml',
  'protocols/wlr-layer-shell-unstable-v1.xml',
  wl_protocol_dir / 'staging/fractional-scale/fractional-scale-v1.xml',
]

wl_proto_headers = []
wl_proto_src = []
foreach proto : wl_proto_xml
  wl_proto_headers += custom_target(
    proto.underscorify() + '_client_header',
    output: '@BASENAME@.h',
    input: proto,
    command: [wayland_scanner, 'client-header', '@INPUT@', '@OUTPUT@'])

  wl_proto_src += custom_target(
    proto.underscorify() + '_private_code',
    output: '@BASENAME@.c',
    input: proto,
    command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@'])
endforeach

wob_sources = ['src/main.c', 'src/image.c', 'src/log.c', 'src/color.c', 'src/config.c', 'src/wob.c', 'src/shm.c', wl_proto_src, wl_proto_headers]
wob_dependencies = [wayland_client, rt, inih, libm]
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

cmocka = dependency('cmocka', required: get_option('tests'))
if cmocka.found()
  test('color', executable(
    'color_test',
    ['test/color_test.c', 'src/color.c'],
    dependencies: [cmocka]
  ))
endif

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
