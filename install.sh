#!/bin/bash
mount -o remount,rw /
chmod 777 build/bin/deploy
chmod 777 S80AiSort
mv S80AiSort /etc/init.d/
mv interfaces /etc/network/
