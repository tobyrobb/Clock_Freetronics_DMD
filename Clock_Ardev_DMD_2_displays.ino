/*

Led Display clock based on my Arduino Dev board and a Freetronics DMD 

20 Sep 2014 Toby Robb *uses freetronics DMD display library

This is a clock sketch for the Arduino Development board running TWO DMD led displays

the dmd display connects to D9,10,11,12,13 and gnd

as per its subboard
 
There is also a temperature thermistor connected


NOTES:
         This clock has serial support enabling setting the time through a bluetooth serial board and a smart phone.
Look through the code for the commands or enter ! in the serial port @ 9600 baud to see commands displayed on clock.

*/

// Includes

#include <Time.h>  
#include <Wire.h>  
#include <DS1307RTC.h>  // a basic DS1307 library that returns time as a time_t

#include <SPI.h>        //SPI.h must be included as DMD is written by SPI (the IDE complains otherwise)
#include <DMD.h>        //
#include <TimerOne.h>   //
#include "SystemFont5x7.h"

// Defines (mostly as per the standard Arddev board)

#define ldrPin A2 // Light dependant resistor pin on board.
#define thermistorPin A3  //Temperature thermistor pin on board.
#define ledPin 8  // Led pin High for one colour Low for another
#define speakerPin 4  // The onboard speaker pin
#define relayPin 2 // Pin for the relay 
#define sparePin 13    //Spare pin breakout on board

#define dataPin A4  // The I2C bus DATA pin  
#define clockPin A5  // The I2C CLOCK pin

#define pot1Pin A0 // Number 1 potentiometer on the board.
#define pot2Pin A1 // Number 2 potentiometer on the board.

#define button1Pin 7  // Button 1 pin
#define button2Pin 12 // Button 2 pin

#define gpio1Pin 5     // General Purpose Input/output 1 pin
#define gpio2Pin 6    // General Purpose Input/output 2 pin
#define gpio3Pin 11    // General Purpose Input/output 3 pin

#define mosfet1Pin  3  // Mosfet 1 drive pin
#define mosfet2Pin  9  // Mosfet 2 drive pin
#define mosfet3Pin  10  // Mosfet 3 drive pin

unsigned int hourNow;  //Self explanatory
unsigned int minuteNow;
unsigned int secondNow;
unsigned int dayNow;
unsigned int monthNow;
unsigned int yearNow;
unsigned int weekdayNow;
unsigned int pollDelay = 5000;   // time between temperature samples in milliSeconds
unsigned int previousMillis; // A variable to hold the last millis time

int currentTemp = 0;
int incomingByte;      // A variable to read incoming serial data into
int brightness = 127;  // Set initial brightness
    
//Fire up the DMD library as dmd

#define DISPLAYS_ACROSS 2
#define DISPLAYS_DOWN 1
DMD dmd(DISPLAYS_ACROSS, DISPLAYS_DOWN);
               
void setup(){
  
  Serial.begin(9600);
  Serial.println("Beginning Setup");

  pinMode(speakerPin, OUTPUT);  // If the speaker is fitted.
  pinMode(mosfet1Pin, OUTPUT);  // If the mosfet is fitted.
  pinMode(thermistorPin, INPUT); //If the thermistor is fitted
  
  currentTemp = Thermistor(analogRead(thermistorPin));           // read the opening temperature

  Wire.begin();
   setSyncProvider(RTC.get);   // the function to get the time from the RTC
  if(timeStatus()!= timeSet) 
     Serial.println("Unable to sync with the RTC");
  else
     Serial.println("RTC has set the system time");      

//initialize TimerOne's interrupt/CPU usage used to scan and refresh the display
   Timer1.initialize( 5000 );           //period in microseconds to call ScanDMD. Anything longer than 5000 (5ms) and you can see flicker.
   Timer1.attachInterrupt( ScanDMD );   //attach the Timer1 interrupt to ScanDMD which goes to dmd.scanDisplayBySPI()

//clear/init the DMD pixels held in RAM
   dmd.clearScreen( true );   //true is normal (all pixels off), false is negative (all pixels on)
   dmd.selectFont(SystemFont5x7);  
   
//opening beep and display
  Serial.println("Beep");
  tone(speakerPin, 1000);          // begin tone at 1000 hertz
  delay(150);                      // wait half a sec
  noTone(speakerPin);              // end beep
  dmd.drawString(  11,  0, "Arduino", 7, GRAPHICS_NORMAL );
  dmd.drawString(  18,  9, "Clock" , 5, GRAPHICS_NORMAL );
  delay(3000);
  dmd.clearScreen( true );   //true is normal (all pixels off), false is negative (all pixels on)

}

