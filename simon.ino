// Simon Memory Game - Arduino Version
// Uses hardware-specific commands and serial monitoring

#include <EEPROM.h>

// Hardware pins
#define RED_LED 2
#define GREEN_LED 3
#define BLUE_LED 4
#define YELLOW_LED 5
#define RED_BTN 6
#define GREEN_BTN 7
#define BLUE_BTN 8
#define YELLOW_BTN 9
#define BUZZER 10

// Memory addresses for EEPROM
#define EEPROM_HIGH_SCORE 0

// Game constants
#define MAX_LEVEL 100
#define INITIAL_SPEED 1000
#define SPEED_DECREASE 50
#define MIN_SPEED 250

// Button states
byte currentButtonState[4] = {HIGH, HIGH, HIGH, HIGH};
byte lastButtonState[4] = {HIGH, HIGH, HIGH, HIGH};
unsigned long lastDebounceTime[4] = {0, 0, 0, 0};
const unsigned long debounceDelay = 50;

// LED and button arrays for easier iteration
const byte ledPins[4] = {RED_LED, GREEN_LED, BLUE_LED, YELLOW_LED};
const byte buttonPins[4] = {RED_BTN, GREEN_BTN, BLUE_BTN, YELLOW_BTN};
const int tones[4] = {440, 554, 659, 880};  // A4, C#5, E5, A5

// Game variables
byte gameSequence[MAX_LEVEL];
byte currentLevel = 0;
byte highScore = 0;
unsigned long currentSpeed = INITIAL_SPEED;
bool gameOver = false;

void setup() {
  Serial.begin(9600);
  Serial.println(F("Simon Game Starting..."));
  
  // Configure pins
  for(byte i = 0; i < 4; i++) {
    pinMode(ledPins[i], OUTPUT);
    pinMode(buttonPins[i], INPUT_PULLUP);
    digitalWrite(ledPins[i], LOW);
  }
  pinMode(BUZZER, OUTPUT);
  
  // Read high score from EEPROM
  highScore = EEPROM.read(EEPROM_HIGH_SCORE);
  Serial.print(F("High Score: "));
  Serial.println(highScore);
  
  // Initialize random seed using noise from analog pin
  randomSeed(analogRead(A0));
  
  // Start-up test sequence
  startupSequence();
}

void loop() {
  if (!gameOver) {
    // Add new step to sequence
    addNewStep();
    
    // Display sequence
    displaySequence();
    
    // Get player input
    if (!readPlayerSequence()) {
      handleGameOver();
    } else {
      levelUp();
    }
  } else {
    // Wait for button press to start new game
    if (checkAnyButton()) {
      resetGame();
    }
  }
}

void startupSequence() {
  Serial.println(F("Running startup sequence..."));
  
  // Individual LED test
  for(byte i = 0; i < 4; i++) {
    digitalWrite(ledPins[i], HIGH);
    tone(BUZZER, tones[i], 200);
    Serial.print(F("Testing LED "));
    Serial.println(i);
    delay(300);
    digitalWrite(ledPins[i], LOW);
    delay(100);
  }
  
  // All LEDs flash twice
  for(byte i = 0; i < 2; i++) {
    for(byte j = 0; j < 4; j++) {
      digitalWrite(ledPins[j], HIGH);
    }
    tone(BUZZER, 1047, 200); // C6
    delay(200);
    for(byte j = 0; j < 4; j++) {
      digitalWrite(ledPins[j], LOW);
    }
    delay(200);
  }
  
  Serial.println(F("Startup complete"));
}

void addNewStep() {
  if(currentLevel < MAX_LEVEL) {
    gameSequence[currentLevel] = random(4);
    Serial.print(F("Added step: "));
    Serial.println(gameSequence[currentLevel]);
  }
}

