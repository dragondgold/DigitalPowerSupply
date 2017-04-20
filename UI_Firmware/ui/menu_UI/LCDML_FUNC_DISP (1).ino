/* ===================================================================== *
 *                                                                       *
 * DISPLAY SYSTEM                                                        *
 *                                                                       *
 * ===================================================================== *
 * every "disp menu function" needs three functions 
 * - void LCDML_DISP_setup(func_name)    
 * - void LCDML_DISP_loop(func_name)     
 * - void LCDML_DISP_loop_end(func_name)
 *
 * EXAMPLE CODE:
    void LCDML_DISP_setup(..menu_func_name..) 
    {
      // setup
      // is called only if it is started

      // starts a trigger event for the loop function every 100 millisecounds
      LCDML_DISP_triggerMenu(100);  
    }
    
    void LCDML_DISP_loop(..menu_func_name..)
    { 
      // loop
      // is called when it is triggert
      // - with LCDML_DISP_triggerMenu( millisecounds ) 
      // - with every button status change

      // check if any button is presed (enter, up, down, left, right)
      if(LCDML_BUTTON_checkAny()) {         
        LCDML_DISP_funcend();
      } 
    }
    
    void LCDML_DISP_loop_end(..menu_func_name..)
    {
      // loop end
      // this functions is ever called when a DISP function is quit
      // you can here reset some global vars or do nothing  
    } 
 * ===================================================================== * 
 */
#include <LCDMenuLib.h>
#include <math.h>

// *********************************************************************
void LCDML_DISP_setup(LCDML_FUNC_BACK)
// *********************************************************************
{
    LCDML_DISP_funcend();
}

void LCDML_DISP_loop(LCDML_FUNC_BACK){} 

void LCDML_DISP_loop_end(LCDML_FUNC_BACK)
{  
  LCDML.goBack();
}  

// *********************************************************************
void LCDML_DISP_setup(LCDML_FUNC_ENDIS)
// *********************************************************************
{
    // use parameter on menu init
    param = LCDML_DISP_getParameter();
    switch(param){
      
      case 0: // TODO
        if(ISAUXENABLE && ISB1ENABLE && ISB2ENABLE){
          setCommandSinDatos(DISABLE_BUCK1);ISB1ENABLE = 0;
          setCommandSinDatos(DISABLE_BUCK2);ISB2ENABLE = 0;
          setCommandSinDatos(DISABLE_AUX);ISAUXENABLE = 0;
        }else{
          setCommandSinDatos(ENABLE_BUCK1);ISB1ENABLE = 1;
          setCommandSinDatos(ENABLE_BUCK2);ISB2ENABLE = 1;
          setCommandSinDatos(ENABLE_AUX);ISAUXENABLE = 1;       
        }
        break;
          
      case 1: // B1
        if(ISB1ENABLE){
          setCommandSinDatos(DISABLE_BUCK1);ISB1ENABLE = 0;
        }else{
          setCommandSinDatos(ENABLE_BUCK1);ISB1ENABLE =1;
        }
        break; 
        
      case 2: // B2 
        if(ISB2ENABLE){
          setCommandSinDatos(DISABLE_BUCK2);ISB2ENABLE = 0;
        }else{
          setCommandSinDatos(ENABLE_BUCK2);ISB2ENABLE =1;        
        }
        break;
          
      case 3: // 5V Y 3V3
        if(ISAUXENABLE){
          setCommandSinDatos(DISABLE_AUX);ISAUXENABLE = 0;
        }else{
          setCommandSinDatos(ENABLE_AUX);ISAUXENABLE = 1; 
        }      
        break;
        
      case 4: // en/dis PID2
        if(isRunningPIDBuck2){
          setCommandSinDatos(DISABLE_B2_PID);isRunningPIDBuck2 = 0;
        }else{
          setCommandSinDatos(ENABLE_B2_PID);isRunningPIDBuck2 = 1; 
        }      
        break;        
    }
    
}

