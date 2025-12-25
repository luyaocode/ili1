#!/bin/bash

echo "开始打包..."
linuxdeployqt monitor -appimage -no-translations
echo "打包结束."
