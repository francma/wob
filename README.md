# wob — Wayland Overlay Bar

[![Build Status](https://github.com/francma/wob/workflows/test/badge.svg)](https://github.com/francma/wob/actions)

![preview](https://martinfranc.eu/wob-preview.svg)

Lightweight overlay volume, brightness (or anything) bar for Wayland compositors with support for [wlr_layer_shell_unstable_v1](https://wayland.app/protocols/wlr-layer-shell-unstable-v1) protocol.

## Release signatures

Releases are signed with [5C6DA024DDE27178073EA103F4B432D5D67990E3](https://keys.openpgp.org/vks/v1/by-fingerprint/5C6DA024DDE27178073EA103F4B432D5D67990E3) and published on [GitHub](https://github.com/francma/wob/releases).

## Installation

### Compiling from source

Install dependencies:

- wayland
- [inih](https://github.com/benhoyt/inih)
- wayland-protocols \*
- meson \*
- [scdoc](https://git.sr.ht/~sircmpwn/scdoc) (optional: man page) \*
- [libseccomp](https://github.com/seccomp/libseccomp) (optional: Linux kernel syscall filtering) \*
- [cmocka](https://cmocka.org/) (optional: tests) \*

\* _compile-time dependency_

Run these commands:

```
git clone git@github.com:francma/wob.git
cd wob
meson build
ninja -C build
sudo ninja -C build install
```

### From packages

[![Packaging status](https://repology.org/badge/tiny-repos/wob.svg)](https://repology.org/project/wob/versions)

## Usage

Launch wob in a terminal, enter a value (positive integer), press return.

```
wob
```

### General case

You may manage a bar for audio volume, backlight intensity, or whatever, using a named pipe. Create a named pipe, e.g. `/tmp/wobpipe`, on your filesystem using.

```
mkfifo /tmp/wobpipe
```

Connect the named pipe to the standard input of a wob instance.

```
tail -f /tmp/wobpipe | wob
```

Set up your environment so that after updating audio volume, backlight intensity, or whatever, to a new value like 43, it writes that value into the pipe:

```
echo 43 > /tmp/wobpipe
```

Adapt this use-case to your workflow (scripts, callbacks, or keybindings handled by the window manager).

See [wob.ini.5](https://github.com/francma/wob/blob/master/wob.ini.5.scd) for styling and positioning options.

See [systemd](etc/systemd/README.md) for systemd integration.

## Examples

See [Contrib space](contrib/README.md).

## Alternatives

- [avizo](https://github.com/misterdanb/avizo) - icons, volume and brightness
- [SwayOSD](https://github.com/ErikReider/SwayOSD) - icons, volume, brightness and keyboard (caps lock, num lock)
- Notification daemon + [custom script](https://github.com/luispabon/sway-dotfiles/blob/master/scripts/notifications/audio-notification.sh)
- [xob - X Overlay Bar](https://github.com/florentc/xob) - X11 only, inspiration for wob

## License

ISC, see [LICENSE](/LICENSE).
