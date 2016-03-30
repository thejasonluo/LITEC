#include <c8051_SDCC.h>
#include <stdio.h>
#include <stdlib.h>
#include <i2c.h>

// Function Prototypes
void Port_Init(void);
void PCA_Init (void);
void XBR0_Init(void);
void SMBO_Init(void);
void Interrupt_Init (void);
void Drive_Motor(void);
void PCA_ISR (void) __interrupt 9;
void Ranger (void);
void Compass (void);
unsigned int ReadRanger(void);
unsigned int ReadCompass(void);

// Global Variables
unsigned char r_count = 0;
unsigned char h_count = 0;
unsigned char new_range = 0;
unsigned char new_heading = 0;
unsigned char counter= 0;

void main(void)
{
    // initialize board
    Sys_Init();
    Port_Init(); //initialize Ports
    XBR0_Init(); //intitialize XBAR
    SMBO_Init(); //Initialize SMBUS
    Interrupt_Init(); //initialize interrupts
    PCA_Init(); //initialize PCA
    putchar(' '); //the quotes in this line may not format correctly
    printf("Start\r\n");
    while(1)
    {
        //Ranger();
        Compass();
    }
}

void Port_Init()
{
    P0MDOUT = 0x60;  //set output pin for SCL & SDA
    P1MDOUT = 0x05;     //set output pin for CEX0 & CEX2
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
    if (CF)
    {
        r_count ++;
        //printf("\r\nr_count: %u\r\n", r_count);
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

void Ranger (void){
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
}


void Compass(void){
    unsigned int heading;
    if (new_heading){        //enough overflows for a new heading
        heading = ReadCompass();
        counter++;
        if (counter == 5){
            printf("heading: %d\r\n", heading);
            counter = 0;
        }
        new_heading = 0;
    }
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

