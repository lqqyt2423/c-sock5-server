#!/bin/bash

set -x

curl 127.0.0.1:7001 --socks5 127.0.0.1:1080
curl "http://[::1]:7001/" --socks5 127.0.0.1:1080
