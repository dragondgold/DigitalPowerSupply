// Include libs
#include <LCDMenuLib.h>
#include <U8glib.h>
#include <EEPROM.h>
#include <stdio.h>
#include "RotaryEncoder.h"
//#include <EnableInterrupt.h>
#include <PinChangeInterrupt.h>

// Lib config
#define _LCDML_DISP_cfg_button_press_time          190    // button press time in ms

// *********************************************************************
// U8GLIB
// *********************************************************************
// https://github.com/olikraus/u8glib/wiki/device
#define cs                10
#define sck               13
#define mosi              11
#define a0                9
#define reset             8
#define brightness        6

#define encoderPhaseA     2
#define encoderPhaseB     3

#define alerta            5

U8GLIB_NHD_C12864_2X u8g(sck, mosi, cs, a0 , reset);  // Software SPI
//U8GLIB_NHD_C12864_2X u8g(cs, a0 , reset);           // Hardware SPI (produce glitches en el display en nuestro caso)

// settings for u8g lib and lcd
#define _LCDML_u8g_lcd_w       128            // lcd width
#define _LCDML_u8g_lcd_h       64             // lcd height
#define _LCDML_u8g_font        u8g_font_6x12  // u8glib font (more fonts under u8g.h line 1520 ...)
#define _LCDML_u8g_font_w      6              // u8glib font width
#define _LCDML_u8g_font_h      13             // u8glib font heigt

// nothing change here
#define _LCDML_u8g_cols_max    (_LCDML_u8g_lcd_w/_LCDML_u8g_font_w)  
#define _LCDML_u8g_rows_max    (_LCDML_u8g_lcd_h/_LCDML_u8g_font_h) 

// rows and cols 
// when you use more rows or cols as allowed change in LCDMenuLib.h the define "_LCDML_DISP_cfg_max_rows" and "_LCDML_DISP_cfg_max_string_length"
// the program needs more ram with this changes
#define _LCDML_u8g_rows        _LCDML_u8g_rows_max  // max rows 
#define _LCDML_u8g_cols        20                   // max cols

// scrollbar width
#define _LCDML_u8g_scrollbar_w 6  // scrollbar width  


// old defines with new content
#define _LCDML_DISP_cols      _LCDML_u8g_cols
#define _LCDML_DISP_rows      _LCDML_u8g_rows  

// List of Commands
#define SET_BUCK1_VOLTAGE_COMMAND       1
#define GET_BUCK1_VOLTAGE_COMMAND       2
#define GET_BUCK1_CURRENT               3

#define SET_BUCK2_VOLTAGE_COMMAND       4
#define GET_BUCK2_VOLTAGE_COMMAND       5
#define GET_BUCK2_CURRENT               7

#define SET_BUCK1_KP                    8
#define SET_BUCK1_KI                    9

#define SET_BUCK2_KP                    10
#define SET_BUCK2_KI                    11

#define DISABLE_B1_PID                  12
#define ENABLE_B1_PID                   13

#define DISABLE_B2_PID                  14
#define ENABLE_B2_PID                   15

#define GET_5V_VOLTAGE                  16
#define GET_3V3_VOLTAGE                 17 
#define GET_5V_CURRENT                  18
#define GET_3V3_CURRENT                 19

#define GET_RTC_TIME                    20
#define GET_RTC_DATE                    21

#define SET_BUCK1_CURRENT_LIMIT         22
#define SET_BUCK2_CURRENT_LIMIT         23
#define SET_5V_CURRENT_LIMIT            24
#define SET_3V3_CURRENT_LIMIT           25

#define ENABLE_BUCK1                    26
#define ENABLE_BUCK2                    27
#define ENABLE_AUX                      28
#define DISABLE_BUCK1                   29
#define DISABLE_BUCK2                   30
#define DISABLE_AUX                     31

#define GET_B1_POWER                    33
#define GET_B2_POWER                    34
#define GET_5V_POWER                    35
#define GET_3V3_POWER                   36

#define SET_RTC_TIME                    37
#define SET_RTC_DATE                    38

#define SHUTDOWN                        32

#define GET_STATUS                      33

#define ACK                             6

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

#define LOWSTEP                         0.1
#define HIGHSTEP                        1

