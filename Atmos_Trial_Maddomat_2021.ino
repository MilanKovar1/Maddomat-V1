#include <Arduino.h>
#include <U8g2lib.h>
#include <EEPROM.h>

#include <OneWire.h>
#include <DallasTemperature.h>
const int oneWireBus = 4;
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

#include <SoftwareSerial.h>
SoftwareSerial mySerial(A9, A8); // RX, TX

#include <max6675.h>
U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, 13, 12, 11, U8X8_PIN_NONE);

//ENKODER
const int PinCLK = 3;
const int PinDT = 2;
const int PinSW = 5;//1


char tisk[7];
bool stavCLK;
bool stavDT;
bool stavSW;
bool stavPred;

byte menu = 0;
byte enkoder = 0;
byte poziceEnkod = 0;

int counter ;
int konstant1 ;
int variabledisplay;
int hystereze ;
int interval ;
int uhel ;

int pocitadlo ;
int counterForPump;

int T1, T2, T3, T4, T5 ;

int mod = 0; // mod 0 -> zátop , mod 1 -> provoz
bool prekroceniTeploty70 = false; //proměná pro určení prvního překročení teploty
bool prvniPrekroceni = false; //proměná pro určení prvního překročení teploty

bool smallFire = false;
bool bigFire = false ;
bool clearFire = false ;

//termoclanek
int pinSO  = 37;
int pinCS  = 35;
int pinSCK = 33;
MAX6675 termoclanek(pinSCK, pinCS, pinSO);

int linear1 = 1;
int lin1;

int control ;
unsigned long stavpred  = 0;

char teplota1[8];
char teplota2[8];
char teplota3[8];
char teplota4[8];
char teplota5[8];

uint8_t sensor1[8] = { 0x28, 0x65, 0x16, 0x00, 0x09, 0x00, 0x00, 0x58 };
uint8_t sensor2[8] = { 0x28, 0x17, 0x47, 0x6C, 0x07, 0x00, 0x00, 0x80 };
uint8_t sensor3[8] = { 0x28, 0xF6, 0xD6, 0x6A, 0x07, 0x00, 0x00, 0xED };
uint8_t sensor4[8] = { 0x28, 0x5A, 0x26, 0x6C, 0x07, 0x00, 0x00, 0x63 };


void(* resetFunc) (void) = 0;

void setup()
{
  Serial.begin(9600);
  u8g2.begin(); //diplay
  sensors.begin(); // čidla
   mySerial.begin(9600);

  pinMode(PinCLK, INPUT_PULLUP);
  pinMode(PinDT, INPUT_PULLUP);
  pinMode(PinSW, INPUT_PULLUP);

  attachInterrupt (digitalPinToInterrupt(PinCLK), readEnkoder, CHANGE);

  pinMode(15, OUTPUT); //rele 1
  pinMode(17, OUTPUT); //rele 2
  pinMode(19, OUTPUT); //rele 3
  pinMode(21, OUTPUT); //rele 4
  pinMode(23, OUTPUT); //rele 5
  pinMode(25, OUTPUT); //rele 6

  pinMode(51, OUTPUT); //piezo
  pinMode(10, OUTPUT); //rele display

  pinMode(46, OUTPUT); //led 1
  pinMode(47, OUTPUT); //led 2
  pinMode(48, OUTPUT); //led 3
  pinMode(50, OUTPUT); //led 4
  pinMode(52, OUTPUT); //led 5
  pinMode(53, OUTPUT); //led 6

  digitalWrite(15, 1 );
  digitalWrite(17, 1);
  digitalWrite(19, 1);
  digitalWrite(21, 1);
  digitalWrite(23, 1);
  digitalWrite(25, 1);

  digitalWrite(51, 0);
  //digitalWrite(10, 0);

  counter = EEPROM.read(1);          // read vaule from eeprom and set to .counter.!
  konstant1 = EEPROM.read(2);        //konstant1
  variabledisplay = EEPROM.read(3);  //zapnutí displaje
  hystereze = EEPROM.read(4);  //hystereze
  interval = EEPROM.read(5);  //interval
  uhel = EEPROM.read(6);  //interval

  // EEPROM.write(6,1);

  getTemperature();
 
  VERSION();
  delay(1000);
  u8g2.clearBuffer();

  menu = 20;//98 20

  sensors.begin(); // čidla
  getTemperature();
  communication();

}

void loop()
{
  DekodeEnkoder();
  Evaluationenkoder();
  DisplayWrite();
  ControlServo1();
 // ControlPump();
  controldisplay() ;


}

