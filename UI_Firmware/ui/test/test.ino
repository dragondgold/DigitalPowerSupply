
#include <U8g2lib.h>
#include <EEPROM.h>
#include <stdio.h>

#define cs                             10
#define sck                            13
#define mosi                           11
#define a0                              9
#define reset                           8
#define brightness                      6
#define rightBtn                        2
#define leftBtn                         3
#define enterBtn                        4
#define fault                           5

// List of Commands
#define ACK                             0x06
#define NACK                            0xFF

#define SET_BUCK_VOLTAGE                1
#define GET_BUCK_VOLTAGE                2
#define GET_BUCK_CURRENT                3
#define GET_BUCK_POWER                  4

#define SET_BUCK_KP                     5
#define SET_BUCK_KI                     7

#define DISABLE_BUCK_PID                8
#define ENABLE_BUCK_PID                 9

#define GET_5V_VOLTAGE                  10
#define GET_3V3_VOLTAGE                 11 
#define GET_5V_CURRENT                  12
#define GET_3V3_CURRENT                 13

#define SET_BUCK_CURRENT_LIMIT          14
#define SET_5V_CURRENT_LIMIT            15
#define SET_3V3_CURRENT_LIMIT           16

#define ENABLE_BUCK                     17
#define ENABLE_AUX                      18
#define DISABLE_BUCK                    19
#define DISABLE_AUX                     20

#define GET_STATUS                      21
#define GET_BUCK_SETPOINT               22
#define GET_ALL_STRING                  23

#define SHUTDOWN                        24

#define TIMEOUT                         8 // timeout for serial port in ms

#define NORESPONDE                      "n/r"

#define setPointMin                     1  // 1V min
#define setPointMax                     20  // 20V max

#define BUCKCURRENTMIN                  0.1
#define BUCKCURRENTMAX                  3.0

#define I5VCURRENTMIN                   0.1
#define I5VCURRENTMAX                   1.0

#define I3V3CURRENTMIN                  0.1
#define I3V3CURRENTMAX                  0.3

#define BRIGHTNESS_ADDRESS              0
#define CONTRAST_ADDRESS                1
#define PROFILES_START_ADDRRESS         2

U8G2_ST7565_NHD_C12864_F_4W_SW_SPI u8g2(U8G2_R2, sck, mosi, cs, a0 , reset);

int timeNow = 0;
volatile String VBuck= "0", IBuck = "0",PBuck = "0",IBLIM = "0";
volatile String V5V= "0", I5V = "0",P5V = "0",I5VLIM = "0";
volatile String V3V3= "0", I3V3 = "0",P3V3 = "0",I3V3LIM = "0";
volatile String values = "";

char buf[20];

String getValue(String data, char separator, int index);
void setCommand(uint8_t command, float value);
void setCommandWithoutData(uint8_t command);
String getCommand(uint8_t command);
void showAllData();

void setup() {

  while(!Serial);                    // wait until serial ready
  Serial.begin(115200);              // start serial
  Serial.setTimeout(TIMEOUT);        // set timeout         
  
  pinMode(brightness, OUTPUT);   // sets the pin as output
  analogWrite(brightness, 100);  // analogRead values go from 0 to 1023, analogWrite values from 0 to 255
  
  u8g2.begin(enterBtn,rightBtn,leftBtn);
  u8g2.setFont(u8g2_font_6x12_tr);

}

/*
const char *string_list = 
  "Altocumulus\n"
  "Altostratus\n"
  "Cirrocumulus\n"
  "Cirrostratus\n"
  "Cirrus\n"
  "Cumulonimbus\n"
  "Cumulus\n"
  "Nimbostratus\n"
  "Stratocumulus\n"
  "Stratus";

uint8_t current_selection = 0;
*/
void loop() {
  u8g2.firstPage();
  do
  {
    showAllData();
  } while (u8g2.nextPage());
  delay(500);
}


