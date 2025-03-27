# Contrib space

## Examples

### Sway

Add these lines to your Sway config file:

```
set $WOBSOCK $XDG_RUNTIME_DIR/wob.sock
exec rm -f $WOBSOCK && mkfifo $WOBSOCK && tail -f $WOBSOCK | wob
```

And bind volume/brightness, so it pipes current value to `$WOBSOCK`.

```
bindsym XF86AudioRaiseVolume exec [raise_and_get_volume_command] > $WOBSOCK
bindsym XF86AudioLowerVolume exec [lower_and_get_volume_command] > $WOBSOCK
bindsym XF86AudioMute exec [mute_and_get_volume_command] > $WOBSOCK

bindsym XF86MonBrightnessUp exec [raise_and_get_brigtness_command] > $WOBSOCK
bindsym XF86MonBrightnessDown exec [lower_and_get_brightness_command] > $WOBSOCK
```

### Volume using Alsa

```
amixer sset Master 5%+ | sed -En 's/.*\[([0-9]+)%\].*/\1/p' | head -1 > $WOBSOCK
amixer sset Master 5%- | sed -En 's/.*\[([0-9]+)%\].*/\1/p' | head -1 > $WOBSOCK
amixer sset Master toggle | sed -En '/\[on\]/ s/.*\[([0-9]+)%\].*/\1/ p; /\[off\]/ s/.*/0/p' > $WOBSOCK
```

### Volume using PulseAudio (pamixer)

```
bindsym XF86AudioRaiseVolume exec pamixer -ui 2 && pamixer --get-volume > $WOBSOCK
bindsym XF86AudioLowerVolume exec pamixer -ud 2 && pamixer --get-volume > $WOBSOCK
bindsym XF86AudioMute exec pamixer --toggle-mute && ( [ "$(pamixer --get-mute)" = "true" ] && echo 0 > $WOBSOCK ) || pamixer --get-volume > $WOBSOCK
```

### Volume PulseAudio (pactl)

```
bindsym XF86AudioRaiseVolume exec pactl set-sink-volume @DEFAULT_SINK@ +5% && pactl get-sink-volume @DEFAULT_SINK@ | awk 'NR==1{print substr($5,1,length($5)-1)}' > $WOBSOCK
bindsym XF86AudioLowerVolume exec pactl set-sink-volume @DEFAULT_SINK@ -5% && pactl get-sink-volume @DEFAULT_SINK@ | awk 'NR==1{print substr($5,1,length($5)-1)}' > $WOBSOCK
bindsym XF86AudioMute exec pactl set-sink-mute @DEFAULT_SINK@ toggle && ( [ "$(pactl get-sink-mute @DEFAULT_SINK@)" = "Mute: yes" ] && echo 0 > $WOBSOCK ) || pactl get-sink-volume @DEFAULT_SINK@ | awk 'NR==1{print substr($5,1,length($5)-1)}' > $WOBSOCK
```

### Volume using Pipewire

```
wpctl set-volume @DEFAULT_AUDIO_SINK@ 2%+ && wpctl get-volume @DEFAULT_AUDIO_SINK@ | sed 's/[^0-9]//g' > $WOBSOCK
wpctl set-volume @DEFAULT_AUDIO_SINK@ 2%- && wpctl get-volume @DEFAULT_AUDIO_SINK@ | sed 's/[^0-9]//g' > $WOBSOCK
wpctl set-mute @DEFAULT_AUDIO_SINK@ toggle && (wpctl get-volume @DEFAULT_AUDIO_SINK@ | grep -q MUTED && echo 0 > $WOBSOCK) || wpctl get-volume @DEFAULT_AUDIO_SINK@ | sed 's/[^0-9]//g' > $WOBSOCK
```

### Volume using Pipewire (alternative, if muted then block vol up/down and show 0):

```
(wpctl get-volume @DEFAULT_AUDIO_SINK@ | grep -q MUTED && echo 0 > $WOBSOCK) || (wpctl set-volume @DEFAULT_AUDIO_SINK@ 2%+ && wpctl get-volume @DEFAULT_AUDIO_SINK@ | sed 's/[^0-9]//g' > $WOBSOCK)
(wpctl get-volume @DEFAULT_AUDIO_SINK@ | grep -q MUTED && echo 0 > $WOBSOCK) || (wpctl set-volume @DEFAULT_AUDIO_SINK@ 2%- && wpctl get-volume @DEFAULT_AUDIO_SINK@ | sed 's/[^0-9]//g' > $WOBSOCK)
wpctl set-mute @DEFAULT_AUDIO_SINK@ toggle && (wpctl get-volume @DEFAULT_AUDIO_SINK@ | grep -q MUTED && echo 0 > $WOBSOCK) || wpctl get-volume @DEFAULT_AUDIO_SINK@ | sed 's/[^0-9]//g' > $WOBSOCK
```

### Brightness using [haikarainen/light](https://github.com/haikarainen/light):

```
light -A 5 && light -G | cut -d'.' -f1 > $WOBSOCK
light -U 5 && light -G | cut -d'.' -f1 > $WOBSOCK
```

### Brightness using [brightnessctl](https://github.com/Hummer12007/brightnessctl):

```
brightnessctl set 5%- | sed -En 's/.*\(([0-9]+)%\).*/\1/p' > $WOBSOCK
brightnessctl set +5% | sed -En 's/.*\(([0-9]+)%\).*/\1/p' > $WOBSOCK
```
