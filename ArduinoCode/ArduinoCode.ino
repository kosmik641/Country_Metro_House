#include <Wire.h>
#define PIN_COUNT 13
uint8_t pinConf[PIN_COUNT] = {2,3,4,5,6,7,8,22,23,24,25,26,27}; //2,3,4,5,6,7,8,22,23,24,25,26,27

void setup() {
  Serial.begin(74880);
  Serial.setTimeout(5);
  for (int i=0;i<PIN_COUNT;i++){
    pinMode(pinConf[i],INPUT_PULLUP);
  }
}

void loop() {
  uint8_t nmbOutBytes = PIN_COUNT/8 + ((PIN_COUNT%8)&&1) + 1;
  uint8_t *outBytes = new uint8_t[nmbOutBytes]();
  outBytes[0] = nmbOutBytes;
  for (int i = 1; i<nmbOutBytes; i++){
     for (int pinNmb=0;pinNmb<8;pinNmb++){
        uint8_t currPin = pinConf[8*i + pinNmb];
        if (currPin > 0){
          outBytes[i] |= !digitalRead(currPin) << pinNmb;
        }  
     }
  }
  Serial.write(outBytes,nmbOutBytes);
  delete[] outBytes;
}
