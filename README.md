# wob — Wayland Overlay Bar

[![Build Status](https://travis-ci.org/francma/wob.svg?branch=master)](https://travis-ci.org/francma/wob)

![preview](https://martinfranc.eu/wob-preview.svg)

A lightweight overlay volume/backlight/progress/anything bar for Wayland. This project is inspired by [xob - X Overlay Bar](https://github.com/florentc/xob).

## Installation

```
git clone --recursive git@github.com:francma/wob.git
ninja -C build-release
sudo ninja -C build-release install
```

There are packages for the following Linux distributions:

- [Arch Linux package on the AUR](https://aur.archlinux.org/packages/wob/)

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

### Sway WM example

```
exec mkfifo /tmp/wobpipe && tail -f /tmp/wobpipe | wob
```

Volume using alsa:

```
bindsym XF86AudioRaiseVolume exec amixer -q set Master 2%+ unmute && amixer sget Master | grep 'Right:' | awk -F'[][]' '{ print substr($2, 0, length($2)-1) }' > /tmp/wobpipe
bindsym XF86AudioLowerVolume exec amixer -q set Master 2%- unmute && amixer sget Master | grep 'Right:' | awk -F'[][]' '{ print substr($2, 0, length($2)-1) }' > /tmp/wobpipe
```

Volume using pulse audio:

```
bindsym XF86AudioRaiseVolume exec pamixer -ui 2 && pamixer --get-volume > /tmp/wobpipe
bindsym XF86AudioLowerVolume exec pamixer -ud 2 && pamixer --get-volume > /tmp/wobpipe
```

Brightness using [haikarainen/light](https://github.com/haikarainen/light):

```
bindsym XF86MonBrightnessUp exec light -A 5 && light -G | cut -d'.' -f1 > /tmp/wobpipe
bindsym XF86MonBrightnessDown exec light -U 5 && light -G | cut -d'.' -f1 > /tmp/wobpipe
```

## License

ISC, see [LICENSE](/LICENSE).
