#!/bin/bash
# Launch xreader under a virtual display and dump the AT-SPI tree while
# it is running, so e2e failures are diagnosable in CI.
set -x
binary="$1"
file="$2"

export GTK_MODULES="gail:atk-bridge"
gsettings set org.gnome.desktop.interface toolkit-accessibility true

xvfb-run -a bash -c "
  '$binary' '$file' >/tmp/xr.out 2>/tmp/xr.err &
  APP=\$!
  sleep 6
  python3 tools/atspi-probe.py
  kill \$APP
  wait \$APP 2>/dev/null || true
"

echo '--- xreader stderr (up to 60 lines) ---'
head -60 /tmp/xr.err
echo '--- xreader stdout (up to 20 lines) ---'
head -20 /tmp/xr.out