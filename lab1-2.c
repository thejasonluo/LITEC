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
unsigned char random(void);

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------

__sbit __at 0xB6 LED0; // LED0, associated with Port 3, Pin 6
__sbit __at 0xB5 LED1; // LED1, associated with Port 3, Pin 5
__sbit __at 0xB3 BILED1; // BILED0, associated with Port 3, Pin 3
__sbit __at 0xB4 BILED2; // BILED1, associated with Port 3, Pin 4
__sbit __at 0xB7 Buzzer; // Buzzer, associated with Port 3, Pin 7
__sbit __at 0xA0 SS; // Slide switch, associated with Port 2 Pin 0
__sbit __at 0xB0 PB1; // Push button 1, associated with Port 3, Pin 0
__sbit __at 0xB1 PB2; // Push button 2, associated with Port 3, Pin 1

unsigned long Counts = 0;
unsigned int turns = 0;
unsigned char oldnum = 10;
unsigned char rand1;
unsigned char answers = 0;

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
    	if (!SS) //if slideswitch is on
    	{
    		if (turns <10)
    		{
				LED0 = 1;
				LED1 = 1;
    			TR0 = 1; //start timer
    			rand1 = random(); //create random number
    			while (rand1 == oldnum){ rand1 = random();}
    			oldnum = rand1;
    			printf ("Turn: %d \n\r",turns + 1);
    			printf("Answers correct: %d\n\r", answers);
				printf("Answers incorrect: %d\n\r", turns + 1 - answers);
    			if (rand1 ==0)
    			{
    				LED0 = 0;
    				LED1 = 1;
    			}
    			else if (rand1 == 1)
    			{
    				LED1 = 0;
    				LED0 = 1;
    			}
    			else 
    			{
    				LED0 = 0;
    				LED1 = 0;
    			}
    			while (Counts < 338){}
    			Counts = 0;
    			turns++;
    			if (!PB1 && PB2)		// If only PB1 is pushed
				{
					if (rand1 == 0)
					{
						BILED1 = 1;	// Turn BILED Green
						BILED2 = 0;
						printf("only button2\n\r");
						answers++;	// increment CORRECT
					}
					else
					{
						BILED1 = 0; // Turn BILED Red
						BILED2 = 1;
						printf("wrong");
					}
				}

				else if (PB1 && !PB2)	// If only PB2 is pushed
				{
					if (rand1 == 1)
					{
						BILED1 = 1;	// Turn BILED Green
						BILED2 = 0;
						printf("only button 1\n\r");
						answers++;	// increment CORRECT
					}
					else
					{
						BILED1 = 0; // Turn BILED Red
						BILED2 = 1;
						printf("wrong\n\r");
					}						
				}

				else if (!PB1 && !PB2)	// If both PB1 and PB2 are pushed
				{
					if (rand1 == 2)
					{
						BILED1 = 1;	// Turn BILED Green
						BILED2 = 0;
						printf("both buttons\n\r");
						answers++;	// increment CORRECT
					}
					else
					{
						BILED1 = 0; // Turn BILED Red
						BILED2 = 1;
						printf("wrong\n\r");
					}
				}

				else	// If neither PB1 or PB2 are pushed
				{
					BILED1 = 0; // Turn BILED Red
					BILED2 = 1;
					printf("didn't press nothin\n\r");
				}

    		}
    		else 
    		{
    			TR0 = 0; //stop timer
				BILED1 = 0;
				BILED2 = 1;
				LED0 = 0;
				LED1 = 0;
    		} 
    	}
    	else //if slideswitch is off 
    	{
    		if (turns < 10)
    		{
    			TR0 = 0;
    			BILED1 = 1;
    			BILED2 = 1;
    			LED0 = 1;
    			LED1 = 1;
    		}
    		else 
    		{
    			TR0 = 0;
    			Counts = 0;
    			TMR0 = 0;
    			BILED1 = 1;
    			BILED2 = 1;
    			LED0 = 1;
    			LED1 = 1;
    			turns = 0;
    			answers = 0;
    		}
    	}
    }
}

//***************
void Port_Init(void)
{
 // Port 3
   P3MDOUT |= 0xF8 ; // set Port 3 output pins to push-pull mode 
   P3MDOUT &= 0xFC; // set Port 3 input pins to open drain mode 
   P3 |= ~0xFC; // set Port 3 input pins to high impedance state aka setting pins 2 and 5 to 1

// Port 2
   P2MDOUT |= 0x01; 
   P2MDOUT &= 0xFE; //set Port 2 pin 1 to open drain mode or input 
   P2 |= ~0xFE;      //set Port 2 input pins to high impedance
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


//timer interrupt
void Timer0_ISR(void) __interrupt 1
{
    Counts++;
}


/*return a random integer number between 0 and 2*/
unsigned char random(void)
{
    return (rand()%3);  // returns random value between 0-2
}