void serialEvent() {
  while (Serial.available()) {
    if(Serial.read() == ACK ){    
        values = Serial.readStringUntil('\0');  
        
        VBuck = getValue(values,";",0);
        IBuck = getValue(values,";",1);
        PBuck = getValue(values,";",2);
        IBLIM = getValue(values,";",3);

        V5V = getValue(values,";",4);
        I5V = getValue(values,";",5);
        P5V = getValue(values,";",6);
        I5VLIM = getValue(values,";",7);

        
        V3V3 = getValue(values,";",8);
        I3V3 = getValue(values,";",9);
        P3V3 = getValue(values,";",10);
        I3V3LIM = getValue(values,";",11);
    } 
  }
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void setCommand(uint8_t command, float value){
  Serial.write(command); 
  timeNow = millis(); 
  while( Serial.available() == 0 && (millis() - timeNow) <= TIMEOUT);
  if(Serial.read() == ACK ){    
    Serial.print(value,2);
    Serial.write('\0');      
  } 
}

void setCommandWithoutData(uint8_t command){
  Serial.write(command); 
  timeNow = millis(); 
  while( Serial.available() == 0 && (millis() - timeNow) <= TIMEOUT);
  if(Serial.read() == ACK ){    
      
  } 
}

String getCommand(uint8_t command){
  Serial.write(command);
  timeNow = millis();
  while(Serial.available() == 0 && (millis() - timeNow) <= TIMEOUT); 
  if(Serial.read()== ACK ){
     return Serial.readStringUntil('\0');
  } else{
     return NORESPONDE;
  }
}

void showAllData(){
    u8g2.drawBox(0,0,117,10);
    u8g2.drawBox(108,8,9,64);
     
    u8g2.setColorIndex(0);
    u8g2.drawStr( 11, 8, ("3V3"));
    u8g2.drawStr( 49, 8, ("5V"));
    u8g2.drawStr( 85, 8, ("B1"));
 
      
    u8g2.drawStr( 110, 20, ("V"));
    u8g2.drawStr( 110, 33, ("A"));
    u8g2.drawStr( 110, 46, ("W"));
    u8g2.drawStr( 110, 59, ("L"));
  
    u8g2.setColorIndex(1);
    u8g2.drawLine(0,0,117,0);
    u8g2.drawLine(36,10,36,64);
    u8g2.drawLine(72,10,72,64);
    u8g2.drawLine(108,10,108,64);
    u8g2.drawLine(117,0,117,64);

    u8g2.drawEllipse(127,8,8,6);
    u8g2.drawEllipse(127,24,8,6);
    u8g2.drawEllipse(127,40,8,6);
    u8g2.drawEllipse(127,56,8,6);

    // Buttons 
    u8g2.drawLine(122,8,124,11);
    u8g2.drawLine(124,11,127,4);
    u8g2.drawTriangle(123,20,127,24,123,28);
    u8g2.drawTriangle(127,36,123,40,127,44);
    u8g2.drawLine(123,54,127,58);
    u8g2.drawLine(123,58,127,54);
    
    u8g2.drawLine(0,10,0,64);
    u8g2.drawLine(0,63,117,63);

    // V3V3
    //dtostrf(SETI3V3LIM,3, 2, buf);
    u8g2.drawStr( 16, 20, (V3V3).c_str());
    u8g2.drawStr( 16, 33, (I3V3).c_str());
    u8g2.drawStr( 16, 46, (P3V3).c_str());
    u8g2.drawStr( 16, 59, (I3V3LIM).c_str());
  
    // V5V
    //dtostrf(SETI5VLIM,3, 2, buf);
    u8g2.drawStr( 51, 20, (V5V).c_str());
    u8g2.drawStr( 51, 33, (I5V).c_str());
    u8g2.drawStr( 51, 46, (P5V).c_str());
    u8g2.drawStr( 51, 59,(I5VLIM).c_str());
  
    // Buck 1 
    u8g2.drawStr( 87, 20, (VBuck).c_str());
    u8g2.drawStr( 87, 33, (IBuck).c_str());
    u8g2.drawStr( 87, 46, (PBuck).c_str());
    u8g2.drawStr( 87, 59, (IBLIM).c_str());
    
    /*
    switch(menu){
      case 0:
          u8g.setColorIndex(0);
          u8g.drawRBox(120,1,9,9,2);
          u8g.setColorIndex(1);
          u8g.drawStr( 122, 9, "M");
        break;
      case 1:
        if(changeParam){ // VB2
          u8g.drawRBox(87,10,120-87,13,3);
          u8g.setColorIndex(0);
          u8g.drawStr( 89, 20, (String(setPoint2)).c_str());
          u8g.setColorIndex(1);
        }else{
          u8g.drawRFrame(87,10,120-87,13,3);
        }
        break;
      case 2:       // IB2
        if(changeParam){
          u8g.drawRBox(87,10+13*3,120-87,13,3);
          u8g.setColorIndex(0);
          u8g.drawStr( 89, 59, (String(SETIB2LIM)).c_str());
          u8g.setColorIndex(1);
        }else{
          u8g.drawRFrame(87,10+13*3,120-87,13,3);
        }
        break;
      case 3:     // B1
        if(changeParam){
          u8g.drawRBox(55,10,120-89,13,3);
          u8g.setColorIndex(0);
          u8g.drawStr( 57, 20, (String(setPoint1)).c_str()); 
          u8g.setColorIndex(1);
        }else{
          u8g.drawRFrame(55,10,120-89,13,3);
        }
  
        break;
      case 4:   //IB1
        if(changeParam){
          u8g.drawRBox(55,10+13*3,120-89,13,3);
          u8g.setColorIndex(0);
          u8g.drawStr( 57, 59, (String(SETIB1LIM)).c_str()); 
          u8g.setColorIndex(1);
        }else{
          u8g.drawRFrame(55,10+13*3,120-89,13,3);
        }    
        break;
      case 5: // I5V
        if(changeParam){
          u8g.drawRBox(28,10+13*3,26,13,3);
          u8g.setColorIndex(0);
          u8g.drawStr( 29, 59, (String(SETI5VLIM)).c_str()); 
          u8g.setColorIndex(1);
        }else{
          u8g.drawRFrame(28,10+13*3,26,13,3);
        }
        break;    
      case 6: // I3V3
        if(changeParam){
          u8g.drawRBox(0,10+13*3,27,13,3);
          u8g.setColorIndex(0);
          u8g.drawStr( 2, 59, (String(SETI3V3LIM)).c_str()); 
          u8g.setColorIndex(1);
        }else{
          u8g.drawRFrame(0,10+13*3,27,13,3); 
        }
        break; 
    } 
*/
}

