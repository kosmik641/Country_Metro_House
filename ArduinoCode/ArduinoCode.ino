#include <Wire.h>
#define PIN_COUNT 13
uint8_t pinConf[PIN_COUNT] = {2,3,4,5,6,7,8,22,23,24,25,26,27};

void setup() {
  Serialbegin(57600);
  Serial.setTimeout(5);
  for (int i=0;i<PIN_COUNT;i++){
    pinMode(pinConf[i],INPUT_PULLUP);
  }
}

void loop() {
  uint8_t *outBytes = new uint8_t[2]();
  uint8_t numbOutBytes = (int)PIN_COUNT/8 + (PIN_COUNT % 8 == 0 ? 0 : 1);
  for (int i = 0; i<numbOutBytes; i++){
     for (int pinNmb=0;pinNmb<8;pinNmb++){
        uint8_t currPin = pinConf[8*i + pinNmb];
        if (currPin > 0){
          outBytes[i] |= !digitalRead(currPin) << pinNmb;
        }  
     }
  }
  Serial.write(outBytes,numbOutBytes);
  delete[] outBytes;
}