void communication()
{
  String i = "https://data.xf.cz/atmos.php?T1=";
  String a ="&T2=" ;
  String b ="&T3=" ;
  String c ="&T4=" ;
  String d ="&T5=" ;
  String o = "&ID=&Cas=" ;
  
  String string = i + T1 + a + T2 + b + T3 + c + T4 + d + T5 + o;

  mySerial.print(string); 
  Serial.print(string);
 
}

void readEnkoder ()
{
  stavCLK = digitalRead(PinCLK);
  stavDT = digitalRead(PinDT);
  delay(5);
}

void  DekodeEnkoder()
{
  if (stavCLK != stavPred)
  {
    
    if (stavDT != stavCLK)
    
    {
      enkoder = 1; // rotation right
    }
    
    else if (stavDT == stavCLK)
    {

      enkoder = 2; //rotation left
    }
  }

  stavPred = stavCLK;
  stavSW = digitalRead(PinSW);

  if (stavSW == 0) // push button
  {
    beep2();
    enkoder = 3;
    delay(500);
  }
}

void Evaluationenkoder()
{

  if (enkoder == 1) //rotace nahoru
  {
    u8g2.clearBuffer();
    /////////////////////////////////////////
    if (menu == 1)
    {
      menu = 2;
    }

    else if (menu == 2)
    {
      menu = 3;
    }

    else if (menu == 3)
    {
      menu = 4;
    }
    else if (menu == 4)
    {
      menu = 1;
    }
    /////////////////////////////////
    //konstanty servo
    else if (menu == 11)
    {
      menu = 12;
    }
    else if (menu == 12)
    {
      menu = 13;
    }
    else if (menu == 13)
    {
      menu = 11;
    }

    else if (menu == 51)
    {
      menu = 52;
    }
    else if (menu == 52)
    {
      menu = 53;
    }
    else if (menu == 53)
    {
      menu = 54;
    }
    else if (menu == 54)
    {
      menu = 57;
    }
    else if (menu == 57)
    {
      menu = 51;
    }
    //hystereze
    else if (menu == 58)
    {
      if (enkoder == 1) {
        hystereze++ ;
        if (hystereze > 6) hystereze = 6 ;
      }
    }
    //interval
    else if (menu == 59)
    {
      if (enkoder == 1) {
        interval++ ;
        if (interval > 6) interval = 6 ;
      }
    }

    //uhel
    else if (menu == 60)
    {
      if (enkoder == 1) {
        uhel++ ;
        if (uhel > 10 ) uhel = 10 ;
      }
    }

    //////////////////////////////
    //couter puze testovací
    else if (menu == 40)
    {
      if (enkoder == 1) counter++ ;
    }
    /////////////////////////////

    /////////////////////////////
    //konstant1
    else if (menu == 55)
    {
      if (enkoder == 1) {
        konstant1++ ;
        if (konstant1 > 85 ) konstant1 = 85 ;
      }
    }
    /////////////////////////////
    //nastaveni
    else if (menu == 31)
    {
      menu = 32;
    }
    //display
    else if (menu == 97)
    {
      if (enkoder == 1) {
        variabledisplay++ ;
        if (variabledisplay > 50 ) variabledisplay = 50 ;

      }
    }
    /////////////////////////////
    //vychozi nastaveni
    else if (menu == 41)
    {
      menu = 42;
    }
    //////////////////////
    {
      poziceEnkod ++;
    }
  }

  else if (enkoder == 2)                                // roation back
  {
    u8g2.clearBuffer();
    //////////////////////////////////
    if (menu == 1)
    {
      menu = 4;
    }

    else if (menu == 2)
    {
      menu = 1;
    }

    else if (menu == 3)
    {
      menu = 2;
    }
    //
    else if (menu == 4)
    {
      menu = 3;
    }

    else if (menu == 40)
    {
      if (enkoder == 2) {

        counter-- ;
      }
    }
    ////////////////////////////////////
    else if (menu == 55)
    {
      if (enkoder == 2) {
        konstant1-- ;
        if (konstant1 < 65 ) konstant1 = 65 ;
      }
    }

    //////////////////////////////
    //konstanty servo
    else if (menu == 11)
    {
      menu = 13;
    }
    else if (menu == 12)
    {
      menu = 11;
    }
    else if (menu == 13)
    {
      menu = 12;
    }
    //
    else if (menu == 51)
    {
      menu = 57;
    }
    else if (menu == 57)
    {
      menu = 54;
    }
    else if (menu == 54)
    {
      menu = 53;
    }
    else if (menu == 53)
    {
      menu = 52;
    }
    else if (menu == 52)
    {
      menu = 51;
    }
    //hystereze
    else if (menu == 58)
    {
      if (enkoder == 2) {
        hystereze-- ;
        if (hystereze < 1) hystereze = 1 ;
      }
    }
    //interval
    else if (menu == 59)
    {
      if (enkoder == 2) {
        interval-- ;
        if (interval < 1) interval = 1 ;
      }
    }

    //uhel
    else if (menu == 60)
    {
      if (enkoder == 2) {
        uhel-- ;
        if (uhel < 1 ) uhel = 1;
      }
    }

    //////////////////////////////////
    //menu 3 nastaveni
    else if (menu == 32)
    {
      menu = 31;
    }
    //konstant1
    else if (menu == 97)
    {
      if (enkoder == 2) {
        variabledisplay-- ;
        if (variabledisplay < 15 ) variabledisplay = 20 ;
      }
    }
    //vychozi nastaveni
    else if (menu == 42)
    {
      menu = 41;
    }
    //////////////////////

    {
      poziceEnkod --;
    }
  }

  else if (enkoder == 3) // stisk
  {

    u8g2.clearBuffer();

    ////////////////////////
    //TEPLOTY KOTEL
    if (menu == 2)
    {
      menu = 20;
    }
    else if (menu == 20)
    {
      menu = 98;
    }
    else if (menu == 98)
    {
      menu = 2;
    }
    /////////////////////////
    //KONSTANTY
    else if (menu == 1)
    {
      menu = 11;
    }
    else if (menu == 13)
    {
      menu = 1;
    }
    //KONSTANTY V MENU
    else if (menu == 12)
    {
      menu = 56;
    }
    else if (menu == 56)
    {
      menu = 12;
    }
    else if (menu == 11)
    {
      menu = 51;
    }
    else if (menu == 51)
    {
      menu = 55;
    }
    else if (menu == 55)
    {
      setkonstant1toeeprom();
      menu = 51;
    }
    else if (menu == 57)
    {
      menu = 11;
    }
    else if (menu == 52)
    {
      menu = 58;
    }
    else if (menu == 58)
    {
      sethysterezetoeeprom();
      menu = 52;
    }

    else if (menu == 54)
    {
      menu = 59;
    }
    else if (menu == 59)
    {
      setintervaltoeeprom();
      menu = 54;
    }

    else if (menu == 53)
    {
      menu = 60;
    }
    else if (menu == 60)
    {
      setuheltoeeprom();
      menu = 53;
    }

    ////////////////////////
    //menu 3 nastaveni
    else if (menu == 3)
    {
      menu = 31;
    }
    else if (menu == 32)
    {
      menu = 3;
    }
    else if (menu == 31)
    {
      menu = 97;
    }
    else if (menu == 97)
    {
      setvariabledisplaytoeeprom();
      menu = 31;
    }
    ////////////////////////
    else if (menu == 4)
    {
      menu = 42;
    }
    else if (menu == 42)
    {
      menu = 2;
    }
    else if (menu == 41)
    {
      setfactorytoeeprom();
      menu = 2;
    }
    ////////////////////

  }

  enkoder = 0;
}


