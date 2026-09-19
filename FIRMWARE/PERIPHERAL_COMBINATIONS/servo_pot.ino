/*
  Project: Potentiometer to Servo Motor Control
  Target Board: VEGA ARIES V2.0 (THEJAS32 RISC-V SoC)
  CONNECTIONS:
  --------------------------------------------------------------------
  Potentiometer:
    - VCC / 5V Pin  --> VEGA 3.3V
    - GND Pin       --> VEGA GND
    - POT (Wiper)   --> VEGA ADC2 (J10 Pin 6 / Analog Pin A2)

  Servo Motor (SG90 / Micro Servo):
    - VCC (Red)     --> VEGA 3.3V
    - GND (Brown)   --> VEGA GND
    - Signal (Orange)--> VEGA PWM2 (J2 Pin 1 / PWM Channel 2)

  BEHAVIOR:
  --------------------------------------------------------------------
    - Turning potentiometer LEFT  --> Servo moves LEFT (towards 0 degrees)
    - Center position             --> Servo at ~90 degrees
    - Turning potentiometer RIGHT --> Servo moves RIGHT (towards 180 degrees)
    - Smooth analog filtering eliminates servo jitter/flutter.
    - Serial Monitor prints ADC value and angle at 115200 baud.
*/

#include <Arduino.h>
#include <Servo.h>

// ==================================================
// PIN DEFINITIONS
// ==================================================
// Potentiometer connected to ADC2 (A2, J10 Pin 6)
const int POT_PIN = A2;

// Servo signal connected to PWM2 (J2 Pin 1 -> PWM Channel 2)
const int SERVO_PWM_CH = 2;

// ADC & SERVO CALIBRATION CONSTANTS
// The onboard ADS1015 on VEGA ARIES V2 has a 2 mV/LSB resolution.
// With a 3.3V reference: 3.3V / 0.002V ≈ 1650 counts.
const int ADC_MIN = 0;
const int ADC_MAX = 1650;

// Servo angle limits
const int ANGLE_MIN = 0;
const int ANGLE_CENTER = 90;
const int ANGLE_MAX = 180;

// OBJECTS & GLOBAL VARIABLES
Servo myServo;

// Smoothing filter variables
float filteredADC = 825.0; // Start at center (~1.65V)
int previousAngle = -1;

unsigned long lastPrintTime = 0;
const unsigned long PRINT_INTERVAL = 100; // Print every 100 ms


void setup()
{
  // Initialize USB Serial Monitor at 115200 baud
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("==================================================");
  Serial.println("  VEGA ARIES V2.0 - Potentiometer -> Servo Control");
  Serial.println("==================================================");
  Serial.println("Connections:");
  Serial.println("  - Potentiometer Wiper -> ADC2 (J10_6)");
  Serial.println("  - Potentiometer VCC   -> 3.3V");
  Serial.println("  - Potentiometer GND   -> GND");
  Serial.println("  - Servo Signal (PWM)  -> PWM2 (J2_1)");
  Serial.println("  - Servo VCC           -> 3.3V");
  Serial.println("  - Servo GND           -> GND");
  Serial.println("==================================================");

  // Attach servo to PWM Channel 2
  myServo.attach(SERVO_PWM_CH);

  // Initialize servo to center position (90 degrees)
  myServo.write(ANGLE_CENTER);
  delay(500);

  Serial.println("System Initialized. Move potentiometer to control servo.");
}


void loop()
{
  // 1. Read raw ADC value from ADC2 (ADS1015 channel 2)
  int rawADC = analogRead(POT_PIN);

  // 2. Exponential Moving Average (EMA) filter for smooth, jitter-free movement
  //    Alpha = 0.25 provides quick response with stable noise suppression
  filteredADC = (filteredADC * 0.75f) + ((float)rawADC * 0.25f);

  // 3. Map filtered ADC value to Servo Angle
  //    Direction: Pot full-LEFT (low ADC)  --> Servo LEFT  (0 deg)
  //               Pot center   (~825 ADC)  --> Servo CENTER (90 deg)
  //               Pot full-RIGHT (high ADC)--> Servo RIGHT (180 deg)
  //
  //    NOTE: map() arguments are (input, inLow, inHigh, outLow, outHigh)
  //    Low ADC  (0)    --> ANGLE_MIN (0)   = Servo LEFT
  //    High ADC (1650) --> ANGLE_MAX (180) = Servo RIGHT
  int targetAngle = map((int)filteredADC, ADC_MIN, ADC_MAX, ANGLE_MIN, ANGLE_MAX);

  // Constrain angle within valid physical range
  targetAngle = constrain(targetAngle, ANGLE_MIN, ANGLE_MAX);

  // If servo direction is still inverted on your hardware, uncomment the line below:
  // targetAngle = ANGLE_MAX - targetAngle;

  // 4. Update servo only when angle changes to reduce bus activity
  if (targetAngle != previousAngle)
  {
    myServo.write(targetAngle);
    previousAngle = targetAngle;
  }

  // 5. Print status to Serial Monitor at periodic intervals (100 ms)
  unsigned long currentTime = millis();
  if (currentTime - lastPrintTime >= PRINT_INTERVAL)
  {
    lastPrintTime = currentTime;

    // Calculate approximate voltage for easy inspection
    float voltage = (float)rawADC * 0.002f;

    Serial.print("Raw ADC: ");
    Serial.print(rawADC);
    Serial.print(" | Voltage: ");
    Serial.print(voltage, 2);
    Serial.print(" V | Servo Angle: ");
    Serial.print(targetAngle);
    Serial.println(" deg");
  }

  // Small delay for ADC conversion stability
  delay(15);
}
