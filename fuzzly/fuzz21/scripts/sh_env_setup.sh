
#echo $DISPLAY
#echo export DISPLAY=:2
#export DISPLAY=:2
#echo $DISPLAY
#xcalc
echo $DISPLAY
echo export DISPLAY=$HOSTNAME:2
export DISPLAY=$HOSTNAME:2
echo $DISPLAY

##################
# Works with passthrough
##################

#echo xcalc
#xcalc

#echo xclock
#xclock

#echo bitmap
#bitmap

#echo xfig
#xfig

#echo gnome-calculator
#gnome-calculator

#echo evince ../data/assignment_5.pdf
#evince ../data/assignment_5.pdf

#echo gedit
#gedit

#echo sublime
#subl

#echo geany
#geany

#echo gimp
#gimp

#echo audacity
#audacity

#echo clamtk
#clamtk

#echo gnome-control-center
#gnome-control-center

#echo filezilla
#filezilla

#echo synaptic
#synaptic

#echo idle
#idle

##################
# Immediately exits gracefully on passthrough
##################

#echo xpaint
#xpaint

#echo firefox
#firefox

#echo chrome
#google-chrome

#echo visual studio code
#code

#echo supertuxkart
#supertuxkart

#echo virtualbox
#virtualbox

#echo VLC
#vlc

#echo matlab
#matlab


##################
# Do not pass messages through agent
##################

#echo libreoffice --writer
#libreoffice --writer

#echo libreoffice --calc
#libreoffice --calc

#echo libreoffice --draw
#libreoffice --draw

#echo libreoffice --web
#libreoffice --web

#echo libreoffice --impress
#libreoffice --impress

#echo Image Viewer
#eog ../data/test_files/pic1.jpg

#echo zoom
#zoom

#echo gnome-system-monitor
#gnome-system-monitor

#echo gnome-calendar
#gnome-calendar

##################
# Immediately crashes on passthrough
##################

#echo xsnow
#xsnow