/*

Led Display clock based on a standard Arduino board, clock shield and a Freetronics DMD 

20 Sep 2014 Toby Robb *uses freetronics DMD display library

This is a clock sketch for the Arduino Development board running TWO DMD led displays

the dmd display connects to D9,10,11,12,13 and gnd

as per its subboard
 
There is also a temperature thermistor connected and a mosfet for dimming


NOTES:
         This clock has mySerial support enabling setting the time through a bluetooth mySerial board and a smart phone.
Look through the code for the commands or enter ! in the mySerial port @ 9600 baud to see commands displayed on clock.

TODO:

Add physical button support
Turn different modes on and off through serial
Add support to send new messages to be scrolled via the serial port
Add holiday messages
Add seasons display
Add daylight savings support
Add rising or cooling temperature display
add a word clock mode eg Seven Forty Four PM

*/

// Includes

#include <Time.h>  
#include <Wire.h>  
#include <DS1307RTC.h>  // a basic DS1307 library that returns time as a time_t
#include <SoftwareSerial.h>

#include <SPI.h>        //SPI.h must be included as DMD is written by SPI (the IDE complains otherwise)
#include <DMD.h>        //
#include <TimerOne.h>   //
#include "SystemFont5x7.h"

// Defines (mostly as per the standard Arddev board)

#define speakerPin 4  // The onboard speaker pin
#define mosfet1Pin  5  // Mosfet 1 drive pin must be a PWM port

#define thermistorPin A3  //Temperature thermistor pin on board.
#define dataPin A4  // The I2C bus DATA pin  
#define clockPin A5  // The I2C CLOCK pin


 int hourNow;  //Self explanatory
 int minuteNow;
 int secondNow;
 int dayNow;
 int monthNow;
 int yearNow;
 int weekdayNow;
 int pollDelay = 3000;   // time between temperature samples in milliSeconds
 int previousMillis; // A variable to hold the last millis time
 int mode = 4; // a variable to hold the mode

int currentTemp = 0;
int incomingByte;      // A variable to read incoming mySerial data into
int brightness = 255;  // Set initial brightness, lower number is brighter with P channel mosfets
    
SoftwareSerial mySerial(2, 3); // RX, TX   // setup a software serial port

//Fire up the DMD library as dmd

#define DISPLAYS_ACROSS 2
#define DISPLAYS_DOWN 1
DMD dmd(DISPLAYS_ACROSS, DISPLAYS_DOWN);
               
