
/*
        maddomat_2.0
                      - Upravena podmínka pro MOD 4
                      - Přidáno vypínaní displeje a wsled pomocí LDR > aby to neblikalo v kotelně psovi

        maddomat_V2.1
                      - kompletní předělání řízení serva u kotle
                      - dodelano při zmačnutí tlacítka spodního natvrdo zavře serva kotel a aku > musí se udelat po výpadku proudu
                      - dokončena funkce dohřívání bojléru teplota na max co to dá ccc na složité nastavení,, když topí kotel per to tam neb
                      - po vyhasnutí, zavře natvrdo servo kotel protože si to zacne brát tplo z aku a kotel nechládl

       maddomat_V2.2
                      - vylepšeno natápění bojleru

       maddomat_V2.3
                      - Dohřívání bojleru se zapíná az je aku 2část > 50c
                      - Když dojde k přetopení aku dole > 80 zapne dohřívání bojleru sníží výkon kotle
                      - Počká než se 1část aku nahřeje na min 70c a poté pouští teplo domů> kotel se plynuleji rozjede
                      - Přidáno nahřívaní bojleru z aku > na 70c
                      - posílá energii aku v % do DB
                      - Upraven screen zobrazování teplot lepší přehled + energie v %

       maddomat_V2.4
                      - změneny hodnoty na natápění bojléru

       maddomat_V2.5
                      - plně funkční odladený fw!

       maddomat_V2.6_Ekviterm

                      - přidána fukce ekviterm podle venkovní teploty
                      - na webu lze aktivovat ekviterní režim
                      - servo na zavření klapky přisunu vzduchu > dokaže zastavit hoření, po odebrání teplé vody opět rozhoří kotel
                      - https://data.xf.cz/maddomatx.php kolonka pin: ( když nastaveno 5 ) pouští do top.okruhu ekv. teplotu

       maddomat_V2.7_Ekviterm

                      - doladění dohřívání bojléru
                      - upraveny screeny na displaji

       maddomat_V2.8_Ekv+letni

                      - přidělán MODe pro letní provoz > když dojde k přehřítí bojléru solarníma panelama spustí se čerpadlo které v zimě nahřívá bojlér z akumulacek
                      - k aktivaci dojde při přepnutí (ovladacích přepínačů co jsou na panelu) smerem nahoru
                      - z duvodu snadné obsluhy zde nebude možnost ovladání přes WEB > snad se to nekdy doděla na HA
                      - nebude zde ani zpětná vazba na web, bridge do HA > kvuli jednoduchosti
                      - již neukazuje záporné procenta energie v AKU
                      - přidán přepínač "dovnitř" který nastavý var=1 a pozadovanout t doma na 22(aby fungovala ekv) a zapne topení natrvdo, přídad když nejede internet

       maddomat_V2.9

                      - z důvodu stalých problému s bojlerovým čidlem, čidlo nečteno z adresy ale je pouze jedno na sbernici takže čteno jako jedno čidlo
                      - předělána podmínka od které se řídí kotlové servo
                      - controlRCservo se řídí při kotlovém servu
                      - zjednodušena funkce ControlRCservo (nelimitje kotel podle spalin pouze pokud se akum blíží k pretopení
                      - když vypadne čidlo nahradí ho nejaká rozumná hodnota

      maddomat_V2.10

                      - zmeneny hodnoty bezpečnostní přemosteni ekviterm

      maddomat_V2.11

                      - znovu změněny bezpečtnostní hodnoty ekvitermního řízení
                      - testovaní průmerování měřených hodnot spalinovým čidlem

      maddomat_V2.1

                      - změna kdy se aktivuje limitace zadní klapkou
                      - jako referenční se používá filtrovaná T13 > fT13
                      - dříve se zapne funkce upoštění přebytečného tepla
                      - při přetopení se nepuští bojlér cerpadlo


        wish list     - vyřešit aby po výpadku proudu při topení, veděl co má dělat, aktualne dost veliky problém > neví v jake poloze jsou serva atd..
                      - odesílat aktualní nastavení na web maddomatu> konstant uhel atd.
                      - natavit vetší konstant 1 po přetopení akumulacek sníží to výkon kotle. < asi blbost// není jurovi to zavře zadní klapku a kotel jen doutná
                      - dostat to do Grafany > nelze jednoduše
                      - zdokumentovat aspon okrajove funkce( po půl roce nevím absolutně jak to bylo myšleno > ruzne funkce)
                      - topiSe promena aby bylo jasne jestli se topí nebo už vyhaslo
                      -
                      - udelat selftest>
                        - nachystáno čtení výsupů SSD relé
                        - postupně zapínat a zjištovat funkčnost
                        - popřípade znemožnit zátop > nevím jak
                      -

*/

void(* resetFunc)(void) = 0;

//displej
#include <U8g2lib.h>
U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, 46, 44, 45, U8X8_PIN_NONE);

//komunikace s ESP
#include <SoftwareSerial.h>
SoftwareSerial mySerial(10, 4); // RX, TX 15 , 14      old 10 , 9

//eeprom
#include <EEPROM.h>

//čidla teplot
#include <OneWire.h>
#include <DallasTemperature.h>

OneWire oneWire1(8); // kotel
DallasTemperature sensors1(&oneWire1);

OneWire oneWire2(9); // akumulacky
DallasTemperature sensors2(&oneWire2);

OneWire oneWire3(5); //bojler
DallasTemperature sensors3(&oneWire3);

OneWire oneWire4(14); // venkovní cidlo
DallasTemperature sensors4(&oneWire4);

#include <max6675.h> // termoclánek
MAX6675 termoclanek(11, 12, 13);


//stavové WS ledky na panelu
#include <Adafruit_NeoPixel.h>
#define pinDIN 47
#define pocetLED 6
Adafruit_NeoPixel rgbWS = Adafruit_NeoPixel(pocetLED, pinDIN, NEO_GRB + NEO_KHZ800);
byte red, green, blue ;
int cas = 500 ;  //startovní rychlost blikaní
int brid;

//RC servo
#include <Servo.h>
Servo myservo;
int uhelRC = 180;


//tlacitka a přepínače na panelu
byte but1;
byte but2;
byte sw1;
byte sw2;
byte sw3; // přidelany pro připad nejede internet a je potrewba topit

bool Last_but1 = 1;
bool Last_but2 = 1;

//přijate z webu hodnoty /r jako receive
int rT1; //nastaveni bojleru
float rT2; //teplota doma
float rT3; //nastavena teplota doma
byte rval; //rozhodovací hodnota pro vytapeni z akumulacek
byte rcheck; // pouze kontrolní hodnota jestli data došla z webu v pořádku

unsigned long previousTime = millis();
unsigned long intervalTime = 60000; // jak často posílat do databáze

//meření napětí 220
bool mv1, mv2, mv3, mv4, mv5, mv6, mv7, mv8; // mearurment voltage, meřeni detektorem sitoveho napajení
// pro kontrolu jestli je ssr rele sepnuto nebo vypnuto, a detekci závady

//servo kotel
int pocitadlo ;
byte stopServoDown = 5;
int angleServo ; //pouze pro info
byte topiSe = 0 ; // promena ktera 0 = netopí se , 1 = topí se

