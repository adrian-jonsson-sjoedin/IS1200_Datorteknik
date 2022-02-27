/* mipslabwork.c

   This file written 2015 by F Lundevall
   Updated 2017-04-21 by F Lundevall

    Modified 2022 by Adrian jonsson Sjödin and Bafoday Barrow
    
    Last updated 2022-02-26 by Adrian Jonsson Sjödin and Bafoday Barrow

   For copyright and licensing, see file COPYING */

#include <stdint.h>   /* Declarations of uint_32 and the like */
#include <pic32mx.h>  /* Declarations of system-specific addresses etc */
#include "mipslab.h"  /* Declatations for these labs */


/* Address of the temperature sensor on the I2C bus */
#define TEMP_SENSOR_ADDR 0x48

/* Temperature sensor internal registers */
typedef enum TempSensorReg TempSensorReg;
enum TempSensorReg {
	TEMP_SENSOR_REG_TEMP,
	TEMP_SENSOR_REG_CONF,
	TEMP_SENSOR_REG_HYST,
	TEMP_SENSOR_REG_LIMIT,
};


int tOutCount = 0;

char textstring[] = "text, more text, and even more text!";
#define TIME2PERIOD  ((80000000 /256) / 10); // the chipkit has a freq. of 80MHz and we're
                                              // using a 16 bit timer

//We need to declare our volatile pointer before we can use it otherwise we get errors 
//when we run make
volatile int* tris_E;
volatile int* port_E;

/* Interrupt Service Routine */
void user_isr( void )
{
  return;
}

/* Lab-specific initialization goes here */
void labinit( void )
{
  /*
  The TRISx registers configure the data direction flow through port I/O pins. The TRISx register 
  bits determine whether a PORTx I/O pin is an input or an output:
  The PORTx registers allow I/O pins to be accessed:
  http://ww1.microchip.com/downloads/en/DeviceDoc/60001120F.pdf
  */
  tris_E = (volatile int*) 0xbf886100; //Given address for TRIS E register
  port_E = (volatile int*) 0xbf886110; //Given address for PORT E register
 // To set PORT E's bit 7-0 as a outputs we need to mask the 8 lsb to 0
 // *tris_E = *tris_E & 0xffffff00; // c) done

 //*port_E = 0x00000000; // initialize the value to zero

  //e)
  TRISD = TRISD | 0x00000fe0; //Set bit 11-5 to 1. The rest left untoched
  /* Initialization of timer
  bit 6-4 TCKPS<2:0>: Timer Input Clock prescaler Select bits
  111 = 1:256 prescale value ~0x70
  so we need to set T2CONSET to 0x70 for 1:256 prescaling
  */
  T2CONSET = 0x70;
 //next we need to set our timeperiod. PR is our period register and when a timer 
  //reaches the specified period, it rolls back to 0 and sets the TxIF bit in the 
  //IFS0 interrupt flag register.
  PR2 = TIME2PERIOD;
  //next we need to reset the timer and then start it again. You start it by setting bit
  //15 to 1 in T2CONSET and you reset it with TMR2 = 0x0 which set bit 15-0 to 0
  TMR2 = 0x0;
  T2CONSET = 0x8000;
  return;
}
/* Wait for I2C bus to become idle */
void i2c_idle() {
	while(I2C1CON & 0x1F || I2C1STAT & (1 << 14)); //TRSTAT
}

/* Send one byte on I2C bus, return ack/nack status of transaction */
bool i2c_send(uint8_t data) {
	i2c_idle();
	I2C1TRN = data;
	i2c_idle();
	return !(I2C1STAT & (1 << 15)); //ACKSTAT
}

/* Receive one byte from I2C bus */
uint8_t i2c_recv() {
	i2c_idle();
	I2C1CONSET = 1 << 3; //RCEN = 1
	i2c_idle();
	I2C1STATCLR = 1 << 6; //I2COV = 0
	return I2C1RCV;
}

/* Send acknowledge conditon on the bus */
void i2c_ack() {
	i2c_idle();
	I2C1CONCLR = 1 << 5; //ACKDT = 0
	I2C1CONSET = 1 << 4; //ACKEN = 1
}

/* Send not-acknowledge conditon on the bus */
void i2c_nack() {
	i2c_idle();
	I2C1CONSET = 1 << 5; //ACKDT = 1
	I2C1CONSET = 1 << 4; //ACKEN = 1
}

/* Send start conditon on the bus */
void i2c_start() {
	i2c_idle();
	I2C1CONSET = 1 << 0; //SEN
	i2c_idle();
}

/* Send restart conditon on the bus */
void i2c_restart() {
	i2c_idle();
	I2C1CONSET = 1 << 1; //RSEN
	i2c_idle();
}

/* Send stop conditon on the bus */
void i2c_stop() {
	i2c_idle();
	I2C1CONSET = 1 << 2; //PEN
	i2c_idle();
}

/* This function is called repetitively from the main program */
void temperature( void ){
  int buttons = getbtns();
  int button1 = getbtns1();
  //int tSwitches = getsw();
    // wont work if we want to be able to press two or more buttons at the same time
    // if(getbtns() != 0x0){
    // switch(buttons){
    //   case 0x0001: //001
    //     mytime = mytime & 0xff0f;
    //     mytime = (tSwitches << 4) | mytime;
    //     break;

    //   case 0x0002://010
    //     mytime = mytime & 0xf0ff;
    //     mytime = (tSwitches << 8) | mytime;
    //     break;

    //   case 0x0004://100
    //     mytime = mytime & 0x0fff;
    //     mytime = (tSwitches << 12) | mytime;
    //     break;
    // }
  //}

  // we do a bitwise and to see which button/s are pressed and if we have two or more 
  //buttons pressed at the same time we will have the corresponding if statement triggered
  // if(buttons & 0x0001){
  //   mytime = mytime & 0xff0f;
  //   mytime = (tSwitches << 4) | mytime;
  // }
  // if(buttons & 0x0002){
  //   mytime = mytime & 0xf0ff;
  //   mytime = (tSwitches << 8) | mytime;
  // }
  // if(buttons & 0x0004){
  //   mytime = mytime & 0x0fff;
  //   mytime = (tSwitches << 12) | mytime;
  // }

  if(IFS(0) & 0x100){//if the 8th bit is 1 increment count
    tOutCount++;

    IFSCLR(0) = 0x00000100; // Clear the timer interrupt status flag
  }
  if(tOutCount == 10){
  
    
    display_string( 0, textstring );
    display_update();
   ;
    
    tOutCount = 0;
  
  }
}