void LCDML_DISP_loop(LCDML_FUNC_ENDIS){
  LCDML_DISP_funcend();
} 

void LCDML_DISP_loop_end(LCDML_FUNC_ENDIS)
{  
  LCDML.goBack();
}  



// *********************************************************************
void LCDML_DISP_setup(LCDML_FUNC_PERFILES)
// *********************************************************************
{
    param = LCDML_DISP_getParameter();
 
    switch(param){
      case 0: // Guardar perfiles
      case 1:
      case 2:
        Serial.println("S1");
        saveProfile(param);
        Serial.println("S2");
        break;
          
      case 3: // Cargar perfiles 
      case 4:
      case 5:
        loadProfile(param - 3);
        break; 
    }

          
}

void LCDML_DISP_loop(LCDML_FUNC_PERFILES){  
  LCDML_DISP_funcend(); 
} 

void LCDML_DISP_loop_end(LCDML_FUNC_PERFILES)
{  
  LCDML.goBack();
}  

/**
 * Save the current settings in the specified profile.
 */
void saveProfile(uint8_t profileNumber)
{  
    uint8_t startAddr = PROFILES_START_ADDRRESS + profileNumber*sizeof(float)*6;
  
    EEPROM.put(startAddr,                   setPoint2);
    EEPROM.put(startAddr+sizeof(float),     SETIB2LIM);
    EEPROM.put(startAddr+sizeof(float)*2,   setPoint1);
    EEPROM.put(startAddr+sizeof(float)*3,   SETIB1LIM);
    EEPROM.put(startAddr+sizeof(float)*4,   SETI5VLIM);
    EEPROM.put(startAddr+sizeof(float)*5,   SETI3V3LIM);
}

void loadProfile(uint8_t profileNumber) 
{
    uint8_t startAddr = PROFILES_START_ADDRRESS + profileNumber*sizeof(float)*6;
    
    EEPROM.get(startAddr,                   setPoint2);
    EEPROM.get(startAddr+sizeof(float),     SETIB2LIM);
    EEPROM.get(startAddr+sizeof(float)*2,   setPoint1);
    EEPROM.get(startAddr+sizeof(float)*3,   SETIB1LIM);
    EEPROM.get(startAddr+sizeof(float)*4,   SETI5VLIM);
    EEPROM.get(startAddr+sizeof(float)*5,   SETI3V3LIM);

    // Check for invalid parameters. This happens when trying to load
    //  a profile that was never saved
    if(isnan(setPoint2)){
      setPoint2 = 0.0;
    }
    if(isnan(SETIB2LIM)){
      SETIB2LIM = BUCKCURRENTMIN;
    }
    if(isnan(setPoint1)){
      setPoint1 = 0.0;
    }
    if(isnan(SETIB1LIM)){
      SETIB1LIM = BUCKCURRENTMIN;
    }
    if(isnan(SETI5VLIM)){
      SETI5VLIM = I5VCURRENTMIN;
    }
    if(isnan(SETI3V3LIM)){
      SETI3V3LIM = I3V3CURRENTMIN;
    }

    setCommand(SET_BUCK2_VOLTAGE_COMMAND, setPoint2);
    setCommand(SET_BUCK2_CURRENT_LIMIT,   SETIB2LIM);
    setCommand(SET_BUCK1_VOLTAGE_COMMAND, setPoint1);
    setCommand(SET_BUCK1_CURRENT_LIMIT,   SETIB1LIM);
    setCommand(SET_5V_CURRENT_LIMIT,      SETI5VLIM);
    setCommand(SET_3V3_CURRENT_LIMIT,     SETI3V3LIM);
}

