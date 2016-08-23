#!/bin/bash
PASSWORD="odroid"

echo $PASSWORD |sudo -S kill $(ps aux | grep '[j]ackd' | awk '{print $2}') 
echo $PASSWORD |sudo -S kill -s SIGINT $(ps aux | grep '[s]imple' | awk '{print $2}') 