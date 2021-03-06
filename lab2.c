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
void Blink_Pause(void);
void End_Won(unsigned char n);
void End_Lost(unsigned char n);
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
__sbit __at 0xA3 PB3; //Push button 3, asscoaited with Port 2, Pin 3
__sbit __at 0xA4 PB4; //Push button 4, associated with Port 2, Pin 4
__sbit __at 0xA6 Buzzer; //Buzzer, associated with Port 2, Pin 6


unsigned long Counts = 0;
unsigned char oldnum = 10;
unsigned char rand1;
unsigned char result;
unsigned char rounds = 0;
unsigned char turns = 0;
unsigned char next = 0; //to determine whether to go to next LED
unsigned int on_time;
unsigned int off_time;
unsigned char colors[10];
unsigned int points_tracker[3] = {0, 0, 0};
unsigned int points = 0;
unsigned char i,j,k;
unsigned char store;
unsigned char new = 1;

//***************
void main(void)
{
    Sys_Init();      // System Initialization
    Port_Init();     // Initialize ports 2 and 3 
    Interrupt_Init();// Initialize Interrupts
    Timer_Init();    // Initialize Timer 0 
    ADC_Init();      // Initialize A/D converter
    putchar(' ');    // the quote fonts may not copy correctly into SiLabs IDE
    printf("Start\r\n");

    while (1) /* the following loop prints the number of overflows that occur
                while the pushbutton is pressed, the BILED is lit while the
                button is pressed */
    { 
      result = read_AD_input(0);
      on_time = ((result * 5) + 200) / 2.96;
      off_time = on_time / 2 ;
      
      //Reset all
  	  LED0 = 1;
      LED1 = 1;
      LED2 = 1;
      LED3 = 1;
      Buzzer = 1;

      TR0 = 1; //turn timer on

      while (SS)
      {
        Blink_Pause();
        new = 0;
      }

      if (!SS) //if switch is on 
      {
        if (new == 1) // if it is a new game
        {
          while (!SS){printf("switch is on\n\r");} //while switch is on wait until you turn off
          while (SS){Blink_Pause(); printf("switch is off\n\r");}  // while switch is off wait until you turn on
          new = 0;
        }
        while ((rounds < 3) && !SS) //while rounds
        {
            while ((turns < 3) && !SS) // switches turns
            {
              next = 1;
              /* Generates array for players*/
              for (i = 0; i< 10; i++)
              {
                colors[i] = random();
              }

              for (i = 0; i < 10; i++)
              {
                printf("%u", colors[i]);
              }
              /*indicates Players */
              if (turns == 0) //indicates Player 1
              {
                BILED1 = 0; //BILED OFF
                BILED2 = 0;
                printf("Player 1, don't mess up\n\r");
                Counts = 0;
                while (Counts < 338){}
              }
              else if (turns == 1) //indicates Player 2
              {
                BILED1 = 0; //RED BILED
                BILED2 = 1;
                printf("Player 2, first is the worst and second is the best\n\r");
                Counts = 0;
                while (Counts < 338){}
              }
              else if (turns == 2)//indicates Player 3
              {
                BILED1 = 1; //GREEN BILED
                BILED2 = 0; 
                printf("Player 3, lol good luck\n\r");
                Counts = 0;
                while (Counts < 338){}
              }

              points = 0; //resets points to 0

              /* goes through each output and checks if player is correct*/
              j = 0;
              while (j < 10 && !SS)
              {
                
                printf("j:%u\n\r", j);
                /*turns on the LED depending on the array inputs in array */
                k = 0;
                while (k <= j && !SS)
                {
                  printf("k: %u\n\r", k);
                  if (colors[k] == 0) //if it is 0 light LED0
                  {
                    LED0 = 0;
                    Counts = 0;
                    while (Counts < on_time){}
                    LED0 = 1;
                  }
                  else if (colors[k] == 1) //if it is 1 light LED1
                  {
                    LED1 = 0;
                    Counts = 0;
                    while (Counts < on_time){}
                    LED1 = 1;
                  } 
                  else if (colors[k] == 2) //if it is 2 light LED2
                  {
                    LED2 = 0;
                    Counts = 0;
                    while (Counts < on_time){}
                    LED2 = 1;
                  }
                  else 
                  {
                    LED3 = 0;
                    Counts = 0;
                    while (Counts < on_time){}
                    LED3 = 1;
                  }

                  Counts = 0;
                  while (Counts < off_time){}
                  k++;
                }
                
                if (SS)
                {
                  break;
                }


                /*checks for input */
                k = 0;
                while (k <= j && !SS)
                {
                  printf("k2:%u\n\r", k);
                  /*wait for a button to be pressed */
                  while(PB1 && PB2 && PB3 && PB4 && !SS);
                  //store value of the button
                  if( !PB1)
                  {
                    store = 0;
                  }
                  else if (!PB2)
                  {
                    store = 1;
                  }
                  else if (!PB3)
                  {
                    store = 2;
                  }
                  else if (!PB4)
                  {
                    store = 3;
                  }
                  printf("Stored value: %u\n\r", store);
                  printf("Correct value: %u\n\r", colors[k]);

                  /* wait for button to be released*/
                  while ((!PB1 || !PB2 || !PB3 || !PB4) && !SS); 

                  if (SS)
                  {
                    break;
                  }
                  /*checks input and if input matches last correct input, give the points.
                  if wrong input break out of loop*/
                  if (store == colors[k])
                  {
                    if (j==k)
                    {
                      points = points + j + 1;
                    }
                    next = 1;
                    printf("correct!\n\r");
                  }
                  else
                  {
                    next = 0;
                    printf("wrong!\n\r");
                    break;
                  }

                  k++;
                }

                if (SS)
                {
                  next = 0;
                  break;
                }
                /*waits for off_time*/
                Counts = 0;
                while (Counts < off_time){}

                /*if there is a incorrect input then exit immediately*/
                if (next == 0)
                {
                  break;
                }

                /* increment counter */
                j++;
              }

              points_tracker[turns] = points_tracker[turns] + points;

              if (next == 1) //if player successfully gets all 10 of them right 
              {
               End_Won(turns);
              }
              else //if player gets at least one wrong
              {
               End_Lost(turns);
              }

              while(SS)
              {
                Blink_Pause();
              } //while slide switch is still off wait until it is turned back on
              turns++; //next player's turn
            }

            if (!SS)
            {
              rounds++; //next round
              turns = 0; //restarts with player one's turn
            }
        }
      }
      else
      {
        if (rounds < 3) //if rounds < 3 and game already started
        {
          Blink_Pause(); /*BILED alternating red/green at frequency about 1 HZ */
          LED0 = 1;
          LED1 = 1;
          LED2 = 1;
          LED3 = 1;
          TR0 = 0;
          TR0 = 1;
          Counts = 0;
          TR0 = 0;

        }
        else
        {
          Blink_Pause();  /*BILED alternating red/green at frequency about 1 HZ */
          turns = 0;
          Counts = 0;
          TMR0 = 0;
          rounds = 0;
          points_tracker[0] = 0;
          points_tracker[1] = 0;
          points_tracker[2] = 0;
          new = 1;
        }
      } 
      TR0 = 0; // turn timer off
      //printf("Ready to start? \r\n");
      //printf("on_time in overflows: %d", on_time);
      //printf("off time in overflows: %d", off_time);
    }

}