// *********************************************************************
void LCDML_DISP_setup(LCDML_FUNC_PANTALLA)
// *********************************************************************
{
    LCDML_BUTTON_resetEnter();
    param = LCDML_DISP_getParameter();

    EEPROM.get(BRIGHTNESS_ADDRESS , brillo);
    EEPROM.get(CONTRAST_ADDRESS, contraste);
    
    uint8_t heigthFrame = 14;
    u8g.firstPage();  
    do {
      u8g.setColorIndex(1);
      u8g.drawBox(0,0,128,heigthFrame-1);
      u8g.drawFrame(0,heigthFrame,128,_LCDML_u8g_lcd_h - heigthFrame);
      u8g.setColorIndex(0);

      if(param == 0) {
        u8g.setPrintPos(1, heigthFrame-4);
        u8g.print("Brillo = %");
        u8g.print(brillo*100/255);  
      }  
      else {
        u8g.setPrintPos(1, heigthFrame-4);
        u8g.print("Contraste = %");
        u8g.print(contraste*100/255);      
      }
      u8g.setColorIndex(1);
      
      switch(param){  
        case 0: // BRILLO
          useHorizontalBar(brillo);
          break;         
        case 1: // CONTRASTE
          useHorizontalBar(contraste);
          break;               
      }
  } while( u8g.nextPage() );  
}

void LCDML_DISP_loop(LCDML_FUNC_PANTALLA){
    uint8_t heigthFrame = 14;
    int8_t percentChange = 0;

    if (LCDML_BUTTON_checkDown()) // check if button left is pressed
    {
      LCDML_BUTTON_resetDown();
      percentChange = -2;
    }
    else if(LCDML_BUTTON_checkUp()) {
      LCDML_BUTTON_resetUp();
      percentChange = 2;
    }
    else {
      percentChange = 0;
    }
    
    if (percentChange != 0)
    {
      switch(param) {  
        case 0: // BRILLO
          if(brillo < 10){
            brillo = 10;
          } else {
            brillo = brillo + (percentChange*255/100);
          }
          analogWrite(brightness, brillo);  // analogRead values go from 0 to 1023, analogWrite values from 0 to 255 
          EEPROM.put(BRIGHTNESS_ADDRESS , brillo);
          break;         
        case 1: // CONTRASTE
          if(contraste < 2){
            contraste = 2;
          } else {
            contraste = contraste + (percentChange*255/100);
          }
          u8g.setContrast(contraste);
          EEPROM.put(CONTRAST_ADDRESS, contraste);
          break;               
      }
      
      // Update lcd content
      u8g.firstPage();  
      do {
        u8g.setColorIndex(1); // Instructs the display to draw with a pixel on.
        u8g.drawBox(0,0,128,heigthFrame-1);
        u8g.drawFrame(0,heigthFrame,128,_LCDML_u8g_lcd_h -heigthFrame);
        u8g.setColorIndex(0); // Instructs the display to draw with a pixel on.
        
        if(param == 0) {
          u8g.setPrintPos(1, heigthFrame-4);
          u8g.print("Brillo = %");
          u8g.print(brillo*100/255);  
        }  
        else {
          u8g.setPrintPos(1, heigthFrame-4);
          u8g.print("Contraste = %");
          u8g.print(contraste*100/255);      
        }
           
        u8g.setColorIndex(1); // Instructs the display to draw with a pixel on.
        
        switch(param){  
          case 0: // BRILLO
            useHorizontalBar(brillo);
            break;         
          case 1: // CONTRASTE
            useHorizontalBar(contraste);
            break;               
        }
      } while( u8g.nextPage() );         
    }
    
    if (LCDML_BUTTON_checkEnter()) // check if button left is pressed
    {
      LCDML_BUTTON_resetEnter();
      LCDML_DISP_funcend();       
    } 
}

void LCDML_DISP_loop_end(LCDML_FUNC_PANTALLA){
  LCDML.goBack();
  } 
 

// *********************************************************************
void LCDML_DISP_setup(LCDML_FUNC_MAIN)
// *********************************************************************
{
    LCDML_BUTTON_resetEnter();
    menu = 0;
    updateParam(); 
    drawParam();
    LCDML_DISP_triggerMenu(20);
}


