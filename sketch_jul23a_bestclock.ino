#include <Wire.h>
#include <TimeLib.h>
#include <DS1307RTC.h>
tmElements_t tm;

const unsigned char byte1[10] = {
  0x83, 0x80, 0x8B, 0x8A, 0x8A, 0x0A, 0x0B, 0x80, 0x8B, 0x8A
};

const unsigned char byte2[10] = {
  0xFF, 0xE1, 0xBF, 0xFF, 0xE5, 0xFF, 0xFF, 0xE7, 0xFF, 0xE7
};

byte tm_buffer[10] = {0x83, 0xED, 0x83, 0xFF, 0x0B, 0x3F, 0x8B, 0xED, 0x81, 0x0C};
//                   |  Grid 1  |   Grid 2  |   Grid 3  |   Grid 4  |   Grid 5  |

//byte tm_buffer[10] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10};
//                   |  Grid 1  |   Grid 2  |   Grid 3  |   Grid 4  |   Grid 5  |

// PINS FOR THE DISPLAY
int tm_dio = 3;
int tm_clk = 2;
int tm_stb = 4;


// FOR THE AUTO BRIGHTNESS
const int brightnesses = 30; // number of data points (seconds) to be averaged over
int currentCycle = 0;
int data[brightnesses + 1];
int bPin = A0;


void setup() {
  
  inits(2);
  writeBuffer();

  Serial.begin(9600);

    /* ==== Writes time from pc ====*/
  /*if (Serial) {
    delay(1000);
    bool parse=false;
    bool config=false;
    if (getDate(__DATE__) && getTime(__TIME__)) {
      parse = true;
      // and configure the RTC with this info
      if (RTC.write(tm)) {
        config = true;
      }
    }
    delay(1000);
  }*//*
  This is how to call it, but can only be done when connected to pc...
  =============================== */
  
  // populates brightness data
  for (int i = 0; i < brightnesses; i++) {
    data[i] = analogRead(A7);
  }
  delay(1000);
}

void loop() {
  clearBuffer();
  getTime();
  writeBuffer();
  checkBrightness();
  delay(1000);
}

void checkBrightness() {
  data[currentCycle] = analogRead(A0);
  currentCycle++;
  if (currentCycle >= brightnesses) {
    currentCycle = 0;
    int temp = 0;
    for (int i = 0; i < brightnesses; i++) {
      temp += data[i];
    }
    if (temp / brightnesses <= 10) {
      setIntensity(0);
    } else if (temp / brightnesses <= 30) {
      setIntensity(1);
    } else {
      setIntensity(2);
    }
    
  }
}

void getTime() {
  tmElements_t tm;
  int PM = 0;
  if (RTC.read(tm)) {
    if (tm.Hour > 12) {
      PM = 1;
    }

    /* am/pm lights */
    if (tm.Hour >= 12) {
      bitWrite(tm_buffer[9], 6, 1);
    } else {
      bitWrite(tm_buffer[9], 5, 1);
    }

    /* prevents displaying zero*/
    if ((tm.Hour-PM*12)/10 != 0) {
      putDigitAt((tm.Hour-PM*12)/10, 3);
    }
  
    putDigitAt((tm.Hour-PM*12)%10, 2);
    putDigitAt(tm.Minute/10, 0);
    putDigitAt(tm.Minute%10, 1);

    /* from 12am - 12:59am will display 12:xx instead of 0:xx */
    if (tm.Hour == 0) {
      putDigitAt(1,3);
      putDigitAt(2,2);
    }
  }
  bitWrite(tm_buffer[4], 4, 1);
}

/*==================DO NOT TOUCH BELOW THIS=====================*/


void putDigitAt(int digit, int pos) { /* Usage: putDigitAt(INTEGER, POSITION (0-3)); where 32:01*/
  tm_buffer[pos*2] = byte1[digit];
  tm_buffer[(pos*2) + 1] = byte2[digit];
  if (digit <=9 && digit >=4 && digit !=7 || digit == 0){
    switch (pos){
      case 0:
        bitWrite(tm_buffer[8],7,1);
        break;
      case 1: 
        bitWrite(tm_buffer[9],2,1);
        break;
      case 2:
        bitWrite(tm_buffer[9], 3, 1);
        break;
      case 3:
        bitWrite(tm_buffer[8], 0, 1);  
        break;      
    }
  }
  else {
    switch (pos) {
      case 0:
        bitWrite(tm_buffer[8],7,0);
        break;
      case 1: 
        bitWrite(tm_buffer[9],2,0);
        break;
      case 2:
        bitWrite(tm_buffer[9], 3, 0);
        break;
      case 3:
        bitWrite(tm_buffer[8], 0, 0);  
        break;      
    }
  }
}

void inits(int intensity) {
  pinMode(tm_dio, OUTPUT);
  pinMode(tm_clk, OUTPUT);
  pinMode(tm_stb, OUTPUT);

  digitalWrite(tm_stb, HIGH);
  digitalWrite(tm_clk, HIGH);

  // delay(200);

  tm_sendCommand(0x40); // command 2

  digitalWrite(tm_stb, LOW);
  tm_sendByte(0xc0); // command 3
  for (int i = 0; i < 16; i++)
    tm_sendByte(0x00); // clear RAM
  digitalWrite(tm_stb, HIGH);

  tm_sendCommand(0x03); // command 1

  setIntensity(intensity);
}

void clearBuffer() {
  for (int i = 0; i < 10; i++) {
    tm_buffer[i] = 0x00;
  }
}

void setIntensity(int intensity)
{
  if (intensity < 0)
    {
      tm_sendCommand(0x80); // command 4
      return;
    }
    tm_sendCommand(0x88 | (intensity % 8));
}

void writeBuffer() {
  tm_sendCommand(0x40); // command 2
  digitalWrite(tm_stb, LOW);
  tm_sendByte(0xc0); // command 3
  for (int i = 0; i < 16; i++){
    tm_sendByte(tm_buffer[i]); // set RAM
  }
  digitalWrite(tm_stb, HIGH);
}

void tm_sendCommand(byte data)
{
  digitalWrite(tm_stb, LOW);
  tm_sendByte(data);
  digitalWrite(tm_stb, HIGH);
}

void tm_sendByte(byte data)
{
  for (int i = 0; i < 8; i++)
    {
      digitalWrite(tm_clk, LOW);
      digitalWrite(tm_dio, data & 1 ? HIGH : LOW);
      data >>= 1;
      digitalWrite(tm_clk, HIGH);
  }
}

void dp() {
  bitWrite(tm_buffer[0], 4, 1);
}



// =================USED TO GET TIME FROM COMPUTER TO RTC========================
/*
const char *monthName[12] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

bool getTime(const char *str)
{
  int Hour, Min, Sec;

  if (sscanf(str, "%d:%d:%d", &Hour, &Min, &Sec) != 3) return false;
  tm.Hour = Hour;
  tm.Minute = Min;
  tm.Second = Sec;
  return true;
}

bool getDate(const char *str)
{
  char Month[12];
  int Day, Year;
  uint8_t monthIndex;

  if (sscanf(str, "%s %d %d", Month, &Day, &Year) != 3) return false;
  for (monthIndex = 0; monthIndex < 12; monthIndex++) {
    if (strcmp(Month, monthName[monthIndex]) == 0) break;
  }
  if (monthIndex >= 12) return false;
  tm.Day = Day;
  tm.Month = monthIndex + 1;
  tm.Year = CalendarYrToTm(Year);
  return true;
}*/

// ================================================================================
