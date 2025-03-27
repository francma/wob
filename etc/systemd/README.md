# Systemd integration

Copy systemd unit files (if not provided by your distribution package):

```
cp etc/systemd/wob.socket ~/.local/share/systemd/user/
cp etc/systemd/wob.service.in ~/.local/share/systemd/user/wob.service
sed -i 's#@bindir@#/usr/bin#' ~/.local/share/systemd/user/wob.service
systemctl daemon-reload --user
```

Enable systemd wob socket:

```
systemctl enable --now --user wob.socket
```

Make sure the wayland environment is imported in your window manager:

```
systemctl --user import-environment WAYLAND_DISPLAY
```

Now the wob will be started by Systemd once we write to the socket. Make sure to NOT also start wob in your window manager configuration.

Now you can input wob values to PIPE located in path:

```
$XDG_RUNTIME_DIR/wob.sock
```

Alternatively, you can get the path above with following command:

```
systemctl show --user wob.socket -p Listen | sed 's/Listen=//' | cut -d' ' -f1
```
