
#include <U8g2lib.h>
#include <EEPROM.h>
#include <stdio.h>
#include <string.h>

#define cs                             10
#define sck                            13
#define mosi                           11
#define a0                              9
#define reset                           8
#define brightness                      6
#define rightBtn                        4
#define leftBtn                         3
#define enterBtn                        2
#define cancelBtn                      12
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

#define TIMEOUT                         10 // timeout for serial port in ms

#define NORESPONDE                      "n/r"

#define BUCKVMIN                        1  // 1V min
#define BUCKVMAX                        25  // 20V max

#define BUCKCURRENTMIN                  0.05
#define BUCKCURRENTMAX                  3.0

#define I5VCURRENTMIN                   0.01
#define I5VCURRENTMAX                   1.0

#define I3V3CURRENTMIN                  0.01
#define I3V3CURRENTMAX                  0.30

#define BRIGHTNESS_ADDRESS              0
#define CONTRAST_ADDRESS                1
#define PROFILES_START_ADDRRESS         2

U8G2_ST7565_NHD_C12864_2_4W_SW_SPI u8g2(U8G2_R2, sck, mosi, cs, a0 , reset);

int timeNow = 0;
String VBuck= "0.0", IBuck = "0.0",PBuck = "0.0",IBLIM = "0.0";
String V5V= "0.0", I5V = "0.0",P5V = "0.0",I5VLIM = "0.0";
String V3V3= "0.0", I3V3 = "0.0",P3V3 = "0.0",I3V3LIM = "0.0";
String values = "";

int menu = 0;
bool changeParam =false;

uint8_t current_selection = 0;
uint8_t brightnessValue = 100;
uint8_t contrastValue = 127;
bool increase = false;
bool decrease = false;
bool modifyingParam = false;
bool modifyStep = false;
bool retrySendCommand = false;
uint8_t stepMultiplier = 1;

float setPointVBuck = 0.0;
float setPointIBuck = 0.0;
float setPointI5V = 0.0;
float setPointI3V3 = 0.0;

const char *menu_list = "Buck\nProfiles\nDisplay\nAbout";

const char *buck_list = "Modify KP\nModify KI";

const char *profile_list = "Save profile\nLoad profile";

const char *display_list = "Change brightness\nChange contrast";

void drawDisplaySettings(uint8_t param);
void drawHorizontalPorcentageBar(uint8_t var);
void setBrightness(uint8_t value);
String getValue(String data, char separator, int index);
uint8_t setCommand(uint8_t command, float value);
void setCommandWithoutData(uint8_t command);
String getCommand(uint8_t command);
void showAllData();

void setup() {

  while(!Serial);                    // wait until serial ready
  Serial.begin(115200);              // start serial
  Serial.setTimeout(TIMEOUT);        // set timeout         

  pinMode(cancelBtn, INPUT);           // set pin to input
  digitalWrite(cancelBtn, HIGH);       // turn on pullup resistors only in this pin because all the rest input have external pullups resistors

  pinMode(brightness, OUTPUT);   // sets the pin as output
  setBrightness(brightnessValue);

  u8g2.setContrast(contrastValue);
  
  u8g2.setFont(u8g2_font_6x12_tr);

  u8g2.begin(/*Select=*/ enterBtn, /*Right/Next=*/ rightBtn, /*Left/Prev=*/ leftBtn, /*Up=*/ A0, /*Down=*/ A3, /*Home/Cancel=*/ cancelBtn); // Arduboy 10 (Production)

}