//aktualně namerené hodnoty z panelu > potenciometry
byte konstant1 ;
byte hystereze1 ;
byte uhel1 ;
byte interval1 ;

byte konstant2 ;
byte hystereze2 ;
byte uhel2 ;
byte interval2 ;

byte E_konstant1, E_hystereze1, E_uhel1, E_interval1, E_konstant2, E_hystereze2, E_uhel2, E_interval2; //data z eeprom


//servo akumulacky //řízení akumulacek
int pocitadlo2 ;
int angleServo2  ;
byte A_mod;   //mod režimu ovládáni akumulacek servo 2
byte P3;
byte proKlidDuse = 0 ; // snad se do budoucna oddělá při přetopení aku fakt na max topí domů

int ekviterm;

//mod 1 provoz z kotle
//mod 2 nahřáti akumulacek
//mod 3 natopené akumulacky > topí zas do domu
//mod 4 topení puze z amululacek


//rízení spínaní bojleru
byte counter2 ;
byte P2 ;

//testovaci potaky
byte potT1, potT2;

// menu displeje
byte menu;
int changeScreen;

//kotel
uint8_t sensor1[8] = { 0x28, 0xC3, 0xE8, 0x2F, 0x07, 0x00, 0x00, 0xAC };
uint8_t sensor2[8] = { 0x28, 0xDF, 0x89, 0x6B, 0x07, 0x00, 0x00, 0xB8 };
uint8_t sensor3[8] = { 0x28, 0x13, 0x54, 0x00, 0x09, 0x00, 0x00, 0x18 };
uint8_t sensor4[8] = { 0x28, 0x8E, 0x4F, 0x6A, 0x07, 0x00, 0x00, 0x90 };

int T1, T2, T3, T4 ;

//teplomery dallas pro aku
uint8_t sensor6[8] = { 0x28, 0x82, 0x46, 0xA5, 0x41, 0x23, 0x0B, 0xE9 };
uint8_t sensor7[8] = { 0x28, 0x19, 0x90, 0x16, 0x80, 0x22, 0x09, 0xFB };
uint8_t sensor8[8] = { 0x28, 0xCD, 0xCA, 0xD8, 0x41, 0x23, 0x0B, 0x67 };
uint8_t sensor10[8] = { 0x28, 0xCF, 0x4A, 0x25, 0x81, 0x22, 0x07, 0x1A };

//byte T1_akum, T2_akum, T3_akum, T4_akum, T5_akum, T6_akum, T7_akum, T8_akum ; // hodnoty čidel teploty v akum

byte T1_aku, T2_aku, T3_aku, T4_aku;
int Aku_energy; //energie v aku v%


//dalsí
uint8_t sensor11[8] = { 0x28, 0x50, 0x51, 0x00, 0x09, 0x00, 0x00, 0xE6 }; // bojler
uint8_t sensor12[8] = { 0x28, 0x17, 0x28, 0x60, 0x07, 0x00, 0x00, 0xEC }; // venkovní

int T11, T12;

//teplomer komín
int T13 ;
int fT13; //filtrovaná teplota

//klapka ventilátor
byte  fanvoltage;
byte  klapka;
byte fanDB;
byte klapkaDB;


void setup() {
  Serial.begin(9600);
  mySerial.begin(2400); // uart pro esp
  u8g2.begin(); //diplay
  rgbWS.begin(); //ws paanel
  sensors1.begin(); // dallas čidla
  sensors2.begin(); // dallas čidla
  sensors3.begin(); // dallas čidla
  sensors4.begin(); // dallas čidla

  myservo.attach(2);
  myservo.write(uhelRC);


  pinMode(53, INPUT_PULLUP);//but1
  pinMode(51, INPUT_PULLUP);//but2
  pinMode(52, INPUT_PULLUP);//sw1
  pinMode(50, INPUT_PULLUP);//sw2

  pinMode(25, INPUT_PULLUP);//sw3 dalsí nejede interne

  // pinMode(7, OUTPUT); //fet na 3.3v out

  pinMode(36, INPUT);//voltage read 1
  pinMode(37, INPUT);//voltage read 2
  pinMode(38, INPUT);//voltage read 3
  pinMode(39, INPUT);//voltage read 4
  pinMode(40, INPUT);//voltage read 5
  pinMode(41, INPUT);//voltage read 6
  pinMode(42, INPUT);//voltage read 7
  pinMode(43, INPUT);//voltage read 8

  pinMode(35, OUTPUT); //ssd rele 1
  pinMode(34, OUTPUT); //ssd rele 2
  pinMode(33, OUTPUT); //ssd rele 3  čerpado okruh doma
  pinMode(32, OUTPUT); //ssd rele 4  čerpadlo na dohřívaní bojléru
  pinMode(31, OUTPUT); //ssd rele 5  Sehro2 nahoru
  pinMode(30, OUTPUT); //ssd rele 6  Servo2 dolu
  pinMode(29, OUTPUT); //ssd rele 7  Servo1 nahoru
  pinMode(28, OUTPUT); //ssd rele 8  Servo2 dolu

  pinMode(20, INPUT_PULLUP); //senzor 220v napetí
  pinMode(21, INPUT_PULLUP); //optozávota klapky komína
  pinMode(22, OUTPUT); //fet 3.3v podvit displej

  //načte promnené z DB
  E_konstant1 = EEPROM.read(1);        //konstant
  E_hystereze1 = EEPROM.read(2);        //hystereze
  E_uhel1 = EEPROM.read(3);             //uhel
  E_interval1 = EEPROM.read(4);         //interval

  E_konstant2 = EEPROM.read(5);        //konstant
  E_hystereze2 = EEPROM.read(6);        //hystereze
  E_uhel2 = EEPROM.read(7);             //uhel
  E_interval2 = EEPROM.read(8);         //interval

  getTemperature();

  Serial.println("************* NEW SESION START *************");

}

void loop() {


  read_panel();
  communication();
  displej_write();
  ControlServo1(); //kotlové
  ControlServo2(); //za AKU
  ohrevBojler();
  serialRead();
  //ControlRCservo(); v contServo1
  delay(10);

}

void ControlRCservo() {
  //165 se dotkne plně zavřené klapky
  //157 bych dal jako limataci
  //151 normalní provoz
  //zatím na zkoušku řízeno pouze touto podmínkou poté se může dále doladit

  if (T13 > 45) {

    if (T4_aku > 45) { //limitace kotle
      uhelRC = 165;
    }
    else {
      uhelRC = 145;
    }
  }

  else {
    uhelRC = 180;
  }

  myservo.write(uhelRC);
  Serial.println("RIDIM SERVO RC");
  uhelRC = uhelRC + topiSe ;
  Serial.println(uhelRC);
}


