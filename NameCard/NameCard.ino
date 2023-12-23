#include "SparkFun_STHS34PF80_Arduino_Library.h"
#include <Wire.h>

#define PRESENSE_TH 1500
#define DELAY_MIN 2
#define DELAY_MAX 10

STHS34PF80_I2C mySensor;
// Values to fill with presence and motion data
int16_t presenceVal = 0;
int16_t motionVal = 0;
float temperatureVal = 0;
int delay_value = 0;
int prevSwitchState = HIGH;
int curSwitchState = HIGH;
int blink_mode = 0;

const int numPins = 16;
const int numGrp = 7;
const int pwmPins[] = {4,5,6,7,15,16,17,18,8,19,20,9,10,11,13,14};
const int pwmPinsGrp[] = {0,1,2,3,4,5,6,0,1,2,3,4,5,6,0,0};

TaskHandle_t thp[1];
TaskHandle_t thp2[1];

void getSensorValue(void *args) {
  while(1){
    sths34pf80_tmos_drdy_status_t dataReady;
    mySensor.getDataReady(&dataReady);

    if(dataReady.drdy == 1){
      sths34pf80_tmos_func_status_t status;
      mySensor.getStatus(&status);

      if(status.pres_flag == 1){
        // Presence Units: cm^-1
        mySensor.getPresenceValue(&presenceVal);
        mySensor.getTemperatureData(&temperatureVal);
      }else{
        ;
      }
    }else{
      ;
    }

    if(presenceVal >= PRESENSE_TH){
      delay_value = (int)(((float)9.0 * (float)presenceVal)/(float)-7000.0 + (float)16.4286);

      if(delay_value < DELAY_MIN){
        delay_value = DELAY_MIN;
      }
      if(delay_value > DELAY_MAX){
        delay_value = DELAY_MAX;
      }
    }else{
      delay_value = DELAY_MAX;
    }
    delay(10);
  }
}

void getButtonState(void *args) {
  while(1){
    curSwitchState = digitalRead(1);
    if(prevSwitchState == HIGH && curSwitchState == LOW){
      if(blink_mode == 0){
        blink_mode = 1;
      }else{
        blink_mode = 0;
      }
    }
    prevSwitchState = curSwitchState;
    delay(100);
  }
}

void setup() {
  for (int i = 0; i < numPins; i++) {
      ledcSetup(i, 5000, 8);          // チャンネル、周波数、解像度を設定
      ledcAttachPin(pwmPins[i], pwmPinsGrp[i]);  // GPIOピンをチャンネルにアタッチ
  }

  ledcSetup(0, 5000, 8);    // チャンネル、周波数、解像度を設定
  ledcAttachPin(2, 0);      // GPIOピンをチャンネルにアタッチ

  for (int i = 0; i < numPins; i++) {
      ledcWrite(i, 255);
  }

  // Serial.begin(115200);

  // Begin I2C
  if(Wire.begin(42,41) == 0){
    // Serial.println("I2C Error - check I2C Address");
    while(1);
  }

  // Establish communication with device 
  if(mySensor.begin() == false){
    // Serial.println("Error setting up device - please check wiring.");
    while(1);
  }

  randomSeed(100);
  pinMode(1, INPUT);

  xTaskCreateUniversal(getSensorValue, "getSensorValue", 4096, NULL, 0, &thp[0], 1);
  xTaskCreateUniversal(getButtonState, "getButtonState", 4096, NULL, 0, &thp2[0], 1);

  delay(1000);
}

// the loop function runs over and over again forever
void loop() {
  // Serial.print(presenceVal);
  // Serial.print(",");
  // Serial.print(temperatureVal);
  // Serial.print(",");
  // Serial.println(delay_value);

  // if(presenceVal < 1500 || temperatureVal < -6){
  if(presenceVal < PRESENSE_TH){
    for (int i = 0; i < numPins; i++) {
      ledcWrite(i, 255);
    }
  }else{
    if(blink_mode == 0){
      // PWM値を逆方向に徐々に変化させる
      for (int dutyCycle = 255; dutyCycle >= 0; dutyCycle--) {
        // 各ピンのPWM値を設定
        for (int i = 0; i < numGrp; i++) {
          ledcWrite(i, dutyCycle);
        }
        delay(delay_value);
      }

      // PWM値を徐々に変化させる
      for (int dutyCycle = 0; dutyCycle <= 255; dutyCycle++) {
        // 各ピンのPWM値を設定
        for (int i = 0; i < numGrp; i++) {
          ledcWrite(i, dutyCycle);
        }
        delay(delay_value);
      }
    }else{
      // PWM値を逆方向に徐々に変化させる
      for (int i = 0; i < numGrp; i++) {
        for (int dutyCycle = 255; dutyCycle >= 0; dutyCycle-=5) {
        // 各ピンのPWM値を設定
          ledcWrite(i, dutyCycle);
          delay(delay_value);
        }
      }

      // PWM値を徐々に変化させる
      for (int i = 0; i < numGrp; i++) {
        for (int dutyCycle = 0; dutyCycle <= 255; dutyCycle+=5) {
        // 各ピンのPWM値を設定
          ledcWrite(i, dutyCycle);
          delay(delay_value);
        }
      }
    }
  }
  delay(10);
}