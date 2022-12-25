wob(1)

# NAME

wob - Wayland Overlay Bar

# DESCRIPTION

wob is a lightweight overlay volume/backlight/progress/anything bar for Wayland.

# SYNOPSIS

*wob* [options...]

# OPTIONS

*-c, --config <file>*
	Configuration file location.

*-v, --verbose*
	Increase verbosity of messages, defaults to errors and warnings only.

*-V, --version*
	Show the version number and quit.

*-h, --help*
	Show help message and quit.

# USAGE

wob reads values to display from standard input in the following formats:

<value>

or

<value> <style>

Where <value> is number in interval from 0 to *max* and <style> is style defined in *wob.ini*(5).

# CONFIGURATION

wob searches for a config file in the following locations, in this order:

1. $XDG_CONFIG_HOME/wob/wob.ini
2. ~/.config/wob/wob.ini
3. /etc/wob/wob.ini

For information on the config file format, see *wob.ini*(5).

# ENVIRONMENT

The following environment variables have an effect on wob:

*WOB_DISABLE_PLEDGE*
	Disable seccomp syscall filtering on Linux. Set this if you are having trouble running wob with tools like valgrind.

# See also

*wob.ini*(5)
