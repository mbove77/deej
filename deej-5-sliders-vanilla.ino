#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>
#include <Bounce2.h>

const int NUM_SLIDERS = 5;
const int analogInputs[NUM_SLIDERS] = {A7, A6, A5, A4, A3};

#define LED_PIN 6
#define BUTTON_PIN 7
#define LONG_PRESS_DURATION 1000

Adafruit_NeoPixel strip(NUM_SLIDERS, LED_PIN, NEO_GRB + NEO_KHZ800);

Bounce debouncer = Bounce();

int ledMode = 1;
int analogSliderValues[NUM_SLIDERS];
int savedSliderValues[NUM_SLIDERS]; // Para guardar valores al hacer mute
bool isMuted = false;
unsigned long buttonPressStartTime = 0;
bool buttonActive = false;

void setup() {
  for (int i = 0; i < NUM_SLIDERS; i++) {
    pinMode(analogInputs[i], INPUT);
  }

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  debouncer.attach(BUTTON_PIN);
  debouncer.interval(25); // intervalo de debounce en ms

  Serial.begin(9600);
  strip.begin();
  strip.setBrightness(100);
  strip.show();

  // Leer modo desde EEPROM
  byte savedMode = EEPROM.read(0);
  if (savedMode <= 9) {
    ledMode = savedMode;
  }
  Serial.println(ledMode);
}

void loop() {
  debouncer.update();
  checkButton();
  updateSliderValues();
  updateLEDs();
  sendSliderValues();
  delay(10);
}

void checkButton() {
  // Detección de pulsación corta/larga
  if (debouncer.fell()) {
    buttonPressStartTime = millis();
    buttonActive = true;
  }
  
  if (debouncer.rose() && buttonActive) {
    buttonActive = false;
    unsigned long pressDuration = millis() - buttonPressStartTime;
    
    if (pressDuration < LONG_PRESS_DURATION) {
      ledMode = (ledMode + 1) % 9;
      EEPROM.write(0, ledMode);
      delay(20);
      Serial.print("Modo cambiado a: ");
      Serial.println(ledMode);
    }
  }
  
  // Detección de pulsación larga (mientras se mantiene presionado)
  if (buttonActive && (millis() - buttonPressStartTime) >= LONG_PRESS_DURATION) {
    buttonActive = false; // Para no repetir la acción
    
    if (!isMuted) {
      for (int i = 0; i < NUM_SLIDERS; i++) {
        savedSliderValues[i] = analogSliderValues[i];
        analogSliderValues[i] = 1023; //Lo ponemos a 1023 por que tengo invertido en deej
      }
      isMuted = true;
    } else {
      for (int i = 0; i < NUM_SLIDERS; i++) {
        analogSliderValues[i] = savedSliderValues[i];
      }
      isMuted = false;
    }
  }
}

void updateSliderValues() {
  if (!isMuted) {
    for (int i = 0; i < NUM_SLIDERS; i++) {
      analogSliderValues[i] = analogRead(analogInputs[i]);
    }
  }
}

void sendSliderValues() {
  String builtString = "";
  for (int i = 0; i < NUM_SLIDERS; i++) {
    builtString += String(analogSliderValues[i]);
    if (i < NUM_SLIDERS - 1) builtString += "|";
  }
  Serial.println(builtString);
}

uint32_t colorRedToCyan(int val, bool invertir = false) {   
  if(invertir) {
    val = 1023 - val;
  }
  if(val < 341) {
    return strip.Color(255, map(val, 0, 340, 0, 165), 0                           );
  } 
  else if(val < 682) { 
    return strip.Color(map(val, 341, 681, 255, 0), map(val, 341, 681, 165, 255), 0);
  }
  else {
    return strip.Color(0, 255, map(val, 682, 1023, 0, 255));
  }
}

uint32_t colorBlueToWhite(int val, bool invertir = false) {
  if(invertir) {
    val = 1023 - val;
  }
  if (val < 512) return strip.Color(map(val, 0, 511, 255, 0), 255, 255);
  return strip.Color(0, map(val, 512, 1023, 255, 0), 255);
}

uint32_t colorWhiteToBlue(int val, bool invertir = false) {
  if(invertir) {
    val = 1023 - val;
  }
  if (val < 512) return strip.Color(map(val, 0, 511, 255, 0), 255, 255);
  return strip.Color(0, map(val, 512, 1023, 255, 0), 255);
}

uint32_t colorBlueToOff(int val, bool invertir = false) {
  if(invertir) {
    val = 1023 - val;
  }
  return strip.Color(0, 0, map(val, 0, 1023, 200, 0));
}

uint32_t colorOffToWhite(int val, bool invertir = false) {
  if(invertir) {
    val = 1023 - val;
  }
  byte b = map(val, 0, 1023, 200, 0); 
  return strip.Color(b, b, b);
}

uint32_t colorPurpleToGreen(int val, bool invertir = false) {
  if(invertir) {
    val = 1023 - val;
  }
  return strip.Color(
    map(val, 0, 1023, 0, 128),
    map(val, 0, 1023, 255, 0),
    map(val, 0, 1023, 0, 128)
  );
}

uint32_t colorRainbow(int val, bool invertir = false) {
  if(invertir) {
    val = 1023 - val;
  }
  uint16_t hue = map(val, 0, 1023, 0, 49151); 
  return strip.gamma32(strip.ColorHSV(hue));
}

const float PULSE_SPEED = 0.4f;           // Controla la velocidad general (menor = más lento)
const float LED_TRANSITION_SPEED = 0.2f;  // Controla el desfase entre LEDs (menor = más lento)

uint32_t colorPulse(int val, int sliderIndex) {
  float t = millis() / (1000.0f / PULSE_SPEED);
  float ledOffset = sliderIndex * LED_TRANSITION_SPEED;
  float pulse = (sin((t + ledOffset) * 2 * PI) + 1.0f) / 2.0f;
  byte brightness = pulse * 200;
  uint16_t hue = map(val, 0, 1023, 0, 65535);
  uint32_t color = strip.ColorHSV(hue, 255, brightness);
  return strip.gamma32(color);
}

void updateLEDs() {
  if (ledMode == 0) {
    for (int i = 0; i < NUM_SLIDERS; i++) {
      strip.setPixelColor(i, 0);
    }
    strip.show();
    return;
  }

  for (int i = 0; i < NUM_SLIDERS; i++) {
    int val = analogSliderValues[i];
    int ledIndex = NUM_SLIDERS - 1 - i;
    uint32_t color;

    switch (ledMode) {
      case 1: color = colorRedToCyan(val, true); break;
      case 2: color = colorBlueToWhite(val); break;
      case 3: color = colorBlueToOff(val); break;
      case 4: color = colorOffToWhite(val); break;
      case 5: color = colorWhiteToBlue(val); break; 
      case 6: color = colorPurpleToGreen(val); break;
      case 7: color = colorRainbow(val); break;
      case 8: color = colorPulse(val, i); break;
      default: color = 0; break; 
    }

    strip.setPixelColor(ledIndex, color);
  }
  strip.show();
}
