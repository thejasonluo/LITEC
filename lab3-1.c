#include <c8051_SDCC.h>
#include <stdio.h>
#include <stdlib.h>

#define pw_min 2027
#define pw_max 3502
#define pw_neut 2764
//-----------------------------------------------------------------------------
// Function Prototypes
//-----------------------------------------------------------------------------
void Port_Init(void);
void PCA_Init (void);
void XBR0_Init(void);
void Interrupt_Init(void);
void Steering_Servo(void);
void Drive_Motor(void);
void PCA_ISR (void) __interrupt 9;
unsigned int next_function(void);

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------
unsigned int PW_CENTER; 
unsigned int motor_pw; 
unsigned int servo_pw; 
unsigned char counts = 0;
unsigned int PW_max;
unsigned int PW_min;

//-----------------------------------------------------------------------------
// Main Function
//-----------------------------------------------------------------------------
void main(void)
{
    // initialize board
    Sys_Init();
    putchar(' '); //the quotes in this line may not format correctly
    Port_Init(); //initialize Ports
    XBR0_Init(); //intitialize XBAR
    Interrupt_Init(); //initialize interrupts
    PCA_Init(); //initialize PCA

    //print beginning message
    printf("Speed Control\r\n");
    motor_pw = pw_neut;         //intializes motor_pw as 1.5ms
    servo_pw = pw_neut;         //intializes servo_pw as 1.5ms
    PCA0CP0 = 0xFFFF - servo_pw;        //intiially centers the front wheels
    counts = 0;         //reset counts
    while (counts < 50){}       //wait one second
    /*Let user determine the center*/
    printf("set center by using R & L\r\n");
    PW_CENTER = next_function();
    /*Let users determine left most and right most limit of the wheels*/
    printf("\r\nSet right limit by using R\r\n");
    PW_max = next_function();
    printf("\r\nSet left limit by using L \r\n");
    PW_min = next_function();
    printf("\r\nUse S or F to control acceleration and R or L to control steering direction");
    while(1)
    {
        Drive_Motor();
        Steering_Servo();
    }
}

void Port_Init()
{
    P1MDOUT = 0x05;  //set output pin for CEX0 or CEX2 in push-pull mode
}

void XBR0_Init()
{
    XBR0 = 0x27;  //configure crossbar as directed in the laboratory

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
    counts ++; //using PCA overflows for timer
    if (CF)
    {
        PCA0 = 28671; // Period of 20ms also the value of PCA_start
        CF = 0; // Clear interrupt flag
    }
    else PCA0CN &= 0xC0;        // all other type 9 interrupts
}

void Drive_Motor()
{
    char input;
    input = getchar();  //wait for a key to be pressed
    if (input =='f')
    {
        if (motor_pw < pw_max)
        {
            motor_pw = motor_pw + 10; //increase the steering pulsewidth by 10
            printf("motor_pw: %d", motor_pw);
        }
    }
    else if (input == 's')
    {
        if (motor_pw > pw_min){
            motor_pw = motor_pw - 10; //decrease the steering pulsewidth by 10
            printf("motor_pw: %d", motor_pw);
        }
    }
    PCA0CP2 = 0xFFFF - motor_pw;
}

void Steering_Servo()
{
    char input;
    input = getchar();
    if(input == 'l') //if 'r' - single character input to increase the pulsewidth
    {
        servo_pw = servo_pw - 10;
        if(servo_pw < PW_min)
        {
            servo_pw = PW_min; //set servo_pw to a minimum value  
        } // check if less than pulsewidth minimum
    }
    else if(input == 'r') //if 'l' - single character input to decrease the pulsewidth
    {
        servo_pw = servo_pw + 10;
        if(servo_pw > PW_max) // check if pulsewidth maximum exceeded
        {
           servo_pw = PW_max; // set servo_pw to a maximum value  
        }
    }
    PCA0CP0 = 0xFFFF - servo_pw;
}


unsigned int next_function(void)
{
    while(1)
    {
        char input;
        //wait for a key to be pressed
        input = getchar();
        if(input == 'r') //if 'r' - single character input to increase the pulsewidth
        {
            servo_pw = servo_pw + 10;
            printf("pw: %d", servo_pw);
        }
        else if(input == 'l') //if 'l' - single character input to decrease the pulsewidth
        {
            servo_pw = servo_pw - 10;
            printf("pw: %d", servo_pw);
        }
        else if (input =='t')
        {
            break;
        }
        PCA0CP0 = 0xFFFF - servo_pw;
    }
    return servo_pw;
}
