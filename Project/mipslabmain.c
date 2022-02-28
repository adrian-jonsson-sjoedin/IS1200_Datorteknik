/* mipslabmain.c

   This file written 2015 by Axel Isaksson,
	modified 2015, 2017 by F Lundevall
		modified 2022 by Adrian Jonsson Sjödin and Bafoday Barrow

   Latest update 2022-02-27 by Adrian Jonsson Sjödin and Bafoday Barrow

   For copyright and licensing, see file COPYING
   */

#include <stdint.h>	 /* Declarations of uint_32 and the like */
#include <pic32mx.h> /* Declarations of system-specific addresses etc */
#include <stdbool.h>
#include <stdio.h>
#include "mipslab.h" /* Declatations for these labs */
#include <float.h>
#include <math.h>

/* Address of the temperature sensor on the I2C bus */
#define TEMP_SENSOR_ADDR 0x48

int tOutCount = 0;
int units = 0;
int type = 0;
int menuPage = 1;
int celcius = 0;
int kelvin = 1;
int farenheit = 0;
int sTemp = 0;

char textstring[] = "text, more text, and even more text!";
#define TIME2PERIOD ((80000000 / 256) / 10); // the chipkit has a freq. of 80MHz and we're
											 // using a 16 bit timer

// We need to declare our volatile pointer before we can use it otherwise we get errors
// when we run make
volatile int *tris_E;
volatile int *port_E;

/* Interrupt Service Routine */
void user_isr(void)
{
	return;
}
// void delay(int cyc)
// {
// 	int i;
// 	for (i = cyc; i > 0; i--)
// 		;
// }
/* Initialization goes here */
void init(void)
{
	/*
	The TRISx registers configure the data direction flow through port I/O pins. The TRISx register
	bits determine whether a PORTx I/O pin is an input or an output:
	The PORTx registers allow I/O pins to be accessed:
	http://ww1.microchip.com/downloads/en/DeviceDoc/60001120F.pdf
	*/
	tris_E = (volatile int *)0xbf886100; // Given address for TRIS E register
	port_E = (volatile int *)0xbf886110; // Given address for PORT E register
	// To set PORT E's bit 7-0 as a outputs we need to mask the 8 lsb to 0
	// *tris_E = *tris_E & 0xffffff00; // c) done

	//*port_E = 0x00000000; // initialize the value to zero

	// e)
	TRISD = TRISD | 0x00000fe0; // Set bit 11-5 to 1. The rest left untoched
	/* Initialization of timer
	bit 6-4 TCKPS<2:0>: Timer Input Clock prescaler Select bits
	111 = 1:256 prescale value ~0x70
	so we need to set T2CONSET to 0x70 for 1:256 prescaling
	*/
	T2CONSET = 0x70;
	// next we need to set our timeperiod. PR is our period register and when a timer
	// reaches the specified period, it rolls back to 0 and sets the TxIF bit in the
	// IFS0 interrupt flag register.
	PR2 = TIME2PERIOD;
	// next we need to reset the timer and then start it again. You start it by setting bit
	// 15 to 1 in T2CONSET and you reset it with TMR2 = 0x0 which set bit 15-0 to 0
	TMR2 = 0x0;
	T2CONSET = 0x8000;
	return;
}

/* Temperature sensor internal registers */
typedef enum TempSensorReg TempSensorReg;
enum TempSensorReg
{
	TEMP_SENSOR_REG_TEMP,
	TEMP_SENSOR_REG_CONF,
	TEMP_SENSOR_REG_HYST,
	TEMP_SENSOR_REG_LIMIT,
};
/* Wait for I2C bus to become idle */
void i2c_idle()
{
	while (I2C1CON & 0x1F || I2C1STAT & (1 << 14))
		; // TRSTAT
}

/* Send one byte on I2C bus, return ack/nack status of transaction */
bool i2c_send(uint8_t data)
{
	i2c_idle();
	I2C1TRN = data;
	i2c_idle();
	return !(I2C1STAT & (1 << 15)); // ACKSTAT
}

/* Receive one byte from I2C bus */
uint8_t i2c_recv()
{
	i2c_idle();
	I2C1CONSET = 1 << 3; // RCEN = 1
	i2c_idle();
	I2C1STATCLR = 1 << 6; // I2COV = 0
	return I2C1RCV;
}

/* Send acknowledge conditon on the bus */
void i2c_ack()
{
	i2c_idle();
	I2C1CONCLR = 1 << 5; // ACKDT = 0
	I2C1CONSET = 1 << 4; // ACKEN = 1
}

/* Send not-acknowledge conditon on the bus */
void i2c_nack()
{
	i2c_idle();
	I2C1CONSET = 1 << 5; // ACKDT = 1
	I2C1CONSET = 1 << 4; // ACKEN = 1
}

/* Send start conditon on the bus */
void i2c_start()
{
	i2c_idle();
	I2C1CONSET = 1 << 0; // SEN
	i2c_idle();
}

