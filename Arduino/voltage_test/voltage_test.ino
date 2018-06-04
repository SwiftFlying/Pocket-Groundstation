
/*
Notes:
LASEREINHORNBACKFISCH
 */

#define R1 400  // Resistor 1
#define R2 100  // Resistor 2
#define alarmADDR 1 // EEPROM Adress
#define layoutADDR 2 // OSD Layout Adress
#define dvrADDR 3 // DVR Settings Adress

#define Voltagedetect 3.5 // Min. Voltage for Detection

#define note 262  // C4

#define BUTTON1_PIN              2  // Button 1
#define BUTTON2_PIN              3  // Button 2
#define BUTTON3_PIN              4  // Button 3

#define beeppin 9 // Beeper Pin

#define DELAY                    20  // Delay per loop in ms
 
#include "U8glib.h"
#include <EEPROM.h>
#include <SoftwareSerial.h>

U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_DEV_0 | U8G_I2C_OPT_FAST);  // Dev 0, Fast I2C / TWI

SoftwareSerial OSDsoft(10, 11); // RX, TX

float voltage;
int lipo;
float alarmvalue = 3.40;
byte alarmvalueEEP;
byte layoutEEP;
byte dvrEEP;
int32_t osddata;
byte blinkosd = 0;
boolean DVRstatus = 0;
boolean RSSIavail;
byte RSSI;
byte VoltageByte;


const unsigned char settings16_bitmap[] PROGMEM = {
  // size is 16 x 16
  0x03,0xC0, // ......####......
  0x03,0xC8, // ......####..#...
  0x3F,0xFC, // ..############..
  0x7F,0xFE, // .##############.
  0x3F,0xFC, // ..############..
  0x3C,0x3C, // ..####....####..
  0xF8,0x1F, // #####......#####
  0xF8,0x1F, // #####......#####
  0xF8,0x1F, // #####......#####
  0xF8,0x1F, // #####......#####
  0x3C,0x3C, // ..####....####..
  0x3F,0xFC, // ..############..
  0x7F,0xFE, // .##############.
  0x3F,0xFC, // ..############..
  0x13,0xC8, // ...#..####..#...
  0x03,0xC0  // ......####......

}
;




const unsigned char DVR16_bitmap[] PROGMEM = 
{
  // size is 16 x 16
  0x00,0x00, // ................
  0x00,0x00, // ................
  0x1F,0x00, // ...#####........
  0x1F,0x80, // ...######.......
  0x00,0x80, // ........#.......
  0x03,0xC0, // ......####......
  0xFF,0xF1, // ############...#
  0xE7,0x77, // ###..###.###.###
  0xC3,0x77, // ##....##.###.###
  0xC3,0xF7, // ##....######.###
  0xC3,0xF7, // ##....######.###
  0xE7,0xF7, // ###..#######.###
  0xFF,0xF1, // ############...#
  0x00,0x00, // ................
  0x00,0x00, // ................
  0x00,0x00  // ................
}
;



   

const uint8_t DVRstatus8_bitmap[] U8G_PROGMEM = 
{
  // size is 8  x 8
  0x7E, // .######.
  0x81, // #......#
  0x99, // #..##..#
  0xBD, // #.####.#
  0xBD, // #.####.#
  0x99, // #..##..#
  0x81, // #......#
  0x7E  // .######.
 }
;


void pause()
{
  beep_criticalt(20);
  while (1);
}

void OSDsend()
{
  osddata += (layoutEEP << 19);
  osddata += (blinkosd << 17);
  osddata += (DVRstatus << 16);
  osddata += (RSSIavail << 15);
  osddata += (RSSI << 8);
  osddata += VoltageByte;
  OSDsoft.write(osddata);
}

void setup()  
{

  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  pinMode(BUTTON3_PIN, INPUT_PULLUP);
  
  alarmvalueEEP = EEPROM.read(alarmADDR);
  layoutEEP = EEPROM.read(layoutADDR);
  dvrEEP = EEPROM.read(dvrADDR);
  
  if (alarmvalueEEP != 0)
  {
    alarmvalue = (alarmvalueEEP / 10.0);
  }

  Serial.begin(9600);
  OSDsoft.begin(9600);

  if ( u8g.getMode() == U8G_MODE_R3G3B2 ) {
    u8g.setColorIndex(255);     // white
  }
  else if ( u8g.getMode() == U8G_MODE_GRAY2BIT ) {
    u8g.setColorIndex(3);         // max intensity
  }
  else if ( u8g.getMode() == U8G_MODE_BW ) {
    u8g.setColorIndex(1);         // pixel on
  }
  else if ( u8g.getMode() == U8G_MODE_HICOLOR ) {
    u8g.setHiColorByRGB(255, 255, 255);
  }
  clearOLED();
  delay(100);



  voltagetest();
  if (voltage > (Voltagedetect * 5.0))
  {
    lipo = 5;
    beep_x(lipo);
  }
  else if (voltage > (Voltagedetect * 4.0))
  {
    lipo = 4;
    beep_x(lipo);
  }
  else if (voltage > (Voltagedetect * 3.0))
  {
    lipo = 3;
    beep_x(lipo);
  }
  else if (voltage > (Voltagedetect * 2.0))
  {
    lipo = 2;
    beep_x(lipo);
  }
  else
  {
    pause();
  }
}



void clearOLED(){
    u8g.firstPage();  
    do {
    } while( u8g.nextPage() );
}


