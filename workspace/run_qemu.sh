#!/bin/bash
set -e
cd "$(dirname "$0")"
qemu-system-x86_64 -drive format=raw,file=kernos.img -display curses -monitor none
