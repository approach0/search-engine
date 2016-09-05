#!/bin/bash
vps_ip=`dig +short approach0.xyz`
echo "push to $vps_ip in 5 sec..."
sleep 5

rsync \
-zauv --exclude='*.html' --progress \
tmp/ rsync://${vps_ip}:8990/corpus
