/*  Names: Jason Luo, Michael Cuozzo
    Section:3
    Date: 2/17/15
    File name: lab1-2
    Description:
*/
/*
  This program demonstrates the use of T0 interrupts. The code will count the
  number of T0 timer overflows that occur while a slide switch is in the ON position.
*/

#include <c8051_SDCC.h>// include files. This file is available online
#include <stdio.h>
#include <stdlib.h>

//-----------------------------------------------------------------------------
// Function Prototypes
//-----------------------------------------------------------------------------
void Port_Init(void);      // Initialize ports for input and output
void Timer_Init(void);     // Initialize Timer 0 
void Interrupt_Init(void); //Initialize interrupts
void Timer0_ISR(void) __interrupt 1;
void ADC_Init(void);
unsigned char random(void);
unsigned char read_AD_input(unsigned char n);

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------

__sbit __at 0xB5 LED0; // LED0, associated with Port 3, Pin 5
__sbit __at 0xB6 LED1; // LED1, associated with Port 3, Pin 6
__sbit __at 0xB7 LED2; //LED2, associated with Port 3, Pin 7 
__sbit __at 0xB2 LED3; //LED3, associated with Port 3, Pin 2
__sbit __at 0xB3 BILED1; //BILED0, associated with Port 3, Pin 3
__sbit __at 0xB4 BILED2; //BILED1, associated with Port 3, Pin 4
__sbit __at 0xA0 SS; //Slide switch, associated with Port 2 Pin 0
__sbit __at 0xB0 PB1; //Push button 1, associated with Port 3, Pin 0
__sbit __at 0xB1 PB2; //Push button 2, associated with Port 3, Pin 1
__sbit __at 0xA2 PB3; //Push button 3, asscoaited with Port 2, Pin 3
__sbit __at 0xA4 PB4; //Push button 4, associated with Port 2, Pin 4


unsigned long Counts = 0;
unsigned int turns = 0;
unsigned char oldnum = 10;
unsigned char rand1;
unsigned char answers = 0;
unsigned char result;

//***************
void main(void)
{
    Sys_Init();      // System Initialization
    Port_Init();     // Initialize ports 2 and 3 
    Interrupt_Init();
    Timer_Init();    // Initialize Timer 0 

    putchar(' ');    // the quote fonts may not copy correctly into SiLabs IDE
    printf("Start\r\n");

    while (1) /* the following loop prints the number of overflows that occur
                while the pushbutton is pressed, the BILED is lit while the
                button is pressed */
    {
    	result = read_AD_input(0);
        LED0 = 1;
        LED1 = 1;
        //LED2 = 1;
        //LED3 = 1;

        if(!SS){}
    }
}

//***************
void Port_Init(void) //UNFINISHED LET MICHAEL TAKE OVER THIS
{
 // Port 3
   P3MDOUT &= 0x00 ; // set Port 3 output pins to push-pull mode 
   P3MDOUT |= 0xFC; // set Port 3 input pins to open drain mode 
   P3 |= ~0xFC; // set Port 3 input pins to high impedance state aka setting pins 2 and 5 to 1

// Port 2
   P2MDOUT |= 0x00; 
   P2MDOUT &= 0x00; //set Port 2 pin 1 to open drain mode or input 
   P2 |= ~0x00;      //set Port 2 inpubt pins to high impedance
}

void Interrupt_Init(void)
{
    IE |= 0x02;      // enable Timer0 Interrupt request (by masking)
    EA = 1;       // enable global interrupts (by sbit)
}
//***************
void Timer_Init(void)
{

    CKCON |= 0x08;  // Timer0 uses SYSCLK as source
    TMOD &= 0xF0;   // clear the 4 least significant bits
    TMOD |= 0x01;   // Timer0 in mode 1
    TR0 = 0;           // Stop Timer0
    TMR0 = 0;           // Clear high & low byte of T0

}

//configure as analog input
void ADC_Init(void)
{
    REF0CN = 0X03; //Set Vref to use internal reference voltage

    ADC1CN = 0x80; // Enable ADC1
    ADC1CF = 0x01; //Set a gain of 1

}

//timer interrupt
void Timer0_ISR(void) __interrupt 1
{
    Counts++;
}


/*return a random integer number between 0 and 3*/
unsigned char random(void)
{
    return (rand()%4);  // returns random value between 0-2
}

unsigned char read_AD_input (unsigned char n) //UNFINISHED
{
    AMX1SL = n; // Set P1.N as the analog input for ADC1
    ADC1CN = ADC1CN & ~0x20;
    ADC1CN = ADC1CN | 0x10;

    while ((ADC1CN & 0x20) == 0x00);

    return ADC1;
}
