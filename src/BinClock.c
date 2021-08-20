/*
 * BinClock.c
 * Jarrod Olivier
 * Modified by Keegan Crankshaw
 * Further Modified By: Mark Njoroge 
 *
 * 
 * MLLSHA035 MTWYON001
 * Date 20 August 2021
*/

#include <signal.h> //for catching signals
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <stdio.h> //For printf functions
#include <stdlib.h> // For system functions

#include "BinClock.h"
#include "CurrentTime.h"

//Global variables
int hours, mins, secs;
long lastInterruptTime = 0; //Used for button debounce
int RTC; //Holds the RTC instance

int HH,MM,SS;

int ledStatus = 0b1;


// Clean up function to avoid damaging used pins
void CleanUp(int sig){
	printf("Cleaning up\n");

	//Set LED to low then input mode
	//Logic here
	wiringPiSetup();
	pinMode(11,0);

	for (int j=0; j < sizeof(BTNS)/sizeof(BTNS[0]); j++) {
		pinMode(BTNS[j],INPUT);
	}

	exit(0);

}

void initGPIO(void){
	/* 
	 * Sets GPIO using wiringPi pins. see pinout.xyz for specific wiringPi pins
	 * You can also use "gpio readall" in the command line to get the pins
	 * Note: wiringPi does not use GPIO or board pin numbers (unless specifically set to that mode)
	 */
	printf("Setting up\n");
	wiringPiSetup(); //This is the default mode. If you want to change pinouts, be aware
	

	RTC = wiringPiI2CSetup(RTCAddr); //Set up the RTC
	
	//Set up the LED
	//Write your Logic here
	wiringPiSetup();
	pinMode(11,1);

	
	printf("LED and RTC done\n");
	
	//Set up the Buttons
	for(int j=0; j < sizeof(BTNS)/sizeof(BTNS[0]); j++){
		pinMode(BTNS[j], INPUT);
		pullUpDnControl(BTNS[j], PUD_UP);
	}
	
	//Attach interrupts to Buttons
	//Write your logic here
	/*
	int waitForInterrupt(5, 1000);
	int waitForInterrupt(30, 1000);
	*/

	pinMode(5, INPUT);
	pullUpDnControl(5, PUD_UP);
	wiringPiISR(5, INT_EDGE_FALLING, &hourInc);

	pinMode(30, INPUT);
	pullUpDnControl(30, PUD_UP);
	wiringPiISR(30, INT_EDGE_FALLING, &minInc);


	printf("BTNS done\n");
	printf("Setup done\n");
}

//int wiringPiISR (int pin, int edgeType, void (*function)(void)) ; 


/*
 * The main function
 * This function is called, and calls all relevant functions we've written
 */
int main(void){
	signal(SIGINT,CleanUp);
	initGPIO();

	//Set random time (3:04PM)
	//You can comment this file out later
	//toggleTime();
	
	wiringPiI2CWriteReg8(RTC, HOUR_REGISTER, 0x0+TIMEZONE);
	wiringPiI2CWriteReg8(RTC, MIN_REGISTER, 0x0);
	wiringPiI2CWriteReg8(RTC, SEC_REGISTER, 0x00);
	
	
	// Repeat this until we shut down
	for (;;){
		//Fetch the time from the RTC
		//Write your logic here
		hours = wiringPiI2CReadReg8(RTC, HOUR_REGISTER);
		mins = wiringPiI2CReadReg8(RTC, MIN_REGISTER);
		secs = hexCompensation(wiringPiI2CReadReg8(RTC, SEC_REGISTER));
		
		//Toggle Seconds LED
		//Write your logic here
		//ledStatus = ~ledStatus;
		/*
		if (ledStatus == 1)
		{
			ledStatus = 0;
		}
		else
		{
			ledStatus = 1;
		}
		*/
		toggleLED();
		digitalWrite(11, ledStatus);
		delay(1000);
		/*
		digitalWrite(11,0);
		delay(1000);
		*/
		
		// Print out the time we have stored on our RTC
		printf("The current time is: %d:%d:%d\n", hours, mins, secs);

		//using a delay to make our program "less CPU hungry"
		//delay(1000); //milliseconds
	}
	return 0;
}

