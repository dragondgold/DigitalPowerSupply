#include <U8g2lib.h>
#include <EEPROM.h>
#include <stdio.h>
#include <string.h>
#include <PinChangeInterrupt.h>

#define cs                             10
#define sck                            13
#define mosi                           11
#define a0                              9
#define reset                           8
#define brightness                      6

#define rightBtn                        3
#define leftBtn                         4
#define enterBtn                        12
#define cancelBtn                       2
#define fault                           5
#define powerGood                       7

// List of Commands
#define ACK                             0x06
#define ACK_STRING                      0x01
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

#define GET_BUCK_KP                     25
#define GET_BUCK_KI                     26

#define TIMEOUT                         20 // timeout for serial port in ms

#define NORESPONDE                      "n/r"

#define BUCKVMIN                        1.0  // 1V min
#define BUCKVMAX                        25.0  // 20V max

#define BUCKCURRENTMIN                  0.05
#define BUCKCURRENTMAX                  3.0

#define I5VCURRENTMIN                   0.01
#define I5VCURRENTMAX                   1.0

#define I3V3CURRENTMIN                  0.01
#define I3V3CURRENTMAX                  0.30

#define CONTRAST_MIN                    190
#define CONTRAST_MAX                    255


#define BRIGHTNESS_MIN                  0
#define BRIGHTNESS_MAX                  255

#define BRIGHTNESS_ADDRESS              0
#define CONTRAST_ADDRESS                2
#define PROFILES_START_ADDRRESS         5
#define PROFILES_ALLOWED                3

#define OK_BTN                          1
#define CANCEL_BTN                      2

U8G2_ST7565_NHD_C12864_2_4W_SW_SPI u8g2(U8G2_R0, sck, mosi, cs, a0 , reset);

String VBuck= "0.0", IBuck = "0.0",PBuck = "0.0",IBLIM = "0.0";
String V5V= "0.0", I5V = "0.0",P5V = "0.0",I5VLIM = "0.0";
String V3V3= "0.0", I3V3 = "0.0",P3V3 = "0.0",I3V3LIM = "0.0";
String values = "";

int menu = 0;
bool changeParam =false;
bool anErrorOcurred = false;
String errorCode = NORESPONDE;

uint8_t current_selection = 0;
int brightnessValue = 100;
int contrastValue = 190;
bool increase = false;
bool decrease = false;
bool modifyingParam = false;
bool modifyStep = false;
bool retrySendCommand = false;
uint8_t stepMultiplier = 1;

volatile float setPointVBuck = 4.0;
volatile float setPointIBuck = 1.0;
volatile float setPointI5V = 0.5;
volatile float setPointI3V3 = 0.15;

uint8_t numberOfProfiles = 0 ; 
String listOfProfiles = "";

// Menu list
const char *menu_list = 
          "Buck\n"
          "Profiles";
          

const char *buck_list =
          "Enable Buck\n"
          "Enable Aux\n"
          "Modify KP\n"
          "Modify KI";

const char *profile_list = 
          "Save current\n"
          "Load existing\n"
          "Delete all";

String  ajuste ="";


// Prototipes
void getAllData();
void showAllData();
void drawDisplaySettings(uint8_t param);
void drawHorizontalPorcentageBar(uint8_t timeNow);
void setBrightness(uint8_t value);
String getValue(String data, char separator, int index);
void createProfile(); 
void loadProfile(uint8_t profileNumber); 
uint8_t setCommand(uint8_t command, float value);
void setCommandWithoutData(uint8_t command);
String getCommand(uint8_t command);
byte getByteCommand(uint8_t command);

