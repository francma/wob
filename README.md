# wob — Wayland Overlay Bar

[![Build Status](https://github.com/francma/wob/workflows/test/badge.svg)](https://github.com/francma/wob/actions)

![preview](https://martinfranc.eu/wob-preview.svg)

A lightweight overlay volume/backlight/progress/anything bar for wlroots based Wayland compositors (requrires support for `wlr_layer_shell_unstable_v1`). This project is inspired by [xob - X Overlay Bar](https://github.com/florentc/xob).

## Release signatures

Releases are signed with [5C6DA024DDE27178073EA103F4B432D5D67990E3](https://keys.openpgp.org/vks/v1/by-fingerprint/5C6DA024DDE27178073EA103F4B432D5D67990E3) and published on [GitHub](https://github.com/francma/wob/releases).

## Installation

### Compiling from source

Install dependencies:

- wayland
- wayland-protocols \*
- meson \*
- [scdoc](https://git.sr.ht/~sircmpwn/scdoc) (optional: man page) \*
- [libseccomp](https://github.com/seccomp/libseccomp) (optional: Linux kernel syscall filtering) \*

\* _compile-time dependecy_

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

You may manage a bar for audio volume, backlight intensity, or whatever, using a named pipe. Create a named pipe, e.g. /tmp/wobpipe, on your filesystem using.

```
mkfifo /tmp/wobpipe
```

Connect the named pipe to the standard input of an wob instance.

```
tail -f /tmp/wobpipe | wob
```

Set up your environment so that after updating audio volume, backlight intensity, or whatever, to a new value like 43, it writes that value into the pipe:

```
echo 43 > /tmp/wobpipe
```

Adapt this use-case to your workflow (scripts, callbacks, or keybindings handled by the window manager).

See [man page](https://github.com/francma/wob/blob/master/wob.1.scd) for styling and positioning options.

### Sway WM example

Add these lines to your Sway config file:

```
set $WOBSOCK $XDG_RUNTIME_DIR/wob.sock
exec mkfifo $WOBSOCK && tail -f $WOBSOCK | wob
```

Volume using alsa:

```
bindsym XF86AudioRaiseVolume exec amixer sset Master 5%+ | sed -En 's/.*\[([0-9]+)%\].*/\1/p' | head -1 > $WOBSOCK
bindsym XF86AudioLowerVolume exec amixer sset Master 5%- | sed -En 's/.*\[([0-9]+)%\].*/\1/p' | head -1 > $WOBSOCK
bindsym XF86AudioMute exec amixer sset Master toggle | sed -En '/\[on\]/ s/.*\[([0-9]+)%\].*/\1/ p; /\[off\]/ s/.*/0/p' | head -1 > $WOBSOCK
```

Volume using pulse audio:

```
bindsym XF86AudioRaiseVolume exec pamixer -ui 2 && pamixer --get-volume > $WOBSOCK
bindsym XF86AudioLowerVolume exec pamixer -ud 2 && pamixer --get-volume > $WOBSOCK
bindsym XF86AudioMute exec pamixer --toggle-mute && ( pamixer --get-mute && echo 0 > $WOBSOCK ) || pamixer --get-volume > $WOBSOCK
```

Brightness using [haikarainen/light](https://github.com/haikarainen/light):

```
bindsym XF86MonBrightnessUp exec light -A 5 && light -G | cut -d'.' -f1 > $WOBSOCK
bindsym XF86MonBrightnessDown exec light -U 5 && light -G | cut -d'.' -f1 > $WOBSOCK
```

Brightness using [brightnessctl](https://github.com/Hummer12007/brightnessctl):

```
bindsym XF86MonBrightnessDown exec brightnessctl set 5%- | sed -En 's/.*\(([0-9]+)%\).*/\1/p' > $WOBSOCK
bindsym XF86MonBrightnessUp exec brightnessctl set +5% | sed -En 's/.*\(([0-9]+)%\).*/\1/p' > $WOBSOCK
```

#### Systemd

Add this line to your config file:

```
exec systemctl --user import-environment DISPLAY WAYLAND_DISPLAY SWAYSOCK
```

Copy systemd unit files (if not provided by your distribution package):

```
cp contrib/systemd/wob.{service,socket} ~/.local/share/systemd/user/
systemctl daemon-reload --user
```

Enable systemd wob socket:

```
systemctl enable --now --user wob.socket
```

## License

ISC, see [LICENSE](/LICENSE).
