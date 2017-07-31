#include <Arduino.h>
#include <U8g2lib.h>
#include <FreqCount.h>
#include <MsTimer2.h>
#include <EEPROM.h>


#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

// Comienzo de logotipo de LUTEC
#define lutec_width 128
#define lutec_height 27
const static unsigned char lutec_bits[] PROGMEM = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x01, 0x00,
   0xfc, 0x00, 0x7c, 0xfc, 0xff, 0x7f, 0xfe, 0xff, 0x3f, 0xff, 0xff, 0x1f,
   0x00, 0xf8, 0x00, 0x00, 0x7c, 0x00, 0x7c, 0xfe, 0xff, 0x3f, 0xff, 0xff,
   0x9f, 0xff, 0xff, 0x0f, 0x00, 0xf8, 0x00, 0x00, 0x7e, 0x00, 0x3e, 0xfe,
   0xff, 0x3f, 0xff, 0xff, 0x8f, 0xff, 0xff, 0x0f, 0x00, 0x7c, 0x00, 0x00,
   0x3e, 0x00, 0x3e, 0xff, 0xff, 0x9f, 0xff, 0xff, 0xcf, 0xff, 0xff, 0x07,
   0x00, 0x7c, 0x00, 0x00, 0x3e, 0x00, 0x1f, 0x80, 0x1f, 0x00, 0x00, 0x00,
   0xc0, 0x07, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x1f, 0x00, 0x1f, 0x80,
   0x0f, 0x00, 0x00, 0x00, 0xe0, 0x07, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00,
   0x1f, 0x80, 0x0f, 0xc0, 0x0f, 0xc0, 0xff, 0xff, 0xe3, 0x03, 0x00, 0x00,
   0x00, 0x1f, 0x00, 0x80, 0x0f, 0x80, 0x0f, 0xc0, 0x07, 0xe0, 0xff, 0xff,
   0xf3, 0x01, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x80, 0x0f, 0xc0, 0x07, 0xc0,
   0x07, 0xe0, 0xff, 0xff, 0xf1, 0x01, 0x00, 0x00, 0x80, 0x0f, 0x00, 0xc0,
   0x07, 0xc0, 0x07, 0xe0, 0x03, 0xf0, 0xff, 0xff, 0xf9, 0x00, 0x00, 0x00,
   0x80, 0x0f, 0x00, 0xe0, 0x07, 0xe0, 0x03, 0xf0, 0x03, 0x00, 0x00, 0x00,
   0xf8, 0x00, 0x00, 0x00, 0xc0, 0x07, 0x00, 0xe0, 0x03, 0xe0, 0x03, 0xf0,
   0x01, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0xc0, 0xff, 0xff, 0xf3,
   0xff, 0xff, 0x01, 0xf8, 0x01, 0xf8, 0xff, 0x7f, 0xfc, 0xff, 0x3f, 0x00,
   0xe0, 0xff, 0xff, 0xf3, 0xff, 0xff, 0x01, 0xf8, 0x00, 0xfc, 0xff, 0x7f,
   0xfe, 0xff, 0x3f, 0x00, 0xf0, 0xff, 0xff, 0xf9, 0xff, 0xff, 0x00, 0xfc,
   0x00, 0xfe, 0xff, 0x3f, 0xfe, 0xff, 0x3f, 0x00, 0xf0, 0xff, 0xff, 0xf9,
   0xff, 0xff, 0x00, 0x7c, 0x00, 0xfe, 0xff, 0x3f, 0xff, 0xff, 0x1f, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
// Fin de logotipo de LUTEC


U8G2_ST7920_128X64_1_SW_SPI u8g2(U8G2_R2, /* clock=*/ 13, /* data=*/ 11, /* CS=*/ 10, /* reset=*/ 9);

// Declaración de prototipos de funciones
void logo(void);
void menu(void);
void modo_simple(void);
void modo_avanzado(void);

