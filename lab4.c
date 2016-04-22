#include <c8051_SDCC.h>
#include <stdio.h>
#include <stdlib.h>
#include <i2c.h>

#define pw_min 2027
#define pw_neut 2764
#define pw_max 3502
#define PCA_START 28672 // 28672 for exactly 20ms

// Function Prototypes
void Port_Init(void);
void PCA_Init (void);
void XBR0_Init(void);
void SMBO_Init(void);
void Interrupt_Init (void);
void PCA_ISR (void) __interrupt 9;
void ADC_Init(void);      //Initialize A/D converter
unsigned char read_AD_input(unsigned char n);       //reads AD input
void Steering_Servo(void);
void Drive_Motor(void);
unsigned int Ranger (void);
unsigned int Compass (void);
unsigned int ReadRanger(void);
unsigned int ReadCompass(void);
void pause(unsigned char n);
unsigned int Set_Heading(void);
unsigned int Set_Gain(void);
unsigned int Read_Battery(void);
int Update_Value(int Constant, unsigned char incr, int maxval, int minval);

// Global Variables
unsigned int motor_pw; 
unsigned int servo_pw; 
unsigned char counts = 0;
unsigned int desired_heading;
unsigned int actual_heading;
signed int error;
char keypad;
unsigned int Counts, nCounts, nOverflows;
unsigned int gain;
unsigned char turn_dist = 25;
unsigned char stop_dist = 20;
unsigned char LCD_count;
unsigned char LCD_update;
unsigned char voltage_count;
unsigned char voltage_update;
unsigned int volt;
unsigned char STOP = 1;
unsigned char turn = 0;


unsigned char r_count = 0;
unsigned char h_count = 0;
unsigned char new_range = 0;
unsigned char new_heading = 0;
unsigned char counter= 0;
unsigned char distance;
unsigned char rand1 = 0;

__sbit __at 0xB6 SS1; // Slideswitch 1 connected to Port 3 Pin 6
__sbit __at 0xB7 SS2; //Slideswitch 2 connected to Port 3 Pin 7

void main(void)
{
    /*initialize board*/
    Sys_Init();
    Port_Init(); //initialize Ports
    XBR0_Init(); //intitialize XBAR
    SMBO_Init(); //Initialize SMBUS
    Interrupt_Init(); //initialize interrupts
    PCA_Init(); //initialize PCA
    ADC_Init();

    putchar(' '); //the quotes in this line may not format correctly
    printf("Start\r\n");
    motor_pw = pw_neut;         //intializes motor_pw as 1.5ms
    servo_pw = pw_neut;         //intializes servo_pw as 1.5ms
    PCA0CP0 = 0xFFFF - servo_pw;        //intiially centers the front wheels
    PCA0CP2 = 0xFFFF - motor_pw;         //initially set speed to zero
    counts = 0;         //reset counts
    while (counts < 50){}       //wait one second
    Counts = 0;
    while (Counts < 1); // Wait a long time (1s) for keypad & LCD to initialize
    lcd_clear();
    //desired_heading = Set_Heading();
    desired_heading = 900;
    printf("desired heading: %d\n\r", (desired_heading / 10));
    lcd_clear();
    //gain = Set_Gain();
    gain = 2;
    printf("desired gain: %d", gain);
    lcd_clear();

    AMX1SL = 0x05;
    lcd_clear();
    pause(50);

    while(1)
    {
        if (new_range){
            distance = Ranger();

            if (rand1 == 0){
                pause(1000);
                distance = Ranger();
                rand1 = 1;
            }
            if (distance < turn_dist && turn == 0){
                desired_heading = desired_heading + 900;
                if (desired_heading > 3599){
                    desired_heading = desired_heading - 3600;
                }
                turn = 1;
            }

            if (distance < stop_dist && turn == 1){
               // printf("STOP!!\n\r");
                STOP = 1;
            }
            else{
                STOP = 0;
            }
            
            Drive_Motor();
            Steering_Servo();
            new_range = 0;
        }

        /*UPDATES LCD DISPLAY*/
        if (LCD_update == 1){        //updates once every second
            volt = Read_Battery();
            lcd_clear();
            lcd_print("Range:%d\nHeading:%d\nVoltage:%dmV", distance, actual_heading, volt);
            printf("%d,%d,%d\n\r", actual_heading, error,motor_pw);
            LCD_update = 0;
        }
    }
}

/*INITIALIZATIONS*/

void Port_Init()
{
    P0MDOUT &= ~0xC3;  //set output pin for SCL & SDA
    
    P1MDOUT |= 0x05;     //set output pin for CEX0 & CEX2
    P1MDOUT &= ~0x30;       //set pins 1.4 and 1.5 for open drain for ADC
    P1MDIN &= ~0x30;        //set pins 1.4 and 1.5 as analog inputs
    P1 |= 0x30;             //set pins 1.4 and 1.5 as high impedence
    
    P3MDOUT |= 0xC0;    
    P3MDOUT &= ~0xC0;
    P3 |= 0x80;
}

void XBR0_Init()
{
    XBR0 = 0x27;  //configure crossbar as directed in the laboratory

}

void SMBO_Init(void){
    SMB0CR = 0x93;      //set SCL to 100Khz
    ENSMB = 1;       //enable SMBUS0
}


