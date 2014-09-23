#!/bin/sh

# sudo apt-get install inotify-tools
# /etc/rc.local
#   su pi -c '/home/pi/bin/watched.sh  > /dev/null 2>&1 &'


WATCHED_DIR="/opt/watched"

inotifywait -m -e ATTRIB "$WATCHED_DIR" |
while read dir eventlist eventfile
do
  #echo $dir 
  #echo $eventlist
  #echo $eventfile
  if [ "$eventfile" = "cctvbf" ]
  then
    echo $eventfile
    /opt/www/Symfony/app/console cctvbf:move toggle 
 
  fi
done
