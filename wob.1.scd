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

*--version*
	Show the version number and quit.

*-v*
	Increase verbosity of messages, defaults to errors and warnings only.

*-t --timeout* <ms> 
	Hide wob after <ms> milliseconds, defaults to 1000.

*-m --max* <%>
	Define the maximum percentage, defaults to 100.

*-W --width* <px>
	Define bar width in pixels, defaults to 400.

*-H --height* <px>
	Define bar height in pixels, defaults to 50.

*-o --offset* <px>
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

*--border-color* <#RRGGBBAA>
	Define border color, defaults to #FFFFFFFF.

*--background-color* <#RRGGBBAA>
	Define background color, defaults to #000000FF.

*--bar-color* <#RRGGBBAA>
	Define bar color, defaults to #FFFFFFFF.

*--overflow-mode <mode>*
	Change the overflow mode. Valid options are `none`, `wrap`, and `nowrap`.

*--overflow-bar-color* <#AARRGGBB>
	Define overflow bar color, defaults to #FFFF0000

*--overflow-background-color* <#AARRGGBB>
	Define overflow background color, defaults to #FF000000

*--overflow-border-color* <#AARRGGBB>
	Define overflow border color, defaults to #FFFFFFFF	

# USAGE

Wob reads values to display from standart input in the following formats:

<value>

or

<value> <#background_color> <#border_color> <#bar_color>

Where <value> is number in interval from 0 to *--max* and <#\*color> is color in #RRGGBBAA format.

# ENVIRONMENT

The following environment variables have an effect on wob:

*WOB_DISABLE_PLEDGE*
	Disable seccomp syscall filtering on Linux. Set this if you are having trouble running wob with tools like valgrind or strace.
