/*

Arduino Dev board

26 Dec 2013

Toby Robb

This is a test sketch for the Arduino Development board running the DMD led display

the dmd display connects to D9,10,11,12,13 and gnd

as per its subboard
 


NOTES:

You must enable the internal pullups for the buttons by setting as inputs then writing HIGH
The leds if fitted also require the pullups to be enabled

*/

// Includes

#include <Servo.h>   // Include the servo library
#include <Time.h>  
#include <Wire.h>  
#include <DS1307RTC.h>  // a basic DS1307 library that returns time as a time_t

#include <SPI.h>        //SPI.h must be included as DMD is written by SPI (the IDE complains otherwise)
#include <DMD.h>        //
#include <TimerOne.h>   //
#include "SystemFont5x7.h"



// Defines

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

unsigned int hourNow;
unsigned int minuteNow;
unsigned int secondNow;
unsigned int dayNow;
unsigned int monthNow;
unsigned int yearNow;

int currentTemp = 0;
int incomingByte;      // a variable to read incoming serial data into
int brightness = 127;
    
//Fire up the DMD library as dmd
//Fire up the DMD library as dmd
#define DISPLAYS_ACROSS 1
#define DISPLAYS_DOWN 1
DMD dmd(DISPLAYS_ACROSS, DISPLAYS_DOWN);
               
void setup(){
  
// Setup the serial

  Serial.begin(9600);
  Serial.println("Beginning Setup");

  pinMode(speakerPin, OUTPUT);  // If the speaker is fitted.
  pinMode(mosfet1Pin, OUTPUT);  // If the mosfet is fitted.
  
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
   
   //beep the buzzer
Serial.println("Beep");
tone(speakerPin, 1000);          // begin tone at 1000 hertz
delay(150);                      // wait half a sec
noTone(speakerPin);              // end beep

// print the temperature after the beep

Serial.print("The Temperature is : ");
currentTemp = Thermistor(analogRead(thermistorPin));           // read ADC and convert it to Celsius
Serial.println(currentTemp);
Serial.println("Printing temperature to the display");
//temperatureOnes = (currentTemp%10);
//temperatureTens = ((currentTemp/10)%10);
showTemperature();
delay(3000);
   dmd.clearScreen( true );

}