//***************
void Port_Init(void) 
{
  // Port 3
 	P3MDOUT |= 0xFC;
	P3MDOUT &= 0xFC;
	P3 |= ~0xFC;
  //P3MDOUT |= 0xE4 ; // set Port 3 output pins to push-pull mode 
  //P3MDOUT &= 0xE4; // set Port 3 input pins to open drain mode 
  //P3 |= ~0xE4; // set Port 3 input pins to high impedance state aka setting pins 2 and 5 to 1

  // Port 2
  P2MDOUT |= 0x40; //Set Port 2 output pins to push-pull mode
  P2MDOUT &= ~0x15; //Set Port 2 input pins to open-drain
  P2 |= 0x15; //Set Port 3 input pins to high impedance
  //P2MDOUT |= 0x01; // set Port 2 output pins to push-pull mode
  //P2MDOUT &= 0xFE; //set Port 2 pin 1 to open drain mode or input 
  //P2 |= ~0xFE;  //set Port 2 input pins to high impedance

  // Port 1
  P1MDIN &= ~0x01;
  P1MDOUT &= ~0x01;
  P1 |= 0x01; //set Port 1 output pins to push-pull mode
}

void Interrupt_Init(void)
{
    IE |= 0x02;   // enable Timer0 Interrupt request (by masking)
    EA = 1;    // enable global interrupts (by sbit)
}
//***************
void Timer_Init(void)
{
    CKCON |= 0x08;  // Timer0 uses SYSCLK as source
    TMOD &= 0xF0;   // clear the 4 least significant bits
    TMOD |= 0x01;   // Timer0 in mode 1
    TR0 = 0;  // Stop Timer0
    TMR0 = 0;  // Clear high & low byte of T0
}

/*configure as analog input */
void ADC_Init(void)
{
    REF0CN = 0x03; //Set Vref to use internal reference voltage

    ADC1CN = 0x80; // Enable ADC1
    ADC1CF |= 0x01; //Set a gain of 1
}

/*timer interrupt*/
void Timer0_ISR(void) __interrupt 1
{
    Counts++;
}


/*returns a random integer number between 0 and 3*/
unsigned char random(void)
{
    return (rand() % 4);  // returns random value between 0-3
}

/* returns A/D result */
unsigned char read_AD_input (unsigned char n) 
{
    AMX1SL = n; // Set P1.N as the analog input for ADC1
    ADC1CN &= ~0x20; //Clear flag from previous ADC1 conversion
    ADC1CN |= 0x10; //Start an A/D cconversion

    while ((ADC1CN & 0x20) == 0x00); //Wait for the conversion to be complete

    return ADC1; //return A/D conversion result
}

/* Blinks LED */
void Blink_Pause(void) 
{
  while (Counts < 169){} //wait half a second
  BILED1 = 0; // turns BILED red 
  BILED2 = 1;
  Counts = 0;
  while (Counts < 169){} //wait half a second
  BILED1 = 1; // turns BILED green 
  BILED2 = 0;
  Counts = 0;
}

/* Displays scores and blinks LED 3 times, input is the Player*/
void End_Won(unsigned char n)
{
  /* display score of Player*/
  printf("Score of Player %d: %d points\n\r\n", n+1 , points_tracker[n]);

  /* Blinks LED 3 times */
  for (i = 0; i < 3; i++)
  {
    LED0 = 0;
    LED1 = 0;
    LED2 = 0;
    LED3 = 0;
    Counts = 0;
    while (Counts < 169){} //waits for half a second
    LED0 = 1;
    LED1 = 1;
    LED2 = 1;
    LED3 = 1;
    Counts = 0;
    while (Counts < 169){} //waits for half a second 
    Counts = 0;
  }

}

/* Displays scores and sounds buzzer, input is the player*/
void End_Lost(unsigned char n)
{
  /* display score of Player*/
  printf("Score of Player %d: %d points\n\r\n", n+1 , points_tracker[n]);
  //Buzzer = 0;
}
