#!/bin/bash
export DISPLAY=:99
Xvfb :99 -screen 0 1024x768x24 &
sleep 2
./build/core/tests/webview_core_unit_tests
kill %1