void displaySequence() {
  Serial.println(F("Displaying sequence..."));
  
  for(byte i = 0; i <= currentLevel; i++) {
    byte currentStep = gameSequence[i];
    
    digitalWrite(ledPins[currentStep], HIGH);
    tone(BUZZER, tones[currentStep]);
    
    Serial.print(F("Step "));
    Serial.print(i);
    Serial.print(F(": LED "));
    Serial.println(currentStep);
    
    delay(currentSpeed);
    
    digitalWrite(ledPins[currentStep], LOW);
    noTone(BUZZER);
    
    delay(currentSpeed/2);
  }
  
  Serial.println(F("Sequence complete"));
}

bool readPlayerSequence() {
  Serial.println(F("Reading player input..."));
  
  for(byte currentStep = 0; currentStep <= currentLevel; currentStep++) {
    unsigned long inputStartTime = millis();
    bool inputReceived = false;
    byte playerInput;
    
    // Wait for input with timeout
    while(!inputReceived && (millis() - inputStartTime < 3000)) {
      for(byte button = 0; button < 4; button++) {
        if(readButton(button)) {
          playerInput = button;
          inputReceived = true;
          
          // Visual and audio feedback
          digitalWrite(ledPins[button], HIGH);
          tone(BUZZER, tones[button]);
          delay(200);
          digitalWrite(ledPins[button], LOW);
          noTone(BUZZER);
          
          Serial.print(F("Player pressed: "));
          Serial.println(button);
          
          break;
        }
      }
    }
    
    // Check for timeout
    if(!inputReceived) {
      Serial.println(F("Input timeout!"));
      return false;
    }
    
    // Check if input matches sequence
    if(playerInput != gameSequence[currentStep]) {
      Serial.println(F("Wrong button!"));
      return false;
    }
  }
  
  Serial.println(F("Sequence correct!"));
  return true;
}

bool readButton(byte button) {
  bool buttonPressed = false;
  byte reading = digitalRead(buttonPins[button]);
  
  if(reading != lastButtonState[button]) {
    lastDebounceTime[button] = millis();
  }
  
  if((millis() - lastDebounceTime[button]) > debounceDelay) {
    if(reading != currentButtonState[button]) {
      currentButtonState[button] = reading;
      if(currentButtonState[button] == LOW) {
        buttonPressed = true;
      }
    }
  }
  
  lastButtonState[button] = reading;
  return buttonPressed;
}

void handleGameOver() {
  gameOver = true;
  Serial.println(F("Game Over!"));
  
  // Update high score if needed
  if(currentLevel > highScore) {
    highScore = currentLevel;
    EEPROM.write(EEPROM_HIGH_SCORE, highScore);
    Serial.print(F("New High Score: "));
    Serial.println(highScore);
  }
  
  // Error sequence
  for(byte i = 0; i < 4; i++) {
    for(byte j = 0; j < 4; j++) {
      digitalWrite(ledPins[j], HIGH);
    }
    tone(BUZZER, 98); // Low error tone
    delay(200);
    
    for(byte j = 0; j < 4; j++) {
      digitalWrite(ledPins[j], LOW);
    }
    noTone(BUZZER);
    delay(200);
  }
  
  // Display score
  Serial.print(F("Final Score: "));
  Serial.println(currentLevel);
  delay(1000);
  
  for(byte i = 0; i < currentLevel; i++) {
    digitalWrite(RED_LED, HIGH);
    tone(BUZZER, 440, 100);
    delay(200);
    digitalWrite(RED_LED, LOW);
    delay(200);
  }
}

void levelUp() {
  currentLevel++;
  currentSpeed = max(MIN_SPEED, INITIAL_SPEED - (currentLevel * SPEED_DECREASE));
  Serial.print(F("Level up! Now at level "));
  Serial.println(currentLevel);
  delay(1000);
}

void resetGame() {
  Serial.println(F("Resetting game..."));
  currentLevel = 0;
  currentSpeed = INITIAL_SPEED;
  gameOver = false;
  delay(500);
  startupSequence();
}

bool checkAnyButton() {
  for(byte i = 0; i < 4; i++) {
    if(readButton(i)) {
      return true;
    }
  }
  return false;
}
