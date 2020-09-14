#!/bin/sh

# assuming this is A0 root directory,
# use relative paths for easy testing
PANEL_DIR=`pwd`/../panel/
JOBD_DIR=$PANEL_DIR/jobd
PROXY_DIR=$PANEL_DIR/proxy

(cd $JOBD_DIR && node jobd.js ../jobs/) &
(cd $PROXY_DIR && node proxy.js)
