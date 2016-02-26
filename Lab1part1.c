/*  Names: Jason Luo and Michael Cuozzo
	Section: 3
	Date: February 5, 2016
	File name: Lab1_part1
	Program description:
	*/

#include <c8051_SDCC.h> // include files. This file is available online
#include <stdio.h>

void Port_Init(void); // Initialize ports for input and output
int sensor1(void); // function which checks Pushbutton1
int sensor2(void); // function that checks the Slide switch
int sensor3(void); //function that checks Pushbutton2
void Set_Outputs(void);// function to set output bits

__sbit __at 0xB6 LED0; // LED0, associated with Port 3 Pin 6
__sbit __at 0xB3 BILED0; // BILED0, associated with Port 3, Pin 3
__sbit __at 0xB4 BILED1; // BILED1, associated with Port 3, Pin 4
__sbit __at 0xB7 Buzzer; // Buzzer, associated with Port 3, Pin 7
__sbit __at 0xA0 SS; // Slide switch, associated with Port 2 Pin 0
__sbit __at 0xB0 PB1; // Push button 1, associated with Port 3, Pin 0
__sbit __at 0xB1 PB2; // Push button 2, associated with Port 3, Pin 1

void main(void)
{
 	Sys_Init(); // System Initialization
 	putchar(' '); // the quote fonts may not copy correctly into SiLabs IDE
 	Port_Init(); // Initialize ports 2 and 3
 	
 	while (1) // infinite loop
 	{
 	// main program manages the function calls
 	Set_Outputs();
 	}
}

/* Port_Init - Initializes Ports 2 and 3 in the desired modes for input and output */
void Port_Init(void)
{
 // Port 3
   P3MDOUT |= 0xD8 ; // set Port 3 output pins to push-pull mode 
   P3MDOUT &= 0xFC; // set Port 3 input pins to open drain mode 
   P3 |= ~0xFC; // set Port 3 input pins to high impedance state aka setting pins 2 and 5 to 1

// Port 2
   P2MDOUT |= 0x01; 
   P2MDOUT &= 0xFE; //set Port 2 pin 1 to open drain mode or input 
   P2 |= ~0xFE;      //set Port 2 input pins to high impedance
}

//Partner 1
/*
void Set_Outputs(void)
{
	BILED0 = 0;
	BILED1 = 0;
	PB1 = 1;
	PB2 = 1;
 	if (sensor2()) // if Slide switch is not activated (off)
 	{
 	LED0 = 0; // Light LED
 	printf("\rSlide switch is off and LED is on\n");
 	}
 	else // if Slide switch is activated (on)
	{
 	LED0 = 1; // turn off LED
 	if (sensor1() == 1 && sensor3() ==1){
 		BILED0 = 1; 
 		BILED1 = 0;
		Buzzer = 1;
 		printf("\rSlide switch is on and both PB1 and PB2 are on \n");
 	}
 	else if (sensor1() == 0 && sensor3() ==0){
 		BILED0 = 0;
 		BILED1 = 1;
		Buzzer = 1;
 		printf("\rSlide switch is on PB1 and PB2 is off, BILED is red \n");
 	}
 	else if (sensor1() == 1 && sensor3() == 0){
 		Buzzer = 0;
 		printf("\rSlide switch is on and PB1 is on, buzzer is on \n");
 	}
 	else{
 		printf("\rStop pressing PB2... \n");
 	}

 }
}
*/

// Partner 2

void Set_Outputs(void)
{
	BILED0 = 0;
	BILED1 = 0;
	PB1 = 1;
	PB2 = 1;
 	if (sensor2()) // if Slide switch is not activated (off)
 	{
		Buzzer = 1;
 		BILED0 = 1; 
		BILED1 = 0;
 		printf("\rSlide switch is off and BILED is green\n");
 	}
 	else // if Slide switch is activated (on)
	{
 		BILED0 = 0;
		BILED1 = 1; 
		LED0 = 1;
		printf("\rSlide switch is on and BILED is red\n");
 	if (sensor1() == 1 & sensor3() ==1){
 		Buzzer = 0;
 		printf("\rSlide switch is on and both PB1 and PB2 are on and buzzer is working\n");
 	}
 	else if (sensor1() == 1 & sensor3() ==0){
 		LED0 = 0;
		Buzzer = 1;
 		printf("\rSlide switch is on and PB2 is on, LED is on \n");
 	}
 	else if (sensor1() == 0 & sensor3() == 1){
 		Buzzer = 0;
 		printf("\rSlide switch is on and PB2 is on, buzzer is on \n");
 	}
 	else{
 		printf("\rPress something... \n");
		Buzzer = 1;
 	}

 }
}


//***************
// Sensor - Returns a 0 if Pushbutton 1 not activated
// or a 1 if Pushbutton 1 is activated.
int sensor1(void)
{
 	if (!PB1) return 1;
 	else return 0;
}

int sensor3(void)
{
	if (!PB2) return 1;
	else return 0;
}
//***************
// Sensor - Returns a 0 if Slide switch is 'off'
// or a 1 if Slide switch is 'on'
int sensor2(void)
{
 	if (SS) return 1;
 	else return 0;
}