void read_panel() { // čte výstupy z pot a tlacite

  but1 = digitalRead(53);
  but2 = digitalRead(51);


  if (Last_but1 == 1 && but1 == 0)
  {
    Serial.println("Ukladam do eeprom");
    setEeprom();
  }
  Last_but1 = but1;


  if (Last_but2 == 1 && but2 == 0)
  {
    nastavRGB(brid, 0  , 0  , 5);
    nastavRGB(brid, 0  , 0  , 4);
    nastavRGB(brid, 0  , 0  , 3);
    nastavRGB(brid, 0  , 0  , 2);
    Serial.println("Manualní zavřerní serv kotle a za aku");
    digitalWrite(28, 1);
    digitalWrite(30, 1);
    delay (60000); // delka jednoho kroku
    digitalWrite(28, 0);
    digitalWrite(30, 0);
    angleServo = 0 ;
    angleServo2 = 0;
    nastavRGB(0, 0  , 0  , 5);
    nastavRGB(0, 0  , 0  , 4);
    nastavRGB(0, 0  , 0  , 3);
    nastavRGB(0, 0  , 0  , 2);
  }
  Last_but2 = but2;



  sw1 = digitalRead(52);
  sw2 = digitalRead(50);
  sw3 = digitalRead(25); ///xx

  if (sw3 == 0 ) {
    nastavRGB(brid, 0  , 0  , 1);
    nastavRGB(brid, 0  , 0  , 6);
  }
  else {
    nastavRGB(0, 0  , 0  , 1);
    nastavRGB(0, 0  , 0  , 6);
  }

  if ((sw1 == 0) && (sw2 == 0)) {
    menu = 5 ;
  }
  else {
    if (sw1 == 0) menu = 3 ;
    if (sw2 == 0) menu = 4 ;
  }

  if ((sw1 == 1) && (sw2 == 1)) {
    menu = 1 ;
  }


  konstant1 = map(analogRead(A15), 0, 1023, 50, 88);
  hystereze1 = map(analogRead(A14), 0, 1023, 1, 6);
  uhel1 = map(analogRead(A13), 0, 1023, 2, 10);
  interval1 = map(analogRead(A12), 0, 1023, 1, 5);

  konstant2 = map(analogRead(A11), 0, 1023, 50, 80);
  hystereze2 = map(analogRead(A10), 0, 1023, 40, 60); // teplota do domu!!
  uhel2 = map(analogRead(A9), 0, 1023, 2, 10);
  interval2 = map(analogRead(A8), 0, 1023, 1, 5);

  potT1 = map(analogRead(A0), 0, 1023, 15, 100);
  potT2 = map(analogRead(A1), 0, 1023, 15, 100);


  //zaplne podsvit displaje když ve rozsvítí v kotelně a nastaví většá jas na ws ledkách
  int ldr = analogRead(A7);

  if (ldr > 150 ) {
    brid = 200 ;
    digitalWrite(22, 1);
  }
  else {
    brid = 15;
    digitalWrite(22, 0);
  }


  //detekce zapnuteho ventilátoru na kotlu a otevřená klapka komín. když aktivní tak se aktivuje po odeslání do DB se zruší.
  fanvoltage = digitalRead(20);
  klapka = digitalRead(21);

  if (fanvoltage == 0) {
    fanDB = 10;
  }

  if (klapka == 1) {
    klapkaDB = 8;
  }

  //výpočet nabití aku v % pomocí lineární interpolace
  int Avg_aku = ((T1_aku + T2_aku + T3_aku + T4_aku) / 4); //prumer teploty obou aku

  Aku_energy = ((Avg_aku - 30) * (100 - 0)) / (83 - 30); //nabití akumulacek v % kdy beru že 0 je 30C a plno 83C

  if (Aku_energy < 0 ) Aku_energy = 0; //nebude posílat do DB záporné hodnoty

}

void ControlServo1()
{

  if (pocitadlo > (E_interval1 * 60) * 1 ) //(pocitadlo > (interval * 60) * 15 ) takle to bylo na minuty
  {
    getTemperature();
    ControlRCservo();

    nastavRGB(0, 0  , brid  , 4);
    nastavRGB(0, 0  , brid  , 3);

    if ((T13 > 45) || (T2 > 65)) {

      //Serial.println("Jsem az v podmínce co ridi servo kotel");

      if (T1 > (E_konstant1 + E_hystereze1))
      {
        servo1up();
      }

      if (T1 < (E_konstant1 - E_hystereze1))
      {
        servo1down();
      }


    }

    if (T13 != 0 ) { //tohle je zde z důvodu ze nejak vypadla komunkace a přisla 0 tohle to zabrání

      if (T13 < 40 ) { // zavře kotlové servo po vyhasnutí

        if (angleServo > 1) {
          digitalWrite(28, 1);
          Serial.println("S1 Dolu konec topení!");
          delay (120000); //
          angleServo = 0 ;
        }
      }
    }

    pocitadlo = 0;
    nastavRGB(0, 0  , 0  , 4);
    nastavRGB(0, 0  , 0  , 3);
  }

  else pocitadlo++ ;
}

void servo1up() {
  nastavRGB(0, brid  , 0  , 5);
  digitalWrite(29, 1);
  digitalWrite(28, 0);
  Serial.println("S1 NAHORU");
  delay (E_uhel1 * 1000); // delka jednoho kroku
  digitalWrite(29, 0);
  digitalWrite(28, 0);
  nastavRGB(0, 0  , 0  , 5);

  for (int i = 0; i < (E_uhel1) ; i++) angleServo++;
  if (angleServo >= 120) angleServo = 120 ;
}

void servo1down() {
  nastavRGB(0, brid  , 0  , 6);
  digitalWrite(29, 0);
  digitalWrite(28, 1);
  Serial.println("S1 DOLU");
  delay (E_uhel1 * 1000); // delka jednoho kroku
  digitalWrite(29, 0);
  digitalWrite(28, 0);
  nastavRGB(0, 0  , 0  , 6);

  for (int i = 0; i < (E_uhel1) ; i++) angleServo--;
  if (angleServo <= -1) angleServo = 0 ;

}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