void LCDML_DISP_loop(LCDML_FUNC_MAIN) 
{    
  if (LCDML_BUTTON_checkDown()) // check if button left is pressed
  {
    LCDML_BUTTON_resetDown(); // reset the left 
    if(!changeParam) {
      menu++;
      if(menu > 6) menu = 0;
    } else{
      switch(menu){
        case 1:   // VB2
          if(giroRapido){
            setPoint2 = setPoint2 - HIGHSTEP;
          }else{
            setPoint2 = setPoint2 - LOWSTEP;                
          }
  
          if(setPoint2 < setPointMin) setPoint2 = setPointMin;
          break;
        case 2:   //IB2
          if(giroRapido){
            SETIB2LIM = SETIB2LIM - HIGHSTEP;
          }else{
            SETIB2LIM = SETIB2LIM - LOWSTEP;                
          }            
          if(SETIB2LIM < BUCKCURRENTMIN) SETIB2LIM = BUCKCURRENTMIN;
          break;
        case 3:   //VB1
          if(giroRapido){
            setPoint1 = setPoint1 - HIGHSTEP;
          }else{
            setPoint1 = setPoint1 - LOWSTEP;                
          }            
          if(setPoint1 < setPointMin) setPoint1 = setPointMin;
          break;
        case 4:   //IB1
          if(giroRapido){
            SETIB1LIM = SETIB1LIM - HIGHSTEP;
          }else{
            SETIB1LIM = SETIB1LIM - LOWSTEP;                
          }            
          if(SETIB1LIM < BUCKCURRENTMIN) SETIB1LIM = BUCKCURRENTMIN;
          break;
        case 5:   //I5V         
          SETI5VLIM = SETI5VLIM - LOWSTEP;
          if(SETI5VLIM < I5VCURRENTMIN) SETI5VLIM = I5VCURRENTMIN;
          break;
        case 6:   //I3V3
          SETI3V3LIM = SETI3V3LIM - LOWSTEP;           
          if(SETI3V3LIM < I3V3CURRENTMIN) SETI3V3LIM = I3V3CURRENTMIN;
          break;
      }
    }     
  }        
  
  if (LCDML_BUTTON_checkUp()) // check if button left is pressed
  {
    LCDML_BUTTON_resetUp(); // reset the left button
  
    if(!changeParam){
      if(menu == 0){
        menu = 6;  
      }else{
        menu--;
      } 
    }else{
      switch(menu){
        case 1:  // VB2
          if(giroRapido){
            setPoint2 = setPoint2 + HIGHSTEP;
          }else{
            setPoint2 = setPoint2 + LOWSTEP;
          }         
          if(setPoint2 > setPointMax) setPoint2 = setPointMax;
          break;
        case 2:  //IB2
          if(giroRapido){
            SETIB2LIM = SETIB2LIM + HIGHSTEP;
          }else{
            SETIB2LIM = SETIB2LIM + LOWSTEP;                
          }     
          if(SETIB2LIM > BUCKCURRENTMAX) SETIB2LIM = BUCKCURRENTMAX;
          break;
        case 3:   //VB1
          if(giroRapido){
            setPoint1 = setPoint1 + HIGHSTEP;
          }else{
            setPoint1 = setPoint1 + LOWSTEP;                
          }     
          if(setPoint1 > setPointMax) setPoint1 = setPointMax;
          break;
        case 4:   // IB1
          if(giroRapido){
            SETIB1LIM = SETIB1LIM + HIGHSTEP;
          }else{
            SETIB1LIM = SETIB1LIM + LOWSTEP;                
          }     
          if(SETIB1LIM > BUCKCURRENTMAX) SETIB1LIM = BUCKCURRENTMAX;
          break;
        case 5:   // I5V
          SETI5VLIM = SETI5VLIM + LOWSTEP;     
          if(SETI5VLIM > I5VCURRENTMAX) SETI5VLIM = I5VCURRENTMAX;
          break;              
        case 6:   // I3V3 
          SETI3V3LIM = SETI3V3LIM + LOWSTEP;
          if(SETI3V3LIM > I3V3CURRENTMAX) SETI3V3LIM = I3V3CURRENTMAX;
          break;
      }     
    }
  } 
  
  if (LCDML_BUTTON_checkEnter()) // check if button left is pressed
  {
    LCDML_BUTTON_resetEnter();
    if(error) {
      if(bufError2.indexOf("5V") >=0 || bufError2.indexOf("3.3V") >=0){
        setCommandSinDatos(ENABLE_AUX);
        ISAUXENABLE = 1;
      } else if(bufError2.indexOf("B1") >= 0){
        setCommandSinDatos(ENABLE_BUCK1);
        ISB1ENABLE = 1;
      } else if(bufError2.indexOf("B2") >= 0){
        setCommandSinDatos(ENABLE_BUCK2);
        ISB2ENABLE = 1;
      }
      error = 0;
      LCDML_DISP_funcend();
    } else{
      if(!changeParam){
        if(menu == 0){
          LCDML_DISP_funcend();
        } else{
          changeParam = true;
          giroRapido = 1;
        }
      } 
      else if(!giroRapido && changeParam) {
         switch(menu){
            case 0:
              LCDML_DISP_funcend();
              break;
            case 1:   // VB2
              if(changeParam){
                setCommand(SET_BUCK2_VOLTAGE_COMMAND, setPoint2);
                changeParam = false;
              }else {
                changeParam = true;
              }
              break;  
            case 2:   // IB2  
              if(changeParam){
                setCommand(SET_BUCK2_CURRENT_LIMIT, SETIB2LIM);
                changeParam = false;
              }else {
                changeParam = true;
              }
              break;
            case 3:   // VB1
              if(changeParam){
                setCommand(SET_BUCK1_VOLTAGE_COMMAND, setPoint1);
                changeParam = false;
              }else {
                changeParam = true;
              }
              break;
            case 4:   // IB1
              if(changeParam){
                setCommand(SET_BUCK1_CURRENT_LIMIT, SETIB1LIM);
                changeParam = false;
              }else {
                changeParam = true;
              }
              break;
            case 5:   // I5V
              if(changeParam){
                setCommand(SET_5V_CURRENT_LIMIT, SETI5VLIM);
                changeParam = false;
              }else {
                changeParam = true;
              }
              break;
            case 6:   //I3V3
              if(changeParam){
                setCommand(SET_3V3_CURRENT_LIMIT, SETI3V3LIM);
                changeParam = false;
              }else {
                changeParam = true;
              }
              break;                  
          }
      }
      else if(menu != 0 && changeParam ){
        giroRapido = !giroRapido;
      }       
    }
  }

  if((millis() - lastParamUpdate) >= UPDATE_PARAMS_EVERY) {
    updateParam();
    lastParamUpdate = millis();
  }
  else if(!drawing){
     drawParam();
  }
}

