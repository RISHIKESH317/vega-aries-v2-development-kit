/*
 * Project       : VEGA ARIES V2 - DS1302 RTC + I2C LCD Clock Display
 * Target Board  : VEGA ARIES V2 (THEJAS32 RISC-V SoC)
 * Environment   : Arduino IDE with VEGA Processor Board Support Package
 *
 * Hardware Connections:
 *
 * DS1302 RTC:
 *   VCC  -> 3.3V
 *   GND  -> GND
 *   CLK  -> GPIO 0
 *   DAT  -> GPIO 1
 *   RST  -> GPIO 2
 *
 * I2C 16x2 LCD:
 *   VCC  -> 5V
 *   GND  -> GND
 *   SDA  -> SDA0
 *   SCL  -> SCL0
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>


// DS1302 GPIO PIN DEFINITIONS
#define RTC_CLK  0
#define RTC_DAT  1
#define RTC_RST  2


// I2C LCD - I2C0

// VEGA ARIES V2 I2C0
// SDA0 -> J10 pin 10
// SCL0 -> J10 pin 12

TwoWire Wire(0);

LiquidCrystal_I2C lcd(0x27, 16, 2);


// RTC SETTING
// FIRST UPLOAD:
//     SET_RTC_ONCE = true

// AFTER RTC IS SET:
//     Change to false and upload again.
//

const bool SET_RTC_ONCE = true;


// TODAY'S DATE AND TIME

// Date : 10/09/2026
// Day  : Thursday

// Time set here:
// 02:31:00
//
// Change only the time if required.

const uint8_t INIT_SECONDS = 0;
const uint8_t INIT_MINUTES = 31;
const uint8_t INIT_HOURS   = 2;

const uint8_t INIT_DAY     = 5;    // 5 = THR
const uint8_t INIT_DATE    = 10;
const uint8_t INIT_MONTH   = 9;
const uint8_t INIT_YEAR    = 26;


// DAY NAMES
const char* const DAYS_OF_WEEK[] =
{
  "",
  "SUN",
  "MON",
  "TUE",
  "WED",
  "THR",
  "FRI",
  "SAT"
};


// BCD CONVERSION

static inline uint8_t decToBcd(uint8_t val)
{
  return ((val / 10) << 4) | (val % 10);
}


static inline uint8_t bcdToDec(uint8_t val)
{
  return ((val >> 4) * 10) + (val & 0x0F);
}


// DS1302 WRITE BYTE
void rtc_writeByte(uint8_t value)
{
  pinMode(RTC_DAT, OUTPUT);

  for (uint8_t i = 0; i < 8; i++)
  {
    digitalWrite(
      RTC_DAT,
      (value & 0x01) ? HIGH : LOW
    );

    delayMicroseconds(2);

    digitalWrite(RTC_CLK, HIGH);
    delayMicroseconds(2);

    digitalWrite(RTC_CLK, LOW);
    delayMicroseconds(2);

    value >>= 1;
  }
}


// DS1302 READ BYTE
uint8_t rtc_readByte()
{
  uint8_t value = 0;

  pinMode(RTC_DAT, INPUT);

  for (uint8_t i = 0; i < 8; i++)
  {
    if (digitalRead(RTC_DAT) == HIGH)
    {
      value |= (1 << i);
    }

    digitalWrite(RTC_CLK, HIGH);
    delayMicroseconds(2);

    digitalWrite(RTC_CLK, LOW);
    delayMicroseconds(2);
  }

  return value;
}


// DS1302 READ REGISTER
uint8_t rtc_readRegister(uint8_t reg)
{
  digitalWrite(RTC_RST, LOW);
  digitalWrite(RTC_CLK, LOW);

  delayMicroseconds(2);

  digitalWrite(RTC_RST, HIGH);

  delayMicroseconds(4);

  // Send read command
  rtc_writeByte(reg);

  uint8_t data = rtc_readByte();

  digitalWrite(RTC_RST, LOW);

  delayMicroseconds(2);

  return data;
}


// DS1302 WRITE REGISTER
void rtc_writeRegister(uint8_t reg, uint8_t data)
{
  digitalWrite(RTC_RST, LOW);
  digitalWrite(RTC_CLK, LOW);

  delayMicroseconds(2);

  digitalWrite(RTC_RST, HIGH);

  delayMicroseconds(4);

  rtc_writeByte(reg);

  rtc_writeByte(data);

  digitalWrite(RTC_RST, LOW);

  delayMicroseconds(2);
}


// RTC INITIALIZATION
void rtc_init()
{
  pinMode(RTC_RST, OUTPUT);
  pinMode(RTC_CLK, OUTPUT);
  pinMode(RTC_DAT, INPUT);

  digitalWrite(RTC_RST, LOW);
  digitalWrite(RTC_CLK, LOW);

  // Disable write protection
  rtc_writeRegister(0x8E, 0x00);

  // Check oscillator
  uint8_t sec = rtc_readRegister(0x81);

  // Clear Clock Halt bit if necessary
  if (sec & 0x80)
  {
    rtc_writeRegister(
      0x80,
      sec & 0x7F
    );
  }
}


// SET RTC
void rtc_setTime(
  uint8_t hours,
  uint8_t minutes,
  uint8_t seconds,
  uint8_t dayOfWeek,
  uint8_t dayOfMonth,
  uint8_t month,
  uint8_t year
)
{
  // Disable write protection
  rtc_writeRegister(0x8E, 0x00);

  // Seconds
  rtc_writeRegister(
    0x80,
    decToBcd(seconds) & 0x7F
  );

  // Minutes
  rtc_writeRegister(
    0x82,
    decToBcd(minutes) & 0x7F
  );

  // Hours - 24 hour mode
  rtc_writeRegister(
    0x84,
    decToBcd(hours) & 0x3F
  );

  // Date
  rtc_writeRegister(
    0x86,
    decToBcd(dayOfMonth) & 0x3F
  );

  // Month
  rtc_writeRegister(
    0x88,
    decToBcd(month) & 0x1F
  );

  // Day of week
  rtc_writeRegister(
    0x8A,
    decToBcd(dayOfWeek) & 0x07
  );

  // Year
  rtc_writeRegister(
    0x8C,
    decToBcd(year % 100)
  );

  // Enable write protection
  rtc_writeRegister(0x8E, 0x80);
}


// READ RTC

void rtc_getTime(
  uint8_t &hours,
  uint8_t &minutes,
  uint8_t &seconds,
  uint8_t &dayOfWeek,
  uint8_t &dayOfMonth,
  uint8_t &month,
  uint8_t &year
)
{
  uint8_t secRaw  = rtc_readRegister(0x81);
  uint8_t minRaw  = rtc_readRegister(0x83);
  uint8_t hourRaw = rtc_readRegister(0x85);
  uint8_t dateRaw = rtc_readRegister(0x87);
  uint8_t monRaw  = rtc_readRegister(0x89);
  uint8_t dayRaw  = rtc_readRegister(0x8B);
  uint8_t yrRaw   = rtc_readRegister(0x8D);

  seconds =
    bcdToDec(secRaw & 0x7F);

  minutes =
    bcdToDec(minRaw & 0x7F);

  hours =
    bcdToDec(hourRaw & 0x3F);

  dayOfMonth =
    bcdToDec(dateRaw & 0x3F);

  month =
    bcdToDec(monRaw & 0x1F);

  dayOfWeek =
    bcdToDec(dayRaw & 0x07);

  year =
    bcdToDec(yrRaw);
}


// HELLO CDAC ANIMATION
void showHelloCDACAnimation()
{
  lcd.clear();
  lcd.home();

  lcd.setCursor(16, 0);
  lcd.print("Hello CDAC...");

  // Scroll into display
  for (int i = 0; i < 16; i++)
  {
    lcd.scrollDisplayLeft();
    delay(180);
  }

  delay(1000);

  // Scroll out
  for (int i = 0; i < 16; i++)
  {
    lcd.scrollDisplayLeft();
    delay(150);
  }

  lcd.clear();
  lcd.home();

  delay(300);
}


// DISPLAY TIME AND DATE
void displayTimeAndDate()
{
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;
  uint8_t dayOfWeek;
  uint8_t dayOfMonth;
  uint8_t month;
  uint8_t year;

  rtc_getTime(
    hours,
    minutes,
    seconds,
    dayOfWeek,
    dayOfMonth,
    month,
    year
  );

  char lineBuffer[48];

  // Time
  snprintf(
    lineBuffer,
    sizeof(lineBuffer),
    "Time :%02u:%02u:%02u  ",
    (unsigned int)hours,
    (unsigned int)minutes,
    (unsigned int)seconds
  );

  lcd.setCursor(0, 0);
  lcd.print(lineBuffer);

  // Date
  snprintf(
    lineBuffer,
    sizeof(lineBuffer),
    "Date :%02u/%02u/%02u  ",
    (unsigned int)dayOfMonth,
    (unsigned int)month,
    (unsigned int)(year % 100)
  );

  lcd.setCursor(0, 1);
  lcd.print(lineBuffer);
}


// DISPLAY DAY
void displayDay()
{
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;
  uint8_t dayOfWeek;
  uint8_t dayOfMonth;
  uint8_t month;
  uint8_t year;

  rtc_getTime(
    hours,
    minutes,
    seconds,
    dayOfWeek,
    dayOfMonth,
    month,
    year
  );

  const char* dayStr;

  if (dayOfWeek >= 1 && dayOfWeek <= 7)
  {
    dayStr = DAYS_OF_WEEK[dayOfWeek];
  }
  else
  {
    dayStr = "---";
  }

  char lineBuffer[48];

  snprintf(
    lineBuffer,
    sizeof(lineBuffer),
    "Day: %-3s        ",
    dayStr
  );

  lcd.setCursor(0, 0);
  lcd.print(lineBuffer);

  lcd.setCursor(0, 1);
  lcd.print("                ");
}


// SETUP
void setup()
{
  // ---------------------------------
  // 1. Initialize DS1302
  // ---------------------------------

  rtc_init();


  // ---------------------------------
  // 2. Set RTC on first upload
  // ---------------------------------

  if (SET_RTC_ONCE)
  {
    rtc_setTime(
      INIT_HOURS,
      INIT_MINUTES,
      INIT_SECONDS,
      INIT_DAY,
      INIT_DATE,
      INIT_MONTH,
      INIT_YEAR
    );
  }

  // 3. Initialize I2C0
  Wire.begin();


  // 4. Initialize LCD
  lcd.init();
  lcd.backlight();


  // 5. Hello CDAC once
  showHelloCDACAnimation();
}


// 
// MAIN LOOP
void loop()
{
  // TIME + DATE

  for (int i = 0; i < 4; i++)
  {
    displayTimeAndDate();

    delay(1000);
  }


  
  // DAY

  displayDay();

  delay(2000);
}
