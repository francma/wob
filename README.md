# wob — Wayland Overlay Bar

[![Build Status](https://travis-ci.com/francma/wob.svg?branch=master)](https://travis-ci.com/francma/wob)

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

First of all, we need to create a
[fifo](https://man7.org/linux/man-pages/man7/fifo.7.html) and start
`wob` with this fifo file as standard input.

Either add this line to your Sway config file:

```
exec mkfifo $SWAYSOCK.wob && tail -f $SWAYSOCK.wob | wob
```

Or, in case you [manage your tools with
systemd](https://github.com/swaywm/sway/issues/5160) you can add two
units that will start the daemon:

`~/.config/systemd/user/wob.socket`

```
[Socket]
ListenFIFO=%t/wob.fifo
SocketMode=0600

[Install]
WantedBy=sockets.target
RequiredBy=wob.service
```

`~/.config/systemd/user/wob.service`

```
[Unit]
Description=A lightweight overlay volume/backlight/progress/anything bar for Wayland
Documentation=man:wob(1)
PartOf=graphical-session.target

[Service]
Type=simple
StandardInput=socket
ExecStart=/usr/bin/wob

[Install]
WantedBy=sway-session.target
```

> **Note:** You need to start and enable the service (in terminal):
>
> ```
> systemctl --user enable --now wob.service
> ```

> **Note:** You'll need to replace `$SWAYSOCK.wob` with
> `/run/user/1000/wob.fifo` where `1000` is your user's ID.

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

## License

ISC, see [LICENSE](/LICENSE).