/*
 * Changes the hour format to 12 hours
 */
int hFormat(int hours){
	/*formats to 12h*/
	if (hours >= 24){
		hours = 0;
	}
	else if (hours > 12){
		hours -= 12;
	}
	return (int)hours;
}


/*
 * hexCompensation
 * This function may not be necessary if you use bit-shifting rather than decimal checking for writing out time values
 * Convert HEX or BCD value to DEC where 0x45 == 0d45 	
 */
int hexCompensation(int units){

	int unitsU = units%0x10;

	if (units >= 0x50){
		units = 50 + unitsU;
	}
	else if (units >= 0x40){
		units = 40 + unitsU;
	}
	else if (units >= 0x30){
		units = 30 + unitsU;
	}
	else if (units >= 0x20){
		units = 20 + unitsU;
	}
	else if (units >= 0x10){
		units = 10 + unitsU;
	}
	return units;
}


/*
 * decCompensation
 * This function "undoes" hexCompensation in order to write the correct base 16 value through I2C
 */
int decCompensation(int units){
	int unitsU = units%10;

	if (units >= 50){
		units = 0x50 + unitsU;
	}
	else if (units >= 40){
		units = 0x40 + unitsU;
	}
	else if (units >= 30){
		units = 0x30 + unitsU;
	}
	else if (units >= 20){
		units = 0x20 + unitsU;
	}
	else if (units >= 10){
		units = 0x10 + unitsU;
	}
	return units;
}


/*
 * hourInc
 * Fetch the hour value off the RTC, increase it by 1, and write back
 * Be sure to cater for there only being 23 hours in a day
 * Software Debouncing should be used
 */
void hourInc(void){
	//Debounce
	long interruptTime = millis();

	if (interruptTime - lastInterruptTime>300){
		printf("Interrupt 1 triggered, %x\n", hours);
		//Fetch RTC Time

		hours = wiringPiI2CReadReg8(RTC, HOUR_REGISTER);
		//Increase hours by 1, ensuring not to overflow
		hours = (hours+0x1)%0x18;
		//Write hours back to the RTC
		wiringPiI2CWriteReg8(RTC, HOUR_REGISTER, hours);
	}
	lastInterruptTime = interruptTime;
}

/* 
 * minInc
 * Fetch the minute value off the RTC, increase it by 1, and write back
 * Be sure to cater for there only being 60 minutes in an hour
 * Software Debouncing should be used
 */
void minInc(void){
	long interruptTime = millis();

	if (interruptTime - lastInterruptTime>300){
		printf("Interrupt 2 triggered, %x\n", mins);
		//Fetch RTC Time
		mins = wiringPiI2CReadReg8(RTC, MIN_REGISTER);
		//Increase minutes by 1, ensuring not to overflow
		mins = (mins+0x1)%0x3C;
		//Write minutes back to the RTC
		wiringPiI2CWriteReg8(RTC, MIN_REGISTER, mins);
	}
	lastInterruptTime = interruptTime;
}

//This interrupt will fetch current time from another script and write it to the clock registers
//This functions will toggle a flag that is checked in main
void toggleTime(void){
	long interruptTime = millis();

	if (interruptTime - lastInterruptTime>200){
		HH = getHours();
		MM = getMins();
		SS = getSecs();

		HH = hFormat(HH);
		HH = decCompensation(HH);
		wiringPiI2CWriteReg8(RTC, HOUR_REGISTER, HH);

		MM = decCompensation(MM);
		wiringPiI2CWriteReg8(RTC, MIN_REGISTER, MM);

		SS = decCompensation(SS);
		wiringPiI2CWriteReg8(RTC, SEC_REGISTER, 0b10000000+SS);

	}
	lastInterruptTime = interruptTime;
}

void toggleLED(void)
{
	if ( ledStatus == 1)
	{
		ledStatus = 0;
	}
	else
	{
		ledStatus = 1;
	}
}
