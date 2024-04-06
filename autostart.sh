#!/bin/sh
# eth0_ip=$(ifconfig eth0 | egrep -o "([0-9]{1,3}\.){3}[0-9]{1,3}" | head -n 1 | cut -d "." -f 3)
# sort_id=$((x=eth0_ip-10, y=x/6*2, z=x%6+1, y+z))
cd /root/work/AiSort
chmod 777 ./build/bin/sort
./build/bin/sort 1


