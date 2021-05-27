#!/bin/bash

killall redis-server
killall nutcracker
rm -rf dir/73*/*
rm -rf dir/logs/*.log
rm -rf dir/pid/*.pid
