/*
 * isr.c:
 *	Wait for Interrupt test program - ISR method
 *
 *	How to test:
 *	  Use the SoC's pull-up and pull down resistors that are avalable
 *	on input pins. So compile & run this program (via sudo), then
 *	in another terminal:
 *		gpio mode 0 up
 *		gpio mode 0 down
 *	at which point it should trigger an interrupt. Toggle the pin
 *	up/down to generate more interrupts to test.
 *
 * Copyright (c) 2013 Gordon Henderson.
 ***********************************************************************
 * This file is part of wiringPi:
 *	https://projects.drogon.net/raspberry-pi/wiringpi/
 *
 *    wiringPi is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    wiringPi is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public License
 *    along with wiringPi.  If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <stdbool.h>

#define LED_PIN 21
#define BUTTON_PIN 2

// globalCounter:
//	Global variable to count interrupts
//	Should be declared volatile to make sure the compiler doesn't cache it.

static volatile int globalCounter [8] ;
static volatile bool blinking;
long const period=3200000;
static volatile start;
/*
 * myInterrupt:
 *********************************************************************************
 */

void myInterrupt0 (void) { 
	++globalCounter [0] ;
	//digitalWrite(2, 1);
	
	while (start ==-1) {                                                                                        
		start = system("echo odroid | sudo -S jackd -d alsa -d hw:1 -S true -r 44100 &");
	}
	//printf("current working dir\n");
	//system("pwd");
	//printf("print jackd dir\n");
	//system("ps aux | grep jackd");
	blinking = true;
	if (globalCounter[0] > 1) {
		int results  = system("~/stethoscope/simple");
	}
	//digitalWrite(2, 0);
	blinking = false;
 }


/*
 *********************************************************************************
 * main
 *********************************************************************************
 */

int main (void)
{
  int gotOne, pin ;
  int myCounter [8] ;
  long count ;
  wiringPiSetup();
  start=-1;
  blinking = false;
  for (pin = 0 ; pin < 8 ; ++pin) 
    globalCounter [pin] = myCounter [pin] = 0 ;
  pinMode(LED_PIN,OUTPUT);
  digitalWrite(LED_PIN, 0);
  // set pin mode to be ouput
  wiringPiISR (BUTTON_PIN, INT_EDGE_FALLING, &myInterrupt0) ;
  
  for (;;)
  {
	if (blinking){
		count = (count+1) % period;
		//printf("count %d",count);
	
		if (count > (period/2) ) {
			digitalWrite(LED_PIN, 0);
		} else {
			digitalWrite(LED_PIN, 1);
		}
	} else {
		count =0;
		digitalWrite(LED_PIN,1);
	}
	
  }

  return 0 ;
}