void ControlServo2() //ovladáni akumulacek
{
  if (pocitadlo2 > (E_interval2 * 80) * 1 )
  {
    nastavRGB(brid, 0  , 0  , 4);
    nastavRGB(brid, 0  , 0  , 3);

    ekviterma() ; //zjistí si požadovanou teplotu

    /*int rT1; //nastaveni bojleru
      float rT2; //teplota doma
      float rT3; //nastavena teplota doma
      byte rval; //rozhodovací hodnota pro vytapeni z akumulacek
      byte rcheck; // pouze kontrolní hodnota jestli data došla z webu v pořádku*/

    // první funkce určení modu pro provoz  --> mod 1 provoz z kotle    mod 2 nahřáti akumulacek    mod 3 natopené akumulacky > topí zas do domu! :)   mod 4 topení puze z amululacek

    A_mod = 0;// nastaví se na 0 pak se po splnení podmínek nastaví


    if (((T13 > 49) || (T2 > 79)) && (T1 > E_konstant1 - E_hystereze1 - 10)) { //mod 1 provoz z kotle do domu
      A_mod = 1;
    }

    if ((rval == 0) && (T13 > 49) && (T1 > E_konstant1 - E_hystereze1 - 10)) { //mod 2 nahřáti akumulacek ..(rT2 > rT3) nahrazen > rval == 0
      A_mod = 2;
    }

    if ((T13 > 49) && (T1 > E_konstant1 - E_hystereze1 - 5) && (T4_aku > 60) ) { //mod 3 natopené akumulacky > limituje kotel <- jeste v tomto modu pokud chce dum teplo tak mu ho dá i pře naplnením aku
      A_mod = 3;
    }

    if ((rval >= 1) && (T13 < 49) && (T1_aku > 30) ) { //mod 4 topení puze z amululacek (rT2 < rT3) && (T1 < (E_konstant1 - E_hystereze1 - 5))
      A_mod = 4;
    }


    ///////////////////////////////////

    if ((A_mod == 1) || (A_mod == 4)) {         // zapne obehové čerpadlo dom okruh
      digitalWrite(33, 1);
      P3 = 12 ;
    }

    /////řídí servo za aku

    if ((A_mod == 1) && (T1_aku > 65)) { // tady to ceká než se nahřeje trochu aku aby se kotel plynule rozjel

      if ( rT1 == 5 ) {
        if (T3 < (ekviterm - 2) )
        {
          servo2up();
        }

        if (T3 > (ekviterm + 2 ))
        {
          servo2down();
        }

      }

      else {

        if (T3 < ((hystereze2) - 2)) //hystereze2 == pozadovaná teplota do domu /ješte snížená teplota o 5 stupnu aby dom ..
        {
          servo2up();
        }

        if (T3 > ((hystereze2) + 2) )
        {
          servo2down();
        }

      }
    }
    ///////
    if (A_mod == 2) { // přiškrtí teplo do domu počká na natopení aku > přejde do modu 3, takze natopeny aku a pokračuje v topení do domu.

      if (T3 > (25 + E_hystereze1)) //hystereze2 == pozadovaná teplota do domu
      {
        servo2down();
      }

      if (T3 < (25 - E_hystereze1) )
      {
        servo2up();
      }
    }
    //////               zmena nechá se az po plno natopit akumulacky
    if (A_mod == 3) {


      if (T4_aku > 84) {
        proKlidDuse = 1;
      }

      else if (T4_aku < 60) {
        proKlidDuse = 0;
      }

      /////////************tady to obejde funkci a pokračuje v topení do domu bez tohoto to v mode 3 čekalo na naplnení spodní nadrze na 82
      if (rval >= 1) {
        proKlidDuse = 1;
      }
      ////////***********

      if (proKlidDuse == 1 ) {

        digitalWrite(33, 1);
        P3 = 13 ;

        if (T3 < (E_hystereze2 - E_hystereze1)) //hystereze2 == pozadovaná teplota do domu
        {
          servo2up();
        }

        if (T3 > (E_hystereze2 + E_hystereze1) )
        {
          servo2down();
        }


      }

      else {
        digitalWrite(33, 0);
        P3 = 0 ;

        if (T3 > 27)
        {

          servo2down();
        }

        if (T3 < 24 )
        {
          servo2up();
        }
      }
    }




    //////
    if (A_mod == 4) {

      if ( rT1 == 5 ) {

        if  (T3 < (ekviterm - 2))
        {
          servo2up();
        }

        if (T3 > (ekviterm + 2) )
        {
          servo2down();
        }

      }

      else {

        if (T3 < ((hystereze2) - 2)) //hystereze2 == pozadovaná teplota do domu /ješte snížená teplota o 5 stupnu aby dom ..
        {
          servo2up();
        }

        if (T3 > ((hystereze2) + 2) )
        {
          servo2down();
        }

      }

    }
    /////////////////////

    if ((A_mod == 0) || (A_mod == 2)) { //snad bude fungovat > netopí se, doma je dostatečná teplota, vypne oběhové čerpadlo a nastaví servo2 do uzavřené polohy(pomocí delay)
      digitalWrite(33, 0); //vypne čerpadlo
      P3 = 0 ;

      if (angleServo2 > 1) { // tohle ve veliká očůrávka ale je to jednoduché a snad funkční

        Serial.println("mod je 0 > zavírám servo na 0");
        digitalWrite(30, 1);
        delay(120000);
        digitalWrite(30, 0);
        angleServo2 = 0 ;
        Serial.println("servo2 zavřeno");
      }
    }

    //////////////////

    Serial.println("MOD servo akumulacky");
    Serial.print(A_mod);
    Serial.println("");
    pocitadlo2 = 0;
    nastavRGB(0, 0  , 0  , 4);
    nastavRGB(0, 0  , 0  , 3);
  }

  else pocitadlo2++ ;
}

void servo2up() {

  if (angleServo2 < 120 ) {
    nastavRGB(0, 0  , brid  , 5);
    digitalWrite(31, 1);
    digitalWrite(30, 0);
    Serial.println("S2 NAHORU");
    delay (E_uhel2 * 1000); // delka jednoho kroku
    digitalWrite(31, 0);
    digitalWrite(30, 0);
    nastavRGB(0, 0  , 0  , 5);
  }

  else {
    Serial.println("S2 nemuze jit nahoru");
    nastavRGB(0, 0  , brid  , 5);
    delay(100);
    nastavRGB(0, 0  , 0  , 5);
    delay(100);
    nastavRGB(0, 0  , brid  , 5);
    delay(100);
    nastavRGB(0, 0  , 0  , 5);
    delay(100);
    nastavRGB(0, 0  , brid  , 5);
    delay(100);
    nastavRGB(0, 0  , 0  , 5);
  }



  if (angleServo2 >= 120) angleServo2 = 120 ;
  for (int i = 0; i < (E_uhel2) ; i++) angleServo2++;
}

void servo2down() {
  if (angleServo2 > 0 ) {
    nastavRGB(0, 0  , brid  , 6);
    digitalWrite(31, 0);
    digitalWrite(30, 1);
    Serial.println("S2 DOLU");
    delay (E_uhel2 * 1000); // delka jednoho kroku
    digitalWrite(31, 0);
    digitalWrite(30, 0);
    nastavRGB(0, 0  , 0  , 6);
  }
  else {
    Serial.println("S2 nemuze jit dolu");
    nastavRGB(0, 0  , brid  , 6);
    delay(100);
    nastavRGB(0, 0  , 0  , 6);
    delay(100);
    nastavRGB(0, 0  , brid  , 6);
    delay(100);
    nastavRGB(0, 0  , 0  , 6);
    delay(100);
    nastavRGB(0, 0  , brid  , 6);
    delay(100);
    nastavRGB(0, 0  , 0  , 6);
  }


  for (int i = 0; i < (E_uhel2) ; i++) angleServo2--;
  if (angleServo2 <= 0) angleServo2 = 0 ; //tohle sice není technicky reálně splnitelné, ale promena stopServoDown to umožní.. jít až do -40 tak proto

}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ekviterma() {


  /* Serial.println(rT1) ;//5= povolen ekviterm
    Serial.println(rT2) ;//teplota doma
    Serial.println(rT3) ;//nastavena teplota doma
    Serial.println(rval) ;//rozhodovací hodnota pro vytapeni z akumulacek
    Serial.println(rcheck) ;// pouze kontrolní hodnota jestli data došla z webu v pořádku
    Serial.println("") ;*/


  // zde je použit výpečet ekviterní křivky pro maximální usporu tepla z akumulaček
  //float Tm ;
  float Tw1_max = 60; // max teplota do systemu
  float Tw2_max = 40; // minimální teplota do domu
  float Ti = rT2; //Vnitřní výpočtová teplota .. tj pozadovaná teplota doma
  float Te = T12 ; // teplota venku
  float Te_min = -20 ; //Minimální venkovní výpočtová teplota
  float n = 1.3 ; //Teplotní exponent soustavy
  ekviterm = ((((Tw1_max + Tw2_max) / 1.76 ) - Ti) * pow(((Te - Ti) / (Te_min - Ti)), (1 / n))) + Ti;

  if ( (ekviterm < 30) || (ekviterm > 55)) ekviterm = 55 ; // když by se neco stalo at to nepusí moc teplé do domu
  Serial.println(ekviterm);

}