#define LIMITEGIROINF                   30    // us
#define LIMITEGIRO                      241   // ms                                       
#define LIMITEGIROSUP                   550   // ms 

#define ELEMENTOSENMENUDEPARAM          6     // elementos 
#define UPDATE_PARAMS_EVERY             1000  // 1000ms

#define BRIGHTNESS_ADDRESS              0
#define CONTRAST_ADDRESS                1
#define PROFILES_START_ADDRRESS         2
         
// *********************************************************************
// LCDML MENU/DISP
// *********************************************************************
// create menu
// menu element count - last element id
// this value must be the same as the last menu element
#define _LCDML_DISP_cnt    19

// LCDML_root        => layer 0 
// LCDML_root_X      => layer 1 
// LCDML_root_X_X    => layer 2 
// LCDML_root_X_X_X  => layer 3 
// LCDML_root_... 	 => layer ... 

// LCDMenuLib_add(id, group, prev_layer_element, new_element_num, lang_char_array, callback_function)
LCDML_DISP_initParam(_LCDML_DISP_cnt);
LCDML_DISP_add      (0  , _LCDML_G1   , LCDML_root        , 1 , "Mostrar Salidas"     , LCDML_FUNC_MAIN);  
LCDML_DISP_add      (1  , _LCDML_G1   , LCDML_root        , 2 , "Estados"             , LCDML_FUNC);
LCDML_DISP_add      (2  , _LCDML_G1   , LCDML_root        , 3 , "Perfiles"            , LCDML_FUNC);
LCDML_DISP_add      (3  , _LCDML_G1   , LCDML_root        , 4 , "Pantalla"            , LCDML_FUNC);

LCDML_DISP_addParam (4  , _LCDML_G1  , LCDML_root_2      , 1 , "Habil/Desab todo"    , LCDML_FUNC_ENDIS, 0);
LCDML_DISP_addParam (5  , _LCDML_G1  , LCDML_root_2      , 2 , "Habil/Desab B1"      , LCDML_FUNC_ENDIS, 1);
LCDML_DISP_addParam (6  , _LCDML_G1  , LCDML_root_2      , 3 , "Habil/Desab B2"      , LCDML_FUNC_ENDIS, 2);
LCDML_DISP_addParam (7  , _LCDML_G1  , LCDML_root_2      , 4 , "Habil/Desab Aux"     , LCDML_FUNC_ENDIS, 3);
LCDML_DISP_addParam (8  , _LCDML_G1  , LCDML_root_2      , 5 , "Habil/Desab PID B2"  , LCDML_FUNC_ENDIS, 4);
LCDML_DISP_add      (9  , _LCDML_G1  , LCDML_root_2      , 6 , "Volver"              , LCDML_FUNC_BACK);

LCDML_DISP_addParam (10  , _LCDML_G1   , LCDML_root_3      , 1 , "Guardar Perfil 1"    , LCDML_FUNC_PERFILES, 0);
LCDML_DISP_addParam (11  , _LCDML_G1   , LCDML_root_3      , 2 , "Guardar Perfil 2"    , LCDML_FUNC_PERFILES, 1);
LCDML_DISP_addParam (12  , _LCDML_G1   , LCDML_root_3      , 3 , "Guardar Perfil 3"    , LCDML_FUNC_PERFILES, 2);
LCDML_DISP_addParam (13  , _LCDML_G1   , LCDML_root_3      , 4 , "Cargar Perfil 1"     , LCDML_FUNC_PERFILES, 3);
LCDML_DISP_addParam (14  , _LCDML_G1   , LCDML_root_3      , 5 , "Cargar Perfil 2"     , LCDML_FUNC_PERFILES, 4);
LCDML_DISP_addParam (15  , _LCDML_G1   , LCDML_root_3      , 6 , "Cargar Perfil 3"     , LCDML_FUNC_PERFILES, 5);
LCDML_DISP_add      (16  , _LCDML_G1   , LCDML_root_3      , 7 , "Volver"              , LCDML_FUNC_BACK);