/* Send restart conditon on the bus */
void i2c_restart()
{
	i2c_idle();
	I2C1CONSET = 1 << 1; // RSEN
	i2c_idle();
}

/* Send stop conditon on the bus */
void i2c_stop()
{
	i2c_idle();
	I2C1CONSET = 1 << 2; // PEN
	i2c_idle();
}
/* Convert 8.8 bit fixed point to string representation*/
char *fixed_to_string(uint32_t num, char *buf)
{
	bool neg = false;
	uint32_t n;
	char *tmp;

	if (num & 0x800000000) // makes it so it's signed
	{
		num = ~num + 1;
		neg = true;
	}

	buf += 4;
	// n = num >> 8;
	n = num >> 16;
	tmp = buf;
	do
	{
		*--tmp = (n % 10) + '0';
		n /= 10;
	} while (n);
	if (neg)
		*--tmp = '-';

	n = num;
	if (!(n & 0xFF))
	{
		*buf = 0;
		return tmp;
	}
	*buf++ = '.';
	while ((n &= 0xFF))
	{
		n *= 10;
		*buf++ = (n >> 8) + '0';
	}
	*buf = 0;

	return tmp;
}
uint32_t strlen1(char *str)
{
	uint32_t n = 0;
	while (*str++)
		n++;
	return n;
}

void getTemperature(void)
{

	uint16_t temp;
	char buf[32], *s, *t;
	/* Send start condition and address of the temperature sensor with
	write mode (lowest bit = 0) until the temperature sensor sends
	acknowledge condition */
	do
	{
		i2c_start();
	} while (!i2c_send(TEMP_SENSOR_ADDR << 1));
	/* Send register number we want to access */
	i2c_send(TEMP_SENSOR_REG_CONF);
	/* Set the config register to 0 */
	i2c_send(0x0);
	/* Send stop condition */
	i2c_stop();

	for (;;)
	{

		int buttons = getbtns();
		/* Send start condition and address of the temperature sensor with
		write flag (lowest bit = 0) until the temperature sensor sends
		acknowledge condition */
		do
		{
			i2c_start();
		} while (!i2c_send(TEMP_SENSOR_ADDR << 1));
		/* Send register number we want to access */
		i2c_send(TEMP_SENSOR_REG_TEMP);

		/* Now send another start condition and address of the temperature sensor with
		read mode (lowest bit = 1) until the temperature sensor sends
		acknowledge condition */
		do
		{
			i2c_start();
		} while (!i2c_send((TEMP_SENSOR_ADDR << 1) | 1));

		/* Now we can start receiving data from the sensor data register */
		temp = i2c_recv() << 8;
		i2c_ack();
		temp |= i2c_recv();
		/* To stop receiving, send nack and stop */
		i2c_nack();
		i2c_stop();
		// T(K) = T(°C) + 273.15
		if (kelvin == 1)
		{
			uint32_t fKelvin = 273.15 + temp;
			s = fixed_to_string(fKelvin, buf);
			// int lenght = sizeof(s) / sizeof(char);
			// float sFloat = 0;
			// for (int i = 0; i < lenght; i++)
			// {
			// 	sFloat = ((int)s[i] - 48) * pow(10, i);
			// }
			// float kelvinFloat = 273.15 + sFloat;

			t = s + strlen1(s);
			*t++ = ' ';
			*t++ = 7;
			*t++ = 'K'; // temperature unit
			*t++ = 0;
		}
		if (buttons & 1)
		{
			sTemp = 0;
			break;
		}
		display_string(1, s); // temperature string

		display_update();
		quicksleep(500000);
		//  delay(1000000);
	}
}

void menu(void)
{

	display_string(0, "Menu:");
	display_string(1, "Chose unit");
	display_string(2, "Measur. Type");
	display_string(3, "Display temperature");
	display_update();
}
void unit(void)
{
	int buttons = getbtns();
	int button1 = getbtn1();
	if (IFS(0) & 0x100)
	{ // if the 8th bit is 1 increment count
		tOutCount++;

		IFSCLR(0) = 0x00000100; // Clear the timer interrupt status flag
	}
	if (tOutCount == 10)
	{
		display_string(0, "Celcius");
		display_string(1, "Kelvin");
		display_string(2, "Farenheit");
		display_string(3, "Back to Menu");
		display_update();
		if (button1 & 0x200)
		{
			units = 0; // breaks out of the while loop in temperature
			menuPage = 1;
			menu(); // loads the main page to display
		}
		tOutCount = 0;
	}
}

void measurementType(void)
{
	int buttons = getbtns();
	int button1 = getbtn1();
	if (IFS(0) & 0x100)
	{ // if the 8th bit is 1 increment count
		tOutCount++;

		IFSCLR(0) = 0x00000100; // Clear the timer interrupt status flag
	}
	if (tOutCount == 10)
	{
		display_string(0, "Measur. Type:");
		display_string(1, "Continuous");
		display_string(2, "Time average");
		display_string(3, "Back to Menu");
		display_update();
		if (button1 & 0x200)
		{
			type = 0; // breaks out of the while loop in temperature
			menuPage = 1;
			menu(); // loads the menu page to display
		}
		tOutCount = 0;
	}
}