void setup() {
  while(!Serial);                       // wait until serial ready
  Serial.begin(115200);                 // start serial
  Serial.setTimeout(TIMEOUT);           // set timeout         

  pinMode(cancelBtn, INPUT_PULLUP);            // set pin to input
  pinMode(powerGood, INPUT_PULLUP);            // set pin to input
  pinMode(fault, INPUT_PULLUP);                // set pin to input
  pinMode(brightness, OUTPUT);          // sets the pin as output

  u8g2.begin(/*Select=*/ enterBtn, /*Right/Next=*/ rightBtn, /*Left/Prev=*/ leftBtn, /*Up=*/ A0, /*Down=*/ A3, /*Home/Cancel=*/ cancelBtn); // Btns

  u8g2.setFont(u8g2_font_6x12_tr);

  //EEPROM.get(BRIGHTNESS_ADDRESS , brightnessValue);
  //EEPROM.get(CONTRAST_ADDRESS , contrastValue);
  EEPROM.get(PROFILES_START_ADDRRESS , numberOfProfiles);
  //u8g2.setContrast(contrastValue);
  setBrightness(brightnessValue);

  // Attach the new PinChangeInterrupt and enable event function below
  attachPCINT(digitalPinToPCINT(fault), showError, CHANGE);
}

//01010000 

void loop() {

  getAllData();
  u8g2.firstPage();
  do
  {
    showAllData();
  } while (u8g2.nextPage());
  delay(30);

  if(anErrorOcurred){  
    errorCode = getByteCommand(GET_STATUS);     
    byte data = (byte)atoi(errorCode.c_str());   
    String status_buck = "Buck status: ";
    String status_5V =   "  5V status: ";
    String status_3V3 =  " 3V3 status: ";

    if (data != 255){
      if(bitRead(data, 0)){
        status_buck += "ERROR";
      }else{
        status_buck += "OK";
      }
  
      if(bitRead(data, 1)){
        status_5V += "ERROR";
      }else{
        status_5V += "OK";
      }
  
      if(bitRead(data, 2)){
        status_3V3 += "ERROR";
      }else{
        status_3V3 += "OK";
      }
    }else{
      status_buck += NORESPONDE;
      status_5V += NORESPONDE;
      status_3V3 += NORESPONDE;
    }
    
    u8g2.userInterfaceMessage(status_buck.c_str(),status_5V.c_str(),status_3V3.c_str(), "  Ok  ");
    setCommandWithoutData(ENABLE_BUCK);
    setCommandWithoutData(ENABLE_AUX);  
  }

}

void showError(){
  if(!digitalRead(fault)){  // fault D5
    anErrorOcurred =true;
  }else{
    anErrorOcurred =false;
  }
}