void loop(){
  

    analogWrite(mosfet1Pin, brightness);
    
    Serial.print(year(), DEC);
    Serial.print('/');
    Serial.print(month(), DEC);
    Serial.print('/');
    Serial.print(day(), DEC);
    Serial.print(' ');
    Serial.print(hour(), DEC);
    Serial.print(':');
    Serial.print(minute(), DEC);
    Serial.print(':');
    Serial.print(second(), DEC);
    Serial.println();

// check if its on the hour

if((second() == 0) && (minute() == 0)){
  Serial.println("its right on the hour");
//beep the buzzer
Serial.println("Beep");
tone(speakerPin, 1200);          // begin tone at 1000 hertz
delay(150);                      // wait half a sec
noTone(speakerPin);              // end beep
delay(1000);

// print the temperature 10 seconds after the beep

Serial.print("The Temperature is : ");
currentTemp = Thermistor(analogRead(thermistorPin));           // read ADC and convert it to Celsius
Serial.println(currentTemp);
Serial.println("Printing temperature to the display");
//temperatureOnes = (currentTemp%10);
//temperatureTens = ((currentTemp/10)%10);
showTemperature();
delay(8000);
dmd.clearScreen( true );

//beep the buzzer
Serial.println("Beep");
tone(speakerPin, 1200);          // begin tone at 1000 hertz
delay(150);                      // wait half a sec
noTone(speakerPin);              // end beep
delay(1000);
}

   hourNow  = hour();
   minuteNow  = minute();
   secondNow  = second();
   dayNow  = day();
   monthNow  = month();
   yearNow  = year();

  
   Serial.println(hourNow);
   Serial.println(minuteNow);
   Serial.println(dayNow);
   Serial.println(monthNow);

   ShowClockNumbers( hourNow, minuteNow, dayNow, monthNow, true );



// see if there's incoming serial data
  if (Serial.available() > 0) {
    // read the oldest byte in the serial buffer:
    incomingByte = Serial.read();
    Serial.print("Incoming byte  =   ");
  
  
if (incomingByte == 'b') {
     //beep the buzzer
Serial.println("Beep");
tone(speakerPin, 1200);          // begin tone at 1000 hertz
delay(150);                      // wait half a sec
noTone(speakerPin);              // end beep
delay(1000);
}

if (incomingByte == 't') {
Serial.print("The Temperature is : ");
currentTemp = Thermistor(analogRead(thermistorPin));           // read ADC and convert it to Celsius
Serial.println(currentTemp);
Serial.println("Printing temperature to the display");
//temperatureOnes = (currentTemp%10);
//temperatureTens = ((currentTemp/10)%10);
showTemperature();
  //beep the buzzer
Serial.println("Beep");
tone(speakerPin, 1200);          // begin tone at 1000 hertz
delay(150);                      // wait half a sec
noTone(speakerPin);              // end beep
delay(8000); 
dmd.clearScreen( true );
   }

   if (incomingByte == 'h') {
  hourNow++;
  if(hourNow  >=24){hourNow = 0;}
  setTime(hourNow,minuteNow,secondNow,dayNow,monthNow,yearNow);
  RTC.set(now());
  Serial.println("Increasing hour");
  delay(100);


}
  
    if (incomingByte == 'm') {
       minuteNow++;
  if(minuteNow >=60){minuteNow = 0;}
 setTime(hourNow,minuteNow,secondNow,dayNow,monthNow,yearNow);
  RTC.set(now());
  Serial.println("Increasing minute");
  delay(100);
}

  if (incomingByte == '>') {
       dayNow++;
  if(dayNow >=31){
                  monthNow++;
                  dayNow = 0;}
  setTime(hourNow,minuteNow,secondNow,dayNow,monthNow,yearNow);
  RTC.set(now());
  Serial.println("Increasing Day");
  delay(100);
}

if (incomingByte == '<') {
       dayNow--;
  if(dayNow <=0){
                  monthNow--;
                  dayNow = 31;}
  setTime(hourNow,minuteNow,secondNow,dayNow,monthNow,yearNow);
  RTC.set(now());
  Serial.println("Increasing Day");
  delay(100);
}

if (incomingByte == '+') {
   //test dimming
    brightness = brightness + 8;
    analogWrite(mosfet1Pin, brightness);
      if(brightness >=255){ 
     brightness = 0;
   }
}
if (incomingByte == '-') {
   //test dimming
    brightness = brightness - 8;
    analogWrite(mosfet1Pin, brightness);
  
   if(brightness <= 0){ 
     brightness = 255;
   }
}

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
  flashing colon is on or off
--------------------------------------------------------------------------------------*/
void ShowClockNumbers( unsigned int uiHour,unsigned int uiMinute, unsigned int uiDay, unsigned int uiMonth, byte bColonOn )
{
 //  dmd.clearScreen( true );
   dmd.selectFont(System5x7);

   dmd.drawChar(  1,  0,'0'+((uiHour%100)/10), GRAPHICS_NORMAL );   // first digit
   dmd.drawChar(  8,  0, '0'+ (uiHour%10),  GRAPHICS_NORMAL );   // second digit
   dmd.drawChar(  14,  0,'.', GRAPHICS_NORMAL );   // colon
   dmd.drawChar( 19,  0, '0'+((uiMinute%100)  /10),   GRAPHICS_NORMAL );      // third digit
   dmd.drawChar( 27,  0, '0'+ (uiMinute%10),          GRAPHICS_NORMAL );   // fourth digit
   dmd.drawChar(  1,  9, '0'+((uiDay%100)  /10),   GRAPHICS_NORMAL );      // first digit
   dmd.drawChar(  8,  9, '0'+ (uiDay%10),          GRAPHICS_NORMAL );   // second digit
   
   switch(uiMonth){
     case 1:
        dmd.drawString(  15,  9, "Jan", 5, GRAPHICS_NORMAL );
        break;
     case 2:
        dmd.drawString(  15,  9, "Feb", 5, GRAPHICS_NORMAL );
        break;
      case 3:
        dmd.drawString(  15,  9, "Mar", 5, GRAPHICS_NORMAL );
        break;
      case 4:
        dmd.drawString(  15,  9, "Apr", 5, GRAPHICS_NORMAL );
        break;
      case 5:
        dmd.drawString(  15,  9, "May", 5, GRAPHICS_NORMAL );
        break;
      case 6:
        dmd.drawString(  15,  9, "Jun", 5, GRAPHICS_NORMAL );
        break;
      case 7:
        dmd.drawString(  15,  9, "Jul", 5, GRAPHICS_NORMAL );
        break;
      case 8:
        dmd.drawString(  15,  9, "Aug", 5, GRAPHICS_NORMAL );
        break;
      case 9:
        dmd.drawString(  15,  9, "Sep", 5, GRAPHICS_NORMAL );
        break;
      case 10:
        dmd.drawString(  15,  9, "Oct", 5, GRAPHICS_NORMAL );
        break;
      case 11:
        dmd.drawString(  15,  9, "Nov", 5, GRAPHICS_NORMAL );
        break;
      case 12:
        dmd.drawString(  15,  9, "Dec", 5, GRAPHICS_NORMAL );
        break;
   
        
 }
     
}

void showTemperature(){
  
   dmd.clearScreen( true );
   dmd.selectFont(System5x7);

   dmd.drawChar(  4,  4,'0'+((currentTemp%100)/10), GRAPHICS_NORMAL );   // first digit
   dmd.drawChar(  11,  4, '0'+ (currentTemp%10),  GRAPHICS_NORMAL );   // second digit
   dmd.drawString(  23,  4, "C    ", 5, GRAPHICS_NORMAL );
   
}  

void showHelp(){
   dmd.clearScreen( true );
   dmd.selectFont(System5x7);
   dmd.drawString(  1,  0, "bthm!", 5, GRAPHICS_NORMAL );
   dmd.drawString(  1,  9, "<>+-" , 5, GRAPHICS_NORMAL );

}  
  
  
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