// Declaración de variables globales
byte a=0,pulsador=19,buzzer=2,rele=3,rele_RF=4,led=6,cooler=7;
byte EEPROM_modo=1,EEPROM_modo_address=0; // 1=NORMAL, 2=AVANZADO
byte EEPROM_carga=1,EEPROM_carga_address=1; // 1=INTERNA, 2=EXTERNA
byte EEPROM_alarma=1,EEPROM_alarma_address=2; // 0=OFF, 13=1.3, 15=1.5, 20=2.0, 25=2.5, 30=3.0
byte EEPROM_coeficiente=1,EEPROM_coeficiente_address=3; // 1=AUTOMÁTICO, 2... 202

void setup(void)
{ 
  u8g2.begin(A5,A3,A4);
  FreqCount.begin(100); // Gate time 100ms
  analogReference(EXTERNAL);
  pinMode(buzzer,OUTPUT);
  pinMode(rele,OUTPUT);

  //Serial.begin(9600);
}

void loop(void)
{
  if(a==0)
  {
    logo();
    a=1;
  }

  if(EEPROM.read(EEPROM_modo_address)==1)
  {
    modo_simple();
  }
  else
  {
    modo_avanzado();
  }
  
  if(digitalRead(pulsador)==0)
    {
      delay(100); // Esta línea junto con la siguiente hacen de debouncer
      while(digitalRead(pulsador)==0);
      digitalWrite(buzzer,HIGH);
      delay(50);
      digitalWrite(buzzer,LOW);
      menu();
    }
  
}

void logo(void)
{
  // Logotipo LUTEC
  u8g2.firstPage();
  do
  {
    u8g2.drawXBMP(0,0,lutec_width,lutec_height,lutec_bits);
  } while (u8g2.nextPage());
  delay(500);
}

void menu(void)
{
  const char *lista_menu=
    "Salir\n"
    "Modo de monitoreo\n"
    "Carga\n"
    "Alarma de alto ROE\n"
    "Coeficiente de acople\n"
    "Version de firmware\n"
    "Reset";
  uint8_t seleccion_menu=1;

  const char *lista_roe=
    "Desactivada\n"
    "1.3:1\n"
    "1.5:1\n"
    "2.0:1\n"
    "2.5:1\n"
    "3.0:1";
  uint8_t seleccion_roe=1;
  
  u8g2.setFont(u8g2_font_6x12_tr);
  do
  {
    seleccion_menu=u8g2.userInterfaceSelectionList("Menu",seleccion_menu,lista_menu);
    switch(seleccion_menu)
    {
      case 1: break;
      case 2: {EEPROM.update(EEPROM_modo_address,u8g2.userInterfaceMessage("Modo de monitoreo","",""," Normal \n Avanzado "));}
              break;
      case 3: {EEPROM.update(EEPROM_carga_address,u8g2.userInterfaceMessage("Carga","",""," Interna \n Externa "));}
              break;
      case 4: {switch(u8g2.userInterfaceSelectionList("Alarma de alto ROE",seleccion_roe,lista_roe))
                                {
                                  case 1: {EEPROM.update(EEPROM_alarma_address,0);} break;
                                  case 2: {EEPROM.update(EEPROM_alarma_address,13);} break; // Guardo los valores de ROE multiplicados por 10 porque
                                  case 3: {EEPROM.update(EEPROM_alarma_address,15);} break; // sólo se puede guardar en EEPROM de 0 a 255
                                  case 4: {EEPROM.update(EEPROM_alarma_address,20);} break;
                                  case 5: {EEPROM.update(EEPROM_alarma_address,25);} break;
                                  case 6: {EEPROM.update(EEPROM_alarma_address,30);} break;
                                };
              }
              break;
      case 5: break;
      case 6: {u8g2.userInterfaceMessage("Version de firmware","","V4.0"," Ok ");}
              break;
      case 7: {EEPROM.update(EEPROM_modo_address,1);
               EEPROM.update(EEPROM_carga_address,1);
               EEPROM.update(EEPROM_alarma_address,0);
               EEPROM.update(EEPROM_coeficiente_address,1);
               u8g2.userInterfaceMessage("Reset hecho","",""," Ok ");}
              break;
    }
  }while(seleccion_menu!=1);
}

