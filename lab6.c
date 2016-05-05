/*	Name: Po-chin Wu, Adam Pomeranz, and Josh Bostick
	Section: 3
	Side: B
	Date: 
 
	File name: Lab 6
	Description: 
 */

#include <c8051_SDCC.h>
#include <stdio.h>
#include <stdlib.h>
#include <i2c.h>

//-----------------------------------------------------------------------------
// Function Prototypes
//-----------------------------------------------------------------------------
void Port_Init(void);
void PCA_Init (void);
void XBR0_Init();
void SMB_Init(void);
void ADC_Init(void);
void PCA_ISR (void) __interrupt 9;
unsigned int Angle_Setup(void);
void Set_Speed(void);
void Set_Steering(void);
unsigned int Read_Ranger(void);
unsigned int Read_Compass(void);
unsigned int Set_Heading(void);
unsigned int Set_Gain(void);
void Set_Rudder(void);
int read_AIN(void);
void wait(int);

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------
unsigned int Rudder_PW_NEUTRAL = 2765;
unsigned int Rudder_PW_MIN = 2028;
unsigned int Rudder_PW_MAX = 3502;
unsigned int Rudder_PW = 2765;              // 1.5ms
unsigned int Angle_PW = 2765;               // 1.5ms

int counts1;	// counter for ranger
int counts2;	// counter for compass
int counts3;	// counter for pauses
int counts4;	// counter for LCD
unsigned char RFlag; 			// flag to indicate that ranger should read a new range
unsigned char HFlag; 			// flag to indicate that compass should read a new heading
unsigned char LCDFlag;			// flag to indicate LCD should reset display
int Vbatt;						// voltage at the battery

int angular_velocity;
int velocity_MAX = 50;
unsigned int current_range;
unsigned int neutral_range = 50;
unsigned int current_heading;
unsigned int desired_heading;
int heading_correction;
unsigned int Kp;                // Heading Proportional gain constant
unsigned int Kd;                // Heading Derivative gain constant
int previous_error = 0;         // set this value
int error;                      // set this value

unsigned int PCA_Start = 28672; //determines period length, 20ms period




//-----------------------------------------------------------------------------
// Main Function
//-----------------------------------------------------------------------------
void main(void)
{
    // initialize everything
    Sys_Init();
    Port_Init();
    XBR0_Init();
    PCA_Init();
    SMB_Init();
    ADC_Init();
	putchar('\r'); //the quotes in this line may not format correctly

	wait(1000); //wait for 1s for keypad/LCD to initialize

    lcd_clear();
    desired_heading = Set_Heading();	//set the desired heading based on keypad input
    
    lcd_clear();
    lcd_print("Set Kp\n");
    Kp = Set_Gain();                    //set the Proportional gain constant based on keypad input
    
    lcd_clear();
    lcd_print("Set Kd\n");
    Kd = Set_Gain();                   //set the Derivative gain constant constant based on keypad input
    
	PCA0CP1 = 0xFFFF - Angle_PW;

	lcd_clear();
	lcd_print("Loading...");
	wait(1000);	//pause for 1s for ranger and ADC to initalize

    HFlag = 1;		//initialize heading flag to read first heading
    RFlag = 1;		//initialize range flag to read first range
    LCDFlag = 1;	//initialize LCD flag to display the first set of values
    
    while(1)
    { 
        if(RFlag)
        {
            current_range = Read_Ranger();
            heading_correction = 1800*(long)(current_range - neutral_range)/40;
			if (heading_correction < -1800)
			{
				heading_correction = -1800;
			}
            else if (heading_correction > 1800)
			{
				heading_correction = 1800;
			}
            RFlag = 0; //clear range flag
        }
        if(HFlag)
        {
            current_heading = Read_Compass();
			Set_Rudder();
            HFlag = 0; //clear range flag
        }
    
		if(LCDFlag) //if LCD should be updated
		{
			Vbatt = (((read_AIN()*2.4)/256)*4235)+0.5;
        
			lcd_clear();
			lcd_print("Battery Voltage:%d\nCurrent Range:%d\nCurrent Heading:%d", Vbatt, current_range, current_heading);
			LCDFlag = 0;
            printf("\r\n %d,%d,%d,%d", desired_heading,current_heading, Rudder_PW, error);
            //printf("\r\n %d,%d,%d,%d", current_heading, desired_heading+heading_correction,error,angular_velocity);
        }
	}
}

//-----------------------------------------------------------------------------
// Port_Init
//-----------------------------------------------------------------------------
//
// Set up ports for input and output
//
void Port_Init()
{
    P0MDOUT &= ~0x0F;	//set pins 0.0~0.3 TX0, RX0, SDA, and SCL to open drain mode
    P1MDOUT |= 0xF0;	//set pins 0.4~0.7 for CEX0~3 in push-pull mode
    P1MDOUT &= ~0x08;	//set pins 1.3 as open drain for battery voltage.
    P1MDIN &= ~0x08;	//set pins 1.4 and 1.5 as analog inputs.
    P1 |= 0x08;			//set pins 1.4 and 1.5 as high impedence.
}

