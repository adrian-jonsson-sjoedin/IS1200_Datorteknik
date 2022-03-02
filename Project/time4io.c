#include <stdint.h>
#include <pic32mx.h>
#include "mipslab.h"

// returns the status of the toggle switches
int getsw(void)
{
  // Shift right five bits the get the data from the switches on the 4 lsb
  // then we mask the rest
  return (PORTD >> 8) & 0x000f;
}
// returns the values of the buttons 2-4
int getbtns(void)
{
  return (PORTD >> 5) & 0x0007;
}
// returns the value of button 1
int getbtn1()
{
  return ((PORTF >> 1) & 0x1) << 9;
}