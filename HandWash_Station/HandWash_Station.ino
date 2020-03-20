
int thresholdVL = 90;

//-------------------VL6180x----------------------------------//
#include "Adafruit_VL6180X.h"
Adafruit_VL6180X vl = Adafruit_VL6180X();

//-------------------PIR---------------------------------------//
int pirPin = 12;               // choose the input pin (for PIR sensor)
int pirState = LOW;             // we start, assuming no motion detected
int pirVal = 0;

//--------------------LCD----------------------------------------//

#include <SoftwareSerial.h>

SoftwareSerial LCD(10, 11); // Arduino SS_RX = pin 10 (unused), Arduino SS_TX = pin 11

//-------------------NEOPIXEL MATRIX-------------------------------//
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>

#define PIN 13
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(8, 8, PIN,
  NEO_MATRIX_BOTTOM    + NEO_MATRIX_LEFT +
  NEO_MATRIX_COLUMNS + NEO_MATRIX_PROGRESSIVE,
  NEO_GRB            + NEO_KHZ800);

#define BLACK    0x0000
#define BLUE     0x001F
#define RED      0xF800
#define GREEN    0x07E0
#define CYAN     0x07FF
#define MAGENTA  0xF81F
#define YELLOW   0xFFE0 
#define WHITE    0xFFFF
//-------------------SD + RTC-------------------------------//
#include "RTClib.h"
#include <SD.h>
RTC_PCF8523 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
const int chipSelect = 10;

//-------------------LEDs-------------------------------//
int pirLED = 0;
int vlLED = 1;
//-------------------Variables------------------------------------//
bool rep_status = 0;
int wash_c = 0;
int people_c = 0;
uint32_t current_time = 0;
uint32_t rep_start = 0;
int delay_time = 15; //seconds

//---------------------------=------------------------------------//

