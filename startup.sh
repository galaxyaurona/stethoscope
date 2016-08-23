#!/bin/bash
PASSWORD="odroid"
cd /home/odroid/stethoscope
(echo $PASSWORD | sudo -S amixer -c 1 set Mic 13)
(echo $PASSWORD | sudo -S amixer -c 1 set PCM 151) 
#sleep 40
(echo $PASSWORD | sudo -S /usr/bin/jackd -d alsa -d hw:1 -S true -r 44100 &)

(echo $PASSWORD | sudo -S /home/odroid/stethoscope/uploader &)
(echo $PASSWORD | sudo -S /home/odroid/stethoscope/isr &) 
