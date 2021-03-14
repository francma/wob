# wob — Wayland Overlay Bar

[![Build Status](https://github.com/francma/wob/workflows/test/badge.svg)](https://github.com/francma/wob/actions)

![preview](https://martinfranc.eu/wob-preview.svg)

A lightweight overlay volume/backlight/progress/anything bar for Wayland. This project is inspired by [xob - X Overlay Bar](https://github.com/florentc/xob).

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

### Configuration

Configuration variables can be provided from the command line or as environment variables. For a list of options, see `wob --help`.
For each command line options, the corresponding environment variable is prefixed by `WOB_` and has the same name converted to uppercase
and with dashes replaced with underscores (e.g. `--bar-color` becomes `WOB_BAR_COLOR`).

#### Logging:

Have these variables set to anything to set logging level:

- WOB_VERBOSE: set log level to INFO
- WOB_DEBUG: set log level to DEBUG
- WOB_ERROR: set log level to ERROR

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
exec mkfifo $SWAYSOCK.wob && tail -f $SWAYSOCK.wob | wob
```

Volume using alsa:

```
bindsym XF86AudioRaiseVolume exec amixer sset Master 5%+ | sed -En 's/.*\[([0-9]+)%\].*/\1/p' | head -1 > $SWAYSOCK.wob
bindsym XF86AudioLowerVolume exec amixer sset Master 5%- | sed -En 's/.*\[([0-9]+)%\].*/\1/p' | head -1 > $SWAYSOCK.wob
bindsym XF86AudioMute exec amixer sset Master toggle | sed -En '/\[on\]/ s/.*\[([0-9]+)%\].*/\1/ p; /\[off\]/ s/.*/0/p' | head -1 > $SWAYSOCK.wob
```

Volume using pulse audio:

```
bindsym XF86AudioRaiseVolume exec pamixer -ui 2 && pamixer --get-volume > $SWAYSOCK.wob
bindsym XF86AudioLowerVolume exec pamixer -ud 2 && pamixer --get-volume > $SWAYSOCK.wob
bindsym XF86AudioMute exec pamixer --toggle-mute && ( pamixer --get-mute && echo 0 > $SWAYSOCK.wob ) || pamixer --get-volume > $SWAYSOCK.wob
```

Brightness using [haikarainen/light](https://github.com/haikarainen/light):

```
bindsym XF86MonBrightnessUp exec light -A 5 && light -G | cut -d'.' -f1 > $SWAYSOCK.wob
bindsym XF86MonBrightnessDown exec light -U 5 && light -G | cut -d'.' -f1 > $SWAYSOCK.wob
```

See the wiki for useful scripts.

#### Sway+Systemd

Copy the `.service` and `.socket` in `~/.local/share/systemd/user`:

```
cp systemd/wob.{service,socket} ~/.local/share/systemd/user/
```

add the following to your sway configuration:

```
set $WOBSOCK $XDG_RUNTIME_DIR/wob.sock
```

replace `$SWAYSOCK.wob` with `$WOBSOCK` in the above examples. Wob can be restarted with systemctl:

```
systemctl --user restart wob
```

Appeareance environment variables can be set in `$XDG_SESSION_HOME/wob.conf` (usually `~/.config/wob.conf`)

Note that `wob` is automatically started whenever something is written to `$WOBSOCKET` due to the systemd socket unit `wob.socket`.

## License

ISC, see [LICENSE](/LICENSE).