void DisplayWrite()
{

  if (menu == 20 && T1 > konstant1 - 10 && bigFire == false) {
    u8g2.setFont(u8g2_font_open_iconic_all_2x_t);
    u8g2.drawGlyph(91, 53, 168);
    u8g2.sendBuffer();
    smallFire = true ;
    bigFire = true ;
  }

  if (menu == 20 && T1 < konstant1 - 10 && smallFire == true ) {

    smallFire = false ;
    bigFire = false ;
    u8g2.clearBuffer();

  }

  if (T1 == 85 || T2 == 85 || T3 == 85 || T4 == 85 ) {
    menu = 81 ;
    beep1();
         servo1up();
         servo1up();
         servo1up();
         delay(8000);
    resetFunc(); // automaticky provede restart 
  }





  switch (menu) {
    case 81: //eror dallas 

      u8g2.setFont(u8g2_font_open_iconic_all_8x_t);
      u8g2.drawGlyph(1, 65, 280);
      u8g2.drawGlyph(70, 65, 280);
      u8g2.sendBuffer();
      break;
      
    case 1:

      u8g2.setFont(u8g2_font_tenthinguys_tf);
      u8g2.drawStr(4, 10, "= MENU KOTELNA =");

      u8g2.setFont(u8g2_font_helvR08_tf);

      u8g2.drawStr(2, 22, "KONSTANTY SERVO <--");

      u8g2.drawStr(2, 36, "TEPLOTY KOTEL");

      u8g2.drawStr(2, 50, "NASTAVENI");

      u8g2.drawStr(2, 64, "VYCHOZI NASTAVENI");
      u8g2.sendBuffer();
      break;

    case 2:
      u8g2.setFont(u8g2_font_tenthinguys_tf);
      u8g2.drawStr(4, 10, "= MENU KOTELNA =");

      u8g2.setFont(u8g2_font_helvR08_tf);

      u8g2.drawStr(2, 22, "KONSTANTY SERVO");

      u8g2.drawStr(2, 36, "TEPLOTY KOTEL <--");

      u8g2.drawStr(2, 50, "NASTAVENI");

      u8g2.drawStr(2, 64, "VYCHOZI NASTAVENI");
      u8g2.sendBuffer();
      break;

    case 3:
      u8g2.setFont(u8g2_font_tenthinguys_tf);
      u8g2.drawStr(4, 10, "= MENU KOTELNA =");

      u8g2.setFont(u8g2_font_helvR08_tf);

      u8g2.drawStr(2, 22, "KONSTANTY SERVO");

      u8g2.drawStr(2, 36, "TEPLOTY KOTEL");

      u8g2.drawStr(2, 50, "NASTAVENI <--");

      u8g2.drawStr(2, 64, "VYCHOZI NASTAVENI");
      u8g2.sendBuffer();
      break;

    case 4:
      u8g2.setFont(u8g2_font_tenthinguys_tf);
      u8g2.drawStr(4, 10, "= MENU KOTELNA =");

      u8g2.setFont(u8g2_font_helvR08_tf);

      u8g2.drawStr(2, 22, "KONSTANTY SERVO");

      u8g2.drawStr(2, 36, "TEPLOTY KOTEL");

      u8g2.drawStr(2, 50, "NASTAVENI");

      u8g2.drawStr(2, 64, "VYCHOZI NASTAVENI <--");
      u8g2.sendBuffer();
      break;

    case 41:
      u8g2.setFont(u8g2_font_tenthinguys_tf);
      u8g2.drawStr(4, 10, "= VYCHOZI NAST =");

      u8g2.setFont(u8g2_font_tom_thumb_4x6_me);

      u8g2.drawStr(5, 20, "Teplota:");
      u8g2.setCursor(55, 20); //107
      u8g2.print(konstant1);

      u8g2.drawStr(73, 20, "Uhel:");
      u8g2.setCursor(113, 20); //107
      u8g2.print(uhel);

      u8g2.drawStr(5, 30, "Hystere:");
      u8g2.setCursor(55, 30); //107
      u8g2.print(hystereze);

      u8g2.drawStr(73, 30, "Inter:");
      u8g2.setCursor(113, 30); //107
      u8g2.print(interval);

      u8g2.setFont(u8g2_font_helvR08_tf);

      u8g2.drawStr(2, 45, "VYCHOZI NAST <--");

      u8g2.drawStr(2, 59, "ZPET");
      u8g2.sendBuffer();
      break;

    case 42:
      u8g2.setFont(u8g2_font_tenthinguys_tf);
      u8g2.drawStr(4, 10, "= VYCHOZI NAST =");

      u8g2.setFont(u8g2_font_tom_thumb_4x6_me);

      u8g2.drawStr(5, 20, "Teplota:");
      u8g2.setCursor(55, 20); //107
      u8g2.print(konstant1);

      u8g2.drawStr(73, 20, "Uhel:");
      u8g2.setCursor(113, 20); //107
      u8g2.print(uhel);

      u8g2.drawStr(5, 30, "Hystere:");
      u8g2.setCursor(55, 30); //107
      u8g2.print(hystereze);

      u8g2.drawStr(73, 30, "Inter:");
      u8g2.setCursor(113, 30); //107
      u8g2.print(interval);

      u8g2.setFont(u8g2_font_helvR08_tf);

      u8g2.drawStr(2, 45, "VYCHOZI NAST");

      u8g2.drawStr(2, 59, "ZPET <--");
      u8g2.sendBuffer();
      break;


    ////////////////////////////////////////////////////////////// KONSTANTY MENU
    case 11:
      u8g2.setFont(u8g2_font_tenthinguys_tf);
      u8g2.drawStr(13, 10, "= KONSTANTY =");

      u8g2.setFont(u8g2_font_helvR08_tf);

      u8g2.drawStr(2, 22, "KONSTANTY SERVO 1 <--");

      u8g2.drawStr(2, 36, "KONSTANTY SERVO 2");

      u8g2.drawStr(2, 50, "ZPET");

      u8g2.sendBuffer();
      break;

    case 12:
      u8g2.setFont(u8g2_font_tenthinguys_tf);
      u8g2.drawStr(13, 10, "= KONSTANTY =");

      u8g2.setFont(u8g2_font_helvR08_tf);

      u8g2.drawStr(2, 22, "KONSTANTY SERVO 1");

      u8g2.drawStr(2, 36, "KONSTANTY SERVO 2 <--");

      u8g2.drawStr(2, 50, "ZPET");

      u8g2.sendBuffer();
      break;

    case 13:
      u8g2.setFont(u8g2_font_tenthinguys_tf);
      u8g2.drawStr(13, 10, "= KONSTANTY =");

      u8g2.setFont(u8g2_font_helvR08_tf);

      u8g2.drawStr(2, 22, "KONSTANTY SERVO 1");

      u8g2.drawStr(2, 36, "KONSTANTY SERVO 2");

      u8g2.drawStr(2, 50, "ZPET <--");

      u8g2.sendBuffer();
      break;
    ///////////////////////////////////////////////////////////////
    //KONSTANTY menu 2

    case 56:
      u8g2.setFont(u8g2_font_tenthinguys_tf);
      u8g2.drawStr(13, 10, "= KONSTANTY =");

      u8g2.setFont(u8g2_font_helvR08_tf);

      u8g2.drawStr(46, 22, "ERROR");

      u8g2.drawStr(42, 36, "SERVO 2 ");

      u8g2.drawStr(10, 50, "!!!NENI PRIPOJENO!!!");

      u8g2.sendBuffer();
      break;

    case 55:
      u8g2.setFont(u8g2_font_tenthinguys_tf);
      u8g2.drawStr(13, 10, "= KONSTANTY =");

      u8g2.setFont(u8g2_font_helvR08_tf);

      u8g2.drawStr(2, 22, "Nasteveni bodu kdy se");

      u8g2.drawStr(2, 36, "zacne poustet tepla voda ");

      u8g2.setCursor(45, 50);
      u8g2.print(konstant1);

      u8g2.drawStr(60, 50, "C");

      u8g2.drawStr(2, 64, "Stiskutim uloz! ");

      u8g2.sendBuffer();
      break;

    ///////////////////////////////////////////////////////////////

    case 40:
      u8g2.setFont(u8g2_font_tenthinguys_tf);
      u8g2.drawStr(10, 10, "= COUNTER =");

      u8g2.setFont(u8g2_font_helvR08_tf);
      u8g2.setCursor(2, 22);
      u8g2.print(counter);
      u8g2.sendBuffer();

      break;
    case 20:
      
      u8g2.setFont(u8g2_font_tenthinguys_tf);
      u8g2.drawStr(30, 9, "= KOTEL =");
      u8g2.drawRFrame(1, 11, 30, 50, 7); // img kotle

      u8g2.setFont(u8g2_font_helvR08_tf);
      u8g2.drawStr(12, 21, "A");
      u8g2.drawStr(12, 30, "T");
      u8g2.drawStr(12, 39, "M");
      u8g2.drawStr(12, 48, "O");
      u8g2.drawStr(12, 57, "S");

      u8g2.drawStr(33, 28, "->");
      u8g2.setCursor(43, 28);
      u8g2.print(T2);
      u8g2.drawStr(55, 28, "C");


      u8g2.drawStr(32, 50, "<-");
      u8g2.setCursor(42, 50);
      u8g2.print(T1);
      u8g2.drawStr(55, 50, "C");

      u8g2.drawLine(64, 10, 64, 62);

      u8g2.setFont(u8g2_font_open_iconic_all_2x_t);
      u8g2.drawGlyph(90, 35, 184); // dum

      u8g2.setFont(u8g2_font_open_iconic_arrow_2x_t);
      u8g2.drawGlyph(73, 35, 66);// sipka >
      u8g2.drawGlyph(107, 35, 66);// sipka2 >

      u8g2.setFont(u8g2_font_helvR08_tf);
      u8g2.setCursor(70, 41);
      u8g2.print(T3);
      u8g2.drawStr(82, 41, "c");

      u8g2.setCursor(107, 41);
      u8g2.print(T4);
      u8g2.drawStr(119, 41, "c");

      u8g2.setCursor(90, 64);
      u8g2.print(T5);
      u8g2.drawStr(109, 64, "c");


      u8g2.sendBuffer();

      break;
    ////////////////////////////////////////////////menu 3 nastavení
    case 31:
      u8g2.setFont(u8g2_font_tenthinguys_tf);
      u8g2.drawStr(13, 10, "= NASTAVENI =");

      u8g2.setFont(u8g2_font_helvR08_tf);

      u8g2.drawStr(2, 22, "ZAPNUTI REGULACE <--");

      u8g2.drawStr(2, 36, "ZPET");

      u8g2.sendBuffer();
      break;

    case 32:
      u8g2.setFont(u8g2_font_tenthinguys_tf);
      u8g2.drawStr(13, 10, "= NASTAVENI =");

      u8g2.setFont(u8g2_font_helvR08_tf);

      u8g2.drawStr(2, 22, "ZAPNUTI REGULACE");

      u8g2.drawStr(2, 36, "ZPET <--");

      u8g2.sendBuffer();
      break;
    //////////////////////////////////////////////////
    //konstanty servooo
    case 51:
      u8g2.setFont(u8g2_font_tenthinguys_tf);
      u8g2.drawStr(4, 10, "= KONSTANTY =");

      u8g2.setFont(u8g2_font_helvR08_tf);

      u8g2.drawStr(2, 22, "UDRZOVANA TAPLOTA <--");//5

      u8g2.drawStr(2, 36, "HYSTEREZE");//3

      u8g2.drawStr(2, 50, "UHEL SERVA");//1

      u8g2.drawStr(2, 64, "CASOVY INTERVAL");//4
      u8g2.sendBuffer();
      break;
    case 52:
      u8g2.setFont(u8g2_font_tenthinguys_tf);
      u8g2.drawStr(4, 10, "= KONSTANTY =");

      u8g2.setFont(u8g2_font_helvR08_tf);

      u8g2.drawStr(2, 22, "UDRZOVANA TAPLOTA");//5

      u8g2.drawStr(2, 36, "HYSTEREZE <--");//3

      u8g2.drawStr(2, 50, "UHEL SERVA");//1

      u8g2.drawStr(2, 64, "CASOVY INTERVAL");//4
      u8g2.sendBuffer();
      break;
    case 53:
      u8g2.setFont(u8g2_font_tenthinguys_tf);
      u8g2.drawStr(4, 10, "= KONSTANTY =");

      u8g2.setFont(u8g2_font_helvR08_tf);

      u8g2.drawStr(2, 22, "UDRZOVANA TAPLOTA");//5

      u8g2.drawStr(2, 36, "HYSTEREZE");//3

      u8g2.drawStr(2, 50, "UHEL SERVA <--");//1

      u8g2.drawStr(2, 64, "CASOVY INTERVAL");//4
      u8g2.sendBuffer();
      break;
    case 54:
      u8g2.setFont(u8g2_font_tenthinguys_tf);
      u8g2.drawStr(4, 10, "= KONSTANTY =");

      u8g2.setFont(u8g2_font_helvR08_tf);

      u8g2.drawStr(2, 22, "UDRZOVANA TAPLOTA");//5

      u8g2.drawStr(2, 36, "HYSTEREZE");//3

      u8g2.drawStr(2, 50, "UHEL SERVA");//1

      u8g2.drawStr(2, 64, "CASOVY INTERVAL <--");//4
      u8g2.sendBuffer();
      break;
    case 57:
      u8g2.setFont(u8g2_font_tenthinguys_tf);
      u8g2.drawStr(4, 10, "= KONSTANTY =");

      u8g2.setFont(u8g2_font_helvR08_tf);

      u8g2.drawStr(2, 22, "HYSTEREZE");//5

      u8g2.drawStr(2, 36, "UHEL SERVA");//3

      u8g2.drawStr(2, 50, "CASOVY INTERVAL");//1

      u8g2.drawStr(2, 64, "ZPET <--");//4
      u8g2.sendBuffer();
      break;

    case 58:
      u8g2.setFont(u8g2_font_tenthinguys_tf);
      u8g2.drawStr(13, 10, "= HYSTEREZE =");

      u8g2.setFont(u8g2_font_helvR08_tf);

      u8g2.drawStr(2, 22, "Nasteveni HYSTEREZE");

      u8g2.drawStr(2, 36, "HODNOTA 1 - 4 !!!!");

      u8g2.drawStr(21, 50, "-->");

      u8g2.setCursor(45, 50);
      u8g2.print(hystereze);

      u8g2.drawStr(60, 50, "<--");

      u8g2.drawStr(2, 64, "Stiskutim uloz ");

      u8g2.sendBuffer();
      break;

    case 59:
      u8g2.setFont(u8g2_font_tenthinguys_tf);
      u8g2.drawStr(0, 10, "=CASOVY INERVAL=");

      u8g2.setFont(u8g2_font_helvR08_tf);

      u8g2.drawStr(2, 22, "Nasteveni casoveho intervalu");

      u8g2.drawStr(2, 36, "V minutach");

      u8g2.setCursor(45, 50);
      u8g2.print(interval);

      u8g2.drawStr(60, 50, "minut");

      u8g2.drawStr(2, 64, "Stiskutim uloz ");

      u8g2.sendBuffer();
      break;

    case 60:
      u8g2.setFont(u8g2_font_tenthinguys_tf);
      u8g2.drawStr(4, 10, "= UHEL SERVA =");

      u8g2.setFont(u8g2_font_helvR08_tf);

      u8g2.drawStr(2, 22, "Nasteveni uhlu posunu");

      u8g2.drawStr(2, 36, "serva  ..1-10..!");

      u8g2.setCursor(45, 50);
      u8g2.print(uhel);

      u8g2.drawStr(60, 50, "STUPNU");

      u8g2.drawStr(2, 64, "Stiskutim uloz ");

      u8g2.sendBuffer();
      break;

    case 97:

      u8g2.setFont(u8g2_font_tenthinguys_tf);
      u8g2.drawStr(13, 10, "= KONSTANTY =");

      u8g2.setFont(u8g2_font_helvR08_tf);

      u8g2.drawStr(2, 22, "Nasteveni bodu kdy se");

      u8g2.drawStr(2, 36, "podle teploty zapne MADDOMAT ");

      u8g2.setCursor(45, 50);
      u8g2.print(variabledisplay);

      u8g2.drawStr(60, 50, "C");

      u8g2.drawStr(2, 64, "Stiskutim uloz! ");

      u8g2.sendBuffer();
      break;
    ////////////////////////////////


    case 98:
      u8g2.setFont(u8g2_font_tenthinguys_tf);
      u8g2.drawStr(25, 10, "= SERVICE =");
      u8g2.setFont(u8g2_font_tom_thumb_4x6_me);

      u8g2.drawStr(5, 17, "Teplota kotel =");
      u8g2.setCursor(97, 17); //107
      u8g2.print(T1);
      u8g2.drawStr(110, 17, "C"); //107+12

      u8g2.drawStr(5, 24, "Vratka kotel:");
      u8g2.setCursor(97, 24);
      u8g2.print(T2);
      u8g2.drawStr(110, 24, "C");

      u8g2.drawStr(5, 31, "Vytup radiator:");
      u8g2.setCursor(97, 31);
      u8g2.print(T3);
      u8g2.drawStr(110, 31, "C");

      u8g2.drawStr(5, 38, "Vstup radiator:");
      u8g2.setCursor(97, 38);
      u8g2.print(T4);
      u8g2.drawStr(110, 38, "C");
      float teplotaC = termoclanek.readCelsius();

      u8g2.drawStr(5, 45, "Teplota komin:");
      u8g2.setCursor(97, 45);
      u8g2.print(teplotaC);
      //u8g2.drawStr(115,38,"C");

      u8g2.sendBuffer();

      break;




  }
}