void loop() {
  u8g2.firstPage();
  do
  {
    showAllData();
  } while (u8g2.nextPage());
  delay(25);
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

    // V3V3 16
    //dtostrf(SETI3V3LIM,3, 2, buf);
    u8g2.drawStr( 5, 20, (V3V3).c_str());
    u8g2.drawStr( 5, 33, (I3V3).c_str());
    u8g2.drawStr( 5, 46, (P3V3).c_str());
    u8g2.drawStr( 5, 59, (I3V3LIM).c_str());
  
    // V5V 51
    //dtostrf(SETI5VLIM,3, 2, buf);
    u8g2.drawStr( 40, 20, (V5V).c_str());
    u8g2.drawStr( 40, 33, (I5V).c_str());
    u8g2.drawStr( 40, 46, (P5V).c_str());
    u8g2.drawStr( 40, 59,(I5VLIM).c_str());
  
    // Buck  87
    u8g2.drawStr( 76, 20, (VBuck).c_str());
    u8g2.drawStr( 76, 33, (IBuck).c_str());
    u8g2.drawStr( 76, 46, (PBuck).c_str());
    u8g2.drawStr( 76, 59, (IBLIM).c_str());
    
    uint8_t menuKey =  u8g2.getMenuEvent();

    increase = false;
    decrease = false;
    if(!menu){
      changeParam = false;
    }
    if(menuKey == U8X8_MSG_GPIO_MENU_NEXT){
      if(!changeParam){
        menu++;
        if(menu > 4){
          menu = 0;
        }
      }else{ // increase selected value
        increase = true;
        decrease = false;
      }
    }else if(menuKey == U8X8_MSG_GPIO_MENU_PREV){
      if(!changeParam){
        menu--;
        if(menu < 0){
          menu = 4;
        }
      }else{ // decrease selected value
        increase = false;
        decrease = true; // decrease
      }
    }else if(menuKey == U8X8_MSG_GPIO_MENU_SELECT){
      if(!modifyingParam){
        changeParam = !changeParam;
        modifyStep = false;
        modifyingParam =true;
      }else{
        modifyingParam =false;
        modifyStep =true;
      }            
    }else if(menuKey == U8X8_MSG_GPIO_MENU_HOME){
      showMenu();     
    }

    if(modifyStep){
      stepMultiplier = 10;  
    }else{
      stepMultiplier = 1;
    }
    
    switch(menu){
      case 0:
          /*
          u8g2.setColorIndex(0);
          u8g2.drawRBox(108,1,9,9,2);
          u8g2.setColorIndex(1);
          u8g2.drawStr( 110, 9, "M");
          */
        break;
      case 1:     // VB1
        if(changeParam){
          if(increase){
            setPointVBuck += 0.1 * stepMultiplier; 
          }
          if(decrease){
            setPointVBuck -= 0.1 * stepMultiplier; 
          }
          if(setPointVBuck > BUCKVMAX){
            setPointVBuck = BUCKVMAX;
          }
          if(setPointVBuck < BUCKVMIN){
            setPointVBuck = BUCKVMIN;
          }

          if(increase || decrease || retrySendCommand ){
            if(setCommand(SET_BUCK_VOLTAGE, setPointVBuck)){
              retrySendCommand =false;
            }else{
              retrySendCommand =true;
            }   
          }

          char buffer[4]; 
          const char *s = dtostrf(setPointVBuck, 1, 2, buffer);
          
          u8g2.drawRBox(74,11,33,12,3);
          u8g2.setColorIndex(0);
          u8g2.drawStr( 76, 20, s);
          u8g2.setColorIndex(1);
        }else{
          u8g2.drawRFrame(74,11,33,12,3);
        }
  
        break;
      case 2:   // IB1
        if(changeParam){

          if(increase){
            setPointIBuck += 0.01 * stepMultiplier; 
          }
          if(decrease){
            setPointIBuck -= 0.01 * stepMultiplier; 
          }
          if(setPointIBuck > BUCKCURRENTMAX){
            setPointIBuck = BUCKCURRENTMAX;
          }
          if(setPointIBuck < BUCKCURRENTMIN){
            setPointIBuck = BUCKCURRENTMIN;
          }

          if(increase || decrease || retrySendCommand){
            if(setCommand(SET_BUCK_CURRENT_LIMIT, setPointIBuck)){
              retrySendCommand =false;
            }else{
              retrySendCommand =true;
            }     
          }

          char buffer[4]; 
          const char *s = dtostrf(setPointIBuck, 1, 2, buffer);
          
          u8g2.drawRBox(74,50,33,12,3);
          u8g2.setColorIndex(0);
          u8g2.drawStr( 76, 59, s);
          u8g2.setColorIndex(1);
        }else{
          u8g2.drawRFrame(74,50,33,12,3);
        }    
        break;
      case 3: // I5V
        if(changeParam){

          if(increase){
            setPointI5V += 0.01 * stepMultiplier; 
          }
          if(decrease){
            setPointI5V -= 0.01 * stepMultiplier; 
          }
          if(setPointI5V > I5VCURRENTMAX){
            setPointI5V = I5VCURRENTMAX;
          }
          if(setPointI5V < I5VCURRENTMIN){
            setPointI5V = I5VCURRENTMIN;
          }

          if(increase || decrease || retrySendCommand){
            if(setCommand(SET_5V_CURRENT_LIMIT, setPointI5V)){
              retrySendCommand =false;
            }else{
              retrySendCommand =true;
            }   
          }

          char buffer[4]; 
          const char *s = dtostrf(setPointI5V, 1, 2, buffer);
                  
          u8g2.drawRBox(38,50,33,12,3);
          u8g2.setColorIndex(0);
          u8g2.drawStr( 40, 59, s);
          u8g2.setColorIndex(1);
        }else{
          u8g2.drawRFrame(38,50,33,12,3);
        }
        break;    
      case 4: // I3V3
        if(changeParam){

          if(increase){
            setPointI3V3 += 0.01 * stepMultiplier; 
          }
          if(decrease){
            setPointI3V3 -= 0.01 * stepMultiplier; 
          }
          if(setPointI3V3 > I3V3CURRENTMAX){
            setPointI3V3 = I3V3CURRENTMAX;
          }
          if(setPointI3V3 < I3V3CURRENTMIN){
            setPointI3V3 = I3V3CURRENTMIN;
          }
          
          if(increase || decrease || retrySendCommand){
            if(setCommand(SET_3V3_CURRENT_LIMIT, setPointI3V3)){
              retrySendCommand =false;
            }else{
              retrySendCommand =true;
            }   
          }
            
          char buffer[4]; 
          const char *s = dtostrf(setPointI3V3, 1, 2, buffer);
                            
          u8g2.drawRBox(2,50,33,12,3);
          u8g2.setColorIndex(0);
          u8g2.drawStr( 5, 59, s);
          u8g2.setColorIndex(1);
        }else{
          u8g2.drawRFrame(2,50,33,12,3); 
        }
        break; 
    } 
}