void loop(){
    
// Better set the brightness of the display
analogWrite(mosfet1Pin, brightness);
    
// gather the data from the Real Time Clock 
   hourNow  = hour();
   minuteNow  = minute();
   secondNow  = second();
   dayNow  = day();
   monthNow  = month();
   yearNow  = year();
   weekdayNow = weekday();

// read the temperature every pollDelay interval 
   int currentMillis = millis();
   if((currentMillis - previousMillis) >= pollDelay){
   currentTemp = Thermistor(analogRead(thermistorPin));           // read ADC and convert it to Celsius
   previousMillis = millis();
  }

// Print out some stuff so we can see in the serial terminal everythings OK
   Serial.print(hourNow);
   Serial.print(":");
   Serial.print(minuteNow);
   Serial.print(" Day:");
   Serial.print(weekdayNow);
   Serial.print(" Date:");
   Serial.print(dayNow);
   Serial.print(":");
   Serial.print(monthNow);
   Serial.print(":");
   Serial.print(yearNow);
   Serial.print(" Temperature: ");
   Serial.print(currentTemp);
   Serial.print(" Brightness: ");
   Serial.println(brightness);
   
// check if its on the hour and beep the clock if you like
if((second() == 0) && (minute() == 0)){
  Serial.println("its right on the hour");
//beep the buzzer
Serial.println("Beep");
tone(speakerPin, 1200);          // begin tone at 1000 hertz
delay(150);                      // wait half a sec
noTone(speakerPin);              // end beep
delay(1000);
}
   
//  LETS SHOW ITEMS IN ORDER LISTED

ShowDisplayData(weekdayNow, dayNow, monthNow, yearNow, minuteNow, true, hourNow, currentTemp );  //Show clock
   
//Time to make available a bunch of serial commands.

// see if there's incoming serial data
  if (Serial.available() > 0) {
    // read the oldest byte in the serial buffer:
    incomingByte = Serial.read();
    Serial.print("Incoming byte  =   ");
  
//if so then lets check for commands and exectue scripts if we find them 

//beep the buzzer
  if (incomingByte == 'B') {
  Serial.println("Beep");
  tone(speakerPin, 1200);          // begin tone at 1000 hertz
  delay(150);                      // wait half a sec
  noTone(speakerPin);              // end beep
  delay(1000);
}

//Increase the hour
  if (incomingByte == 'H') {
  hourNow++;
  if(hourNow  >=23){hourNow = 23;}
  setTime(hourNow,minuteNow,secondNow,dayNow,monthNow,yearNow);
  RTC.set(now());
  Serial.println("Increasing hour");
  delay(100);
}

//Decrease the hour
  if (incomingByte == 'h') {
  hourNow--;
  if(hourNow <=0){hourNow = 0;}
  setTime(hourNow,minuteNow,secondNow,dayNow,monthNow,yearNow);
  RTC.set(now());
  Serial.println("Decreasing hour");
  delay(100);
}

//Increase the minute
  if (incomingByte == 'M') {
  minuteNow++;
  if(minuteNow >=59){minuteNow = 59;}
  setTime(hourNow,minuteNow,secondNow,dayNow,monthNow,yearNow);
  RTC.set(now());
  Serial.println("Increasing minute");
  delay(100);
}

//decrease the minute
  if (incomingByte == 'm') {
  minuteNow--;
  setTime(hourNow,minuteNow,secondNow,dayNow,monthNow,yearNow);
  RTC.set(now());
  Serial.println("Decreasing minute");
  delay(100);
}

// Increase the day
  if (incomingByte == 'D') {
  dayNow++;
  if(dayNow >=31){dayNow = 0;}
  setTime(hourNow,minuteNow,secondNow,dayNow,monthNow,yearNow);
  RTC.set(now());
  Serial.println("Increasing Day");
  delay(100);
}

// Decrease the day
  if (incomingByte == 'd') {
  dayNow--;
  if(dayNow <=0){dayNow = 31;}
  setTime(hourNow,minuteNow,secondNow,dayNow,monthNow,yearNow);
  RTC.set(now());
  Serial.println("Decreasing Day");
  delay(100);
}

// Increase the month
  if (incomingByte == 'N') {
  monthNow++;
  if(monthNow >=12){monthNow = 12;}
  setTime(hourNow,minuteNow,secondNow,dayNow,monthNow,yearNow);
  RTC.set(now());
  Serial.println("Increasing Month");
  delay(100);
}

// Decrease the month
  if (incomingByte == 'n') {
  monthNow--;
  if(monthNow <=1){monthNow = 1;}
  setTime(hourNow,minuteNow,secondNow,dayNow,monthNow,yearNow);
  RTC.set(now());
  Serial.println("Decreasing Month");
  delay(100);
}

// Increase year
  if (incomingByte == 'Y') {
  yearNow++;
  if(yearNow >=2050){yearNow = 2050;}
  setTime(hourNow,minuteNow,secondNow,dayNow,monthNow,yearNow);
  RTC.set(now());
  Serial.println("Increasing Year");
  delay(100);
}

// Decrease year
  if (incomingByte == 'y') {
  yearNow--;
  if(yearNow <= 2000){yearNow = 2000;}
  setTime(hourNow,minuteNow,secondNow,dayNow,monthNow,yearNow);
  RTC.set(now());
  Serial.println("Decreasing Year");
  delay(100);
}

// increase brightness
  if (incomingByte == '+') {
  brightness = brightness + 8;
  analogWrite(mosfet1Pin, brightness);
    if(brightness >=255){ 
    brightness = 255;
   }
  Serial.println("Decreasing Brightness");
}

// decrease brightness
  if (incomingByte == '-') {
  brightness = brightness - 8;
  analogWrite(mosfet1Pin, brightness);
    if(brightness <= 0){ 
    brightness = 0;
   }
  Serial.println("Decreasing Brightness");
}

// Show the Info page 1
  if (incomingByte == 'I') {
   dmd.clearScreen( true );
   delay(1000);
   dmd.drawMarquee("  This is an Arduino based clock using the Freetronics DMD LED display",70,(32*DISPLAYS_ACROSS)-1,4);
   long start=millis();
   long timer=start;
   boolean ret=false;
   while(!ret){
     if ((timer+35) < millis()) {
       ret=dmd.stepMarquee(-1,0);
       timer=millis();
     }
   }
  delay(3000); 
  dmd.clearScreen( true );
 }

// Show the Info page 2
if (incomingByte == 'i') {  
  dmd.clearScreen( true );
   delay(1000);  
   dmd.drawMarquee("For information, code and to build your own check out www.tobyrobb.com",70,(32*DISPLAYS_ACROSS)-1,4);
   long start=millis();
   long timer=start;
   boolean ret=false;
   while(!ret){
     if ((timer+35) < millis()) {
       ret=dmd.stepMarquee(-1,0);
       timer=millis();
     }
   }
  delay(3000); 
  dmd.clearScreen( true );
 }

// Show the help page
  if (incomingByte == '!') {
  showHelp();
  delay(3000); 
  dmd.clearScreen( true );
 }

}
}



