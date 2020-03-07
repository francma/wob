wob(1)

# NAME

wob - Wayland Overlay Bar

# DESCRIPTION

wob is a lightweight overlay volume/backlight/progress/anything bar for Wayland.

# SYNOPSIS

*wob* [options...]

# OPTIONS

*-h, --help*
	Show help message and quit.

*-v, --version*
	Show the version number and quit.

*-t --timeout* <ms> 
	Hide wob after <ms> milliseconds, defaults to 1000.

*-m --max* <%>
	Define the maximum percentage, defaults to 100.

*-W --width* <px>
	Define bar width in pixels, defaults to 400.

*-H --height* <px>
	Define bar height in pixels, defaults to 50.

*-o --output* <px>
	Define border offset in pixels, defaults to 4.

*-b --border* <px>
	Define border size in pixels, defaults to 4.

*-p --padding* <px>
	Define bar padding in pixels, defaults to 4.

*-a --anchor* <side>
	Define anchor point, one of 'top', 'left', 'right', 'bottom', 'center' (default).
	May be specified multiple times.

*-M --margin* <px>
	Define anchor margin in pixels, defaults to 0.

*-O --output* <name>
	Define output to show bar on or '\*' for all. If ommited, focused output is chosen.
	May be specified multiple times.

*--border-color* <#argb>
	Define border color

*--background-color* <#argb>
	Define background color

*--bar-color* <#argb>
	Define bar color

# USAGE

Launch wob in a terminal, enter a value (positive integer), press return.

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

# ENVIRONMENT

The following environment variables have an effect on wob:

*WOB_DISABLE_PLEDGE*
	Disable seccomp call on Linux. Set this if you are havind trouble running wob with tools like valgrind or strace.
