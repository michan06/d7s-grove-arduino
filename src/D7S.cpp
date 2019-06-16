#include "D7S.h"

D7SClass::D7SClass()
{
   //reset handler array
   for (int i = 0; i < 4; i++)
   {
      _handlers[i] = NULL;
   }

   //reset events variable
   _events = 0;
}

//used to initialize Wire
void D7SClass::begin()
{
   WireD7S.begin();
}

//return the currect state
d7s_status D7SClass::getState()
{
   return (d7s_status)(read8bit(0x10, 0x00) & 0x07);
}

//change the axis selection mode
void D7SClass::setAxis(d7s_axis_settings axisMode)
{
   //check if axisMode is valid
   if (axisMode < 0 or axisMode > 4)
   {
      return;
   }
   //read the CTRL register at 0x1004
   uint8_t reg = read8bit(0x10, 0x04);
   //new register value with the threshold
   reg = (axisMode << 4) | (reg & 0x0F);
   //update register
   write8bit(0x10, 0x04, reg);
}

//get instantaneus SI (during an earthquake) [cm/s]
float D7SClass::getInstantaneusSI()
{
   // ref No. 9915800-4B http://akizukidenshi.com/download/ds/omron/D7S-A0001_a.pdf
   // P17 main_si 0x0000～0xFFFF (0.0～6553.5)*小数点以下第1位固定 SI値 kine
   return ((float)read16bit(0x20, 0x00)/10.0);
}

//get instantaneus PGA (during an earthquake) [cm/s^2]
uint16_t D7SClass::getInstantaneusPGA()
{
   // ref No. 9915800-4B http://akizukidenshi.com/download/ds/omron/D7S-A0001_a.pdf
   // P17 main_pga PGA(2軸合成最大加速度) 0x0000～0xFFFF (0～65535)*整数固定 gal
   return ((uint16_t)read16bit(0x20, 0x02));
}

//initialize the d7s (start the initial installation mode)
void D7SClass::initialize()
{
   Serial.printf("current mode:%d\n",read8bit(0x10, 0x03));
   //write INITIAL INSTALLATION MODE command
   write8bit(0x10, 0x03, 0x02);
   // return(read8bit(0x10, 0x03));
   Serial.printf("current mode:%d\n",read8bit(0x10, 0x03));
}

//after each earthquakes it's important to reset the events calling resetEvents() to prevent polluting the new data with the old one
//return true if the collapse condition is met (it's the sencond bit of _events)
uint8_t D7SClass::isInCollapse()
{
   //updating the _events variable
   readEvents();
   //return the second bit of _events
   return (_events & 0x02) >> 1;
}

//return true if the shutoff condition is met (it's the first bit of _events)
uint8_t D7SClass::isInShutoff()
{
   //updating the _events variable
   readEvents();
   //return the second bit of _events
   return _events & 0x01;
}

//reset shutoff/collapse events
void D7SClass::resetEvents()
{
   //reset the EVENT register (read to zero-ing it)
   read8bit(0x10, 0x02);
   //reset the events variable
   _events = 0;
}

//return true if an earthquake is occuring
uint8_t D7SClass::isEarthquakeOccuring()
{
   //if D7S is in NORMAL MODE NOT IN STANBY (after the first 4 sec to initial delay) there is an earthquake
   return getState() == NORMAL_MODE_NOT_IN_STANBY;
}

uint8_t D7SClass::isReady()
{
   // delay(delayWait);
   return getState() == NORMAL_MODE;
}

//read 8 bit from the specified register
uint8_t D7SClass::read8bit(uint8_t regH, uint8_t regL)
{
   // delay(delayWait);
   //setting up i2c connection
   WireD7S.beginTransmission(D7S_ADDRESS);

   // delay(delayWait);
   //write register address
   WireD7S.write(regH);
   delay(delayWait);
   WireD7S.write(regL);
   delay(delayWait);

   //send RE-START message
   uint8_t status = WireD7S.endTransmission(false);

   //if the status != 0 there is an error
   if (status != 0)
   {
      //retry
      return read8bit(regH, regL);
   }

   // delay(delayWait);
   //request 1 byte
   WireD7S.requestFrom(D7S_ADDRESS, 1);
   //wait until the data is received
   while (WireD7S.available() < 1)
      ;
   //read the data
   uint8_t data = WireD7S.read();

   //return the data
   return data;
}

//read 16 bit from the specified register
uint16_t D7SClass::read16bit(uint8_t regH, uint8_t regL)
{

   delay(delayWait);
   //setting up i2c connection
   WireD7S.beginTransmission(D7S_ADDRESS);

   delay(delayWait);
   //write register address
   WireD7S.write(regH);
   delay(delayWait);
   WireD7S.write(regL);
   delay(delayWait);

   //send RE-START message
   uint8_t status = WireD7S.endTransmission(false);

   delay(delayWait);
   //if the status != 0 there is an error
   if (status != 0)
   {
      //retry again
      return read16bit(regH, regL);
   }

   delay(delayWait);
   //request 2 byte
   WireD7S.requestFrom(D7S_ADDRESS, 2);
   //wait until the data is received
   while (WireD7S.available() < 2)
      ;

   delay(delayWait);
   uint8_t msb = WireD7S.read();
   delay(delayWait);
   uint8_t lsb = WireD7S.read();

   //return the data
   return (msb << 8) | lsb;
}

//write 8 bit to the register specified
void D7SClass::write8bit(uint8_t regH, uint8_t regL, uint8_t val)
{

   delay(delayWait);
   //setting up i2c connection
   WireD7S.beginTransmission(D7S_ADDRESS);

   delay(delayWait);
   //write register address
   WireD7S.write(regH);
   delay(delayWait);
   WireD7S.write(regL);
   delay(delayWait);

   //write data
   WireD7S.write(val);
   delay(delayWait); //delay to prevent freezing
   //closing the connection (STOP message)
   uint8_t status = WireD7S.endTransmission(true);
}

//read the event (SHUTOFF/COLLAPSE) from the EVENT register
void D7SClass::readEvents()
{
   delay(delayWait);
   //read the EVENT register at 0x1002 and obtaining only the first two bits
   uint8_t events = read8bit(0x10, 0x02) & 0x03;
   //updating the _events variable
   _events |= events;
   delay(delayWait);
}

//extern object
D7SClass D7S;