void setup(){
  
  mySerial.begin(9600);
  mySerial.println("Beginning Setup");

  pinMode(speakerPin, OUTPUT);  // If the speaker is fitted.
  pinMode(mosfet1Pin, OUTPUT);  // If the mosfet is fitted.
  
  pinMode(thermistorPin, INPUT); //If the thermistor is fitted

  analogWrite(mosfet1Pin, brightness);  //Set the boot up brightness

  currentTemp = Thermistor(analogRead(thermistorPin));           // read the opening temperature

  Wire.begin();
   setSyncProvider(RTC.get);   // the function to get the time from the RTC
  if(timeStatus()!= timeSet) 
     mySerial.println("Unable to sync with the RTC");
  else
     mySerial.println("RTC has set the system time");      

//initialize TimerOne's interrupt/CPU usage used to scan and refresh the display
   Timer1.initialize( 5000 );           //period in microseconds to call ScanDMD. Anything longer than 5000 (5ms) and you can see flicker.
   Timer1.attachInterrupt( ScanDMD );   //attach the Timer1 interrupt to ScanDMD which goes to dmd.scanDisplayBySPI()

//clear/init the DMD pixels held in RAM
   dmd.clearScreen( true );   //true is normal (all pixels off), false is negative (all pixels on)
   dmd.selectFont(SystemFont5x7);  
   
//opening beep and display
  mySerial.println("Beep");
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

//  every pollDelay interval read the temperature and print the time to serial
   int currentMillis = millis();
   if((currentMillis - previousMillis) >= pollDelay){
   currentTemp = Thermistor(analogRead(thermistorPin));           // read ADC and convert it to Celsius
   previousMillis = millis();
   // Print out some stuff so we can see in the mySerial terminal everythings OK
   mySerial.print(hourNow);
   mySerial.print(":");
   mySerial.print(minuteNow);
   mySerial.print(" Day:");
   mySerial.print(weekdayNow);
   mySerial.print(" Date:");
   mySerial.print(dayNow);
   mySerial.print(":");
   mySerial.print(monthNow);
   mySerial.print(":");
   mySerial.print(yearNow);
   mySerial.print(" Temperature: ");
   mySerial.print(currentTemp);
   mySerial.print(" Brightness: ");
   mySerial.println(brightness);
  }


   
// check if its on the hour and beep the clock if you like
if((second() == 0) && (minute() == 0)){
  mySerial.println("its right on the hour");
//beep the buzzer
mySerial.println("Beep");
tone(speakerPin, 1200);          // begin tone at 1200 hertz
delay(150);                      // wait half a sec
noTone(speakerPin);              // end beep
delay(1000);
}
   
//  LETS SHOW ITEMS IN ORDER LISTED

ShowDisplayData(mode);  //Show clock
   
//Time to make available a bunch of mySerial commands.

// see if there's incoming mySerial data
  if (mySerial.available() > 0) {
    // read the oldest byte in the mySerial buffer:
    incomingByte = mySerial.read();
    mySerial.print("Incoming byte  =   ");
  
//if so then lets check for commands and exectue scripts if we find them 

//change modes
  if (incomingByte == '1') {
      mode = 1;
      mySerial.print("Mode: ");
      mySerial.println(mode);
      dmd.clearScreen( true );   //true is normal (all pixels off), false is negative (all pixels on)
  }
  
  if (incomingByte == '2') {
      mode = 2;
      mySerial.print("Mode: ");
      mySerial.println(mode);
      dmd.clearScreen( true );   //true is normal (all pixels off), false is negative (all pixels on)
  }
  
  if (incomingByte == '3') {
      mode = 3;
      mySerial.print("Mode: ");
      mySerial.println(mode);
      dmd.clearScreen( true );   //true is normal (all pixels off), false is negative (all pixels on)
  }
  
  if (incomingByte == '4') {
      mode = 4;
      mySerial.print("Mode: ");
      mySerial.println(mode);
      dmd.clearScreen( true );   //true is normal (all pixels off), false is negative (all pixels on)
  }
  if (incomingByte == '5') {
      mode = 5;
      mySerial.print("Mode: ");
      mySerial.println(mode);
      dmd.clearScreen( true );   //true is normal (all pixels off), false is negative (all pixels on)
  }
  
//beep the buzzer
  if (incomingByte == 'B') {
  mySerial.println("Beep");
  tone(speakerPin, 1200);          // begin tone at 1000 hertz
  delay(150);                      // wait half a sec
  noTone(speakerPin);              // end beep
  delay(1000);
}

//Increase the hour
  if (incomingByte == 'H') {
  hourNow++;
  if(hourNow  > 23){hourNow = 0;}
  setTime(hourNow,minuteNow,secondNow,dayNow,monthNow,yearNow);
  RTC.set(now());
  mySerial.println("Increasing hour");
  delay(100);
}

//Decrease the hour
  if (incomingByte == 'h') {
  hourNow--;
  if(hourNow < 0){hourNow = 23;}
  setTime(hourNow,minuteNow,secondNow,dayNow,monthNow,yearNow);
  RTC.set(now());
  mySerial.println("Decreasing hour");
  delay(100);
}

//Increase the minute
  if (incomingByte == 'M') {
  minuteNow++;
  if(minuteNow > 59){minuteNow = 0;}
  setTime(hourNow,minuteNow,secondNow,dayNow,monthNow,yearNow);
  RTC.set(now());
  mySerial.println("Increasing minute");
  delay(100);
}

//decrease the minute
  if (incomingByte == 'm') {
  minuteNow--;
  if(minuteNow < 0){minuteNow = 59;}
  setTime(hourNow,minuteNow,secondNow,dayNow,monthNow,yearNow);
  RTC.set(now());
  mySerial.println("Decreasing minute");
  delay(100);
}

// Increase the day
  if (incomingByte == 'D') {
  dayNow++;
  if(dayNow > 31){dayNow = 0;}
  setTime(hourNow,minuteNow,secondNow,dayNow,monthNow,yearNow);
  RTC.set(now());
  mySerial.println("Increasing Day");
  delay(100);
}

// Decrease the day
  if (incomingByte == 'd') {
  dayNow--;
  if(dayNow < 0){dayNow = 31;}
  setTime(hourNow,minuteNow,secondNow,dayNow,monthNow,yearNow);
  RTC.set(now());
  mySerial.println("Decreasing Day");
  delay(100);
}

// Increase the month
  if (incomingByte == 'N') {
  monthNow++;
  if(monthNow > 12){monthNow = 1;}
  setTime(hourNow,minuteNow,secondNow,dayNow,monthNow,yearNow);
  RTC.set(now());
  mySerial.println("Increasing Month");
  delay(100);
}

// Decrease the month
  if (incomingByte == 'n') {
  monthNow--;
  if(monthNow < 1){monthNow = 12;}
  setTime(hourNow,minuteNow,secondNow,dayNow,monthNow,yearNow);
  RTC.set(now());
  mySerial.println("Decreasing Month");
  delay(100);
}

// Increase year
  if (incomingByte == 'Y') {
  yearNow++;
  if(yearNow > 2099){yearNow = 0;}
  setTime(hourNow,minuteNow,secondNow,dayNow,monthNow,yearNow);
  RTC.set(now());
  mySerial.println("Increasing Year");
  delay(100);
}

// Decrease year
  if (incomingByte == 'y') {
  yearNow--;
  if(yearNow < 0){yearNow = 2099;}
  setTime(hourNow,minuteNow,secondNow,dayNow,monthNow,yearNow);
  RTC.set(now());
  mySerial.println("Decreasing Year");
  delay(100);
}

// increase brightness
 
  if (incomingByte == '+') {
  brightness = brightness + 8;
    if(brightness > 255){ 
    brightness = 255;
    analogWrite(mosfet1Pin, brightness);
   }
   mySerial.println("Increasing Brightness");
} 

// decrease brightness

if (incomingByte == '-') {
  brightness = brightness - 8;
    if(brightness < 0){ 
    brightness = 0;
    analogWrite(mosfet1Pin, brightness);
   }
    mySerial.println("Decreasing Brightness");
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
void ShowDisplayData(unsigned int uiMode )
{  //weekdayNow, dayNow, monthNow, minuteNow, true, hourNow, currentTemp
  
 //  dmd.clearScreen( true );

// check the mode
// then run corect mode display

switch(mode){
  case 1:
    mode1Display(weekdayNow, dayNow, monthNow, yearNow, minuteNow, true, hourNow, currentTemp);
    break;
  case 2:
    mode2Display(weekdayNow, dayNow, monthNow, yearNow, minuteNow, true, hourNow, currentTemp);
    break;
  case 3:
    mode3Display(weekdayNow, dayNow, monthNow, yearNow, minuteNow, true, hourNow, currentTemp);
    break;
  case 4:
    mode4Display(weekdayNow, dayNow, monthNow, yearNow, minuteNow, true, hourNow, currentTemp);
    break;
  case 5:
    mode5Display(weekdayNow, dayNow, monthNow, yearNow, minuteNow, true, hourNow, currentTemp);
    break;
  default:
    mode1Display(weekdayNow, dayNow, monthNow, yearNow, minuteNow, true, hourNow, currentTemp);
    break;
 }
}

void mode1Display(unsigned int uiWeekday,unsigned int uiDay, unsigned int uiMonth, unsigned int uiYear, unsigned int uiMinute, byte bColonOn, unsigned int uiHour,  unsigned int uiTemperature){

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

//end of the display routine
}

void mode2Display(unsigned int uiWeekday,unsigned int uiDay, unsigned int uiMonth, unsigned int uiYear, unsigned int uiMinute, byte bColonOn, unsigned int uiHour,  unsigned int uiTemperature){

 
  //draw the day 
   dmd.drawChar(  0,  9, '0'+((uiDay%100)  /10),   GRAPHICS_NORMAL );      // first digit
   dmd.drawChar(  6,  9, '0'+ (uiDay%10),          GRAPHICS_NORMAL );   // second digit

 //draw the season
   switch(uiMonth){
     case 1:
        dmd.drawString(  0,  0, "Summer", 6, GRAPHICS_NORMAL );
        break;
     case 2:
        dmd.drawString(  0,  0, "Summer", 6, GRAPHICS_NORMAL );
        break;
      case 3:
        dmd.drawString(  0,  0, "Autumn", 6, GRAPHICS_NORMAL );
        break;
      case 4:
        dmd.drawString(  0,  0, "Autumn", 6, GRAPHICS_NORMAL );
        break;
      case 5:
        dmd.drawString(  0,  0, "Autumn", 6, GRAPHICS_NORMAL );
        break;
      case 6:
        dmd.drawString(  0,  0, "Winter", 6, GRAPHICS_NORMAL );
        break;
      case 7:
        dmd.drawString(  0,  0, "Winter", 6, GRAPHICS_NORMAL );
        break;
      case 8:
        dmd.drawString(  0,  0, "Winter", 6, GRAPHICS_NORMAL );
        break;
      case 9:
        dmd.drawString(  0,  0, "Spring", 6, GRAPHICS_NORMAL );
        break;
      case 10:
        dmd.drawString(  0,  0, "Spring", 6, GRAPHICS_NORMAL );
        break;
      case 11:
        dmd.drawString(  0,  0, "Spring", 6, GRAPHICS_NORMAL );
        break;
      case 12:
        dmd.drawString(  0,  0, "Summer", 6, GRAPHICS_NORMAL );
        break;
 }

switch(uiMonth){
     case 1:
        dmd.drawString(  14,  9, "Jan", 3, GRAPHICS_NORMAL );
        break;
     case 2:
        dmd.drawString(  14,  9, "Feb", 3, GRAPHICS_NORMAL );
        break;
      case 3:
        dmd.drawString(  14,  9, "Mar", 3, GRAPHICS_NORMAL );
        break;
      case 4:
        dmd.drawString(  14,  9, "Apr", 3, GRAPHICS_NORMAL );
        break;
      case 5:
        dmd.drawString(  14,  9, "May", 3, GRAPHICS_NORMAL );
        break;
      case 6:
        dmd.drawString(  14,  9, "Jun", 3, GRAPHICS_NORMAL );
        break;
      case 7:
        dmd.drawString(  14,  9, "Jul", 3, GRAPHICS_NORMAL );
        break;
      case 8:
        dmd.drawString(  14,  9, "Aug", 3, GRAPHICS_NORMAL );
        break;
      case 9:
        dmd.drawString(  14,  9, "Sep", 3, GRAPHICS_NORMAL );
        break;
      case 10:
        dmd.drawString(  14,  9, "Oct", 3, GRAPHICS_NORMAL );
        break;
      case 11:
        dmd.drawString(  14,  9, "Nov", 3, GRAPHICS_NORMAL );
        break;
      case 12:
        dmd.drawString(  14,  9, "Dec", 3, GRAPHICS_NORMAL );
        break;
 }
 
//draw the time
   dmd.selectFont(System5x7);
   dmd.drawChar(  35,  9,'0'+((uiHour%100)/10), GRAPHICS_NORMAL );   // first digit
   dmd.drawChar(  41,  9, '0'+ (uiHour%10),  GRAPHICS_NORMAL );   // second digit
   dmd.drawChar(  47,  9,'.', GRAPHICS_NORMAL );   // colon
   dmd.drawChar( 53,  9, '0'+((uiMinute%100)  /10),   GRAPHICS_NORMAL );      // third digit
   dmd.drawChar( 59,  9, '0'+ (uiMinute%10),          GRAPHICS_NORMAL );   // fourth digit
   
// now show temperature
   dmd.drawChar(  45,  0,'0'+((uiTemperature%100)/10), GRAPHICS_NORMAL );   // first digit
   dmd.drawChar(  52,  0, '0'+ (uiTemperature%10),  GRAPHICS_NORMAL );   // second digit
   dmd.drawString(  59, 0, "C", 1, GRAPHICS_NORMAL );  // show the word degreees

//end of the display routine
}

void mode3Display(unsigned int uiWeekday,unsigned int uiDay, unsigned int uiMonth, unsigned int uiYear, unsigned int uiMinute, byte bColonOn, unsigned int uiHour,  unsigned int uiTemperature){

// now show temperature
   dmd.drawChar(  3,  0,'0'+((uiTemperature%100)/10), GRAPHICS_NORMAL );   // first digit
   dmd.drawChar(  9,  0, '0'+ (uiTemperature%10),  GRAPHICS_NORMAL );   // second digit
   dmd.drawString(  20, 0, "Degrees", 7, GRAPHICS_NORMAL );  // show the word degreees
 
//draw the time
   dmd.selectFont(System5x7);
   dmd.drawChar(  18,  9,'0'+((uiHour%100)/10), GRAPHICS_NORMAL );   // first digit
   dmd.drawChar(  24,  9, '0'+ (uiHour%10),  GRAPHICS_NORMAL );   // second digit
   dmd.drawChar(  30,  9,'.', GRAPHICS_NORMAL );   // colon
   dmd.drawChar( 35,  9, '0'+((uiMinute%100)  /10),   GRAPHICS_NORMAL );      // third digit
   dmd.drawChar( 41,  9, '0'+ (uiMinute%10),          GRAPHICS_NORMAL );   // fourth digit
   

//end of the display routine
}

void mode4Display(unsigned int uiWeekday,unsigned int uiDay, unsigned int uiMonth, unsigned int uiYear, unsigned int uiMinute, byte bColonOn, unsigned int uiHour,  unsigned int uiTemperature){

 // Show day of week
   switch(uiWeekday){
     case 1:
        dmd.drawString(  1,  0, "Sunday", 6, GRAPHICS_NORMAL );
        break;
     case 2:
        dmd.drawString(  1,  0, "Monday", 6, GRAPHICS_NORMAL );
        break;
     case 3:
        dmd.drawString(  1,  0, "Tuesday", 7, GRAPHICS_NORMAL );
        break;
     case 4:
        dmd.drawString(  1,  0, "Wednesday", 9, GRAPHICS_NORMAL );
        break;
     case 5:
        dmd.drawString(  1,  0, "Thursday", 8, GRAPHICS_NORMAL );
        break;
     case 6:
        dmd.drawString(  1,  0, "Friday", 6, GRAPHICS_NORMAL );
        break;
     case 7:
        dmd.drawString(  1,  0, "Saturday", 8, GRAPHICS_NORMAL );
        break;
  }
 
  //draw the day 
   dmd.drawChar(  52,  0, '0'+((uiDay%100)  /10),   GRAPHICS_NORMAL );      // first digit
   dmd.drawChar(  59,  0, '0'+ (uiDay%10),          GRAPHICS_NORMAL );   // second digit

//draw the time
   dmd.selectFont(System5x7);
   dmd.drawChar(  3,  9,'0'+((uiHour%100)/10), GRAPHICS_NORMAL );   // first digit
   dmd.drawChar(  10,  9, '0'+ (uiHour%10),  GRAPHICS_NORMAL );   // second digit
   dmd.drawChar(  16,  9,'.', GRAPHICS_NORMAL );   // colon
   dmd.drawChar( 21,  9, '0'+((uiMinute%100)  /10),   GRAPHICS_NORMAL );      // third digit
   dmd.drawChar( 27,  9, '0'+ (uiMinute%10),          GRAPHICS_NORMAL );   // fourth digit

// now show temperature
   dmd.drawChar(  47,  9,'0'+((uiTemperature%100)/10), GRAPHICS_NORMAL );   // first digit
   dmd.drawChar(  53,  9, '0'+ (uiTemperature%10),  GRAPHICS_NORMAL );   // second digit
   dmd.drawString(  59, 9, "C", 7, GRAPHICS_NORMAL );  // show the word degreees

//end of the display routine
}

void mode5Display(unsigned int uiWeekday,unsigned int uiDay, unsigned int uiMonth, unsigned int uiYear, unsigned int uiMinute, byte bColonOn, unsigned int uiHour,  unsigned int uiTemperature){

   // Show hour
   switch(uiHour){
     case 1:
        dmd.drawString(  0,  0, "One", 3, GRAPHICS_NORMAL );
        dmd.drawString(  46,  9, "AM", 2, GRAPHICS_NORMAL );
        break;
     case 2:
        dmd.drawString(  0,  0, "Two", 3, GRAPHICS_NORMAL );
        dmd.drawString(  46,  9, "AM", 2, GRAPHICS_NORMAL );
        break;
     case 3:
        dmd.drawString(  0,  0, "Three", 5, GRAPHICS_NORMAL );
        dmd.drawString(  46,  9, "AM", 2, GRAPHICS_NORMAL );
        break;
     case 4:
        dmd.drawString(  0,  0, "Four", 4, GRAPHICS_NORMAL );
        dmd.drawString(  46,  9, "AM", 2, GRAPHICS_NORMAL );
        break;
     case 5:
        dmd.drawString(  0,  0, "Five", 4, GRAPHICS_NORMAL );
        dmd.drawString(  46,  9, "AM", 2, GRAPHICS_NORMAL );
        break;
     case 6:
        dmd.drawString(  0,  0, "Six", 3, GRAPHICS_NORMAL );
        dmd.drawString(  46,  9, "AM", 2, GRAPHICS_NORMAL );
        break;
     case 7:
        dmd.drawString(  0,  0, "Seven", 5, GRAPHICS_NORMAL );
        dmd.drawString(  46,  9, "AM", 2, GRAPHICS_NORMAL );
        break;
     case 8:
        dmd.drawString(  0,  0, "Eight", 5, GRAPHICS_NORMAL );
        dmd.drawString(  46,  9, "AM", 2, GRAPHICS_NORMAL );
        break;
     case 9:
        dmd.drawString(  0,  0, "Nine", 4, GRAPHICS_NORMAL );
        dmd.drawString(  46,  9, "AM", 2, GRAPHICS_NORMAL );
        break;
     case 10:
        dmd.drawString(  0,  0, "Ten", 3, GRAPHICS_NORMAL );
        dmd.drawString(  46,  9, "AM", 2, GRAPHICS_NORMAL );
        break;
     case 11:
        dmd.drawString(  0,  0, "Eleven", 6, GRAPHICS_NORMAL );
        dmd.drawString(  46,  9, "AM", 2, GRAPHICS_NORMAL );
        break;
     case 12:
        dmd.drawString(  0,  0, "Twelve", 6, GRAPHICS_NORMAL );
        dmd.drawString(  46,  9, "PM", 2, GRAPHICS_NORMAL );
        break;
     case 13:
        dmd.drawString(  0,  0, "One", 3, GRAPHICS_NORMAL );
        dmd.drawString(  46,  9, "PM", 2, GRAPHICS_NORMAL );
        break;
     case 14:
        dmd.drawString(  0,  0, "Two", 3, GRAPHICS_NORMAL );
        dmd.drawString(  46,  9, "PM", 2, GRAPHICS_NORMAL );
        break;
     case 15:
        dmd.drawString(  0,  0, "Three", 5, GRAPHICS_NORMAL );
        dmd.drawString(  46,  9, "PM", 2, GRAPHICS_NORMAL );
        break;
     case 16:
        dmd.drawString(  0,  0, "Four", 4, GRAPHICS_NORMAL );
        dmd.drawString(  46,  9, "PM", 2, GRAPHICS_NORMAL );
        break;
     case 17:
        dmd.drawString(  0,  0, "Five", 4, GRAPHICS_NORMAL );
        dmd.drawString(  46,  9, "PM", 2, GRAPHICS_NORMAL );
        break;
     case 18:
        dmd.drawString(  0,  0, "Six", 3, GRAPHICS_NORMAL );
        dmd.drawString(  46,  9, "PM", 2, GRAPHICS_NORMAL );
        break;
     case 19:
        dmd.drawString(  0,  0, "Seven", 5, GRAPHICS_NORMAL );
        dmd.drawString(  46,  9, "PM", 2, GRAPHICS_NORMAL );
        break;
     case 20:
        dmd.drawString(  0,  0, "Eight", 5, GRAPHICS_NORMAL );
        dmd.drawString(  46,  9, "PM", 2, GRAPHICS_NORMAL );
        break;
     case 21:
        dmd.drawString(  0,  0, "Nine", 4, GRAPHICS_NORMAL );
        dmd.drawString(  46,  9, "PM", 2, GRAPHICS_NORMAL );
        break;
     case 22:
        dmd.drawString(  0,  0, "Ten", 3, GRAPHICS_NORMAL );
        dmd.drawString(  46,  9, "PM", 2, GRAPHICS_NORMAL );
        break;
     case 23:
        dmd.drawString(  0,  0, "Eleven", 6, GRAPHICS_NORMAL );
        dmd.drawString(  46,  9, "PM", 2, GRAPHICS_NORMAL );
        break;
     case 24:
        dmd.drawString(  0,  0, "Twelve", 6, GRAPHICS_NORMAL );
        dmd.drawString(  46,  9, "AM", 2, GRAPHICS_NORMAL );
        break;
}

// Show hour

if(uiMinute<=19){    // only run this below 20


if(uiMinute == 1){
        dmd.drawString(  22,  9, "One", 3, GRAPHICS_NORMAL );
}

if(uiMinute == 2){
        dmd.drawString(  22,  9, "Two", 3, GRAPHICS_NORMAL );
}

// end of below 20 display
}


if(uiMinute>=19){    // only run this above 20

if(uiMinute >=20 && uiMinute < 30){
        dmd.drawString(  30,  0, "Twenty", 6, GRAPHICS_NORMAL );
}
if(uiMinute >=30 && uiMinute < 40){
        dmd.drawString(  30,  0, "Thirty", 6, GRAPHICS_NORMAL );
}
if(uiMinute >=40 && uiMinute < 50){
        dmd.drawString(  35,  0, "Forty", 5, GRAPHICS_NORMAL );
}
if(uiMinute >=50 && uiMinute < 60){
        dmd.drawString(  35,  0, "Fifty", 5, GRAPHICS_NORMAL );
}

 
if(uiMinute%10 == 1){
        dmd.drawString(  11,  9, "One", 5, GRAPHICS_NORMAL );
}

if(uiMinute%10 == 2){
        dmd.drawString(  11,  9, "Two", 5, GRAPHICS_NORMAL );
}

if(uiMinute%10 == 3){
        dmd.drawString(  11,  9, "Three", 5, GRAPHICS_NORMAL );
}

if(uiMinute%10 == 4){
        dmd.drawString(  11,  9, "Four", 5, GRAPHICS_NORMAL );
}

if(uiMinute%10 == 5){
        dmd.drawString(  11,  9, "Five", 5, GRAPHICS_NORMAL );
}

if(uiMinute%10 == 6){
        dmd.drawString(  11,  9, "Six", 5, GRAPHICS_NORMAL );
}

if(uiMinute%10 == 7){
        dmd.drawString(  11,  9, "Seven", 5, GRAPHICS_NORMAL );
}

if(uiMinute%10 == 8){
        dmd.drawString(  11,  9, "Eight", 5, GRAPHICS_NORMAL );
}

if(uiMinute%10 == 9){
        dmd.drawString(  11,  9, "Nine", 5, GRAPHICS_NORMAL );
}

// end of above 20 display
}

//end of the display routine
}

//end of modes

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
//  mySerial.print("ADC: "); mySerial.print(RawADC); mySerial.print("/1024");  // Print out RAW ADC Number
//  mySerial.print(", Volts: "); printDouble(((RawADC*5)/1024.0),3);   // 4.860 volts is what my USB Port outputs.
//  mySerial.print(", Resistance: "); mySerial.print(Resistance); mySerial.print("ohms");
// // END- Remove these lines for the function not to display anything

 // Uncomment this line for the function to return Fahrenheit instead.
 //Temp = (Temp * 9.0)/ 5.0 + 32.0; // Convert to Fahrenheit
 return Temp;  // Return the Temperature
}