void modo_simple(void)
{
  float pf,pr,roe,vf,dbm_f,vr,dbm_r,roe_set;


    // Foward
    pf=analogRead(A0)*2.5/1023;
    //dbm_f=32.370*vf-11.147; // VHF
    //dbm_f=31.423*vf-14.079; // UHF
    //pf=pow(10,dbm_f/10)/1000;

    // Reverse
    pr=analogRead(A1)*2.5/1023;
    //dbm_r=33.519*vr-13.372; // VHF
    //dbm_r=34.120*vr-19.932; // UHF
    //pr=pow(10,dbm_r/10)/1000;

    // SWR
    roe=(1 + sqrt(pr/pf)) / (1 - sqrt(pr/pf));
    
    //pf=p*pow(10,37.48/10); //146 MHz
    //pr=pow(10,(40*(analogRead(A1)*2.5/1023)-29.8)/10)/1000;
    //roe=(1+sqrt(pr/pf))/(1-sqrt(pr/pf));
    //pf=(pow(10,(40*(analogRead(A0)*2.5/1023)-29.8)/10)/1000)/pow(10,0.5);

    /*Serial.println(pf);
    Serial.println(pr);
    Serial.println(roe);
    delay(200);*/
  
  // Página principal
  u8g2.firstPage();
  do
  { 
    float count=FreqCount.read();
    
    // Recuadro
    u8g2.drawFrame(0,0,128,64);
    
    // Líneas horizontales
    u8g2.drawHLine(0,23,128);
    u8g2.drawHLine(0,39,128);

    // Líneas verticales
    u8g2.drawVLine(64,0,23);
    u8g2.drawVLine(42,39,25);
    u8g2.drawVLine(85,39,25);
    
    // Descripciones del gráfico   
    u8g2.setFont(u8g2_font_micro_mr); // 5 pixels high
    u8g2.drawStr(13,7,"INCIDENTE");
    u8g2.drawStr(91,7,"ROE");
    u8g2.drawStr(2,34,"FRECUENCIA");
    u8g2.drawStr(2,46,"COEF ACOPL");
    u8g2.drawStr(44,46,"ALARMA ROE");
    u8g2.drawStr(96,46,"CARGA");

    // Unidades del gráfico
    u8g2.setFont(u8g2_font_7x13_mr);
    u8g2.drawStr(49,20,"W");
    u8g2.drawStr(102,36,"MHz");
    
    // Variables del gráfico
    u8g2.setFont(u8g2_font_7x13B_mn); // 9 pixels high
    //u8g2.drawStr(10,20,"50.00");
    u8g2.setCursor(10,20);
    u8g2.print(pf,3);
    u8g2.setCursor(82,20);
    u8g2.print(roe,2);
    u8g2.setCursor(50,36);
    u8g2.print(count*64/100000,3); // Muestra frecuencia
    u8g2.setFont(u8g2_font_5x8_mf); // 7 pixels high
    u8g2.setCursor(4,57);
    u8g2.print(pr,3);

    // Alarma de ROE
    u8g2.setCursor(57,57);
    if(EEPROM.read(EEPROM_alarma_address)==0)
    {
      u8g2.print("OFF");
    }
    else
    {
      roe_set=EEPROM.read(EEPROM_alarma_address);
      u8g2.print(roe_set/10,1);
    }
    
    // Carga interna (temperatura) o externa  
    u8g2.setCursor(92,57);
    if(EEPROM.read(EEPROM_carga_address)==1)
    {
      digitalWrite(rele,LOW); // Relé apagado: carga interna
      u8g2.print(analogRead(A2)*1.1/10.24-2,1); // Muestra temperatura disipador, un decimal de presición
      u8g2.setCursor(112,57);
      u8g2.print(char(176)); // Caractér de grado '°'
      u8g2.print("C");
    }
    else
    {
      digitalWrite(rele,HIGH); // Relé encendido: carga externa
      u8g2.setCursor(89,57);
      u8g2.print("EXTERNA");
    }
    
  }while (u8g2.nextPage());
}

