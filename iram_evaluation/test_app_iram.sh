# David T. Nguyen
# July 2014
# dunguk@gmail.com


# usage:
# ./test_app_iram.sh $1
# tests an application
# $1 - application to be tested in form package name

APP=$1

# turn screen on
#input keyevent 26
#sleep 1

# launch app
# monkey -v -s 10 -p $APP 1
# sleep 8

# issue 1000 user events
monkey -v -s 10 --pct-appswitch 0 -p $APP 1000
sleep 1

# close the app with the home button
input keyevent 3
sleep 1

# turn screen off
#input keyevent 26

