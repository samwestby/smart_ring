// I2Cdev and MPU6050 must be installed as libraries, or else the .cpp/.h files
// for both classes must be in the include path of your project
#include "I2Cdev.h"
#include "MPU6050.h"
#include "TinyWireM.h"
// Arduino Wire library is required if I2Cdev I2CDEV_ARDUINO_WIRE implementation
// is used in I2Cdev.h

MPU6050 accelgyro;

int16_t ax, ay, az;
int16_t gx, gy, gz;


#define USE_ACCEL 3


#define LED_PIN 1
//bool blinkState = false;

uint16_t lastReport;

const int numReadings = 13;


#if defined(USE_ACCEL)
const int numAxis = USE_ACCEL;
const int AX = 0;
const int AY = 1;
const int AZ = 2;
#endif

int32_t readings[numAxis][numReadings];  // the reading history
int32_t readIndex[numAxis];              // the index of the current reading
int32_t total[numAxis];                  // the running total
int32_t average[numAxis];                // the average


// x, y around -5k
boolean flat = false;
uint32_t flatStarted = 0;
uint32_t flatDuration = 0;
uint32_t flatLastEnded = 0;

// x > 8k
boolean palmRight = false;
uint32_t palmRightStarted = 0;
uint32_t palmRightDuration = 0;
//uint32_t palmRightLastEnded = 0;

// x < -20k
boolean palmLeft = false;
uint32_t palmLeftStarted = 0;
uint32_t palmLeftDuration = 0;
//uint32_t palmLeftLastEnded = 0;


// y > 9k
boolean palmFront = false;
uint32_t palmFrontStarted = 0;
uint32_t palmFrontDuration = 0;
uint32_t palmFrontLastEnded = 0;

// y < -20k
boolean palmBack = false;
uint32_t palmBackStarted = 0;
uint32_t palmBackDuration = 0;
uint32_t palmBackLastEnded = 0;

boolean stopGesture = false;
boolean shooGesture = false;
boolean clockwiseGesture = false;
boolean counterClockwiseGesture = false;

boolean glowing = false;
uint32_t glowStart = 0;
const uint32_t glowDuration = 100;
#include <SoftwareSerial.h>// import the serial library
SoftwareSerial bluetoothSerial(3, 4); // RX, TX
int BluetoothData; // the data given from Computer

void setup() {
  lastReport = millis();
    // join I2C bus (I2Cdev library doesn't do this automatically)
    TinyWireM.begin();
    #if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
        TinyWireM.begin();
    #elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
        Fastwire::setup(400, true);
    #endif

    // initialize serial communication
    // (38400 chosen because it works as well at 8MHz as it does at 16MHz, but
    // it's really up to you depending on your project)
//    Serial.begin(38400);
    bluetoothSerial.begin(9600);
//    bluetoothSerial.println("Bluetooth On please press 1 or 0 blink LED ..");
    // initialize device
//    DEBUG_PRINTLN("Initializing I2C devices...");
    accelgyro.initialize();

    // supply your own gyro offsets here, scaled for min sensitivity
    accelgyro.setXGyroOffset(-1100);
    accelgyro.setYGyroOffset(271);
    accelgyro.setZGyroOffset(-60);
    accelgyro.setXAccelOffset(-2509);
    accelgyro.setYAccelOffset(-101);
    accelgyro.setZAccelOffset(925); // 1688 factory default for my test chip

    // configure Arduino LED for
    pinMode(LED_PIN, OUTPUT);
    

    // zero-fill all the arrays:
    for (int axis = 0; axis < numAxis; axis++) {
        readIndex[axis] = 0;
        total[axis] = 0;
        average[axis] = 0;
        for (int i = 0; i<numReadings; i++){
            readings[axis][i] = 0;
        }
    }
}