void Interrupt_Init()
{
    IE |= 0x02; 
    EIE1 |= 0x08;    // enable PCA interrupts
    EA = 1;          // enable all interrupts
}
void PCA_Init(void)
{
    PCA0MD &= ~0X0E;        //Clear low bits of PCA0MD
    PCA0MD = 0x81;   // SYSCLK/12, enable CF interrupts, suspend when idle
    PCA0CPM2 = 0xC2; // 16 bit, enable compare, enable PWM
    PCA0CPM0 = 0xC2; 
    PCA0CN |= 0x40;  // enable PCA
}

void PCA_ISR ( void ) __interrupt 9
{
    counts ++;      //using PCA overflows for timer from Lab 3-1
    if (CF)
    {
        PCA0 = 28671; // Period of 20ms also the value of PCA_start 
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
        CF = 0; // Clear interrupt flag

        LCD_count++;
        if (LCD_count > 50){        //20 overflows is 400ms
            LCD_count = 0;
            LCD_update = 1;
        }

        voltage_count++;
        if (voltage_count > 50){
            voltage_count = 0;
            voltage_update = 1;
        }

        nOverflows++;               // continuous overflow counter
        nCounts++;
        PCA0L = PCA_START & 0xFF;   // low byte of start count
        PCA0H = PCA_START >> 8;     // high byte of start count
        if (nCounts > 50)
        {
            nCounts = 0;
            Counts++;               // seconds counter
        }
    }
    else 
        PCA0CN &= 0xC0;        // all other type 9 interrupts
}

void ADC_Init(void)
{
    REF0CN = 0x03; //Set Vref to use internal reference voltage sets pin

    ADC1CN = 0x80; // Enable ADC1
    ADC1CF |= 0x01; //Set a gain of 1
}

unsigned char read_AD_input (unsigned char n) 
{
    AMX1SL = n; // Set P1.N as the analog input for ADC1
    pause(20);
    ADC1CN &= ~0x20; //Clear flag from previous ADC1 conversion
    ADC1CN |= 0x10; //Start an A/D cconversion

    while ((ADC1CN & 0x20) == 0x00); //Wait for the conversion to be complete
    return ADC1; //return A/D conversion result
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


/*FUNCTIONS FROM LAB 3-1*/
void Drive_Motor(){
    unsigned char val;
        if (SS1 == 1 && STOP == 0){
            val = read_AD_input(5);
            pause(20);
            //printf("val: %d\n\r", val);
            motor_pw = 2028 + 5.78*val;
            //printf("motor_pw: %d\n\r", motor_pw);
            PCA0CP2 = 0xFFFF - motor_pw;
        }
        else {
            PCA0CP2 = 0xFFFF - pw_neut;                     //else goes to neutral
        }
}

void Steering_Servo()
{
    if(new_heading){
        if (SS1 ==1){
            actual_heading = Compass();
            //printf("Actual heading: %d\n\r", actual_heading);
            error = desired_heading - actual_heading;
            
            if (error > 1800){
                error = error - 3600;
            }
            if (error <-1800){
                error = error + 3600;
            }

            servo_pw = (error*gain) + pw_neut;
            if (servo_pw > pw_max){
                servo_pw = pw_max;
            }
            else if (servo_pw < pw_min){
                servo_pw = pw_min;
            }
            PCA0CP0 = 0xFFFF - servo_pw;
        }
        else{
            PCA0CP0 = 0xFFFF - pw_neut;
        }
        new_heading = 0;
    }
}


/*FUNCTIONS FROM LAB 3-2*/
unsigned int Ranger (void){
    unsigned int range = 0;
    unsigned char addr = 0xE0;
    unsigned char Data[2];
    /*waits 80 ms*/
    //printf("\r\nnew_range: %u\r\n", new_range);
    if (new_range){
        new_range = 0;  //clear new_range
        range = ReadRanger();   //read ranger

        Data[0] = 0x51;
        i2c_write_data(addr,0,Data,1); //write one byte to reg 0 at addr

        //printf("range: %d\r\n", range); //print range
    }
    return range;
}


unsigned int Compass(void){
    unsigned int heading;
    if (new_heading){        //enough overflows for a new heading
        heading = ReadCompass();
        counter++;
        if (counter == 5){
            counter = 0;
        }
        new_heading = 0;
    }
    return heading;
}

unsigned int ReadRanger(void){
    unsigned char Data[2];  //Data is an array with length of 2
    unsigned int range = 0;
    unsigned char addr=0xE0; //address of ranger is 0xE0
    i2c_read_data(addr,2,Data,2); //read two bytes, starting at reg 2
    range = (((unsigned int) Data[0] << 8 )| Data[1]);
    return range; 
}

unsigned int ReadCompass(void){
    unsigned char addr = 0xC0;      //address of the sensor, 0xC0 for the compass
    unsigned char Data[2];      //Data is an array with length of 2
    unsigned int heading;       //the heading return in degrees between 0 and 359
    i2c_read_data(addr,2,Data,2);       //read two byte, starting at reg 2
    heading = (((unsigned int) Data[0] << 8 ) | Data[1] );      //combine two values
    //heading has units of 1/10 of a degree
    return heading;
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
