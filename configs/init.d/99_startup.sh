#!/bin/scsh

echo !s9 > /tmp/splash
echo Finishing up... > /tmp/splash
echo !q > /tmp/splash

/bin/getty 1 &
/bin/getty 2 &
/bin/getty 3 &
/bin/getty 4 &
/bin/getty 5 &
/bin/getty 6 &

/bin/compositor --startup /bin/lockscreen > /dev/tty0
