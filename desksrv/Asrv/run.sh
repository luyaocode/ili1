#!/bin/bash
cd "$(dirname "$0")"
export QT_PLUGIN_PATH=$PWD/plugins:$QT_PLUGIN_PATH
export LD_LIBRARY_PATH=$PWD:$PWD/libs:$LD_LIBRARY_PATH
for dir in $PWD/plugins/*/; do
    export LD_LIBRARY_PATH=$dir:$LD_LIBRARY_PATH
done
touch /tmp/Asrv.log
nohup ./Asrv > /tmp/Asrv.log 2>&1 &
echo "Asrv start"
tail /tmp/Asrv.log -f