void loop() {
    // read raw accel/gyro measurements from device
    accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    #ifdef USE_ACCEL
        smooth(AX, ax);
        smooth(AY, ay);
        smooth(AZ, az);
    #endif


    checkFlat();
    checkPalmLeft();
    checkPalmRight();
    checkPalmBack();
    checkPalmFront();
    
    checkStopGesture();
    checkShooGesture();
    checkClockwiseGesture();
    checkCounterClockwiseGesture();

    if (glowing && millis() - glowStart > glowDuration){
      digitalWrite(LED_PIN, LOW);
    }




void checkFlat(){
    #ifdef USE_ACCEL
    if( average[AX] < 0 && average[AX] > -13000 && average[AY] < 0 && average[AY] > -10000){
        if(!flat){
//            Serial.println("Flat");
            flatStarted = millis();
        }
        flatLastEnded = millis();
        flatDuration = millis() - flatStarted;
        flat = true;
    } else {
        flat = false;
    }
    #endif
}

void checkPalmLeft(){
    #ifdef USE_ACCEL
    if( average[AX] < -19000) {
        
        if(!palmLeft){
            palmLeftStarted = millis();
        }
//        palmLeftLastEnded = millis();
        palmLeftDuration = millis() - palmLeftStarted;
        palmLeft = true;
    } else {
        palmLeft = false;
    }
    #endif
}

void checkPalmRight(){
    #ifdef USE_ACCEL
    if( average[AX] > 8000 ){
        
        if(!palmRight){
//            Serial.println("palmRight");
            palmRightStarted = millis();
        }
//        palmRightLastEnded = millis();
        palmRightDuration = millis() - palmRightStarted;
        palmRight = true;
    } else {
        palmRight = false;
    }
    #endif
}

void checkPalmFront(){
    #ifdef USE_ACCEL
    if( average[AY] > 8000){
//        Serial.println("palmFront");
        if(!palmFront){
//            Serial.println("palmFront");
            palmFrontStarted = millis();
        }
//        palmFrontLastEnded = millis();
        palmFrontDuration = millis() - palmFrontStarted;
        palmFront = true;
    } else {
        palmFront = false;
    }
    #endif
}

void checkPalmBack(){
    #ifdef USE_ACCEL
    if( average[AY] < -20000){
        
        if(!palmBack){
//            Serial.println("palmBack");
            palmBackStarted = millis();
        }
        palmBackLastEnded = millis();
        palmBackDuration = millis() - palmBackStarted;
        palmBack = true;
    } else {
        palmBack = false;
    }
    #endif
}


boolean checkStopGesture(){
    if(palmFront && palmFrontDuration > 100 && millis() - flatLastEnded < 1000 ){
        // Sword of Omens, Give Me Sight Beyond Sight!
        if (!stopGesture){
//          Serial.println("Stop Gesture made"); 
          bluetoothSerial.println("0");
          stopGesture = true;
          digitalWrite(LED_PIN, HIGH);
          glowing = true;
          glowStart = millis();
        }
        return true;
    }
    stopGesture = false;
    return false;
}


boolean flatToPalmDown = false;

boolean checkShooGesture(){
    shooGesture = false;
    if(palmBack && millis() - flatLastEnded < 1000 ){
        flatToPalmDown = true;
        return false;
    }
    if (flatToPalmDown){
        if (millis() - palmBackLastEnded > 1000){
          flatToPalmDown = false;
          return false;
        }
        if (flat && flatDuration > 50){
          if (!shooGesture){
            flatToPalmDown = false;
            shooGesture = true;
//            Serial.println("shooGesture");
            bluetoothSerial.println("1");
            digitalWrite(LED_PIN, HIGH);
            glowing = true;
            glowStart = millis();
          }
          return true;
        }
    }
    return false;    
}

boolean checkClockwiseGesture(){
    if(palmLeft && palmLeftDuration > 50 && millis() - flatLastEnded < 500 ){
        if (!clockwiseGesture){
//          Serial.println("clockwiseGesture made"); 
          bluetoothSerial.println("2");
          clockwiseGesture = true;
          digitalWrite(LED_PIN, HIGH);
          glowing = true;
          glowStart = millis();
        }
        return true;
    }
    clockwiseGesture = false;
    return false;
}
boolean checkCounterClockwiseGesture(){
    if(palmRight && palmRightDuration > 50 && millis() - flatLastEnded < 500 ){
        if (!counterClockwiseGesture){
//          Serial.println("counterClockwiseGesture made"); 
          counterClockwiseGesture = true;
          digitalWrite(LED_PIN, HIGH);
          bluetoothSerial.println("3");
          glowing = true;
          glowStart = millis();
        }
        return true;
    }
    counterClockwiseGesture = false;
    return false;
}

void smooth(int axis, int32_t val) {
    // pop and subtract the last reading:
    total[axis] -= readings[axis][readIndex[axis]];
    total[axis] += val;

    // add value to running total
    readings[axis][readIndex[axis]] = val;
    readIndex[axis]++;

    if(readIndex[axis] >= numReadings)
        readIndex[axis] = 0;

    // calculate the average:
    average[axis] = total[axis] / numReadings;
}