LCDML_DISP_addParam (17 , _LCDML_G1  , LCDML_root_4    , 1  , "Brillo"             , LCDML_FUNC_PANTALLA, 0);    
LCDML_DISP_addParam (18 , _LCDML_G1  , LCDML_root_4    , 2  , "Contraste"          , LCDML_FUNC_PANTALLA, 1);
LCDML_DISP_add      (19 , _LCDML_G1  , LCDML_root_4    , 3  , "Volver"            , LCDML_FUNC_BACK);

LCDML_DISP_createMenu(_LCDML_DISP_cnt);

// Global variables
uint8_t contraste = 25;
uint8_t brillo = 130;
unsigned char state = 0;
const String defaultTitle = "Fuente NXT";
String nextTitle = "Fuente de laboratorio";
String currentTitle = "Fuente de laboratorio";
String previousTitle = "Fuente de laboratorio";
volatile uint8_t previousLayer = 0;
char buf[20];
String bufError = "";
String bufError1 = "";
String bufError2 = "";
volatile bool drawTitle = false;

String VBuck1= "0", IBuck1 = "0",PBuck1 = "0",IB1LIM = "0";
String VBuck2= "0", IBuck2 = "0",PBuck2 = "0",IB2LIM = "0";
String V5V= "0", I5V = "0",P5V = "0",I5VLIM = "0";
String V3V3= "0", I3V3 = "0",P3V3 = "0",I3V3LIM = "0";
String value = "";

volatile float setPoint1 = 5;
volatile float setPoint2 = 5;

volatile float SETIB1LIM = BUCKCURRENTMAX;
volatile float SETIB2LIM = BUCKCURRENTMAX;
volatile float SETI5VLIM = I5VCURRENTMAX;
volatile float SETI3V3LIM = I3V3CURRENTMAX;

uint8_t menu = 0;

bool changeParam = false;
bool drawing = false;
bool firsttime = true;
unsigned long lastParamUpdate = 0;

unsigned long var = 0;

// Encoder variables
volatile byte aFlag = 0; // let's us know when we're expecting a rising edge on pinA to signal that the encoder has arrived at a detent
volatile byte bFlag = 0; // let's us know when we're expecting a rising edge on pinB to signal that the encoder has arrived at a detent (opposite direction to when aFlag is set)
volatile byte reading = 0; //somewhere to store the direct values we read from our interrupt pins before checking to see if we have moved a whole detent

byte giroRapido = 1;

uint8_t param = 0;

byte ISAUXENABLE = 1;
byte ISB1ENABLE = 1;
byte ISB2ENABLE = 1;

byte error = 0;

uint8_t cantidadPerfiles =  0;

bool enter = false;

bool isRunningPIDBuck2 = true;

RotaryEncoder encoder(2, 3);

// ********************************************************************* 
// LCDML BACKEND (core of the menu, do not change here anything yet)
// ********************************************************************* 
// define backend function  
#define _LCDML_BACK_cnt    1  // last backend function id

LCDML_BACK_init(_LCDML_BACK_cnt);
LCDML_BACK_new_timebased_dynamic (0  , ( 10UL )         , _LCDML_start  , LCDML_BACKEND_control);
LCDML_BACK_new_timebased_dynamic (1  , ( 10000000UL )   , _LCDML_stop   , LCDML_BACKEND_menu);
LCDML_BACK_create();