void ohrevBojler() {

  if (counter2 >= 120 ) {
    nastavRGB(0, brid  , 0  , 4);
    nastavRGB(0, brid  , 0 , 3);
    int T_bojler = T11 ;

    if (menu == 1) { // vyply letni provoz

      if ( (T11 > 52) && ( T3 > 70) ) {    //int T_bojler = T11 ;
        zap_cerpadlo_bojler();
      }
      else {

        if ( ((A_mod == 0) || (A_mod == 1) || (A_mod == 2) || (A_mod == 3) || (A_mod == 4)) && (T_bojler > 53) && (T1_aku > 72) ) {

          if (T_bojler < (65)) {
            zap_cerpadlo_bojler();
          }

          else if (T_bojler > (70)) {
            vyp_cerpadlo_bojler();
          }
        }
      }

      if (T1_aku < 70) {        // když by v průběhu dohřívání bojleru klesla T1_aku pod 80 tohle zastaví čerpadlo
        vyp_cerpadlo_bojler();
      }

    }

    else { // letni MODE

      if (T_bojler > (80)) {
        zap_cerpadlo_bojler();
      }

      else if (T_bojler < (70)) {
        vyp_cerpadlo_bojler();
      }

    }



    counter2 = 0 ;
    nastavRGB(0, 0  , 0  , 4);
    nastavRGB(0, 0  , 0 , 3);
  }

  counter2++;

}


void zap_cerpadlo_bojler() {
  digitalWrite(32, 1); // Relé se zapne
  P2 = 15 ; // promena do DB že je čerpadlo zapnuto
  nastavRGB(brid, 0  , 0  , 2);
}
void vyp_cerpadlo_bojler() {
  digitalWrite(32, 0); // Relé se vypne
  P2 = 0 ;
  nastavRGB(0, 0  , 0  , 2);
}


void displej_write() {

  switch (menu) {

    case 1:
      if (changeScreen < 100) {

        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_tenthinguys_tf);
        u8g2.drawStr(6, 10, "=KOTEL + AKUM =");


        u8g2.setFont(u8g2_font_helvR08_tf);

        u8g2.drawStr(2, 22, "Kotel:");
        u8g2.setCursor(40, 22);
        u8g2.print(T2);

        u8g2.drawStr(2, 36, "Zpatec:");
        u8g2.setCursor(40, 36);
        u8g2.print(T1);

        u8g2.drawStr(2, 50, "Vystup:");
        u8g2.setCursor(40, 50);
        u8g2.print(T3);

        u8g2.drawStr(2, 64, "Vratka:");
        u8g2.setCursor(40, 64);
        u8g2.print(T4);

        //aku 2
        u8g2.drawStr(65, 20, "AKU");

        u8g2.setCursor(70, 31);
        u8g2.print(T1_aku);

        u8g2.setCursor(70, 42);
        u8g2.print(T2_aku);

        u8g2.setCursor(70, 53);
        u8g2.print(T3_aku);

        u8g2.setCursor(70, 64);
        u8g2.print(T4_aku);


        //aku 1
        u8g2.drawStr(98, 22, "ENE");

        u8g2.setCursor(100, 36);
        u8g2.print(Aku_energy);



        u8g2.setCursor(90, 50);
        u8g2.print(ekviterm);


        u8g2.setCursor(112, 50);
        u8g2.print(T12);


        u8g2.sendBuffer();
      }

      else {
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_tenthinguys_tf);
        u8g2.drawStr(6, 10, "=KOTEL + AKUM =");

        u8g2.setFont(u8g2_font_helvR08_tf);

        u8g2.drawStr(2, 22, "Komin:");
        u8g2.setCursor(40, 22);
        u8g2.print(T13);

        u8g2.drawStr(2, 36, "Uhel1:");
        u8g2.setCursor(40, 36);
        u8g2.print(angleServo);

        u8g2.drawStr(2, 50, "Uhel2:");
        u8g2.setCursor(40, 50);
        u8g2.print(angleServo2);

        u8g2.drawStr(2, 64, "MODE:");
        u8g2.setCursor(40, 64);
        u8g2.print(A_mod);
        //**
        u8g2.drawStr(60, 22, "T-doma:");
        u8g2.setCursor(97, 22);
        u8g2.print(rT2);

        u8g2.drawStr(60, 36, "T-nast:");
        u8g2.setCursor(97, 36);
        u8g2.print(rT3);

        u8g2.drawStr(60, 50, "T-boj:");
        u8g2.setCursor(90, 50);
        u8g2.print(rT1);

        u8g2.drawStr(110, 50, "(");
        u8g2.setCursor(112, 50);
        u8g2.print(T11);
        u8g2.drawStr(122, 50, ")");



        //mod 1 provoz z kotle    mod 2 nahřáti akumulacek    mod 3 natopené akumulacky > topí zas do domu! :)   mod 4 topení puze z amululacek
        if (A_mod == 0) {
          u8g2.drawStr(60, 64, "CEKAM..");
        }

        if (A_mod == 2) {
          u8g2.drawStr(60, 64, "KOTEL>AKU");
        }

        if ((A_mod == 1) || (A_mod == 3)) {
          u8g2.drawStr(60, 64, "KOTEL>DUM");
        }

        if (A_mod == 4) {
          u8g2.drawStr(60, 64, "AKU>DUM");
        }

        u8g2.sendBuffer();

        if (changeScreen > 180) changeScreen = 0;

      }

      changeScreen++;

      break;

    case 2:
      /*u8g2.setFont(u8g2_font_ncenB10_tr);  // první kraj.. kolik od vrchu
        u8g2.drawStr(17, 15, "Nast. teploty");
        u8g2.drawStr(43, 30, "doma :");
        u8g2.setCursor(45, 45);
        u8g2.print(teplota);*/

      break;

    case 3:
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_tenthinguys_tf);
      u8g2.drawStr(12, 10, "= NAS. KOTEL =");

      u8g2.setFont(u8g2_font_helvR08_tf);
      u8g2.drawStr(2, 22, "Udrzov teplota:");
      u8g2.setCursor(85, 22);
      u8g2.print(konstant1);

      u8g2.setCursor(100, 22);
      u8g2.print(E_konstant1);

      u8g2.drawStr(2, 36, "Hystereze:");
      u8g2.setCursor(85, 36);
      u8g2.print(hystereze1);
      u8g2.setCursor(100, 36);
      u8g2.print(E_hystereze1);

      u8g2.drawStr(2, 50, "Uhel serva:");
      u8g2.setCursor(85, 50);
      u8g2.print(uhel1);
      u8g2.setCursor(100, 50);
      u8g2.print(E_uhel1);

      u8g2.drawStr(2, 64, "Cas interval:");
      u8g2.setCursor(85, 64);
      u8g2.print(interval1);
      u8g2.setCursor(100, 64);
      u8g2.print(E_interval1);

      u8g2.sendBuffer();

      break;

    case 4:
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_tenthinguys_tf);
      u8g2.drawStr(15, 10, "= NAS. AKUM =");

      u8g2.setFont(u8g2_font_helvR08_tf);
      u8g2.drawStr(2, 22, "Pozad.Tepl Aku:");
      u8g2.setCursor(85, 22);
      u8g2.print(konstant2);
      u8g2.setCursor(100, 22);
      u8g2.print(E_konstant2);

      u8g2.drawStr(2, 36, "Tepl do domu:");
      u8g2.setCursor(85, 36);
      u8g2.print(hystereze2);
      u8g2.setCursor(100, 36);
      u8g2.print(E_hystereze2);

      u8g2.drawStr(2, 50, "Uhel serva:");
      u8g2.setCursor(85, 50);
      u8g2.print(uhel2);
      u8g2.setCursor(100, 50);
      u8g2.print(E_uhel2);

      u8g2.drawStr(2, 64, "Cas interval:");
      u8g2.setCursor(85, 64);
      u8g2.print(interval2);
      u8g2.setCursor(100, 64);
      u8g2.print(E_interval2);

      u8g2.sendBuffer();
      break;


    case 5:
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_tenthinguys_tf);
      u8g2.drawStr(6, 10, "= LETNI PROVOZ =");

      u8g2.setFont(u8g2_font_helvR08_tf);

      u8g2.drawStr(25, 22, "MOD pro pripad");

      u8g2.drawStr(8, 36, "pretopeni bojleru solarem");

      u8g2.drawStr(25, 50, "Teplota bojler:");
      u8g2.setCursor(90, 50);
      u8g2.print(T11);

      u8g2.drawStr(5, 64, "PRI TOPENI VYPNOUT!!!");


      u8g2.sendBuffer();
      break;

  }


}