void LCDML_DISP_loop_end(LCDML_FUNC_MAIN)
{
  // this functions is ever called when a DISP function is quit
  // you can here reset some global va
}

// *********************************************************************
//  UTILITIES
// *********************************************************************
 void updateParam(){
     //cli(); //stop interrupts happening before we read pin values  
    (value = getCommand(GET_BUCK1_VOLTAGE_COMMAND));if(value != NORESPONDE) VBuck1=value;else VBuck1=VBuck1;
    (value = getCommand(GET_BUCK1_CURRENT));if(value != NORESPONDE) IBuck1=value;else IBuck1=IBuck1;

    (value = getCommand(GET_BUCK2_VOLTAGE_COMMAND));if(value != NORESPONDE) VBuck2=value;else VBuck2=VBuck2;
    (value = getCommand(GET_BUCK2_CURRENT));if(value != NORESPONDE) IBuck2=value;else IBuck2=IBuck2;
    
    (value = getCommand(GET_5V_VOLTAGE));if(value != NORESPONDE) V5V=value;else V5V=V5V;
    (value = getCommand(GET_5V_CURRENT));if(value != NORESPONDE) I5V=value;else I5V=I5V;
       
    (value = getCommand(GET_3V3_VOLTAGE));if(value != NORESPONDE) V3V3=value;else V3V3=V3V3;
    (value = getCommand(GET_3V3_CURRENT));if(value != NORESPONDE) I3V3=value;else I3V3=I3V3;

    PBuck1= VBuck1.toFloat()*IBuck1.toFloat();
    PBuck2= VBuck2.toFloat()*IBuck2.toFloat();
    P5V= V5V.toFloat()*I5V.toFloat();
    P3V3= V3V3.toFloat()*I3V3.toFloat();  
}

