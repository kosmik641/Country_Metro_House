// Enter used pins here:
#define PIN_MAP 2,3,4,5,6,7,8,22,23,24,25,26,27

uint8_t pinConf[] = {PIN_MAP};
uint8_t pinCount = sizeof(pinConf)/sizeof(uint8_t);
uint8_t nmbOutBytes = pinCount/8 + ((pinCount%8)&&1) + 1;
uint8_t currPin = 0;
uint32_t lastTick;

void setup() {
  Serial.begin(115200);
  for (int i=0;i < pinCount;i++) pinMode(pinConf[i],INPUT_PULLUP);
  lastTick = millis();
}

void loop() {
  if (millis() - lastTick < 14) return; //66-67 TPS
  lastTick = millis();
  
  uint8_t *outBytes = new uint8_t[nmbOutBytes]();
  outBytes[0] = nmbOutBytes;
  for (int i = 1; i < nmbOutBytes; i++){
     for (int pinNmb = 0; pinNmb < 8; pinNmb++){
        currPin = pinConf[8*(i-1) + pinNmb];
        if (currPin) outBytes[i] |= !digitalRead(currPin) << pinNmb;
     }
  }
  Serial.write(outBytes,nmbOutBytes);
  delete[] outBytes; 
}
