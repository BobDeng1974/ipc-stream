#!/bin/sh

echo $PWD
if [ -d ../../build-arm ];then
	cp -rvf ../../build-arm/src/sample/ipc-rtmp-stream /home/work/share/
else
	echo "ipc-stream not found"
fi
