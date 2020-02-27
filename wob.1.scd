wob(1)

# NAME

wob - Wayland Overlay Bar

# DESCRIPTION

wob is a lightweight overlay volume/backlight/progress/anything bar for Wayland.

# SYNOPSIS

*wob* [options...]

# OPTIONS

*-h*
	Show help message and quit.

*-v*
	Show the version number and quit.

*-t* <ms> 
	Hide wob after <ms> milliseconds, defaults to 1000.

*-m* <%>
	Define the maximum percentage, defaults to 100.

*-W* <px>
	Define bar width in pixels, defaults to 400.

*-H* <px>
	Define bar height in pixels, defaults to 50.

*-o* <px>
	Define border offset in pixels, defaults to 4.

*-b* <px>
	Define border size in pixels, defaults to 4.

*-p* <px>
	Define bar padding in pixels, defaults to 4.

*-a* <side>
	Define anchor point, one of 'top', 'left', 'right', 'bottom', 'center' (default).
	May be specified multiple times.

*-M* <px>
	Define anchor margin in pixels, defaults to 0.

*-O* <name>
	Define output to show bar on or '\*' for all. If ommited, focused output is chosen.
	May be specified multiple times.

# USAGE

Launch wob in a terminal, enter a value (positive integer), press return.

## General case

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

## Sway WM example

Add these lines to your Sway config file:

```
exec mkfifo $SWAYSOCK.wob && tail -f $SWAYSOCK.wob | wob
```

Volume using alsa:

```
bindsym XF86AudioRaiseVolume exec amixer -q set Master 2%+ unmute && amixer sget Master | grep 'Right:' | awk -F'[][]' '{ print substr($2, 0, length($2)-1) }' > $SWAYSOCK.wob++
bindsym XF86AudioLowerVolume exec amixer -q set Master 2%- unmute && amixer sget Master | grep 'Right:' | awk -F'[][]' '{ print substr($2, 0, length($2)-1) }' > $SWAYSOCK.wob++
bindsym XF86AudioMute exec (amixer get Master | grep off > /dev/null && amixer -q set Master unmute && amixer sget Master | grep 'Right:' | awk -F'[][]' '{ print substr($2, 0, length($2)-1) }' > $SWAYSOCK.wob) || (amixer -q set Master mute && echo 0 > $SWAYSOCK.wob)
```

Volume using pulse audio:

```
bindsym XF86AudioRaiseVolume exec pamixer -ui 2 && pamixer --get-volume > $SWAYSOCK.wob
bindsym XF86AudioLowerVolume exec pamixer -ud 2 && pamixer --get-volume > $SWAYSOCK.wob
```

Brightness using haikarainen/light:

```
bindsym XF86MonBrightnessUp exec light -A 5 && light -G | cut -d'.' -f1 > $SWAYSOCK.wob
bindsym XF86MonBrightnessDown exec light -U 5 && light -G | cut -d'.' -f1 > $SWAYSOCK.wob
```