// all the functions begin here

void ScanDMD()
{ 
  dmd.scanDisplayBySPI();
}

/*--------------------------------------------------------------------------------------
  Show clock numerals on the screen from a 4 digit time value, and select whether the
  flashing colon is on or off, show temperature, show day of week
--------------------------------------------------------------------------------------*/
void ShowDisplayData( unsigned int uiWeekday,unsigned int uiDay, unsigned int uiMonth, unsigned int uiYear, unsigned int uiMinute, byte bColonOn, unsigned int uiHour,  unsigned int uiTemperature )
{  //weekdayNow, dayNow, monthNow, minuteNow, true, hourNow, currentTemp
  
 //  dmd.clearScreen( true );

 // Show day of week
   switch(uiWeekday){
     case 1:
        dmd.drawString(  1,  0, "Sun", 3, GRAPHICS_NORMAL );
        
        break;
     case 2:
        dmd.drawString(  1,  0, "Mon", 3, GRAPHICS_NORMAL );
        break;
     case 3:
        dmd.drawString(  1,  0, "Tue", 3, GRAPHICS_NORMAL );
        break;
     case 4:
        dmd.drawString(  1,  0, "Wed", 3, GRAPHICS_NORMAL );
        break;
     case 5:
        dmd.drawString(  1,  0, "Thu", 3, GRAPHICS_NORMAL );
        break;
     case 6:
        dmd.drawString(  1,  0, "Fri", 3, GRAPHICS_NORMAL );
        break;
     case 7:
        dmd.drawString(  1,  0, "Sat", 3, GRAPHICS_NORMAL );
        break;
  }
 
  //draw the day 
   dmd.drawChar(  20,  0, '0'+((uiDay%100)  /10),   GRAPHICS_NORMAL );      // first digit
   dmd.drawChar(  26,  0, '0'+ (uiDay%10),          GRAPHICS_NORMAL );   // second digit

 //draw the month
   switch(uiMonth){
     case 1:
        dmd.drawString(  33,  0, "Jan", 3, GRAPHICS_NORMAL );
        break;
     case 2:
        dmd.drawString(  33,  0, "Feb", 3, GRAPHICS_NORMAL );
        break;
      case 3:
        dmd.drawString(  33,  0, "Mar", 3, GRAPHICS_NORMAL );
        break;
      case 4:
        dmd.drawString(  33,  0, "Apr", 3, GRAPHICS_NORMAL );
        break;
      case 5:
        dmd.drawString(  33,  0, "May", 3, GRAPHICS_NORMAL );
        break;
      case 6:
        dmd.drawString(  33,  0, "Jun", 3, GRAPHICS_NORMAL );
        break;
      case 7:
        dmd.drawString(  33,  0, "Jul", 3, GRAPHICS_NORMAL );
        break;
      case 8:
        dmd.drawString(  33,  0, "Aug", 3, GRAPHICS_NORMAL );
        break;
      case 9:
        dmd.drawString(  33,  0, "Sep", 3, GRAPHICS_NORMAL );
        break;
      case 10:
        dmd.drawString(  33,  0, "Oct", 3, GRAPHICS_NORMAL );
        break;
      case 11:
        dmd.drawString(  33,  0, "Nov", 3, GRAPHICS_NORMAL );
        break;
      case 12:
        dmd.drawString(  33,  0, "Dec", 3, GRAPHICS_NORMAL );
        break;
 }
 
//draw the year
   dmd.drawChar(  52,  0, '0'+((uiYear%100)  /10),   GRAPHICS_NORMAL );      // first digit
   dmd.drawChar(  58,  0, '0'+ (uiYear%10),          GRAPHICS_NORMAL );   // second digit
   
//draw the time
   dmd.selectFont(System5x7);
   dmd.drawChar(  2,  9,'0'+((uiHour%100)/10), GRAPHICS_NORMAL );   // first digit
   dmd.drawChar(  8,  9, '0'+ (uiHour%10),  GRAPHICS_NORMAL );   // second digit
   dmd.drawChar(  14,  9,'.', GRAPHICS_NORMAL );   // colon
   dmd.drawChar( 19,  9, '0'+((uiMinute%100)  /10),   GRAPHICS_NORMAL );      // third digit
   dmd.drawChar( 25,  9, '0'+ (uiMinute%10),          GRAPHICS_NORMAL );   // fourth digit
   
// now show temperature
   dmd.drawChar(  33,  9,'0'+((uiTemperature%100)/10), GRAPHICS_NORMAL );   // first digit
   dmd.drawChar(  39,  9, '0'+ (uiTemperature%10),  GRAPHICS_NORMAL );   // second digit
   dmd.drawString(  46, 9, "Deg", 3, GRAPHICS_NORMAL );  // show the word degreees
}

