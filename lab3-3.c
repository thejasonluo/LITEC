#include <c8051_SDCC.h>
#include <stdio.h>
#include <stdlib.h>
#include <i2c.h>

#define pw_min 2027
#define pw_neut 2764
#define pw_max 3502

// Function Prototypes
void Port_Init(void);
void PCA_Init (void);
void XBR0_Init(void);
void SMBO_Init(void);
void Interrupt_Init (void);
void PCA_ISR (void) __interrupt 9;
void Steering_Servo(void);
void Drive_Motor(void);
unsigned int Ranger (void);
unsigned int Compass (void);
unsigned int ReadRanger(void);
unsigned int ReadCompass(void);

// Global Variables
unsigned int motor_pw; 
unsigned int servo_pw; 
unsigned char counts = 0;
unsigned int desired_heading = 2700;
unsigned int actual_heading;
signed int error;


unsigned char r_count = 0;
unsigned char h_count = 0;
unsigned char new_range = 0;
unsigned char new_heading = 0;
unsigned char counter= 0;
unsigned char distance;

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

    putchar(' '); //the quotes in this line may not format correctly
    printf("Start\r\n");
    //print beginning message from Lab 3-1
    printf("Speed Control\r\n");
    motor_pw = pw_neut;         //intializes motor_pw as 1.5ms
    servo_pw = pw_neut;         //intializes servo_pw as 1.5ms
    PCA0CP0 = 0xFFFF - servo_pw;        //intiially centers the front wheels
    counts = 0;         //reset counts
    while (counts < 50){}       //wait one second
    while(1)
    {
        //Drive_Motor();
        Steering_Servo();
    }
}

/*INITIALIZATIONS*/

void Port_Init()
{
    P0MDOUT = 0x60;  //set output pin for SCL & SDA
    P1MDOUT = 0x05;     //set output pin for CEX0 & CEX2
    P3MDOUT = 0xC0;     //set output pin
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
    EIE1 |= 0x08;    // enable PCA interrupts
    EA = 1;          // enable all interrupts
}
void PCA_Init(void)
{
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
            //printf("this goes through");
            r_count = 0;
        }
        h_count ++;
        if (h_count >= 2){
            new_heading = 1;        //2 overflows is about 40ms
            h_count = 0;
        }
        CF = 0; // Clear interrupt flag
    }
    else 
        PCA0CN &= 0xC0;        // all other type 9 interrupts
}


/*FUNCTIONS FROM LAB 3-1*/

void Drive_Motor(){
    if (new_range){
        distance = Ranger();
        if (SS1 == 1){
            if (distance <= 10){     //if less than 10 cm
                motor_pw = pw_max;      //full power forward
            //printf("motor_pw: %d", motor_pw);
            }
            else if (distance > 90){     //if more than 90 cm
            motor_pw = pw_min;      //full power reverse
            //printf("motor_pw: %d", motor_pw);
            }
            else if (distance < 50 && distance > 40){       //if between 40-50cm
            motor_pw = pw_neut;     //neutral
            //printf("motor_pw: %d", motor_pw);
            }
            else if (distance > 10 && distance <= 40){      //if between 10-40
            motor_pw = 3748 - (distance*24.6);
            //printf("motor_pw: %d", motor_pw);
            }
            else{       //if between 50-90
            motor_pw = 3684 - (distance * 18.4);
            //printf("motor_pw: %d\n\r", motor_pw);
            }
        }
    }
    printf("motor_pw: %d\n\r", motor_pw);
    PCA0CP2 = 0xFFFF - motor_pw;
}

void Steering_Servo()
{
    unsigned char k;
    if(new_heading){
        actual_heading = Compass();
        printf("Actual heading: %d\n\r", actual_heading);
        printf("Desired heading: %d\n\r", desired_heading);
        error = desired_heading - actual_heading;
        //printf("Error before: %d\n\r", error);
        if (error < 0){
            if (abs(error) > 1800){
                error = (3600 - abs(error));
            }
        }
        else if (error > 1800){
            error = error - 3600;
            //printf("k");
        }
        printf("Error after: %d\n\r", error);

        servo_pw = (error/2) + pw_neut;
        if (servo_pw > pw_max){
            servo_pw = pw_max;
        }
        else if (servo_pw < pw_min){
            servo_pw = pw_min;
        }
        //printf("servo_pw: %d\n\r", servo_pw);
        PCA0CP0 = 0xFFFF - servo_pw;
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

        printf("range: %d\r\n", range); //print range
    }
    return range;
}


unsigned int Compass(void){
    unsigned int heading;
    if (new_heading){        //enough overflows for a new heading
        heading = ReadCompass();
        counter++;
        if (counter == 5){
            //printf("heading: %d\r\n", heading);
            counter = 0;
        }
        new_heading = 0;
    }
    return heading;
}

unsigned int ReadRanger(void){
    unsigned char Data[2];
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