void showTemperature(void)
{
	int buttons = getbtns();
	int button1 = getbtn1();
	if (IFS(0) & 0x100)
	{ // if the 8th bit is 1 increment count
		tOutCount++;

		IFSCLR(0) = 0x00000100; // Clear the timer interrupt status flag
	}
	if (tOutCount == 10)
	{
		display_string(0, "Current temperature");
		display_string(2, "Back to menu");
		display_string(3, "");
		display_update();
		getTemperature(); // temperature on string 1
		if (sTemp == 0)
		{
			menuPage = 1;
			menu();
		}
		// if (button1 & 0x200)
		// {
		// 	menuPage = 1;
		// 	sTemp = 0;
		// 	menu();
		// }
		// if (sTemp == 1)
		// {
		// 	getTemperature();
		// }
		tOutCount = 0;
	}
}

/* This function is called repetitively from the main function */
void temperatureLoop(void)
{

	int buttons = getbtns();
	int button1 = getbtn1();

	if (IFS(0) & 0x100)
	{ // if the 8th bit is 1 increment count
		tOutCount++;

		IFSCLR(0) = 0x00000100; // Clear the timer interrupt status flag
	}
	if (tOutCount == 10)
	{
		if ((button1 & 0x200) && menuPage == 1)
		{
			sTemp = 1;
			menuPage = 0;
			while (sTemp == 1)
			{
				showTemperature();
			}
		}
		else if ((buttons & 2) && menuPage == 1) // button3
		{
			units = 1;
			menuPage = 0;
			while (units == 1)
			{
				unit();
			}
		}
		else if ((buttons & 1) && menuPage == 1) // button 2
		{
			type = 1;
			menuPage = 0;
			while (type == 1)
			{
				measurementType();
			}
		}

		tOutCount = 0;
	}
}

int main(void)
{
	uint16_t temp;
	char buf[32], *s, *t;
	/*
  This will set the peripheral bus clock to the same frequency
  as the sysclock. That means 80 MHz, when the microcontroller
  is running at 80 MHz. Changed 2017, as recommended by Axel.
*/
	SYSKEY = 0xAA996655; /* Unlock OSCCON, step 1 */
	SYSKEY = 0x556699AA; /* Unlock OSCCON, step 2 */
	while (OSCCON & (1 << 21))
		;				  /* Wait until PBDIV ready */
	OSCCONCLR = 0x180000; /* clear PBDIV bit <0,1> */
	while (OSCCON & (1 << 21))
		;		  /* Wait until PBDIV ready */
	SYSKEY = 0x0; /* Lock OSCCON */

	/* Set up output pins */
	AD1PCFG = 0xFFFF;
	ODCE = 0x0;
	TRISECLR = 0xFF;
	PORTE = 0x0;

	/* Output pins for display signals */
	PORTF = 0xFFFF;
	PORTG = (1 << 9);
	ODCF = 0x0;
	ODCG = 0x0;
	TRISFCLR = 0x70;
	TRISGCLR = 0x200;

	/* Set up input pins */
	TRISDSET = (1 << 8);
	TRISFSET = (1 << 1);

	/* Set up SPI as master */
	SPI2CON = 0;
	SPI2BRG = 4;
	/* SPI2STAT bit SPIROV = 0; */
	SPI2STATCLR = 0x40;
	/* SPI2CON bit CKP = 1; */
	SPI2CONSET = 0x40;
	/* SPI2CON bit MSTEN = 1; */
	SPI2CONSET = 0x20;
	/* SPI2CON bit ON = 1; */
	SPI2CONSET = 0x8000;

	/* Set up i2c */
	I2C1CON = 0x0;
	/* I2C Baud rate should be less than 400 kHz, is generated by dividing
	the 40 MHz peripheral bus clock down */
	I2C1BRG = 0x0C2;
	I2C1STAT = 0x0;
	I2C1CONSET = 1 << 13; // SIDL = 1
	I2C1CONSET = 1 << 15; // ON = 1
	temp = I2C1RCV;		  // Clear receive buffer

	// Introduction display
	display_init();
	display_string(0, "KTH/ICT ");
	display_string(1, "Project");
	display_string(2, "Group 38");
	display_string(3, "Temp. Sensor");
	display_update();

	/* Black screen before menu pops up */
	quicksleep(10000000);
	display_string(0, "");
	display_string(1, "");
	display_string(2, "");
	display_string(3, "");
	display_update();

	init(); /* Do any requiered initialization */
	menu();

	while (1)
	{
		temperatureLoop(); /* The main process */
	}
	return 0;
}