void setEeprom() {
  nastavRGB(0, brid  , 0  , 2);
  nastavRGB(brid, 0  , 0  , 3);
  nastavRGB(brid, 0  , 0  , 4);
  nastavRGB(0, brid  , 0  , 5);

  EEPROM.update(1, konstant1) ;
  EEPROM.update(2, hystereze1) ;
  EEPROM.update(3, uhel1) ;
  EEPROM.update(4, interval1) ;

  EEPROM.update(5, konstant2) ;
  EEPROM.update(6, hystereze2) ;
  EEPROM.update(7, uhel2) ;
  EEPROM.update(8, interval2) ;

  E_konstant1 = EEPROM.read(1);        //konstant
  E_hystereze1 = EEPROM.read(2);        //hystereze
  E_uhel1 = EEPROM.read(3);             //uhel
  E_interval1 = EEPROM.read(4);         //interval

  E_konstant2 = EEPROM.read(5);        //konstant
  E_hystereze2 = EEPROM.read(6);        //hystereze
  E_uhel2 = EEPROM.read(7);             //uhel
  E_interval2 = EEPROM.read(8);         //interval

  nastavRGB(0, 0  , 0  , 2);
  nastavRGB(0, 0  , 0  , 3);
  nastavRGB(0, 0  , 0  , 4);
  nastavRGB(0, 0  , 0  , 5);



  //varialbeDB();
}


void communication()
{
  unsigned long currentTime = millis();
  if (currentTime - previousTime > intervalTime) {
    previousTime = currentTime;

    nastavRGB(0, brid  , 0  , 1);
    //a, b, c, č, d, e, f, g, h, ch, i, j, k, l, m, n, o, p, q, r, ř, s, š, t, u, v, w, x, y, z, ž.

    String a = "T1=" ;
    String b = "&T2=" ;
    String c = "&T3=" ;
    String d = "&T4=" ;

    String e = "&T5=" ;
    String f = "&T6=" ;
    String g = "&T7=" ;
    String h = "&T8=" ;
    String i = "&T9=" ; // misto teploty aku se posíla energie v %
    String j = "&T10=" ; // uhel RC serva

    String k = "&T11=" ;//bojler
    String l = "&T12=" ;//venkov
    String m = "&T13=" ; //komín
    String n = "&T14=" ; //použito na MODE pro servo2

    String o = "&F1=" ;//ventilátor kotel
    String p = "&P1=" ; //cerpadlo kotel nijak se nedetekue .. použito na ekviterm
    String q = "&P2=" ; // cerpadlo bojler
    String r = "&P3=" ; // cerpadlo dum
    String s = "&S1=" ; //uhel serva kotel
    String t = "&S2=" ; //uhel serva aku
    String u = "&K1=" ;//tatím nezapojeno pouzito na fT13

    String all = a + T1 + b + T2 + c + T3 + d + T4 + e + T1_aku + f + T2_aku + g + T3_aku + h + T4_aku + i + Aku_energy + j + uhelRC + k + T11 + l + T12 + m + T13 + n + A_mod + o + fanDB + p + ekviterm + q + P2 + r + P3 + s + angleServo + t + angleServo2 + u + fT13;
    //T1=20&T2=22&T3=27&T4=25&T5=129&T6=55&T7=34&T8=55&T9=69&T10=53&T11=56&T12=-127&T13=21&T14=0&F1=0&P1=1&P2=15&P3=0&S1=0&S2=0&K1=8

    mySerial.print(all);
    Serial.println(all);
    all = "";

    klapkaDB = 0; //po odeslaní se vyruší
    fanDB = 0;

    delay(20);
    nastavRGB(0, 0  , 0  , 1);
  }

  if (mySerial.available() > 0) { //příjem dat z ESP
    nastavRGB(0, 0  , brid  , 1);
    String data_rcvd = mySerial.readStringUntil('\n');  //swSer('\n')
    Serial.println(data_rcvd);
    data_rcvd.trim(); //odstraní nechtené mezery před stringem

    char *token = strtok((char *)data_rcvd.c_str(), ",");
    int index = 1; // Index pro sledování, kterou proměnnou nastavujeme

    while (token != NULL) {
      switch (index) {
        case 1:
          rT1 = atof(token); // Převedení řetězce na float a uložení do T1
          break;
        case 2:
          rT2 = atof(token);
          break;
        case 3:
          rT3 = atof(token);
          break;
        case 4:
          rval = atof(token);
          break;
        case 5:
          rcheck = atof(token);
          break;
        default:
          break;
      }

      token = strtok(NULL, ",");
      index++;
    }


    Serial.println("Prijate data T1 T2 T2 VAL CHECK") ;
    Serial.println(rT1) ;//5= povolen ekviterm
    Serial.println(rT2) ;//teplota doma
    Serial.println(rT3) ;//nastavena teplota doma
    Serial.println(rval) ;//rozhodovací hodnota pro vytapeni z akumulacek
    Serial.println(rcheck) ;// pouze kontrolní hodnota jestli data došla z webu v pořádku
    Serial.println("") ;

    if ( (rT2 < 15) || (rT2 > 30))  rT2 = 25 ;


    data_rcvd = "";
    nastavRGB(0, 0  , 0  , 1);
  }
  if (sw3 == 0) { // zde po přepnití tlačítka zapne ekv režim a začne topit :)
    rT1 = 5;
    rval = 1;
    rT2 = 23;
    rT3 = 11;
  }
}