//Display the help screen
void showHelp(){
   dmd.clearScreen( true );
   tone(speakerPin, 1200);          // begin tone at 1000 hertz
   delay(150);                      // wait half a sec
   noTone(speakerPin);              // end beep
   dmd.selectFont(System5x7);
   dmd.drawString(  0,  0, "Hh Mm Dd Nn", 11, GRAPHICS_NORMAL );
   dmd.drawString(  0,  9, "Yy B  +- Ii" ,11, GRAPHICS_NORMAL );
   delay(1000);
}  

//Conversion code for the thermistor

double Thermistor(int RawADC) {
 // Inputs ADC Value from Thermistor and outputs Temperature in Celsius
 //  requires: include <math.h>
 // Utilizes the Steinhart-Hart Thermistor Equation:
 //    Temperature in Kelvin = 1 / {A + B[ln(R)] + C[ln(R)]^3}
 //    where A = 0.001129148, B = 0.000234125 and C = 8.76741E-08
 long Resistance;  double Temp;  // Dual-Purpose variable to save space.
 Resistance=((10240000/RawADC) - 10000);  // Assuming a 10k Thermistor.  Calculation is actually: Resistance = (1024 * BalanceResistor/ADC) - BalanceResistor
 Temp = log(Resistance); // Saving the Log(resistance) so not to calculate it 4 times later. // "Temp" means "Temporary" on this line.
 Temp = 1 / (0.001129148 + (0.000234125 * Temp) + (0.0000000876741 * Temp * Temp * Temp));   // Now it means both "Temporary" and "Temperature"
 Temp = Temp - 273.15;  // Convert Kelvin to Celsius                                         // Now it only means "Temperature"

// // BEGIN- Remove these lines for the function not to display anything
//  Serial.print("ADC: "); Serial.print(RawADC); Serial.print("/1024");  // Print out RAW ADC Number
//  Serial.print(", Volts: "); printDouble(((RawADC*5)/1024.0),3);   // 4.860 volts is what my USB Port outputs.
//  Serial.print(", Resistance: "); Serial.print(Resistance); Serial.print("ohms");
// // END- Remove these lines for the function not to display anything

 // Uncomment this line for the function to return Fahrenheit instead.
 //Temp = (Temp * 9.0)/ 5.0 + 32.0; // Convert to Fahrenheit
 return Temp;  // Return the Temperature
}
