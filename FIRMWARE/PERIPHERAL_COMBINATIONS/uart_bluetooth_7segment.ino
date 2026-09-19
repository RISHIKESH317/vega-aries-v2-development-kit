#include <HardwareSerial.h>

HardwareSerial Bluetooth(1);

// PIN CONFIGURATION

// Segments
const int segmentPins[7] = {
  3, 4, 5, 6, 7, 8, 9
};

// Digits
const int digitPins[4] = {
  10, 11, 12, 13
};


// 7-SEGMENT PATTERNS
// Order: A B C D E F G
// HIGH = ON
// LOW  = OFF

const byte digitPattern[10][7] =
{
  {1,1,1,1,1,1,0},   // 0
  {0,1,1,0,0,0,0},   // 1
  {1,1,0,1,1,0,1},   // 2
  {1,1,1,1,0,0,1},   // 3
  {0,1,1,0,0,1,1},   // 4
  {1,0,1,1,0,1,1},   // 5
  {1,0,1,1,1,1,1},   // 6
  {1,1,1,0,0,0,0},   // 7
  {1,1,1,1,1,1,1},   // 8
  {1,1,1,1,0,1,1}    // 9
};


// DISPLAY DATA
volatile char displayBuffer[4] = {
  '0', '0', '0', '0'
};


// Temporary Bluetooth buffer
char receivedBuffer[4];

int receivePosition = 0;


// TURN ALL DISPLAYS OFF
void allDigitsOff()
{
  digitalWrite(10, LOW);
  digitalWrite(11, LOW);
  digitalWrite(12, LOW);
  digitalWrite(13, LOW);
}


// SET SEGMENTS
void setSegments(char number)
{
  int n = number - '0';

  if (n < 0 || n > 9)
    return;

  digitalWrite(3, digitPattern[n][0]);  // A
  digitalWrite(4, digitPattern[n][1]);  // B
  digitalWrite(5, digitPattern[n][2]);  // C
  digitalWrite(6, digitPattern[n][3]);  // D
  digitalWrite(7, digitPattern[n][4]);  // E
  digitalWrite(8, digitPattern[n][5]);  // F
  digitalWrite(9, digitPattern[n][6]);  // G
}


// MULTIPLEX DISPLAY
void refreshDisplay()
{
  static int currentDigit = 0;
  static unsigned long previousTime = 0;

  unsigned long now = micros();

  // Change digit every 3000 microseconds
  if (now - previousTime >= 3000)
  {
    previousTime = now;

    // Turn everything OFF first
    allDigitsOff();

    // Set segments for current digit
    setSegments(displayBuffer[currentDigit]);

    // Turn ON current digit
    digitalWrite(digitPins[currentDigit], HIGH);

    // Move to next digit
    currentDigit++;

    if (currentDigit >= 4)
      currentDigit = 0;
  }
}


// BLUETOOTH RECEIVE
void readBluetooth()
{
  while (Bluetooth.available())
  {
    char c = Bluetooth.read();

    // Ignore ENTER / NEWLINE
    if (c == '\r' || c == '\n')
      continue;

    // Numbers only
    if (c >= '0' && c <= '9')
    {
      receivedBuffer[receivePosition] = c;

      Serial.print("Received: ");
      Serial.println(c);

      receivePosition++;

      // Four numbers received
      if (receivePosition == 4)
      {
        // Disable displays while changing data
        allDigitsOff();

        // Copy complete 4-digit value
        displayBuffer[0] = receivedBuffer[0];
        displayBuffer[1] = receivedBuffer[1];
        displayBuffer[2] = receivedBuffer[2];
        displayBuffer[3] = receivedBuffer[3];

        Serial.print("Displaying: ");

        Serial.print(displayBuffer[0]);
        Serial.print(displayBuffer[1]);
        Serial.print(displayBuffer[2]);
        Serial.println(displayBuffer[3]);

        // Start collecting next number
        receivePosition = 0;
      }
    }
  }
}


// SETUP
void setup()
{
  // Segment pins
  for (int i = 0; i < 7; i++)
  {
    pinMode(segmentPins[i], OUTPUT);
    digitalWrite(segmentPins[i], LOW);
  }

  // Digit pins
  for (int i = 0; i < 4; i++)
  {
    pinMode(digitPins[i], OUTPUT);
    digitalWrite(digitPins[i], LOW);
  }

  // USB Serial
  Serial.begin(115200);

  // Bluetooth UART1
  Bluetooth.begin(9600);

  Serial.println();
  Serial.println("================================");
  Serial.println("Bluetooth 4 Digit Display");
  Serial.println("================================");
  Serial.println("Send exactly 4 numbers");
  Serial.println("Example: 6565");
}


// MAIN LOOP
void loop()
{
  // Bluetooth
  readBluetooth();

  // Display
  refreshDisplay();
}
