#include <Arduino.h>

#include "cap1188.h"

#define SERIAL_BAUDRATE 115200

#define BUZZER_PIN  GPIO_NUM_32
#define BUZZER_FREQ 500              // Buzzer audible frequency

#define BUZZER_GREEN_ON_US  500000   // Microseconds to keep buzzer on during a Green light
#define BUZZER_GREEN_OFF_US 1500000  // Microseconds to keep buzzer off during a Green light
#define BUZZER_RED_US       250000   // Microseconds to alternate buzzer during a Red light

#define GREEN_LED_PIN  GPIO_NUM_13
#define YELLOW_LED_PIN GPIO_NUM_15
#define RED_LED_PIN    GPIO_NUM_2

#define CAP1188_ADDRESS       0x28
#define CAP1188_SENSITIVITY   5
#define CAP1188_INTERRUPT_PIN GPIO_NUM_37

// Buzzer timer and associated state
hw_timer_t*   buzzerTimer  = nullptr;
volatile bool isBuzzerOn   = false;
volatile bool isLightGreen = false;

// Capacitive touch over I2C
Adafruit_CAP1188 cap = Adafruit_CAP1188();

// onBuzzerTimer is the interrupt service routine for the Buzzer timer
void IRAM_ATTR onBuzzerTimer() {
  // Flip the buzzer state
  isBuzzerOn ? noTone(BUZZER_PIN) : tone(BUZZER_PIN, BUZZER_FREQ);
  isBuzzerOn = !isBuzzerOn;

  // Determine when we should fire the ISR next
  uint64_t nextDelay = 0;
  if (isLightGreen) {
    isBuzzerOn ? nextDelay = BUZZER_GREEN_ON_US : nextDelay = BUZZER_GREEN_OFF_US;
  } else {
    nextDelay = BUZZER_RED_US;
  }

  // Schedule the next alarm
  uint64_t currentAlarmVal = timerAlarmRead(buzzerTimer);
  timerAlarmWrite(buzzerTimer, currentAlarmVal + nextDelay, false);
  timerAlarmEnable(buzzerTimer);

  Serial.println(timerAlarmRead(buzzerTimer));
}

void doRedCycle(Adafruit_CAP1188* cap);

void setup() {
  #ifdef SERIAL_BAUDRATE
  Serial.begin(SERIAL_BAUDRATE);
  #endif

  // Set buzzer pin to output mode
  pinMode(BUZZER_PIN, OUTPUT);

  // Initialize buzzer interrupt routine
  buzzerTimer = timerBegin(0, APB_CLK_FREQ / 1000000, true);
  timerAttachInterrupt(buzzerTimer, &onBuzzerTimer, true);
  onBuzzerTimer();

  // Set LED pin output modes
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);

  // Write default LED states
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);

  // Initialize capacitive touch
  if (!cap.begin(CAP1188_ADDRESS)) {
    Serial.println("CAP1188 initialization failed");
    while (1);
  }

  // Set capacitive touch sensitivity
  if (!setCapSensitivity(&cap, CAP1188_SENSITIVITY)) {
    Serial.println("CAP1188 sensitivity initialization failed");
    while (1);
  }
  
  // Do initial red cycle
  doRedCycle(&cap);
}

void doRedCycle(Adafruit_CAP1188* cap) {
  // Switch to red
  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, HIGH);
  isLightGreen = false;

  // Remain in red for 10 seconds
  delay(10000);

  // Switch to red-yellow for 2 seconds
  digitalWrite(YELLOW_LED_PIN, HIGH);
  delay(2000);

  // Go back to green
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, HIGH);
  isLightGreen = true;

  // Cycle touch before returning so it resets
  (*cap).touched();
}

void loop() {
  while (cap.touched() == 0) delay(50);

  Serial.println("touched!");
  
  // Wait for 5 seconds until turning yellow
  delay(5000);

  // Switch to yellow
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, HIGH);

  // Wait 2 more seconds
  delay(2000);

  doRedCycle(&cap);

  return;
}