void useHorizontalBar(uint8_t var){
    u8g.drawRFrame(12,20,101,16,4);
    u8g.drawBox(13,21, var*100/255,14);
}

void draw() {
  if(error){
    drawError();
  }
  else{
    u8g.drawBox(0,0,128,10);
    u8g.drawBox(120,10,13,64);
    
    u8g.setColorIndex(0);
    u8g.drawStr( 4, 8, ("3V3"));
    u8g.drawStr( 35, 8, ("5V"));
    u8g.drawStr( 69, 8, ("B1"));
    u8g.drawStr( 99, 8, ("B2"));  
      
    u8g.drawStr( 122, 20, ("V"));
    u8g.drawStr( 122, 33, ("A"));
    u8g.drawStr( 122, 46, ("W"));
    u8g.drawStr( 122, 59, ("L"));
  
    u8g.setColorIndex(0);
    u8g.drawStr( 122, 8, "M");
    u8g.drawRFrame(120,0,9,10,2);
    u8g.setColorIndex(1);
    
    u8g.drawLine(27,10,27,64);
    u8g.drawLine(54,10,54,64);
    u8g.drawLine(86,10,86,64);
    
    u8g.drawLine(0,10,0,64);
    u8g.drawLine(0,63,128,63);

    // V3V3
    dtostrf(SETI3V3LIM,3, 2, buf);
    u8g.drawStr( 2, 20, (V3V3).c_str());
    u8g.drawStr( 2, 33, (I3V3).c_str());
    u8g.drawStr( 2, 46, (P3V3).c_str());
    u8g.drawStr( 2, 59, buf);
  
    // V5V
    dtostrf(SETI5VLIM,3, 2, buf);
    u8g.drawStr( 29, 20, (V5V).c_str());
    u8g.drawStr( 29, 33, (I5V).c_str());
    u8g.drawStr( 29, 46, (P5V).c_str());
    u8g.drawStr( 29, 59,buf);
  
    // Buck 1 
    dtostrf(SETIB1LIM,4, 2, buf);
    u8g.drawStr( 57, 20, (VBuck1).c_str());
    u8g.drawStr( 57, 33, (IBuck1).c_str());
    u8g.drawStr( 57, 46, (PBuck1).c_str());
    u8g.drawStr( 57, 59, buf);
    
    
    // Buck 2
    dtostrf(SETIB2LIM,4, 2, buf);
    u8g.drawStr( 89, 20, (VBuck2).c_str());
    u8g.drawStr( 89, 33, (IBuck2).c_str());
    u8g.drawStr( 89, 46, (PBuck2).c_str());
    u8g.drawStr( 89, 59, buf);
  
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
  }
}

void drawError(){
  u8g.drawBox(0, 0, 128, 12);
  u8g.setColorIndex(0);
  u8g.drawStr( 7, 10, "Un error ocurrio...");

  u8g.setColorIndex(1); 
  u8g.drawFrame(0, 12, 128, 64-12);
  u8g.setPrintPos(6, 24);
  u8g.print(bufError1.c_str());

  u8g.setPrintPos(6, 38);
  u8g.print(bufError2.c_str());
  u8g.drawStr(12, 60, "Habilitar Salida?" );
  u8g.drawHLine(0, 49, 128);
}

void drawParam() {
    u8g.firstPage();
    drawing = true;
    do {
      draw();
    } while( u8g.nextPage() ); 
    //updateParam(); 
    drawing = false;
}

