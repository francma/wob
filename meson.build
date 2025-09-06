project(
  'wob',
  'c',
  version: '0.16',
  license: 'ISC',
  default_options: [
    'c_std=c99',
    'warning_level=2',
    'werror=true',
  ],
  meson_version: '>=0.60.0',
)

add_project_arguments(
  [
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
fs = import('fs')

prefix = get_option('prefix')

# minimal version with fractional scaling protocol
wayland_protos = dependency('wayland-protocols', version: '>=1.31')
wl_protocol_dir = wayland_protos.get_variable('pkgdatadir')
wayland_scanner = find_program('wayland-scanner')
wayland_client = dependency('wayland-client')
rt = cc.find_library('rt')
libm = cc.find_library('m')
seccomp = dependency('libseccomp', required: get_option('seccomp'))
inih = dependency('inih')

sysconfdir = get_option('sysconfdir')
if not fs.is_absolute(sysconfdir)
  sysconfdir = prefix / sysconfdir
endif

global_configuration_h = configuration_data()

global_configuration_h.set_quoted('WOB_VERSION', meson.project_version())
global_configuration_h.set_quoted('WOB_ETC_CONFIG_FOLDER_PATH', sysconfdir)

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
  global_configuration_h.set('WOB_PLEDGE_ENABLED', 'true')
else
  wob_sources += 'src/pledge.c'
  global_configuration_h.set('WOB_PLEDGE_ENABLED', 'false')
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

if get_option('systemd-unit-files').enabled()
  systemd_unit_dir = prefix / 'lib' / 'systemd' / 'user'

  conf_data = configuration_data()
  conf_data.set('bindir', prefix / get_option('bindir'))

  configure_file(
    input: 'etc/systemd/wob.service.in',
    output: 'wob.service',
    configuration: conf_data,
    install_dir: systemd_unit_dir,
  )
  install_data('etc/systemd/wob.socket', install_dir: systemd_unit_dir)
endif

configure_file(output: 'global_configuration.h', configuration: global_configuration_h)