void setup()
{
  Serial.begin(57600);
  //---------------------------------SD + RTC
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (! rtc.initialized()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
  Serial.print("Initializing SD card...");

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  
  Serial.println("card initialized.");

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open("datalog.csv", FILE_WRITE);
  if (dataFile){
    dataFile.println("......................");
    dataFile.println("Date, Time, Range, People Count, Hand Wash Count, Rep Status");
    dataFile.close(); 
  }
  else
  {
    Serial.println("Couldn't open data file");
    return;
  }
  


  //------------------------------Simple Stuff
  pinMode(pirLED, OUTPUT);
  pinMode(vlLED, OUTPUT);

  //--------------------------------PIR
  pinMode(pirPin, INPUT);     // declare sensor as input
  
  //--------------------------------VL6180X
  Serial.println("Adafruit VL6180x test!");
  if (! vl.begin()) {
    Serial.println("Failed to find sensor");
    while (1);
  }
  Serial.println("Sensor found!");
  //------------------------------BIG LCD
  LCD.begin(9600);// all SerLCDs come at 9600 Baud by default
  clearScreen();
 //------------------------------MATRIX
  matrix.begin();
  matrix.setBrightness(40);
  matrix.fillRect(0,0,8,8,WHITE);
  matrix.show();

} 

void loop()
{
  DateTime now = rtc.now();
  // VL6180X
  uint8_t range = vl.readRange();
  Serial.print("Range: "); Serial.println(range);
  //PIR
  pirVal = digitalRead(pirPin);  // read input value
  Serial.print("PIR: "); Serial.println(pirVal);
  const_output(wash_c);
  rep_status = 0;
  current_time = now.unixtime();

  if (pirVal == HIGH && pirState == LOW){
    pirState = HIGH;
    warn_output();
    digitalWrite(pirLED,HIGH);
    people_c++;
    Serial.print("People Count: "); Serial.println(people_c);
    rep_start = now.unixtime();
    Serial.print("Rep Time: "); Serial.println(rep_start);
    delay(1000);
  }

  current_time = now.unixtime();
  if (pirState == HIGH && current_time - rep_start < delay_time){
    current_time = now.unixtime();
    uint8_t range = vl.readRange();
    Serial.print("Current Time: "); Serial.println(current_time);
    Serial.print("Rep Start Time: "); Serial.println(rep_start);
    Serial.print("Delay Time: "); Serial.println(delay_time);
    Serial.print("Range in if: "); Serial.println(range);
    warn_output();
    if(range < thresholdVL){
       good_output();
       wash_c++;
       Serial.print("Wash Count: "); Serial.println(wash_c);
       digitalWrite(vlLED,HIGH);
       rep_status = 1;
       logging(range, people_c, wash_c, rep_status); 
       delay(10000); //10 seconds
       const_output(wash_c);
       pirState = LOW;
       digitalWrite(vlLED,LOW); 
    }
  }
  else if (pirState == HIGH && current_time - rep_start >= delay_time){
    rep_status = 0;
    bad_output();
    logging(range, people_c, wash_c, rep_status);
    delay(4000);
    matrix.clear();
    matrix.show();
    pirState = LOW; 
  }

  if (pirVal == LOW){
    digitalWrite(pirLED,LOW);
  }

  delay(200);
}

//--------------------------------Now Time---------------------------------------//

void logging(int logR, int ppl_c, int hand_c, int stat){
    DateTime now = rtc.now();
    String ahoraDate = String(int(now.month())) + "/" + String(int(now.day())) + "/" + String(int(now.year()));
    String ahoraTime = String(int(now.hour())) + ":" + String(int(now.minute())) + ":" + String(int(now.second()));
    String dataString = String(ahoraDate)  + ", " + String(ahoraTime) + ", " + String(logR) + ", " + String(ppl_c) + ", " + String(hand_c) + ", " + String(stat);
    //String dataString = String(now.unixtime())  + ", " + String(logR) + ", " + String(ppl_c) + ", " + String(hand_c) + ", " + String(stat);
    
    File dataFile = SD.open("datalog.csv", FILE_WRITE);
    if(dataFile){
      dataFile.println(dataString);
      dataFile.close();
      Serial.println(dataString);
    }
    else{
      Serial.println("Couldn't access file");
    }
}

//--------------------------------CONSTANT OUPUT---------------------------------------//
void const_output(int out_num) {
      clearScreen();
      selectLineOne();
      LCD.print("Wash Count: ");
      selectLineTwo();
      LCD.print(out_num, DEC);
      matrix.clear();
      matrix.show();
}

//--------------------------------GOOD OUPUT---------------------------------------//
void good_output() {
      clearScreen();
      selectLineOne();
      LCD.print(" High Five ");
      matrix.fillRect(0,0,8,8,GREEN);
      matrix.show();
      delay(100);
}
//--------------------------------BAD OUPUT---------------------------------------//
void bad_output() {
      clearScreen();
      selectLineOne();
      LCD.print("Maybe Next Time :(");
      matrix.fillRect(0,0,8,8,RED);
      matrix.show();
      delay(100);
}
//--------------------------------WARNING OUPUT---------------------------------------//
void warn_output() {
      clearScreen();
      selectLineOne();
      LCD.print("Please Wash Your Hands");      
      matrix.fillRect(0,0,8,8,YELLOW);
      matrix.show();
      delay(100);
}


//------------------------------------LCD COMMANDS-------------------------------------//
void clearScreen()
{
  //clears the screen, you will use this a lot!
  LCD.write(0xFE);
  LCD.write(0x01); 
}
//-------------------------------------------------------------------------------------------
void selectLineOne()
{ 
  //puts the cursor at line 0 char 0.
  LCD.write(0xFE); //command flag
  LCD.write(128); //position
}
//-------------------------------------------------------------------------------------------
void selectLineTwo()
{ 
  //puts the cursor at line 0 char 0.
  LCD.write(0xFE); //command flag
  LCD.write(192); //position
}
//-------------------------------------------------------------------------------------------
void turnDisplayOff()
{
  //this tunrs the display off, but leaves the backlight on. 
  LCD.write(0xFE); //command flag
  LCD.write(8); // 0x08
}
//-------------------------------------------------------------------------------------------
void turnDisplayOn()
{
  //this turns the dispaly back ON
  LCD.write(0xFE); //command flag
  LCD.write(12); // 0x0C
}
//-----------------------------------------------------------------------------------
