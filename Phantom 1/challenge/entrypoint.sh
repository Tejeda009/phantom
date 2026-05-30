#!/bin/bash
set -e
exec /usr/sbin/xinetd -dontfork -f /etc/xinetd.d/phantom.conf