void getTemperature() {
  nastavRGB(0, brid  , brid  , 3);

  sensors1.requestTemperatures();
  T1 = sensors1.getTempC(sensor1);  //
  T2 = sensors1.getTempC(sensor2); //
  T3 = sensors1.getTempC(sensor3); //
  T4 = sensors1.getTempC(sensor4); //

  sensors2.requestTemperatures();
  T1_aku = sensors2.getTempC(sensor8); //
  T2_aku = sensors2.getTempC(sensor6); //
  T3_aku = sensors2.getTempC(sensor10); //
  T4_aku = sensors2.getTempC(sensor7); //


  sensors3.requestTemperatures();
  T11 = sensors3.getTempCByIndex(0);  //bojlér
  if  ( T11 == -127 ) T11 = 80;

  sensors4.requestTemperatures();
  T12 = sensors4.getTempC(sensor12); // venkovní
  if  ( (T12 == -127) || (T12 > 50) ) T12 = -5;

  T13 = termoclanek.readCelsius(); // spaliny
  fT13 = getFilteredTemperature(T13);  // Vyhlazení hodnoty

  Serial.print("Raw: ");
  Serial.print(T13);
  Serial.print("  Filtered: ");
  Serial.println(fT13);

  if ( T13 <= 15 ) T13 = 80 ;

  if (T13 > 100 ) topiSe = 1;
  if (T13 < 70  ) topiSe = 0;

  nastavRGB(0, 0  , 0  , 3);

}

float filteredTemp = 0;  // Globální proměnná, aby uchovávala historii
float alpha = 0.7;       // Vyhlazovací konstanta

float getFilteredTemperature(float newTemp) {
  filteredTemp = alpha * newTemp + (1 - alpha) * filteredTemp;
  return filteredTemp;
}


void serialRead() {               // pomocník na odladění, psát do serioveho monitoru
  if (Serial.available() > 0) {
    String inString = Serial.readString();
    inString.trim(); //odstraní pripadne mezery před stringem

    if (inString == "S1U") {
      servo1up();
      inString = "" ;
    }

    if (inString == "S1D") {
      servo1down();
      inString = "" ;
    }

    if (inString == "S2U") {
      servo2up();
      inString = "" ;
    }

    if (inString == "S2D") {
      servo2down();
      inString = "" ;
    }

    if (inString == "S2U60") {
      servo2up60();
      inString = "" ;
    }

    if (inString == "S2D60") {
      servo2down60();
      inString = "" ;
    }

    if (inString == "S1U20") {
      servo1up20();
      inString = "" ;
    }

    if (inString == "S1D20") {
      servo1down20();
      inString = "" ;
    }

    if (inString == "C1ON") {
      cerpadlo1on();
      inString = "" ;
    }

    if (inString == "C1OFF") {
      cerpadlo1off();
      inString = "" ;
    }

    if (inString == "C2ON") {
      cerpadlo2on();
      inString = "" ;
    }

    if (inString == "C2OFF") {
      cerpadlo2off();
      inString = "" ;
    }

    if (inString == "RC+") {
      myservo.write(165);
      Serial.println(165);
      inString = "" ;
    }

    if (inString == "RC-") {
      myservo.write(145);
      Serial.println(145);
      inString = "" ;
    }

/*if (T13 > 45) {

    if (T4_aku > 45) { //limitace kotle
      uhelRC = 165;
    }
    else {
      uhelRC = 145;
    }*/

    if (inString == "help") {
      Serial.println("");
      Serial.println("------------HELP------------");

      Serial.println("reboot - restart");
      Serial.println("S1U - Servo kotel nahoru");
      Serial.println("S1D - Servo kotel dolu");
      Serial.println("S2U - Servo aku nahoru");
      Serial.println("S2D - Servo aku nahoru");
      Serial.println("S2U60 - Servo aku nahoru 60 sekund");
      Serial.println("S2D60 - Servo aku nahoru 60 sekund");
      Serial.println("C1ON - Zapne cerpadlo top. ok");
      Serial.println("C1OFF - Vypne cerpadlo top. ok");
      Serial.println("C2ON - Zapne cerpadlo bojler");
      Serial.println("C2OFF - Vypne cerpadlo bojler");
      Serial.println("RC+ - Pootevře servo o 1");
      Serial.println("RC+ - Prizavre servo o 1");

      Serial.println("------------END------------");
      Serial.println("");
      inString = "" ;
    }

    if (inString == "reboot") {
      Serial.println("Reboot...") ;
      delay(250);
      Serial.println("3");
      delay(250);
      Serial.println("2");
      delay(250);
      Serial.println("1");
      resetFunc();
    }
  }
}

void servo2up60() {
  digitalWrite(31, 1);
  Serial.println("S2 NAHORU 60ekund");
  delay (60000); // delka jednoho kroku
  digitalWrite(31, 0);
}


void servo2down60() {
  Serial.println("S2 DOLU 60sekund");
  digitalWrite(30, 1);
  delay (60000); // delka jednoho kroku
  digitalWrite(30, 0);
  angleServo2 = 0 ;

}

void servo1up20() {
  digitalWrite(29, 1);
  Serial.println("S1 NAHORU 20sekund");
  delay (20000); // delka jednoho kroku
  digitalWrite(29, 0);
}


void servo1down20() {
  Serial.println("S1 DOLU 20sekund");
  digitalWrite(28, 1);
  delay (20000); // delka jednoho kroku
  digitalWrite(28, 0);
  angleServo = 0 ;

}

void cerpadlo1on() {
  Serial.println("Cerpadlo top. ok. zapnuto");
  digitalWrite(33, 1);

}
void cerpadlo1off() {
  digitalWrite(33, 0);
  Serial.println("Cerpadlo top. ok. vypnuto");
}

void cerpadlo2on() {
  Serial.println("Cerpadlo bojler zapnuto");
  digitalWrite(32, 1);

}
void cerpadlo2off() {
  Serial.println("Cerpadlo bojler vypnuto");
  digitalWrite(32, 0);

}


void nastavRGB (byte r, byte g, byte b, int cislo) {
  uint32_t barva;
  barva = rgbWS.Color(r, g, b);
  rgbWS.setPixelColor(cislo - 1, barva);
  rgbWS.show();
}

