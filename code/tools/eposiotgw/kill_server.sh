#!/bin/bash
for i in `ps aux | grep "python -u -m gateway_daemon" | grep -v "grep" | awk '{print $2}'`; do kill -9 $i; done