// *********************************************************************
// SETUP
// *********************************************************************
void setup()
{  
  // serial init; only be needed if serial control is used 
  while(!Serial);                    // wait until serial ready
  Serial.begin(115200);              // start serial
  Serial.setTimeout(TIMEOUT);        // set timeout         

  // Slerta
  pinMode(alerta, INPUT); // set pinA as an input, pulled HIGH to the logic voltage (5V or 3.3V for most cases)
  digitalWrite(alerta, LOW);       // turn on pullup resistors 
  // Attach the new PinChangeInterrupt and enable event function below
  //attachPCINT(digitalPinToPCINT(alerta), showError, CHANGE);

  // Config encoder interrupts
  pinMode(encoderPhaseA, INPUT); // set pinA as an input, pulled HIGH to the logic voltage (5V or 3.3V for most cases)
  digitalWrite(encoderPhaseA, HIGH);       // turn on pullup resistors 
  pinMode(encoderPhaseB, INPUT); // set pinB as an input, pulled HIGH to the logic voltage (5V or 3.3V for most cases)
  digitalWrite(encoderPhaseB, HIGH);       // turn on pullup resistors 

  
  attachInterrupt(0, PinB, RISING); // set an interrupt on PinA, looking for a rising edge signal and executing the "PinA" Interrupt Service Routine (below)
  attachInterrupt(1, PinA, RISING); // set an interrupt on PinB, looking for a rising edge signal and executing the "PinB" Interrupt Service Routine (below)
  attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(4), enterButton, FALLING);

  // initialize PWM for backlight
  pinMode(brightness, OUTPUT);   // sets the pin as output
  analogWrite(brightness, brillo);  // analogRead values go from 0 to 1023, analogWrite values from 0 to 255

  // Enable all items with _LCDML_G1
  LCDML_DISP_groupEnable(_LCDML_G1); // enable group 1
  
  // LCDMenu Setup
  u8g.setRot180();
  LCDML_setup(_LCDML_BACK_cnt);  

  // Send initial parameters
  setCommand(SET_BUCK2_VOLTAGE_COMMAND, setPoint2);
  setCommand(SET_BUCK2_CURRENT_LIMIT,   SETIB2LIM);
  setCommand(SET_BUCK1_VOLTAGE_COMMAND, setPoint1);
  setCommand(SET_BUCK1_CURRENT_LIMIT,   SETIB1LIM);
  setCommand(SET_5V_CURRENT_LIMIT,      SETI5VLIM);
  setCommand(SET_3V3_CURRENT_LIMIT,     SETI3V3LIM);

  LCDML_BUTTON_enter();
  LCDML_BUTTON_resetEnter();
}

// *********************************************************************
// LOOP
// *********************************************************************
void loop()
{ 
  // this function must called here, do not delete it
  LCDML_run(_LCDML_priority);
  showError();
}

// *********************************************************************
//  MY FUNCTIONS
// *********************************************************************

void setCommand(uint8_t command, float value){
  Serial.write(command); 
  var = millis(); 
  while( Serial.available() == 0 && (millis() - var) <= TIMEOUT);
  if(Serial.read() == ACK ){    
    Serial.print(value,2);
    Serial.write('\0');      
  } 
}

void setCommandSinDatos(uint8_t command){
  Serial.write(command); 
  var = millis(); 
  while( Serial.available() == 0 && (millis() - var) <= TIMEOUT);
  if(Serial.read() == ACK ){    
      
  } 
}

String getCommand(uint8_t command){
  Serial.write(command);
  var = millis();
  while(Serial.available() == 0 && (millis() - var) <= TIMEOUT); 
  if(Serial.read()== ACK ){
     return Serial.readStringUntil('\0');
  } else{
     return NORESPONDE;
  }
}

void PinA(){
  encoder.tick();
  if(encoder.getPosition() < 0) {
    encoder.setPosition(0);
    LCDML_BUTTON_down();
  }
}

void PinB(){
  encoder.tick();
  if(encoder.getPosition() > 0) {
    encoder.setPosition(0);
    LCDML_BUTTON_up();    
  }
}

void enterButton (void) {
  if(enter==false){
  enter =true;
  delay(20);
  while(digitalRead(4)==0);
  }
}

void showError(void) {
  if(digitalRead(alerta) && error != 1){
    bufError = getCommand(GET_STATUS);
    if(bufError != "n/r"){
      bufError1 = bufError.substring(0, bufError.indexOf('\n'));
      bufError2 = bufError.substring(bufError.indexOf('\n') + 1);
    }
    // Enable all items with _LCDML_G1
    //LCDML_DISP_groupDisable(_LCDML_G1); // enable group 1
    error = 1;
  }
}

// *********************************************************************
// check some errors - do not change here anything
// *********************************************************************
# if(_LCDML_DISP_rows > _LCDML_DISP_cfg_max_rows)
# error change value of _LCDML_DISP_cfg_max_rows in LCDMenuLib.h
# endif
# if(_LCDML_DISP_cols > _LCDML_DISP_cfg_max_string_length)
# error change value of _LCDML_DISP_cfg_max_string_length in LCDMenuLib.h
# endif
