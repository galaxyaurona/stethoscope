#!/bin/bash
PASSWORD="odroid"
cd /home/odroid/stethoscope
#sleep 40
(echo $PASSWORD | sudo -S /usr/bin/jackd -d alsa -d hw:1 -S true -r 44100 &)

(echo $PASSWORD | sudo -S /home/odroid/stethoscope/uploader &)
(echo $PASSWORD | sudo -S /home/odroid/stethoscope/isr &) 