//-----------------------------------------------------------------------------
// XBR0_Init
//-----------------------------------------------------------------------------
//
// Set up the crossbar
//
void XBR0_Init()
{
    XBR0 = 0x25;  //configure crossbar as directed in the laboratory
    
}

//-----------------------------------------------------------------------------
// PCA_Init
//-----------------------------------------------------------------------------
//
// Set up Programmable Counter Array
//
void PCA_Init(void)
{
    PCA0CPM0 = 0xC2;	// CCM0 in 16-bit compare mode Rudder speed
    PCA0CPM1 = 0xC2;	// CCM0 in 16-bit compare mode Thrust Angle servo
    PCA0CPM2 = 0xC2;	// CCM0 in 16-bit compare mode Right thrust
    PCA0CPM3 = 0xC2;	// CCM0 in 16-bit compare mode Left thrust
    
    PCA0MD &= ~0x0E;	// Clear low bits of PCA0MD
    PCA0MD |= 0x81; 	// Enable CF interrupt and set the counter to SYSCLK/12
    PCA0CN = 0x40; 		// Enable PCA counter
    
    EIE1 |= 0x08; 		// Enable PCA interrupt
    EA = 1; 			// Enable global interrupt
}

//-----------------------------------------------------------------------------
// SMB_Init
//-----------------------------------------------------------------------------
//
// Set up the SM Bus
//
void SMB_Init(void)
{
    SMB0CR = 0x93;	//Set SMB0 clock to 100kHz
    ENSMB = 1;		// Enable SMBus
}

//-----------------------------------------------------------------------------
// ADC_Init
//-----------------------------------------------------------------------------
//
// Set up the analog to digital converter
void ADC_Init(void)
{
    REF0CN |= 0x03; //Set ADC to use internal reference
    ADC1CF |= 0x01; //Set gain to 1
    ADC1CN |= 0x80; //Enable ADC1
    AMX1SL = 0x03;
}

//-----------------------------------------------------------------------------
// PCA_ISR
//-----------------------------------------------------------------------------
//
// Interrupt Service Routine for Programmable Counter Array Overflow Interrupt
//
void PCA_ISR ( void ) __interrupt 9
{
    if (CF)
    {
        CF = 0; // Clear overflow flag
        
        PCA0L = PCA_Start; // Low byte of start count
        PCA0H = PCA_Start>>8; // High byte of start count (20 ms)
        
        counts1++; // counter for ranger
        counts2++; // counter for compass
        counts3++; // counter for 1s
        counts4++; // counter for LCD
        
        if (counts1 > 4) //80ms
        {
            RFlag = 1;
            counts1 = 0;
        }
        
        if (counts2 > 2) //40ms
        {
            HFlag = 1;
            counts2 = 0;
        }
        
        if(counts4 > 20) //400ms
        {
            LCDFlag = 1;
            counts4 = 0;
        }
        
    }
    PCA0CN &= 0xC0; // Handle other PCA interrupt sources
}

//-----------------------------------------------------------------------------
// Read_Compass
//-----------------------------------------------------------------------------
//
// Reads and returns the heading value.
//
unsigned int Read_Compass(void)
{
    unsigned char Data[2];
    unsigned int heading =0;
    unsigned char addr=0xC0; // the address of the compass is 0xC0
    i2c_read_data(addr, 2, Data, 2);	// read two bytes, starting at reg 2
    heading = (((unsigned int)Data[0] << 8) | Data[1]);
    
    return heading;
}

//-----------------------------------------------------------------------------
// Read_Ranger
//-----------------------------------------------------------------------------
//
// Reads and returns the range value.
//
unsigned int Read_Ranger(void)
{
    unsigned char Data[2];
    unsigned int range =0;
    unsigned char addr=0xE0; // the address of the ranger is 0xE0
    i2c_read_data(addr, 2, Data, 2);	// read two bytes, starting at reg 2
    range = (((unsigned int)Data[0] << 8) | Data[1]);
    
    // send a ping to the ranger
    Data[0] = 0x51;
    i2c_write_data(addr, 0,Data, 1);
    
    return range;
}