void modo_avanzado(void)
{
  float roe_set;
  
  u8g2.firstPage();
  do
  {
    float count=FreqCount.read();
    
    // Recuadro
    u8g2.drawFrame(0,0,128,64);

    // Líneas horizontales
    u8g2.drawHLine(0,9,128);
    u8g2.drawHLine(0,18,128);
    u8g2.drawHLine(0,27,128);
    u8g2.drawHLine(0,36,128);
    u8g2.drawHLine(0,45,128);
    u8g2.drawHLine(0,54,128);

    // Líneas verticales
    u8g2.drawVLine(80,9,18);
    u8g2.drawVLine(45,27,9);
    u8g2.drawVLine(55,36,9);
    u8g2.drawVLine(64,45,9);
    u8g2.drawVLine(59,54,9);

    // Descripciones del gráfico   
    u8g2.setFont(u8g2_font_micro_mr); // 5 pixels high
    u8g2.drawStr(3,7,"FRECUENCIA:");
    u8g2.drawStr(3,16,"INCIDENTE:");
    u8g2.drawStr(3,25,"REFLEJADA:");
    u8g2.drawStr(3,34,"ROE:");
    u8g2.drawStr(47,34,"EFICIENCIA:");
    u8g2.drawStr(2,43,"|Z|:");
    u8g2.drawStr(57,43,"COEF ACPL:");
    u8g2.drawStr(2,52,"|RL|:");
    u8g2.drawStr(66,52,"|S11|:");
    u8g2.drawStr(3,61,"ALARMA:");
    u8g2.drawStr(61,61,"CARGA:");

    // Variables del gráfico
    u8g2.setCursor(66,7);
    u8g2.print(count*64/100000,3); // Muestra frecuencia
    u8g2.print(" MHz");  
    u8g2.drawStr(47,16,"50.00 W");
    u8g2.drawStr(87,16,"16.99 dBm");
    u8g2.drawStr(47,25,"10.00 W");
    u8g2.drawStr(87,25,"10.00 dBm");
    u8g2.drawStr(22,34,"10.00");
    u8g2.drawStr(92,34,"100.00 %");
    u8g2.drawStr(18,43,"310.0 OHM");
    u8g2.drawStr(98,43,"146.000");
    u8g2.drawStr(27,52,"10.00 dB");
    u8g2.drawStr(92,52,"10.00 dB");
    

    // Alarma de ROE
    u8g2.setCursor(39,61);
    if(EEPROM.read(EEPROM_alarma_address)==0)
    {
      u8g2.print("OFF");
    }
    else
    {
      roe_set=EEPROM.read(EEPROM_alarma_address);
      u8g2.print(roe_set/10,1);
    }

    // Carga interna (temperatura) o externa  
    u8g2.setCursor(94,61);
    if(EEPROM.read(EEPROM_carga_address)==1)
    {
      digitalWrite(rele,LOW); // Relé apagado: carga interna
      u8g2.print(analogRead(A2)*1.1/10.24-2,1); // Muestra temperatura disipador, un decimal de presición
      u8g2.setCursor(110,61);
      u8g2.print("'C");
    }
    else
    {
      digitalWrite(rele,HIGH); // Relé encendido: carga externa
      u8g2.setCursor(94,61);
      u8g2.print("EXTERNA");
    }
    
  }while (u8g2.nextPage());  
}

