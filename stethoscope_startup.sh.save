#!/bin/bash
PASSWORD="odroid" 
echo $PASSWORD |sudo -S kill $(ps aux | grep '[j]ackd' | awk '{print $2}') 
echo $PASSWORD |sudo -S kill $(ps aux | grep '[s]imple' | awk '{print $2}') 
sleep 2
echo $PASSWORD 
(echo $PASSWORD | sudo -S jackd -d alsa -d hw:1 -S true -r 44100   &) 
sleep 2
( echo $PASSWORD | sudo -S ~/simple  &)

#sleep 10

#echo $PASSWORD |sudo -S kill -s SIGINT $(ps aux | grep '[s]imple' | awk '{print $2}') 
