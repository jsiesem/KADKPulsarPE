//#define WITH_LCD 1
#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_MAX31865.h>
#include <ClickEncoder.h>
#include <Buzzer.h>
#include <EEPROM.h>
#include <TimerOne.h>

//#ifdef WITH_LCD
//#include <LiquidCrystal.h>
//
//#define LCD_RS       8
//#define LCD_RW       9
//#define LCD_EN      10
//#define LCD_D4       4
//#define LCD_D5       5
//#define LCD_D6       6
//#define LCD_D7       7
//
//#define LCD_CHARS   20
//#define LCD_LINES    4
//
//LiquidCrystal lcd(LCD_RS, LCD_RW, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
//#endif
#define buzz A3
Buzzer buzzer(buzz);

ClickEncoder *encoder;
int16_t last, value;
//value = 220;

Adafruit_MAX31865 thermo03 = Adafruit_MAX31865(8, 9, 10, 11); //T1 NOZZ
Adafruit_MAX31865 thermo01 = Adafruit_MAX31865(5, 9, 10, 11); //T2 TOP
Adafruit_MAX31865 thermo02 = Adafruit_MAX31865(A1, 9, 10, 11); //T3 BOT
#define RREF      430.0
#define RNOMINAL  100.0
#define relayNOZ 4
#define relayBUT 2
#define relayTOP 3
int tempN01 = 0;
int tempN02 = 0;
int tempN03 = 0;
int tempTolerance = 5;
int tempToleranceNeg = tempTolerance;
int setTemp01 = 0;
bool relayTogg01 = false;
bool relayTogg02 = false;
bool relayTogg03 = false;
bool flipflop = false;
bool initEncVal = false;
long timestamp;
int encoderStartVal = 210;
//SCREEN
U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, /* clock=*/ 11, /* data=*/ 9, /* CS=*/ A4, /* reset=*/ U8X8_PIN_NONE);

//U8G2_SSD1306_128X64_NONAME_F_4W_SW_SPI u8g2(U8G2_R0, 11, 10, 9, A4, U8X8_PIN_NONE);

typedef u8g2_uint_t u8g_uint_t;
#define SECONDS 10
uint8_t flip_color = 0;
uint8_t draw_color = 1;

void doBuzzStartup() {
  buzzer.begin(0);
  buzzer.sound(NOTE_C3, 200);
  buzzer.sound(NOTE_C4, 55);
  buzzer.end(100);
}

void draw(int encValue) {
  // assign default color value
  //u8g2.setColorIndex(draw_color);

  //u8g2.clearBuffer();
  //u8g2.setBitmapMode(true /* transparent*/);
  //u8g2.setDrawColor(0);// Black

  //u8g2.setFont(u8g2_font_luBS10_te);
  //u8g2.drawStr(108, 1, "Set");  //set temp

  u8g2.setFont(u8g2_font_luBS18_tn);
  u8g2.drawStr(26, 51, convert_FPS(value));  //set temp
  //
  u8g2.drawStr(61, 51, convert_FPS(tempN01)); //temp 01
  u8g2.drawStr(88, 51, convert_FPS(tempN02));  //temp 02
  u8g2.drawStr(115, 51, convert_FPS(tempN03));  //temp 03

  //u8g2.setFont(u8g2_font_inb16_mn );
  //u8g2.drawStr(67, 0, "+"); //temp 01


  //u8g2.setDrawColor(2); // XOR
  //u8g2.drawBox(100, 10, 125, 62);
  //u8g2.drawStr(0,20, convert_FPS(fps));
  //u8g2.sendBuffer();
}


void timerIsr() {
  encoder->service();
}


//#ifdef WITH_LCD
//void displayAccelerationStatus() {
//  lcd.setCursor(0, 1);
//  lcd.print("Acceleration ");
//  lcd.print(encoder->getAccelerationEnabled() ? "on " : "off");
//}
//#endif

const char *convert_FPS(uint32_t fps) {

  static char buf[3];
  //if(fps)
  strcpy(buf, u8g2_u16toa( (uint32_t)(fps), 3));
  //  buf[3] =  '.';
  //  buf[4] = (fps % 10) + '0';
  //  buf[5] = '\0';
  return buf;
}