void showMenu(){
      uint8_t current_selection_main_menu = 1;
      while(current_selection_main_menu){
        current_selection_main_menu = u8g2.userInterfaceSelectionList(
                                        "SMPS NXT MENU",
                                        current_selection_main_menu, 
                                        menu_list);
        current_selection = current_selection_main_menu;
        switch(current_selection_main_menu){            
          case 1:
            current_selection = u8g2.userInterfaceSelectionList(
                                "Buck",
                                1, 
                                buck_list);
            break;
          case 2:
            while(current_selection){
                current_selection = u8g2.userInterfaceSelectionList(
                                "Profiles",
                                1, 
                                profile_list);
                if(current_selection == 1){ // change brightness
                   
                }else if(current_selection == 2){ // change contrast
                
                }   
            }                 
            break;
          case 3:
            while(current_selection){
              current_selection = u8g2.userInterfaceSelectionList(
                                  "Display",
                                  1, 
                                  display_list);
              drawDisplaySettings(current_selection);                   
            }          
            break;                  
          case 4:

            break;                
      }            
    }
}

void drawDisplaySettings(uint8_t param){ // param = 1 -> Brightness | param = 2 -> Contrast
      
      if(!param){
        return -1; 
      }
      
      //EEPROM.get(BRIGHTNESS_ADDRESS , brillo);
      //EEPROM.get(CONTRAST_ADDRESS, contraste);

      uint8_t heigthFrame = 14;
      uint8_t key =  1;
      uint8_t initialValue = 1;

      switch(param){
        case 1:
          initialValue = brightnessValue;
          break;
        case 2:
          initialValue = contrastValue;
          break;
      }

      Serial.print("param: " + String(param) + "\n");
      while(key != 83 && key != 80){
        key = u8g2.getMenuEvent();
        if(key == U8X8_MSG_GPIO_MENU_NEXT){
            switch(param){
              case 1:
                brightnessValue += 4;
                setBrightness(brightnessValue);
                break;
              case 2:
                contrastValue += 2;
                u8g2.setContrast(contrastValue);
                break;
            }
        }else if(key == U8X8_MSG_GPIO_MENU_PREV){
            switch(param){
              case 1:
                brightnessValue -= 4;
                setBrightness(brightnessValue);
                break;
              case 2:
                contrastValue -= 2;
                
                u8g2.setContrast(contrastValue);
                break;
            }
        }else if(key == U8X8_MSG_GPIO_MENU_SELECT){
            switch(param){
              case 1:
                brightnessValue = initialValue;
                setBrightness(brightnessValue);
                break;
              case 2:
                contrastValue = initialValue;
                u8g2.setContrast(contrastValue);
                break;
            }
            key = 80; 
        }else if(key == U8X8_MSG_GPIO_MENU_HOME){
            switch(param){
              case 1:
                brightnessValue = initialValue;
                setBrightness(brightnessValue);
                break;
              case 2:
                contrastValue = initialValue;
                u8g2.setContrast(contrastValue);
                break;
            }
            key = 83;    
        }
        u8g2.firstPage();  
        do {
          u8g2.setColorIndex(1);
          u8g2.drawBox(0,0,128,heigthFrame-1);
          u8g2.drawFrame(0,heigthFrame,128,64 - heigthFrame);
          u8g2.setColorIndex(0);
          switch(param){  
            case 1: // BRILLO
              u8g2.setCursor(1, heigthFrame-4);
              u8g2.print("Brightness = " + String(brightnessValue*100/256) + "%");
              drawHorizontalPorcentageBar(brightnessValue);
              break;         
            case 2: // CONTRASTE
              u8g2.setCursor(1, heigthFrame-4);
              u8g2.print("Contrast = " + String(contrastValue*100/256) + "%");              
              drawHorizontalPorcentageBar(contrastValue);
              break;               
          }

      } while( u8g2.nextPage() );
       delay(25);   
  }
}

void drawHorizontalPorcentageBar(uint8_t var){
  u8g2.setColorIndex(1);
  u8g2.drawRFrame(12,20,101,16,4);
  u8g2.drawBox(13,21, var*100/256,14);
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

void setBrightness(uint8_t value){
  analogWrite(brightness, value);  // analogRead values go from 0 to 1023, analogWrite values from 0 to 255
}

uint8_t setCommand(uint8_t command, float value){
  Serial.write(command); 
  timeNow = millis(); 
  while( Serial.available() == 0 && (millis() - timeNow) <= TIMEOUT);
  if(Serial.read() == ACK ){    
    Serial.print(value,2);
    Serial.write('\0');
    return 1;      
  }
  return 0; 
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