void voltagetest() 
{

  int sensorValue = analogRead(A0); // read the input on analog pin 0:
  voltage = sensorValue * (5.0 / 1023.0) * ((R1 + R2) / R2); // Convert the analog reading (which goes from 0 - 1023) to a voltage, considering the voltage divider:
  // Serial.println(voltage);   // print out the value you read:
  voltage = round(voltage * 10) / 10.0; //round the result

  float cellvoltage = voltage / lipo;

  u8g.drawFrame(96, 16 - 1, 29, 20);
  u8g.drawBox(96, 16, 29, 6);
  u8g.setPrintPos(101, 31);
  u8g.print(alarmvalue);
  u8g.setColorIndex(0);
  u8g.setPrintPos(101, 21);
  u8g.setFont(u8g_font_micro);
  u8g.print("ALARM");
  u8g.setColorIndex(1);

  delay(10);

  if (cellvoltage > 4.0) // case if voltage is above 4.0v
  { 
    u8g.drawBox(2, 5, 2, 2);
    u8g.drawBox(5, 5, 2, 2);
    u8g.drawBox(8, 5, 2, 2);
    u8g.drawFrame(0, 5 - 2, 12, 6);
    u8g.drawBox(12, 5, 1, 2);
    Serial.println(cellvoltage, 1);
    Serial.print("Battery full");
  } 
  else if (cellvoltage < 3.9 && voltage > 3.4) // case if voltage is below 3.4
  {
    Serial.println(cellvoltage, 1);
    Serial.print("Battery half");
    u8g.drawBox(2, 5, 2, 2);
    u8g.drawBox(5, 5, 2, 2);
    u8g.drawFrame(0, 5 - 2, 12, 6);
    u8g.drawBox(12, 5, 1, 2);
  } 
  else if (cellvoltage < 3.4) // case if voltage is below 3.4
  { 
    Serial.print("Battery low");
    Serial.println(cellvoltage, 1);
    u8g.drawBox(2, 5, 2, 2);
    u8g.drawFrame(0, 5 - 2, 12, 6);
    u8g.drawBox(12, 5, 1, 2);
  } 
  else 
  {
    u8g.drawFrame(0, 5 - 2, 12, 6);
    u8g.drawBox(12, 5, 1, 2);
  }

  if (cellvoltage < (alarmvalue)) // case if voltage is under the set alarm value
  { 
    beep_criticalt(225);
    Serial.print("Battery alarm!");
  } 

}




void loop() 
{
  // handle buttons

  int buttonState1 = !digitalRead(BUTTON1_PIN); // pin low -> pressed
  if (buttonState1 == 1) 
  {
    beep(1);
    Serial.print("Button1 pressed");
  }

  int buttonState2 = !digitalRead(BUTTON2_PIN); // pin low -> pressed
  if (buttonState2 == 1) 
  {
    beep(1);
    Serial.print("Button2 pressed");
    if (alarmvalue > 2.70) 
    {
      alarmvalue -= 0.10;
      alarmvalueEEP = alarmvalue * 10.0;
      EEPROM.write(alarmADDR, alarmvalueEEP);
    }
  }

  int buttonState3 = !digitalRead(BUTTON3_PIN); // pin low -> pressed
  if (buttonState3 == 1) 
  {
    beep(1);
    Serial.print("Button3 pressed");
    if (alarmvalue < 3.50) 
    {
      alarmvalue += 0.10;
      alarmvalueEEP = alarmvalue * 10.0;
      EEPROM.write(alarmADDR, alarmvalueEEP);
    }
  }

  u8g.firstPage();
  do {
    voltagetest();

    // graphic commands to redraw the complete screen should be placed here
    u8g.setFont(u8g_font_5x7);
    u8g.setPrintPos(34, 9);
    u8g.print("BATTERY");
    u8g.setFont(u8g_font_5x7);
    u8g.setPrintPos(18, 9);
    u8g.print(lipo);
    u8g.setFont(u8g_font_5x7);
    u8g.setPrintPos(24, 9);
    u8g.print("S");
    u8g.setFont(u8g_font_profont22);
    u8g.setPrintPos(0, 32);
    u8g.print(voltage, 1);
    u8g.setPrintPos(52, 32);
    u8g.print("v");

    u8g.drawBitmapP(96, 2, 1, 8, DVRstatus8_bitmap);

    u8g.drawFrame(1, 46 - 2, 124, 18);
    u8g.setFont(u8g_font_5x7);
    u8g.setPrintPos(30, 55);
    u8g.print("Press for Menu");

    u8g.setPrintPos(111, 9);
    u8g.print("OSD");

  }
  while (u8g.nextPage());
  delay(DELAY);
}


// Beep-Stuff

void beep(unsigned char delayms) 
{
  tone(beeppin, note, 10); // 10ms beep (C4 Tone)
}

void beep_long(unsigned char delayms) 
{
  tone(beeppin, note, 2000); // 2000ms beep (C4 Tone)
}

void beep_warning(unsigned char delayms) 
{
  tone(beeppin, note, 200); // 200ms beep (C4 Tone)
  delay(400);
  tone(beeppin, note, 200); // 200ms beep (C4 Tone)
}

void beep_criticalt(unsigned char delayms) 
{
  tone(beeppin, note, 400); // 400ms beep (C4 Tone)
  delay(800);
  tone(beeppin, note, 400); // 400ms beep (C4 Tone)
  delay(200);
}

void beep_x(byte b) 
{
  for (int i = 1; i <= b; i++) 
  {
    tone(beeppin, note, 200);
    delay(400);
  }
}









