/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.

  Most Arduinos have an on-board LED you can control. On the UNO, MEGA and ZERO 
  it is attached to digital pin 13, on MKR1000 on pin 6. LED_BUILTIN takes care 
  of use the correct LED pin whatever is the board used.
  If you want to know what pin the on-board LED is connected to on your Arduino model, check
  the Technical Specs of your board  at https://www.arduino.cc/en/Main/Products
  
  This example code is in the public domain.

  modified 8 May 2014
  by Scott Fitzgerald
  
  modified 2 Sep 2016
  by Arturo Guadalupi
*/


#include "U8glib.h"

#define ENCODER_USE_INTERRUPTS
#include <Encoder.h>

#define cs           10
#define sck          13
#define mosi         11
#define a0           9
#define reset        8
#define brightness   6

#define drawWhite    0
#define drawBlack    1




String kp = "0.175",ki="371.22",kd= "0";

String VBuck1= "0", IBuck1 = "0",PBuck1 = "0";
String VBuck2= "0", IBuck2 = "0",PBuck2 = "0";
String V5V= "0", I5V = "0",P5V = "0";
String V3V3= "0", I3V3 = "0",P3V3 = "0";

volatile unsigned int encoder0Pos = 0;
volatile float setPoint1 = 5;
float oldPosition  = setPoint1*10/400;


const float setPointMin = 1;
const float setPointMax = 30;



U8GLIB_NHD_C12864 u8g(sck, mosi, cs, a0 , reset);
Encoder myEnc(2,3);


void draw(){
  
  //u8g.drawStr( 0, 16, ("PID aplicado a una fuente de alimentacion"));
 /*
  u8g.setColorIndex(1); // Instructs the display to draw with a pixel on.
  u8g.drawBox(0,9,63,35);
  u8g.setColorIndex(0); // Instructs the display to draw with a pixel on.
  u8g.drawStr( 3, 18, ("PID"));
  u8g.drawStr( 3, 26, ("KP=" + kp).c_str());
  u8g.drawStr( 3, 34, ("KI=" + ki).c_str());
  u8g.drawStr( 3, 42, ("KD=" + kd).c_str());

  u8g.setColorIndex(1); // Instructs the display to draw with a pixel on.
  u8g.drawFrame(0,46,63,18);
  u8g.drawStr( 3, 59, ("3.3V"));

  u8g.drawFrame(65,46,63,18);
  u8g.drawStr( 32, 59, ("100mA"));

  u8g.drawFrame(65,9,63,35);

  u8g.drawStr( 72, 59, ("5V"));
  u8g.drawStr( 105, 59, ("1.00A"));
*/


  u8g.drawStr( 0, 8, ("Set Point: " + String(setPoint1) + " V").c_str());
  // VBUCK1
  u8g.drawStr( 23, 18, ("3V3   5V  BUCK"));
  u8g.drawStr( 0, 30, ("[V]" + VBuck1).c_str());
  u8g.drawStr( 0, 45, ("[A]" + IBuck1).c_str());
  u8g.drawStr( 0, 60, ("[W]" + PBuck1).c_str());

  // V5V
  u8g.drawStr( 60, 30, (V5V).c_str());
  u8g.drawStr( 60, 45, (I5V).c_str());
  u8g.drawStr( 60, 60, (P5V).c_str());

  // V3V3
  u8g.drawStr( 90, 30, (V3V3).c_str());
  u8g.drawStr( 90, 45, (I3V3).c_str());
  u8g.drawStr( 90, 60, (P3V3).c_str());
  u8g.drawLine(17, 9, 17, 63);
  u8g.drawLine(47, 9, 47, 63);
  u8g.drawLine(77, 9, 77, 63);

}


void setup() {

  // initialize serial:
  Serial.begin(9600);
  while (!Serial) {
     // wait for serial port to connect. Needed for native USB port only
  }

  
  // Initialize encoder
  myEnc.write(setPoint1*40);
  
  // initialize PWM for backlight
  pinMode(brightness, OUTPUT);   // sets the pin as output
  analogWrite(brightness, 200);  // analogRead values go from 0 to 1023, analogWrite values from 0 to 255
  
   // initialize LCD 
  u8g.setFont(u8g_font_6x12);
  u8g.setColorIndex(drawBlack); // Instructs the display to draw with a pixel on.
  u8g.setContrast(0);  
}

// the loop function runs over and over again forever
void loop() {

  u8g.firstPage();
  do {  


    /*
    getVBuck();
    
    getV5V();
    getV3V3();
    getI5V();
    getI3V3();
    getIBuck();
    */
    checkEncoder();
    draw();
   
  } while( u8g.nextPage() );
  delay(200); 
  //getVBuck();
}

void checkEncoder(){
  float newPosition = myEnc.read();
  if (newPosition != oldPosition) {
    oldPosition = newPosition;
    setPoint1 = (oldPosition/400)*10;
    if(setPoint1 < setPointMin){
      setPoint1 =  oldPosition = setPointMin;
      myEnc.write(setPointMin*40);
    }
    if(setPoint1 > setPointMax){
      setPoint1 = oldPosition = setPointMax;
      myEnc.write(setPointMax*40);
    }
  } 
}

void sendSetPoint(){
  Serial.write(1);
  //while (Serial.available() == 0);
  if(Serial.read()== 6 ){
    Serial.print(setPoint1,2);
  }
}

void getVBuck(){
  Serial.write(2);
  while (Serial.available() == 0);
  if(Serial.read()== 6 ){
     VBuck1 = Serial.readStringUntil('\0');
  }
}

void getV5V(){
  Serial.write(9);
  while (Serial.available() == 0);
  if(Serial.read()== 6 ){
     V5V = Serial.readStringUntil('\0');
  }
}

void getV3V3(){
  Serial.write(10);
  while (Serial.available() == 0);
  if(Serial.read()== 6 ){
     V3V3 = Serial.readStringUntil('\0');
  }
}

void getIBuck(){
  Serial.write(3);
  while (Serial.available() == 0);
  if(Serial.read()== 6 ){
     IBuck1 = Serial.readStringUntil('\0');
  }
}

void getI5V(){
  Serial.write(11);
  while (Serial.available() == 0);
  if(Serial.read()== 6 ){
     I5V = Serial.readStringUntil('\0');
  }
}

void getI3V3(){
  Serial.write(12);
  while (Serial.available() == 0);
  if(Serial.read()== 6 ){
     I3V3 = Serial.readStringUntil('\0');
  }
}