/*
  20:19:35.726 -> 0x28, 0x60, 0x1E, 0x41, 0x80, 0x22, 0x06, 0x83  },
  20:19:35.726 ->   {
  20:19:35.726 -> 0x28, 0x82, 0x46, 0xA5, 0x41, 0x23, 0x0B, 0xE9  },
  20:19:35.773 ->   {
  20:19:35.773 -> 0x28, 0x19, 0x90, 0x16, 0x80, 0x22, 0x09, 0xFB  },
  20:19:35.773 ->   {
  20:19:35.773 -> 0x28, 0xCD, 0xCA, 0xD8, 0x41, 0x23, 0x0B, 0x67  },
  20:19:35.820 ->   {
  20:19:35.820 -> 0x28, 0xCB, 0x4A, 0xD6, 0x41, 0x23, 0x0B, 0x0B  },
  20:19:35.820 ->   {
  20:19:35.820 -> 0x28, 0xCF, 0x4A, 0x25, 0x81, 0x22, 0x07, 0x1A  },
  20:19:35.820 -> };

*/
/*
  void test_rele_volt() {

  digitalWrite(35, 1);
  //delay(250);
  digitalWrite(34, 1);
  //delay(250);
  digitalWrite(33, 1);
  //  delay(250);
  digitalWrite(32, 1);
  // delay(250);
  digitalWrite(31, 1);
  //delay(250);
  digitalWrite(30, 1);
  //delay(250);
  digitalWrite(29, 1);
  //delay(250);
  digitalWrite(28, 1);
  //delay(250);

  Serial.println("");
  Serial.println("Volt read all on");
  mv1 = digitalRead(36) ;
  mv2 = digitalRead(37) ;
  mv3 = digitalRead(38) ;
  mv4 = digitalRead(39) ;
  mv5 = digitalRead(40) ;
  mv6 = digitalRead(41) ;
  mv7 = digitalRead(42) ;
  mv8 = digitalRead(43) ;
  //delay(500);
  Serial.print(mv1) ;
  Serial.print(mv2) ;
  Serial.print(mv3) ;
  Serial.print(mv4) ;
  Serial.print(mv5) ;
  Serial.print(mv6) ;
  Serial.print(mv7) ;
  Serial.print(mv8) ;
  Serial.print("Read end") ;


  digitalWrite(35, 0);
  digitalWrite(34, 0);
  digitalWrite(33, 0);
  digitalWrite(32, 0);
  digitalWrite(31, 0);
  digitalWrite(30, 0);
  digitalWrite(29, 0);
  digitalWrite(28, 0);
  delay(100);

  Serial.println("");
  Serial.println("Volt read all off");
  mv1 = digitalRead(36) ;
  mv2 = digitalRead(37) ;
  mv3 = digitalRead(38) ;
  mv4 = digitalRead(39) ;
  mv5 = digitalRead(40) ;
  mv6 = digitalRead(41) ;
  mv7 = digitalRead(42) ;
  mv8 = digitalRead(43) ;

  Serial.print(mv1) ;
  Serial.print(mv2) ;
  Serial.print(mv3) ;
  Serial.print(mv4) ;
  Serial.print(mv5) ;
  Serial.print(mv6) ;
  Serial.print(mv7) ;
  Serial.print(mv8) ;
  Serial.print("Read end") ;


  }
  void test() {
  u8g2.clearBuffer();
  nastavRGB(150, 0  , 0  , 4);
  nastavRGB(150, 0  , 0  , 3);

  u8g2.setFont(u8g2_font_helvR08_tf);
  u8g2.drawStr(2, 14, "P1:");
  u8g2.setCursor(20, 14);
  u8g2.print(pot1);
  u8g2.drawStr(2, 28, "P2:");
  u8g2.setCursor(20, 28);
  u8g2.print(pot2);
  u8g2.drawStr(2, 42, "P3:");
  u8g2.setCursor(20, 42);
  u8g2.print(pot3);
  u8g2.drawStr(2, 56, "P4:");
  u8g2.setCursor(20, 56);
  u8g2.print(pot4);


  u8g2.drawStr(45, 14, "P5:");
  u8g2.setCursor(63, 14);
  u8g2.print(pot5);
  u8g2.drawStr(45, 28, "P6:");
  u8g2.setCursor(63, 28);
  u8g2.print(pot6);
  u8g2.drawStr(45, 42, "P7:");
  u8g2.setCursor(63, 42);
  u8g2.print(pot7);
  u8g2.drawStr(45, 56, "P8:");
  u8g2.setCursor(63, 56);
  u8g2.print(pot8);

  u8g2.drawStr(90, 14, "B1:");
  u8g2.setCursor(108, 14);
  u8g2.print(but1);
  u8g2.drawStr(90, 28, "B2:");
  u8g2.setCursor(108, 28);
  u8g2.print(but2);
  u8g2.drawStr(90, 42, "S3:");
  u8g2.setCursor(108, 42);
  u8g2.print(sw1);
  u8g2.drawStr(90, 56, "S4:");
  u8g2.setCursor(108, 56);
  u8g2.print(sw2);

  u8g2.sendBuffer();
  }



  void ControlServo1()
  {
  if (pocitadlo > (E_interval1 * 60) * 1 ) //(pocitadlo > (interval * 60) * 15 ) takle to bylo na minuty
  {
    getTemperature();
    nastavRGB(0, 0  , brid  , 4);
    nastavRGB(0, 0  , brid  , 3);


    if (mod == 0)
    {
      if (T1 >= (E_konstant1 - 12) && prvniPrekroceni == false)
      {
        servo1up();


        prvniPrekroceni = true;
        prekroceniTeploty70 = true;
      }

      if (T1 > (E_konstant1 - E_hystereze1) - 2 )
      {
        mod = 1;
        prekroceniTeploty70 = true ;
      }

      if (T1 < (E_konstant1 - 10) && prekroceniTeploty70 == true)
      {
        //servo1close();
        servo1down();
        prekroceniTeploty70 = false;
        prvniPrekroceni = false;
      }

    }

    else if (mod == 1)
    {
      if (T1 > (E_konstant1 + E_hystereze1))
      {
        servo1up();
        stopServoDown = 40 ; //funkce aby rele necvakalo
      }

      if (T1 <= (E_konstant1 - E_hystereze1) && T1 >= (E_konstant1 - 10))
      {
        if (stopServoDown > 1 ) {
          servo1down();
        }
        stopServoDown--;
        if (stopServoDown == -1) stopServoDown = 0 ;



      }

      if (T1 <= (E_konstant1 - 10))
      {
        mod = 0;
      }
    }

    pocitadlo = 0;
    u8g2.clearBuffer();
    nastavRGB(0, 0  , 0  , 4);
    nastavRGB(0, 0  , 0  , 3);
  }

  else pocitadlo++ ;
  }

  void ohrevBojler() { //sepne dohřívání bojleru když je teplota akumulacek dostatecná.
  if (counter2 >= 120 ) {
    nastavRGB(0, brid  , 0  , 4);
    nastavRGB(0, brid  , 0 , 3);

    int T_bojler = T11 ;
    float hystereze = 4.0;

    if (rT1 > 10) { // když je na domácí jednotce nastaveno mín než 10stupnu C tak je funkce vypnta

      if ((T1_akum >= (rT1 + 5)) && (T4_akum >= (rT1 + 5))) {  //teplota v aku 1 a 2 je vetší než třaeba 60.    && (T_bojler < rT1)

        if (T_bojler < (rT1 - hystereze)) {
          // Teplota je o 3 nebo více stupňů nižší než nastavená teplota
          digitalWrite(32, 1); // Relé se zapne
          P2 = 15 ; // promena do DB že je čerpadlo zapnuto
          nastavRGB(brid, 0  , 0  , 2);
        }

        else if (T_bojler > (rT1 + hystereze)) {
          // Teplota je o 3 nebo více stupňů vyšší než nastavená teplota
          digitalWrite(32, 0); // Relé se vypne
          P2 = 0 ;
          nastavRGB(0, 0  , 0  , 2);
        }

      }

      else {
        digitalWrite(32, 0);
        P2 = 0 ;
        nastavRGB(0, 0  , 0  , 2);
        //Serial.println("nedohrivam bojler") ;
      }

    }

    counter2 = 0 ;
    nastavRGB(0, 0  , 0  , 4);
    nastavRGB(0, 0  , 0 , 3);
  }

  counter2++;

  }


*/