void showAllData(){
    u8g2.drawBox(0,0,117,10);
    u8g2.drawBox(108,8,9,64);
     
    u8g2.setColorIndex(0);

    u8g2.drawStr( 11, 8, "3V3");
    u8g2.drawStr( 49, 8, "5V");
    u8g2.drawStr( 85, 8, "B");
 
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

    // Ajuste
    u8g2.setColorIndex(0);
    u8g2.drawStr( 110, 8, (ajuste).c_str());
    u8g2.setColorIndex(1);
       
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
      }else{ // decrease selected value
        increase = false;
        decrease = true;
      }
    }else if(menuKey == U8X8_MSG_GPIO_MENU_PREV){
      if(!changeParam){
        menu--;
        if(menu < 0){
          menu = 4;
        }
      }else{ // increase selected value
        increase = true;
        decrease = false; // decrease
      }
    }else if(menuKey == U8X8_MSG_GPIO_MENU_SELECT){
      if(!modifyingParam){
        changeParam = !changeParam;
        modifyStep = false;
        modifyingParam =true;
        retrySendCommand = true;
        ajuste ="";
        switch(menu){
          case 0 : 
            retrySendCommand = false;
            break;
          
          case 1: // VB
            
            if(setCommand(SET_BUCK_VOLTAGE, setPointVBuck)){
              retrySendCommand =false;
            }else{
              retrySendCommand =true;
            }   
            
            break;
          case 2: // IB
            if(setCommand(SET_BUCK_CURRENT_LIMIT, setPointIBuck)){
              retrySendCommand =false;
            }else{
              retrySendCommand =true;
            }   
            break;
          case 3: // I5V
            if(setCommand(SET_5V_CURRENT_LIMIT, setPointI5V)){
              retrySendCommand =false;
            }else{
              retrySendCommand =true;
            }              
            break;
          case 4: // I3V3
            if(setCommand(SET_3V3_CURRENT_LIMIT, setPointI3V3)){
              retrySendCommand =false;
            }else{
              retrySendCommand =true;
            }              
            break;      
        }
            
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

        break;
      case 1:     // VB1
        if(changeParam){
          if(increase){
            setPointVBuck += 0.10 * stepMultiplier; 
          }
          if(decrease){
            setPointVBuck -= 0.10 * stepMultiplier; 
          }
          if(setPointVBuck > BUCKVMAX){
            setPointVBuck = BUCKVMAX;
          }
          if(setPointVBuck < BUCKVMIN){
            setPointVBuck = BUCKVMIN;
          }

          if(stepMultiplier == 1){
            ajuste ="F";
          }else{
            ajuste ="G";
          }

          if(increase || decrease || retrySendCommand){
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
            setPointIBuck += 0.010 * stepMultiplier; 
          }
          if(decrease){
            setPointIBuck -= 0.010 * stepMultiplier; 
          }
          if(setPointIBuck > BUCKCURRENTMAX){
            setPointIBuck = BUCKCURRENTMAX;
          }
          if(setPointIBuck < BUCKCURRENTMIN){
            setPointIBuck = BUCKCURRENTMIN;
          }

          if(stepMultiplier == 1){
            ajuste ="F";
          }else{
            ajuste ="G";
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
            setPointI5V += 0.010 * stepMultiplier; 
          }
          if(decrease){
            setPointI5V -= 0.010 * stepMultiplier; 
          }
          if(setPointI5V > I5VCURRENTMAX){
            setPointI5V = I5VCURRENTMAX;
          }
          if(setPointI5V < I5VCURRENTMIN){
            setPointI5V = I5VCURRENTMIN;
          }

          if(stepMultiplier == 1){
            ajuste ="F";
          }else{
            ajuste ="G";
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
            setPointI3V3 += 0.010 * stepMultiplier; 
          }
          if(decrease){
            setPointI3V3 -= 0.010 * stepMultiplier; 
          }
          if(setPointI3V3 > I3V3CURRENTMAX){
            setPointI3V3 = I3V3CURRENTMAX;
          }
          if(setPointI3V3 < I3V3CURRENTMIN){
            setPointI3V3 = I3V3CURRENTMIN;
          }
          
          if(stepMultiplier == 1){
            ajuste ="F";
          }else{
            ajuste ="G";
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
          switch(current_selection){
            case 1:   // enable buck 
              setCommandWithoutData(ENABLE_BUCK); 
              break;
            case 2:   // enable aux
              setCommandWithoutData(ENABLE_AUX); 
              break;                                    
          }
          break;
        case 2:
          while(current_selection){
              current_selection = u8g2.userInterfaceSelectionList(
                              "Profiles",
                              1, 
                              profile_list);
              switch(current_selection){
                case 1:   // Save profile
                  if(numberOfProfiles < PROFILES_ALLOWED){
                    saveProfile();
                  }else{
                    overwriteProfile(showProfiles());
                  }
                  break;
                case 2:   // Load profile
                  loadProfile(showProfiles());
                  break;
                case 3:   // Delete all
                  deleteAllProfiles();
                  break;                                     
              } 
          }                 
          break;
        case 3:  // About
        
          break;                
    }            
  }
}

/*
void drawDisplaySettings(uint8_t param){ // param = 1 -> Brightness | param = 2 -> Contrast
      
      if(!param){
        return -1; 
      }

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

      while(key != U8X8_MSG_GPIO_MENU_SELECT && key != U8X8_MSG_GPIO_MENU_HOME){
        key = u8g2.getMenuEvent();
        if(key == U8X8_MSG_GPIO_MENU_NEXT){
            switch(param){
              case 1:
                brightnessValue += 4;
                if(brightnessValue >= BRIGHTNESS_MAX){
                  brightnessValue = BRIGHTNESS_MAX;
                }
                setBrightness(brightnessValue);
                break;
              case 2:
                contrastValue += 2;
                if(contrastValue >= CONTRAST_MAX){
                  contrastValue = CONTRAST_MAX;
                }
                u8g2.setContrast(contrastValue);
                break;
            }
        }else if(key == U8X8_MSG_GPIO_MENU_PREV){
            switch(param){
              case 1:
                brightnessValue -= 4;
                setBrightness(brightnessValue);
                if(brightnessValue <= BRIGHTNESS_MIN){
                  brightnessValue = BRIGHTNESS_MIN;
                }
                break;
              case 2:
                contrastValue -= 2;
                if(contrastValue <= CONTRAST_MIN){
                  contrastValue = CONTRAST_MIN;
                }                
                u8g2.setContrast(contrastValue);
                break;
            }
        }else if(key == U8X8_MSG_GPIO_MENU_SELECT){
            switch(param){
              case 1:
                EEPROM.put(BRIGHTNESS_ADDRESS , brightnessValue);
                setBrightness(brightnessValue);
                break;
              case 2:
                EEPROM.put(CONTRAST_ADDRESS , contrastValue);
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
              u8g2.print("Contrast = " + String((contrastValue-CONTRAST_MIN)*100/(CONTRAST_MAX - CONTRAST_MIN)) + "%");              
              drawHorizontalPorcentageBar((contrastValue - CONTRAST_MIN) * 255/(CONTRAST_MAX - CONTRAST_MIN));
              break;               
          }

      } while( u8g2.nextPage() );
       delay(25);   
  }
}

void drawHorizontalPorcentageBar(uint8_t timeNow){
  u8g2.setColorIndex(1);
  u8g2.drawRFrame(12,20,101,16,4);
  u8g2.drawBox(13,21, timeNow*100/256,14);
}
*/

void getAllData(){
    String values = getCommand(GET_ALL_STRING);
    if(values !=  NORESPONDE){
      VBuck = getValue(values,';',0);
      IBuck = getValue(values,';',1);
      PBuck = getValue(values,';',2);
      IBLIM = getValue(values,';',3);
  
      V5V = getValue(values,';',4);
      I5V = getValue(values,';',5);
      P5V = getValue(values,';',6);
      I5VLIM = getValue(values,';',7);
     
      V3V3 = getValue(values,';',8);
      I3V3 = getValue(values,';',9);
      P3V3 = getValue(values,';',10);
      I3V3LIM = getValue(values,';',11);      
    }
}

String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

uint8_t showProfiles(){
    // save profiles in listOfProfiles
    listOfProfiles = "";
    if(numberOfProfiles > 0){
      for(uint8_t n = 0; n < numberOfProfiles ; n++){
        uint8_t startAddr = PROFILES_START_ADDRRESS + sizeof(uint8_t) +  n*sizeof(float)*4;
        float vBuck = 0.0;
        float iBuck = 0.0;
        float i5V = 0.0;
        float i3V3 = 0.0;
    
        EEPROM.get(startAddr,                   vBuck);
        EEPROM.get(startAddr+sizeof(float),     iBuck);
        EEPROM.get(startAddr+sizeof(float)*2,   i5V);
        EEPROM.get(startAddr+sizeof(float)*3,   i3V3);
        listOfProfiles += String(vBuck) + " " + String(iBuck)+ " " + String(i5V) +" " +  String(i3V3);
        if(n != (numberOfProfiles -1) ){
          listOfProfiles += "\n";     
        }
      }
      uint8_t selection = u8g2.userInterfaceSelectionList(
                          " List of profiles \n  VB   IB  I5V  I3V3  ",
                          1, 
                          listOfProfiles.c_str());
      if(!selection ){// cancel 
        selection = 255;                   
      }else{
        selection--;
      }
      return selection;
    }else{
      u8g2.userInterfaceMessage("", "No profiles created" ," " , " Ok ");
      return 255;
    }
}

void loadProfile(uint8_t profileNumber) 
{
    if(profileNumber == 255){
     return 0;
    }  
    uint8_t startAddr = PROFILES_START_ADDRRESS + sizeof(uint8_t) +  profileNumber*sizeof(float)*4;
    float vBuck = 0.0;
    float iBuck = 0.0;
    float i5V = 0.0;
    float i3V3 = 0.0;

    EEPROM.get(startAddr,                   vBuck);
    EEPROM.get(startAddr+sizeof(float),     iBuck);
    EEPROM.get(startAddr+sizeof(float)*2,   i5V);
    EEPROM.get(startAddr+sizeof(float)*3,   i3V3);

    // Check for invalid parameters. This happens when trying to load
    //  a profile that was never saved
    if(isnan(vBuck)){
      vBuck = BUCKVMIN;
    }
    if(isnan(iBuck)){
      iBuck = BUCKCURRENTMIN;
    }
    if(isnan(i5V)){
      i5V = I5VCURRENTMIN;
    }
    if(isnan(i3V3)){
      i3V3 = I3V3CURRENTMIN;
    }

    String profileName1 =  "  VB   IB  I5V  I3V3  ";
    String profileName2 =  String(vBuck) + " " + String(iBuck)+ " " + String(i5V) +" " +  String(i3V3) + " ?";  
    uint8_t selection =  u8g2.userInterfaceMessage("Are you sure to load", profileName1.c_str(),profileName2.c_str() , " Ok \n Cancel ");
    
    switch(selection){
      case OK_BTN:
        setPointVBuck = vBuck;
        setPointIBuck = iBuck;
        setPointI5V = i5V;
        setPointI3V3 = i3V3;
      
        setCommand(SET_BUCK_VOLTAGE,setPointVBuck);
        setCommand(SET_BUCK_CURRENT_LIMIT,setPointIBuck);
        setCommand(SET_5V_CURRENT_LIMIT,setPointI5V);
        setCommand(SET_3V3_CURRENT_LIMIT,setPointI3V3);
        u8g2.userInterfaceMessage("Profile loaded ", "correctly!","", " Ok "); 
        break;
    }
}


void saveProfile(){

  numberOfProfiles++;
  uint8_t startAddr = PROFILES_START_ADDRRESS + sizeof(uint8_t) + numberOfProfiles * sizeof(float)* 4 ; // profileNumber*sizeof(float)*6;
  String profileName1 =  "  VB   IB  I5V  I3V3  ";
  String profileName2 =  String(setPointVBuck) + " " + String(setPointIBuck)+ " " + String(setPointI5V) +" " +  String(setPointI3V3) + " ?";   
  uint8_t selection =  u8g2.userInterfaceMessage("Add new profile", profileName1.c_str(),profileName2.c_str() , " Ok \n Cancel ");
  switch(selection){
    case OK_BTN:
      EEPROM.put(startAddr,                   setPointVBuck);
      EEPROM.put(startAddr+sizeof(float),     setPointIBuck);
      EEPROM.put(startAddr+sizeof(float)*2,   setPointI5V);
      EEPROM.put(startAddr+sizeof(float)*3,   setPointI3V3);
      EEPROM.put(PROFILES_START_ADDRRESS,   numberOfProfiles);

      u8g2.userInterfaceMessage("Profile saved ", "correctly!","", " Ok ");
      break;
    case CANCEL_BTN:
      numberOfProfiles--;
      break;
  }
}

void overwriteProfile(uint8_t order){
  if(order == 255){
    return 0;
  }
  uint8_t startAddr = PROFILES_START_ADDRRESS + sizeof(uint8_t) + order * sizeof(float)* 4 ; // profileNumber*sizeof(float)*6;

  float vBuck = 0.0;
  float iBuck = 0.0;
  float i5V = 0.0;
  float i3V3 = 0.0;

  EEPROM.get(startAddr,                   vBuck);
  EEPROM.get(startAddr+sizeof(float),     iBuck);
  EEPROM.get(startAddr+sizeof(float)*2,   i5V);
  EEPROM.get(startAddr+sizeof(float)*3,   i3V3);

  String profileName1 =  "  VB   IB  I5V  I3V3  ";
  String profileName2 =  String(vBuck) + " " + String(iBuck)+ " " + String(i5V) +" " +  String(i3V3) + " ?"; 
  uint8_t selection =  u8g2.userInterfaceMessage(" Overwrite profile: ", profileName1.c_str() ,profileName2.c_str() , " Ok \n Cancel ");

  switch(selection){
    case OK_BTN:
      EEPROM.put(startAddr,                   setPointVBuck);
      EEPROM.put(startAddr+sizeof(float),     setPointIBuck);
      EEPROM.put(startAddr+sizeof(float)*2,   setPointI5V);
      EEPROM.put(startAddr+sizeof(float)*3,   setPointI3V3);
      u8g2.userInterfaceMessage("Profile overwrited ", "correctly!","", " Ok ");
      break;
  }
}

void deleteAllProfiles(){
  if(!numberOfProfiles){
    u8g2.userInterfaceMessage("", "No profiles created" ," " , " Ok ");
  }else{
    uint8_t selection = u8g2.userInterfaceMessage("Are you sure to ", "delete all existing"," profiles? ", " Ok \n Cancel");
    if(selection == OK_BTN){
        EEPROM.put(PROFILES_START_ADDRRESS,   0);
        uint8_t selection = u8g2.userInterfaceMessage("Profiles deleted ", "sucessfully","", " Ok ");
        numberOfProfiles = 0;
    }    
  }
}

void setBrightness(uint8_t value){
  analogWrite(brightness, value);  // analogRead values go from 0 to 1023, analogWrite values from 0 to 255
}

uint8_t setCommand(uint8_t command, float value){
  flushSerial();
  Serial.write(command);  
  uint8_t counter = 0;
  while(counter <= TIMEOUT &&  Serial.available() == 0){
     delayMicroseconds(1000);
     counter++;
  }
  //Serial.print("\n timeout value: " + String(counter) + "ms  value:"+ String(value) +"\n");
  if(Serial.read() == ACK ){    
    Serial.print(value,2);
    Serial.write('\0');
    return 1;      
  }
  return 0; 
}

void setCommandWithoutData(uint8_t command){
  flushSerial();
  Serial.write(command);
  uint8_t counter = 0;
  while(counter <= TIMEOUT &&  Serial.available() == 0){
     delayMicroseconds(1000);
     counter++;
  }
  if(Serial.read() == ACK ){    
      
  } 
}

String getCommand(uint8_t command){
  flushSerial();
  Serial.write(command);
  uint8_t counter = 0;
  //timeNow = micros()*1000;
  while(counter <= 100 &&  Serial.available() == 0){
     delayMicroseconds(1000);
     counter++;
  }
  if(Serial.read()== ACK ){
     return Serial.readStringUntil('\0');
  } else{
     return NORESPONDE;
  }
}

byte getByteCommand(uint8_t command){
  flushSerial();
  Serial.write(command);
  uint8_t counter = 0;
  //timeNow = micros()*1000;
  while(counter <= 100 &&  Serial.available() == 0){
     delayMicroseconds(1000);
     counter++;
  }
  if(Serial.read()== ACK ){
     byte a[1];
     Serial.readBytes(a, 1);
     return a[0];
  } else{
     return 255;
  }
}

void flushSerial(){
  while(Serial.available() > 0) {
    char t = Serial.read();
  }
}

