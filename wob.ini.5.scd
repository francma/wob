wob.ini(5)

# NAME

wob.ini - wob configuration file

You can run `wob -vv` to find the default values.

# SECTION: default

*timeout*
	Timeout after which wob hides itself, in milliseconds.

*max*
	This value will be represented as fully filled bar.

*width*
	Width of wob, in pixels.

*height*
	Height of wob, in pixels.

*border_offset*
	Border offset, in pixels.

*border_size*
	Border size, in pixels.

*bar_padding*
	Bar padding, in pixels.

*anchor*
	Anchor point, combination of *top*, *left*, *right*, *bottom*, *center*.

	Example: *bottom* *right*

*margin*
	Anchor margin, in pixels. Either as a single value or 4 values (top right bottom left).

*border_color*
	Border color, in RRGGBB[AA] format.

*background_color*
	Background color, in RRGGBB[AA] format.

*bar_color*
	Bar color, in RRGGBB[AA] format.

*overflow_mode*
	Overflow mode, one of *wrap*, and *nowrap*.

	*wrap*: wrap value so *max* + 20 will be displayed as 20

	*nowrap* values > *max* will be displayed as just *max*

*overflow_bar_color*
	Overflow bar color, in RRGGBB[AA] format.

*overflow_background_color*
	Overflow background color, in RRGGBB[AA] format.

*overflow_border_color*
	Overflow border color, in RRGGBB[AA] format.

*orientation*
	Orientation of the bar, one of *horizontal* and *vertical*.

	*width* and *height* is kept as is, you most likely want to set *height* greater than *width* in *vertical* mode

# SECTION: output.*

Replace *\** with user friendly name of your choosing.

*match*
	Substring to match the output.

	Example: DP-1, Dell U2722DE

*width*
	Width of wob, in pixels.

*height*
	Height of wob, in pixels.

*border_offset*
	Border offset, in pixels.

*border_size*
	Border size, in pixels.

*bar_padding*
	Bar padding, in pixels.

*anchor*
	Anchor point, combination of *top*, *left*, *right*, *bottom*, *center*.

	Example: *bottom* *right*

*margin*
	Anchor margin, in pixels. Either as a single value or 4 values (top right bottom left).

# SECTION: style.*

*border_color*
	Border color, in RRGGBB[AA] format.

*background_color*
	Background color, in RRGGBB[AA] format.

*bar_color*
	Bar color, in RRGGBB[AA] format.

*overflow_bar_color*
	Overflow bar color, in RRGGBB[AA] format.

*overflow_background_color*
	Overflow background color, in RRGGBB[AA] format.

*overflow_border_color*
	Overflow border color, in RRGGBB[AA] format.

# EXAMPLE

```
timeout = 1000
max = 100
bar_color = FFFFFF

[output.left]
name = DP-1

[output.ips]
match = DELL U2722DE
width = 200
height = 30

[style.muted]
background_color = 032cfc
```

# See also

*wob*(1)
