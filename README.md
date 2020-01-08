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

There are packages for the following Linux distributions:

- [Arch Linux package on the AUR](https://aur.archlinux.org/packages/wob/) [![AUR package](https://repology.org/badge/version-for-repo/aur/wob.svg?header=)](https://repology.org/project/wob/versions)
- [Alpine Linux package](https://pkgs.alpinelinux.org/packages?name=wob&branch=edge) [![Alpine Linux Edge package](https://repology.org/badge/version-for-repo/alpine_edge/wob.svg?header=)](https://repology.org/project/wob/versions)
- [NixOS package](https://github.com/NixOS/nixpkgs/blob/master/pkgs/tools/misc/wob/default.nix) [![nixpkgs unstable package](https://repology.org/badge/version-for-repo/nix_unstable/wob.svg?header=)](https://repology.org/project/wob/versions)
- [Void Linux package](https://github.com/void-linux/void-packages/blob/master/srcpkgs/wob/template) [![Void Linux x86_64 package](https://repology.org/badge/version-for-repo/void_x86_64/wob.svg?header=)](https://repology.org/project/wob/versions)

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

Add these lines to your Sway config file:

```
exec mkfifo $SWAYSOCK.wob && tail -f $SWAYSOCK.wob | wob
```

Volume using alsa:

```
bindsym XF86AudioRaiseVolume exec amixer -q set Master 2%+ unmute && amixer sget Master | grep 'Right:' | awk -F'[][]' '{ print substr($2, 0, length($2)-1) }' > $SWAYSOCK.wob
bindsym XF86AudioLowerVolume exec amixer -q set Master 2%- unmute && amixer sget Master | grep 'Right:' | awk -F'[][]' '{ print substr($2, 0, length($2)-1) }' > $SWAYSOCK.wob
bindsym XF86AudioMute exec (amixer get Master | grep off > /dev/null && amixer -q set Master unmute && amixer sget Master | grep 'Right:' | awk -F'[][]' '{ print substr($2, 0, length($2)-1) }' > $SWAYSOCK.wob) || (amixer -q set Master mute && echo 0 > $SWAYSOCK.wob)
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

## License

ISC, see [LICENSE](/LICENSE).