void updateValT() {
  tempN01 = round(thermo01.temperature(RNOMINAL, RREF));
  //Sensor 2 seems not to  be working, will be coupled with tempNO1
  //should oterwise be
  //tempN02 = round(thermo01.temperature(RNOMINAL, RREF));
  
  tempN02 = (tempN01 + tempN03) / 2;
  tempN03 = round(thermo03.temperature(RNOMINAL, RREF));
  delay(10);
}

void doTemp() {

  if ((tempN01) <= (setTemp01 - tempTolerance)) {
    relayTogg01 = true;
  } else if ((tempN01) >= (setTemp01 - tempToleranceNeg)) {
    relayTogg01 = false;
  }

  if ((tempN02) <= (setTemp01 - tempTolerance)) {
    relayTogg02 = true;
  } else if ((tempN02) >= (setTemp01 - tempToleranceNeg)) {
    relayTogg02 = false;
  }

  if ((tempN03) <= (setTemp01 - tempTolerance)) {
    relayTogg03 = true;
  } else if ((tempN03) >= (setTemp01 - tempToleranceNeg)) {
    relayTogg03 = false;
  }
}

void setRelayIndicators() {
  //heat displ01
  u8g2.setFont(u8g2_font_luBS12_tn);
  if (relayTogg01 == true) {
    u8g2.drawStr(61, 65, "+"); //temp 01
    digitalWrite(relayTOP, HIGH);
  } else {
    u8g2.drawStr(61, 61, "-"); //temp 01
    digitalWrite(relayTOP, LOW);
  }
  if (relayTogg02 == true) {
    u8g2.drawStr(88, 65, "+"); //temp 01
    //digitalWrite(relayBUT, HIGH);
  } else {
    u8g2.drawStr(88, 61, "-"); //temp 01
    //digitalWrite(relayBUT, LOW);
  }
  if (relayTogg03 == true) {
    u8g2.drawStr(115, 65, "+"); //temp 01
    digitalWrite(relayNOZ, HIGH);
  } else {
    u8g2.drawStr(115, 61, "-"); //temp 01
    digitalWrite(relayNOZ, LOW);
  }
}

void setup() {
  //Serial.begin(9600);

  thermo01.begin(MAX31865_2WIRE);
  thermo02.begin(MAX31865_2WIRE);
  thermo03.begin(MAX31865_2WIRE);

  pinMode(relayNOZ, OUTPUT);
  pinMode(relayBUT, OUTPUT);
  pinMode(relayTOP, OUTPUT);

  encoder = new ClickEncoder(A0, A1, A2, 4);
  encoder->setAccelerationEnabled(true);

  //#ifdef WITH_LCD
  //  lcd.begin(LCD_CHARS, LCD_LINES);
  //  lcd.clear();
  //  displayAccelerationStatus();
  //#endif

  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr);

  last = -1;

  u8g2.begin();
  u8g2.setFontDirection(3);
  u8g2.setFont(u8g2_font_luBS18_tn);

  //u8g2.setFont(u8g2_font_8x13B_tf);
  //u8g2.setFont(u8g2_font_inb30_mn);
  draw_color = 1;
  timestamp = millis();

}

void loop() {
  if (initEncVal == false) {
    value = encoderStartVal;
    initEncVal  = true;
  }
  value += encoder->getValue();

  if (value != last) {
    last = value;
    Serial.print("Encoder Value: ");
    Serial.println(value);
    //#ifdef WITH_LCD
    //    lcd.setCursor(0, 0);
    //    lcd.print("         ");
    //    lcd.setCursor(0, 0);
    //    lcd.print(value);
    //#endif
    setTemp01 = value;
  }
  //  if (value < 0) {
  //    encoder->reset();
  //
  //
  //  }


  ///////////////////////////

  //if ((millis()-timestamp)<5000) {
  //if(flipflop==false){
  //digitalWrite(7, LOW);
  //updateValT();
  doTemp();

  delay(10);
  //flipflop = true;
  //} else {
  //digitalWrite(7, HIGH);
  //  digitalWrite(8, LOW);
  //  digitalWrite(5, LOW);
  //  digitalWrite(1, LOW);
  if ((  millis() - timestamp) > 500) {
    if (flipflop == true) {
      u8g2.clearBuffer();
      draw(value);
      setRelayIndicators();
      u8g2.sendBuffer();
      delay(50);
      timestamp = millis();
      flipflop = false;
    } else {
      updateValT();
      flipflop = true;
    }
  }


}
