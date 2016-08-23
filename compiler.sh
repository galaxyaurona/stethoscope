#!/bin/bash
gcc -o simple simple_client.c `pkg-config --cflags --libs jack` -lsndfile -lpthread -l wiringPi

gcc -o uploader uploader.c `pkg-config --libs libcurl`

gcc -o isr isr.c -lpthread -l wiringPi