//-----------------------------------------------------------------------------
// Angle_Setup
//-----------------------------------------------------------------------------
//
// Sets up vertical pulsewidth for angle
//
unsigned int Angle_Setup()
{
    char input;
    char set_done = 0;
    lcd_print("press 1 to go Up\npress 3 to go Down\npress # to end");
    while (!set_done) {
        
        while (read_keypad() == -1) //wait for a key to be pressed
        {
            wait(20);
        }
        input = read_keypad();
        
        wait(50); 	//allows user to hold down buttons
        //reads a keypress every 50ms
        
        if((input-48) == 3)  // input to increase the pulsewidth
        {
            Angle_PW += 10;
            PCA0CP1 = 0xFFFF - Angle_PW;
        }
        else if((input-48) == 1)  // input to decrease the pulsewidth
        {
            Angle_PW -= 10;
            PCA0CP1 = 0xFFFF - Angle_PW;
        }
        else if(input == 35) //ASCII for #, input to signal done
        {
            set_done = 1;
            while (read_keypad() != -1)//wait for key release if finished key was pressed
            {
                wait(20);
            }
        }
    }
    return Angle_PW;
}
//-----------------------------------------------------------------------------
// Set_Heading
//-----------------------------------------------------------------------------
//
// Allows the user to set the desired heading on the keypad.
//
unsigned int Set_Heading(void)
{
	unsigned int keypad2;
    char keypad;
    char choice;
    while(1)
    {
        lcd_clear();
        lcd_print("Desired Heading:\n1) Enter angle\n2) Choose from list");
        
        while (read_keypad() == -1)
        {
            wait(20);
        }
        choice = read_keypad();
        
        while (read_keypad() != -1)
        {
            wait(20);
        }
        
        lcd_clear();
        if ((choice-48) == 1)
        {
            lcd_print("Enter angle(degrees)");
            keypad2 = kpd_input(0);
            wait(20);
            return keypad2*10;
        }
        else if ((choice-48) == 2)
        {
            while(1)
            {
                lcd_print("Select from the list1)0    2)90\n3)180  4)270");
                
                while (read_keypad() == -1)
                {
                    wait(20);
                }
                keypad = read_keypad();
                
                if (4>=(keypad-48)>=1)
                {
                    return ((keypad-48)-1)*900;
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
// Set_Gain
//-----------------------------------------------------------------------------
//
// Allows the user to set the desired gain on the keypad.
//
unsigned int Set_Gain(void)
{
    unsigned int keypad;
    while(1)
    {
        lcd_print("Enter 100 x Gain:");
        
        keypad = kpd_input(0);
        wait(20);			 	// 1 count -> (65536-PCA_START) x 12/22118400 = 20ms
        // This pauses for 1 PCA0 counter clock cycle (20ms)
        // If the keypad is read too frequently (no delay), it will
        // lock up and stop responding. Must power down to reset.
        return keypad;
    }
}
//-----------------------------------------------------------------------------
// Set_Rudder
//-----------------------------------------------------------------------------
//
// Controls the Gondola fans using PWM
//
void Set_Rudder(void)
{
	//error = desired_heading - current_heading + heading_correction;
    error = desired_heading - current_heading;
	
	if (error > 1800)
    {
        error -= 3600; 
	}
	if (error < -1800)
	{
        error += 3600;
    }	

	angular_velocity = error - previous_error;

	if(angular_velocity > velocity_MAX) //if gondola is spining too quickly
	{
		Rudder_PW = Rudder_PW_MAX;
	}
	else if (angular_velocity < -1*velocity_MAX) //if gondola is spining too quickly
	{
		Rudder_PW = Rudder_PW_MIN;
	}
	else
	{
		//set rudder PW based on error and change in error
		if (((long)Kp*error)/100+((long)Kd*(angular_velocity))/100+Rudder_PW_NEUTRAL > Rudder_PW_MAX) 
		{
			Rudder_PW = Rudder_PW_MAX;
		}
		else if (((long)Kp*error)/100+((long)Kd*(angular_velocity))/100+Rudder_PW_NEUTRAL < Rudder_PW_MIN) 
		{
			Rudder_PW = Rudder_PW_MIN;
		}
		else
		{
			Rudder_PW = ((long)Kp*error)/100+((long)Kd*(angular_velocity))/100+Rudder_PW_NEUTRAL;
		}
	}
	
	previous_error = error;
	PCA0CP0 = 0xFFFF - Rudder_PW;
}

//-----------------------------------------------------------------------------
// read_AIN
//-----------------------------------------------------------------------------
//
// Reads from the ADC and returns value
//
int read_AIN(void)
{
    ADC1CN &= ~0x20;	//clear ADC flag
    ADC1CN |= 0x10;		//start the A/D conversion
    while((ADC1CN & 0x20) == 0x00); //wait for conversion to complete
    
    return ADC1;
}

//-----------------------------------------------------------------------------
// wait
//-----------------------------------------------------------------------------
//
// waits for a set number of ms
//
void wait(int ms)
{
    int c = ms/20;
    if (c<1)
    {
        c = 1;
    }
    counts3 = 0;
    while(counts3 < c);
}

