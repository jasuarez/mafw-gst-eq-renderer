#!/bin/sh
#
# Launches mafw-gst-eq-renderer on start

test -f /usr/lib/mafw-plugin/mafw-gst-eq-renderer.so && \
sudo /usr/bin/mafw.sh start mafw-gst-eq-renderer -7 || \
true