void setvauletoeeprom() {
  EEPROM.update(1, counter) ;
  beep1();
}

void setkonstant1toeeprom() {
  EEPROM.update(2, konstant1) ;
  beep1();
}

void setvariabledisplaytoeeprom() {
  EEPROM.update(3, variabledisplay) ;
  beep1();
}

void sethysterezetoeeprom() {
  EEPROM.update(4, hystereze) ;
  beep1();
}

void setintervaltoeeprom() {
  EEPROM.update(5, interval) ;
  beep1();
}

void setuheltoeeprom() {
  EEPROM.update(6, uhel) ;
  beep1();
}
void setfactorytoeeprom() {
  EEPROM.update(6, 3) ;            //uhel
  EEPROM.update(2, 76) ;       //konstant1
  EEPROM.update(3, 35) ; //zapnuti displeje
  EEPROM.update(4, 1) ;       //hystereze
  EEPROM.update(5, 1) ;       //interval
  beep1();
  konstant1 = EEPROM.read(2);        //konstant1
  variabledisplay = EEPROM.read(3);  //zapnutí displaje
  hystereze = EEPROM.read(4);  //hystereze
  interval = EEPROM.read(5);  //interval
  uhel = EEPROM.read(6);  //interval


}
void ControlServo1()
{
  if (pocitadlo > (interval * 60) * 4 ) //(pocitadlo > (interval * 60) * 15 ) takle to bylo na minuty
  {
    getTemperature();
    
    Serial.println("Go Go cotrol servo!!!");
    Serial.println("spoustim communikaci");
    communication();
    bigFire = false ;


    if (mod == 0)
    {
      if (T1 >= (konstant1 - 12) && prvniPrekroceni == false)
      {
         servo1up();
         servo1up();
         servo1up();
         servo1up();
           
        prvniPrekroceni = true;
        prekroceniTeploty70 = true;
      }

      if (T1 > (konstant1 - hystereze) - 2 )
      {
        mod = 1;
        prekroceniTeploty70 = true ;
      }

      if (T1 < (konstant1 - 10) && prekroceniTeploty70 == true)
      {
        servo1close();
        prekroceniTeploty70 = false;
        prvniPrekroceni = false;
      }

    }

    else if (mod == 1)
    {
      if (T1 > (konstant1 + hystereze))
      {
        servo1up();
      }

      if (T1 <= (konstant1 - hystereze) && T1 >= (konstant1 - 10))
      {
        servo1down();
      }

      if (T1 <= (konstant1 - 10))
      {
        mod = 0;
      }
    }
    pocitadlo = 0;
    u8g2.clearBuffer();
  }
  else pocitadlo++ ;

}

