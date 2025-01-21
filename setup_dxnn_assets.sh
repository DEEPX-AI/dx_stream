#!/bin/bash
sudo mkdir -p /mnt/regression_storage
sudo mount -o nolock 192.168.30.201:/do/regression /mnt/regression_storage

ls -al /mnt/regression_storage/atd/dxnn_assets.tar.gz

cp /mnt/regression_storage/atd/dxnn_assets.tar.gz .

tar xvfz ./dxnn_assets.tar.gz

rm -rf ./xvfz dxnn_assets.tar.gz
