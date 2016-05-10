/*	Name: Jason Luo, Michael Cuozzo
	Section: 3
	Side: B
	Date: 4/25/16
 
	File name: Lab 3-3
	Description: Reads from the ultrasonic ranger and electric compass using I2C
	and sets motor speed and servo position using PWM.
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
void XBR0_Init(void);
void SMB_Init(void);
void ADC_Init(void);
void PCA_ISR (void) __interrupt 9;
void Steering_Servo(void);
char Set_Gain();
void pause(unsigned char n);
unsigned int Read_Battery(void);
unsigned int Set_Heading(void);
unsigned int Ranger (void);
unsigned int Compass (void);
unsigned int ReadRanger(void);
unsigned int ReadCompass(void);

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------
unsigned int pw_min = 2028;
unsigned int pw_neut = 2765;
unsigned int pw_max = 3502;
unsigned char r_count = 0;
unsigned char h_count = 0;
unsigned char new_range;
unsigned char new_heading;
unsigned int distance;

unsigned int error;
unsigned int prev_error;
unsigned int desired_heading;
unsigned int actual_heading;
unsigned int motor_pw;
unsigned int volt;
unsigned int LCD_count;
unsigned char LCD_update;
int correction;
char kp = 12;
char kd = 0;
char counter = 0;



//-----------------------------------------------------------------------------
// Main Function
//-----------------------------------------------------------------------------
void main(void)
{
    // initialize everything
    Sys_Init();
    putchar(' '); //the quotes in this line may not format correctly
    Port_Init();
    XBR0_Init();
    PCA_Init();
    SMB_Init();
    //print beginning message
    printf("Start\n\r");
    pause(50);      //wait 1s for LCD to intialize

    //set desired heading
    lcd_clear();
    desired_heading = Set_Heading();
    
    //set Kp gain
    lcd_clear();
    lcd_print("Set Kp gain\n\r");
    kp = Set_Gain();
    
    //set Kd gain
    lcd_clear();
    lcd_print("Set Kd gain\n\r");
    kd = Set_Gain();

    PCA0CP0 = 0xFFFF - pw_neut;        //sets rudder fan to neutral
    PCA0CP1 = 0xFFFF - pw_neut;        //sets thrust angle to 0
    PCA0CP2 = 0xFFFF - pw_neut;        //sets left thrust to 0
    PCA0CP3 = 0xFFFF - pw_neut;        //sets right thrust to 0

    pause(50);      //wait for ranger to intialize
    
    while(1)
    {
        if (new_range){
            distance = Ranger();
            correction = 1800*(long)(distance - 50)/40;
            if (correction < -1800){
                correction = -1800;
            }
            else if (correction > 1800){
                correction = 1800;
            }
            new_range = 0;
        }
        if (new_heading){
            Steering_Servo();
            new_heading = 0;
        }

        if (LCD_update == 1){        //updates once every second
            volt = Read_Battery();
            lcd_clear();
            lcd_print("Range:%d\nHeading:%d\nVoltage:%dmV", distance, actual_heading, volt);
            printf("%d,%d,%d\n\r", actual_heading, distance ,servo_pw);
            LCD_update = 0;
        }

        printf("%d,%d,%d,%d,%d", desired_heading, distance, heading, motor_pw, volt);
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
    //P0MDOUT |= 0xC0;	//set pins 0.6 and 0.7 for SDA and SCL in push-pull mode
    P1MDOUT |= 0xF0;	//configure for CCM and analog
    //P3MDOUT	&= ~0xC0;	//set pins 3.6 and 3.7 for digital input
    //P3 |= 0xC0;
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
    PCA0CPM0 = 0xC2;	// CCM0 in 16-bit compare mode Rudder fan
    PCA0CPM1 = 0xC2;	// CCM0 in 16-bit compare mode thrust angle
    PCA0CPM2 = 0xC2;	// CCM0 in 16-bit compare mode right thrust fan
    PCA0CPM3 = 0xC2;	// CCM0 in 16-bit compare mode left thrust fan
    
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
// PCA_ISR
//-----------------------------------------------------------------------------
//
// Interrupt Service Routine for Programmable Counter Array Overflow Interrupt
//

void ADC_Init(void)
{
    REF0CN |= 0x03; //Set ADC to use internal reference
    ADC1CF |= 0x01; //Set gain to 1
    ADC1CN |= 0x80; //Enable ADC1
    AMX1SL = 0x03;
}

void PCA_ISR ( void ) __interrupt 9
{
    if (CF)
    {
        PCA0 = 28672;

        r_count ++;
        if (r_count >= 4){
            new_range = 1;      //4 overflows is 80 ms
            r_count = 0;
        }

        h_count ++;
        if (h_count >= 2){
            new_heading = 1;        //2 overflows is about 40ms
            h_count = 0;
        }

        LCD_count++;
        if (LCD_count > 50){        //20 overflows is 400ms
            LCD_count = 0;
            LCD_update = 1;
        }

        CF = 0; // Clear overflow flag
    }
    PCA0CN &= 0xC0; // Handle other PCA interrupt sources
}


void Steering_Servo(void)
{
    actual_heading = Compass();
    //printf("Actual heading: %d\n\r", actual_heading);
    error = desired_heading - actual_heading + correction;
    
    if (error > 1800){
        error = error - 3600;
    }
    if (error <-1800){
        error = error + 3600;
    }    

    if ((error-prev_error) > pw_max){
        motor_pw = pw_max;
    }
    else if ((error - prev_error) < pw_min){
        motor_pw = pw_min;
    }
    else{
        motor_pw = (long)pw_neut+(long)error*(long)kp+(long)kd*(long)(error-prev_error);
        if (motor_pw > pw_max){
            motor_pw = pw_max;
        }
        else if (motor_pw < pw_min){
            motor_pw = pw_min;
        }
    }

    prev_error = error;
    PCA0CP0 = 0xFFFF - motor_pw;
}


unsigned int Set_Heading(void){
    lcd_clear();
    lcd_print("Set Heading 1)0 2)903)180 4)270 5)custom\r");

    while(read_keypad() == -1){pause(20);}
    keypad = read_keypad();
    while (read_keypad() != -1) {pause(20);}      //wait until key  is released
    if ((keypad - 48) < 5){
        lcd_clear();
        lcd_print("Selected %d degrees\n\r", (keypad-49)*90);
        return (keypad-49)*900;
    }
    else {         //if 5 is pressed
        lcd_clear();
        while(read_keypad() == -1){pause(20);}     //wait until key is pressed
        keypad = read_keypad();         //read keypad
        while (read_keypad() != -1) {pause(20);}      //wait until key  is released
        keypad = kpd_input(0);
        pause(20);

        return keypad*10;
    }
}

unsigned int Set_Gain(void){
    char keypad;
    while (1){
        lcd_clear();
        lcd_print("Please set gain");

        keypad = kpd_input(0);
        pause(20);

        return keypad;
    }  
}


/*Functions from Lab 4 Prelab */
void pause(unsigned char n)
{
    nCounts = 0;
    n = n/20;
    while (nCounts < n);// 1 count -> (65536-PCA_START) x 12/22118400 = 20ms
}   

unsigned int Read_Battery(void){
    unsigned int val;
    val = read_AD_input(4);
    return val*8.63;
}