void ControlPump() {

  /*if (counterForPump > 500 ) {

    if( (T2 >40) && (T5 > 65) ||(T2 > 60)  ) {   //teplota kotel > 45 a zárověn teplota spalin je >80c 
      digitalWrite(25, 0);
      digitalWrite(23, 0);

      
      digitalWrite(50, 1);
      
      
    }
    else{
      digitalWrite(25, 1);
      digitalWrite(23, 1);

     
      digitalWrite(50, 0);
      
    }
    
    
  }
  else counterForPump++ ;*/
}



void servo1up() {
  digitalWrite(15, 0);
  digitalWrite(17, 1);
  delay (uhel * 1000); // delka jednoho kroku
  digitalWrite(15, 1);
  digitalWrite(17, 1);

}

void servo1down() {
  digitalWrite(15, 1);
  digitalWrite(17, 0);
  delay (uhel * 1000); // delka jednoho kroku
  digitalWrite(15, 1);
  digitalWrite(17, 1);


}

void servo1close() {
  digitalWrite(15, 1);
  digitalWrite(17, 0);
  delay (100000); // uplně zavřít servo;
  digitalWrite(15, 1);
  digitalWrite(17, 1);


}

void getTemperature() {
             
  sensors.requestTemperatures();

  T1 = sensors.getTempC(sensor1);  //kotel zpatecka
  dtostrf(T1, 2, 2, teplota1);

  T2 = sensors.getTempC(sensor2); //kotel výstup 
  dtostrf(T2, 2, 2, teplota2);

  T3 = sensors.getTempC(sensor3); //vstup radiator
  dtostrf(T3, 2, 2, teplota3);

  T4 = sensors.getTempC(sensor4); // vystup radiator 
  dtostrf(T4, 2, 2, teplota4);

  T5 = termoclanek.readCelsius();
  dtostrf(T5, 2, 2, teplota5);


}

void controldisplay() {
  if (T1 >= variabledisplay) digitalWrite(10, 0);
  else digitalWrite(10, 1);
}

void VERSION() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_tenthinguys_tf);
  u8g2.drawStr(15, 10, "= FIRMWARE =");

  u8g2.setFont(u8g2_font_helvR08_tf);


  u8g2.drawStr(15, 36, "Actual version V3.51");



  u8g2.sendBuffer();
}

void beep1() {
  digitalWrite(51, 1);
  delay(300);
  digitalWrite(51, 0);
}

void beep2() {
  digitalWrite(51, 1);
  delay(5);
  digitalWrite(51, 0);
}
