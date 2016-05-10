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
void read_accel(void);
void pause(unsigned char n);
unsigned int Set_Gain(void);
unsigned int Read_Battery(void);
void calibrate_offset(void);

// Global Variables
int motor_pw; 
int servo_pw; 
unsigned char LCD_count;
unsigned char LCD_update;
unsigned char nCounts;
unsigned int volt;
unsigned char STOP;
unsigned char turn = 0;

int pw_max_s = 3185;
int pw_neut_s = 2715;
int pw_min_s = 2195;

unsigned char counter= 0;
unsigned char distance;
unsigned char rand1 = 0;
char accel_count;
char new_accel;

unsigned int kdx;       //y-axis drive feedback gain
unsigned int kdy;        //x-axis drive feedback gain
unsigned int ks;       //steering gain
int gx;
int gy;
unsigned int val;
int x_offset;
int y_offset;

__sbit __at 0xB6 SS1; // Slideswitch 1 connected to Port 3 Pin 6

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
    Accel_Init();

    putchar(' '); //the quotes in this line may not format correctly
    printf("Start\r\n");
    motor_pw = pw_neut;         //intializes motor_pw as 1.5ms
    servo_pw = pw_neut;         //intializes servo_pw as 1.5ms
    PCA0CP0 = 0xFFFF - servo_pw;        //intiially centers the front wheels
    PCA0CP2 = 0xFFFF - motor_pw;         //initially set speed to zero
    pause(50);
    lcd_clear();

    //set Kdx gain
    lcd_clear();
    lcd_print("Set Kdx\n\r");
    kdx = Set_Gain();
    printf("Kdx = %d\n\r", kdx);
    
    //set Ks gain
    lcd_clear();
    lcd_print("Set Ks\n\r");
    ks = Set_Gain();
    printf("Ks = %d\n\r", ks);

    //calibrate accelerometer
    lcd_clear();
    lcd_print("Press any key to \ncalibrate");
    while (read_keypad() == -1){
        pause(20);
    }
    lcd_clear();
    lcd_print("Calibrating...");
    calibrate_offset();
    lcd_clear();

    AMX1SL = 0x05;
    pause(50);
    while(1){     
        if (new_accel){
            read_accel();
            val = read_AD_input(5);
            kdy = (val*50)/256;
            Drive_Motor();
            Steering_Servo();
            new_accel = 0;
        }
        /*UPDATES LCD DISPLAY*/
        if (LCD_update == 1){        //updates once every second
            AMX1SL = 0x04;
            pause(20);
            volt = Read_Battery();
            lcd_clear();
            lcd_print("X Accel.:%d\nY Accel:%d\nMotor PW:%d\nBattery:%dmV",gx-x_offset,gy-y_offset,motor_pw,volt);
            printf("%d,%d,%d,%d,%d,%d,%d,%d,%d\n\r", gx-x_offset, x_offset, gy-y_offset, y_offset, kdx, kdy, ks, motor_pw ,servo_pw);
            AMX1SL = 0x05;
            pause(20);
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
    P3 |= 0xC0;
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
    PCA0CPM2 |= 0xC2; // 16 bit, enable compare, enable PWM
    PCA0CPM0 |= 0xC2; 
    PCA0MD &= ~0X0E;        //Clear low bits of PCA0MD
    PCA0MD = 0x81;   // SYSCLK/12, enable CF interrupts, suspend when idle
    PCA0CN |= 0x40;  // enable PCA
}

void PCA_ISR ( void ) __interrupt 9
{
    if (CF)
    {
        PCA0L = PCA_START;   // low byte of start count
        PCA0H = PCA_START >> 8;     // high byte of start count
        CF = 0; // Clear interrupt flag

        LCD_count++;
        if (LCD_count > 20){        //20 overflows is 400ms
            LCD_count = 0;
            LCD_update = 1;
        }

        accel_count++;
        if (accel_count > 8){
            new_accel = 1;
            accel_count = 0;
        }

        nCounts++;

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
    motor_pw = pw_neut + ((long)kdy * (long)(gy - y_offset));
    if (gx > 0){
        motor_pw += ((long)kdx * (long)abs(gx - x_offset));
    }
    else{
        motor_pw -= ((long)kdx * (long)abs(gx - x_offset));
    }

    if (motor_pw < pw_min || motor_pw < 0){
        motor_pw = pw_min;
    }
    else if (motor_pw > pw_max){
        motor_pw = pw_max;
    }

    PCA0CP2 = 0xFFFF - motor_pw;
}

void Steering_Servo()
{   
    servo_pw = pw_neut_s - ((long)ks * (gx - x_offset));
    if (servo_pw < pw_min_s){
        servo_pw = pw_min_s;
    }
    else if (servo_pw > pw_max_s){
        servo_pw = pw_max_s;
    }
    PCA0CP0 = 0xFFFF - servo_pw;
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

void read_accel(void){
    int avg_gx = 0;
    int avg_gy = 0;
    char n;
    unsigned char Data[4];
    for (n = 0; n < 8; n++){
        Data[0] = 0;
        while ((Data[0] & 0x03) != 0x03){
            pause(20);      //wait 20 ms
            i2c_read_data(0x30,0x27,Data,1);       //read status
        }

        i2c_read_data(0x30,0x28|0x80,Data,4);      //assert MSB to read mult Bytes
        avg_gx += ((Data[1] << 8) >> 4);
        avg_gy += ((Data[3] << 8) >> 4);
    }

    avg_gx = avg_gx / 8;
    avg_gy = avg_gy / 8;

    gx = avg_gx;
    gy = avg_gy;
}

void calibrate_offset (void){
    char n;
    unsigned char Data[4];
    int sum_x = 0;
    int sum_y = 0;
    for (n = 0; n < 64; n++){
        Data[0] = 0;
        while ((Data[0] & 0x03) != 0x03){
            pause(20);      //wait 20 ms
            i2c_read_data(0x30,0x27,Data,1);       //read status
        }
        i2c_read_data(0x30,0x28|0x80,Data,4);      //assert MSB to read mult Bytes
        
        sum_x += ((Data[1] << 8) >> 4);
        sum_y += ((Data[3] << 8) >> 4);
    }

    x_offset = sum_x / 64;
    y_offset = sum_y / 64;

}