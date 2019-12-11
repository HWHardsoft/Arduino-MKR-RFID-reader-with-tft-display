/*
 *  Application note: Simple RFID Terminal for ArduiTouch and Arduino MKR  
 *  Version 1.0
 *  Copyright (C) 2019  Hartmut Wendt  www.zihatec.de
 *  
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/   

 

/*______Import Libraries_______*/
#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include <XPT2046_Touchscreen.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include "usergraphics.h"
/*______End of Libraries_______*/


/*__Pin definitions for the Arduino MKR__*/
#define TFT_CS   A3
#define TFT_DC   0
#define TFT_MOSI 8
#define TFT_CLK  9
#define TFT_MISO 10
#define TFT_LED  A2  


#define HAVE_TOUCHPAD
#define TOUCH_CS A4
#define TOUCH_IRQ 1

#define BEEPER 2
#define RELAY A0   // optional relay output 

// RFID
#define RST_PIN   6     // SPI Reset Pin
#define SS_PIN    7    // SPI Slave Select Pin

/*_______End of definitions______*/

 

/*____Calibrate Touchscreen_____*/
#define MINPRESSURE 10      // minimum required force for touch event
#define TS_MINX 370
#define TS_MINY 470
#define TS_MAXX 3700
#define TS_MAXY 3600
/*______End of Calibration______*/


/*___Keylock spezific definitions___*/
byte blue_uid[] = {0x09, 0x8D, 0x9D, 0xA3};
#define relay_on_time 30 // will set the relay on for 3s 
/*___End of Keylock spezific definitions___*/


Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
XPT2046_Touchscreen touch(TOUCH_CS);
MFRC522 mfrc522(SS_PIN, RST_PIN);





int X,Y;
long Num1,Num2,Number;
char action;
boolean result = false;
bool Touch_pressed = false;
TS_Point p;
int RELAYTMR = 0; // Timer for relay control
int blue_check = false;
int red_check = false;
String s1;


void setup() {
  Serial.begin(115200); //Use serial monitor for debugging

  pinMode(TFT_LED, OUTPUT); // define as output for backlight control
  pinMode(RELAY, OUTPUT);   // define as output for optional relay
  digitalWrite(RELAY, LOW); // LOW to turn relay off   

  Serial.println("Init TFT and Touch...");
  tft.begin();
  touch.begin();
  tft.setRotation(3);
  Serial.print("tftx ="); Serial.print(tft.width()); Serial.print(" tfty ="); Serial.println(tft.height());
  
  tft.fillScreen(ILI9341_BLACK);
  digitalWrite(TFT_LED, HIGH);    // HIGH to turn backlight off - will hide the display during drawing
  Serial.println("Init RFC522 module....");
  mfrc522.PCD_Init();  // Init MFRC522 module
}


void loop() {
  // only after successful transponder detection the reader will read the UID
  // PICC = proximity integrated circuit card = wireless chip card
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial() ) {
    Serial.print("Gelesene UID:");
    s1 = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      // Abstand zwischen HEX-Zahlen und führende Null bei Byte < 16
      s1 = s1 + (mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
      s1 = s1 + String(mfrc522.uid.uidByte[i], HEX);            
    } 
    s1.toUpperCase();
    Serial.println(s1);
    
    
    // check current UID with blue_uid
    blue_check = true;
    for (int j=0; j<4; j++) {
      if (mfrc522.uid.uidByte[j] != blue_uid[j]) {
        blue_check = false;
      }
    }
 
    if (blue_check) 
    {
      Granted_Screen();
    } else {
       Denied_Screen(s1);
    }
  }
}  

/********************************************************************//**
 * @brief     detects a touch event and converts touch data 
 * @param[in] None
 * @return    boolean (true = touch pressed, false = touch unpressed) 
 *********************************************************************/
bool Touch_Event() {
  p = touch.getPoint(); 
  delay(1);
  #ifdef touch_yellow_header
    p.x = map(p.x, TS_MINX, TS_MAXX, 320, 0); // yellow header
  #else
    p.x = map(p.x, TS_MINX, TS_MAXX, 0, 320); // black header
  #endif
  p.y = map(p.y, TS_MINY, TS_MAXY, 0, 240);
  if (p.z > MINPRESSURE) return true;  
  return false;  
}





/********************************************************************//**
 * @brief     shows the intro screen in setup procedure
 * @param[in] None
 * @return    None
 *********************************************************************/
void IntroScreen()
{
  //Draw the Result Box
  tft.fillRect(0, 0, 240, 320, ILI9341_WHITE);
  tft.drawRGBBitmap(20,80, Zihatec_Logo,200,60);
  tft.setTextSize(0);
  tft.setTextColor(ILI9341_BLACK);
  tft.setFont(&FreeSansBold9pt7b);  

  tft.setCursor(42, 190);
  tft.println("ArduiTouch MKR");
  
  tft.setCursor(55, 215);
  tft.println("RFID example");
}


/********************************************************************//**
 * @brief     shows the granted screen on tft & switch optional relay
 * @param[in] None
 * @return    None
 *********************************************************************/
void Granted_Screen(){
      
  // show symbol
  tft.drawBitmap(0,50, access_granted,128,128,ILI9341_GREEN);
  tft.setTextSize(0);
  tft.setTextColor(ILI9341_WHITE); 
  tft.setFont(&FreeSansBold24pt7b); 
  tft.setCursor(140, 90 );
  tft.println("Access");
  tft.setCursor(140, 150);
  tft.println("granted"); 
  digitalWrite(TFT_LED, LOW);    // LOW to turn backlight on
  digitalWrite(RELAY, HIGH);     // HIGH to turn relay on
  tone(BEEPER,1000,800);
  delay(1200);
  digitalWrite(TFT_LED, HIGH);   // HIGH to turn backlight off 
  tft.fillRect(0, 0, 320, 240, ILI9341_BLACK); // clear screen
  digitalWrite(RELAY, LOW); // LOW to turn relay off
}


/********************************************************************//**
 * @brief     shows the access denied screen on tft
 * @param[in] None
 * @return    None
 *********************************************************************/
void Denied_Screen(String sUID) {

  // show symbol
  tft.drawBitmap(0,40, access_denied,128,128,ILI9341_RED);
  // show result text
  tft.setTextSize(0);
  tft.setTextColor(ILI9341_WHITE); 
  tft.setFont(&FreeSansBold24pt7b); 
  tft.setCursor(140, 90 );
  tft.println("Access");
  tft.setCursor(140, 150);
  tft.println("denied"); 
  tft.setFont(&FreeSans9pt7b);
  // show UID
  tft.setCursor(90, 200);
  tft.println("UID:");
  tft.setCursor(140, 200);
  tft.println(sUID); 
  digitalWrite(TFT_LED, LOW); // LOW to turn backlight on
  for (int i=0;i< 3;i++) {
          tone(BEEPER,4000);
          delay(100);
          noTone(BEEPER);
          delay(50);      
  }   
  delay(1500);
  digitalWrite(TFT_LED, HIGH);   // HIGH to turn backlight off 
  tft.fillRect(0, 0, 320, 240, ILI9341_BLACK);  // clear screen
}



/********************************************************************//**
 * @brief     plays ack tone (beep) after button pressing
 * @param[in] None
 * @return    None
 *********************************************************************/
void Button_ACK_Tone(){
  tone(BEEPER,4000,100);
}
