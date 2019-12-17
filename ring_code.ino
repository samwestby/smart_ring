// I2Cdev and MPU6050 must be installed as libraries, or else the .cpp/.h files
// for both classes must be in the include path of your project
#include "I2Cdev.h"
#include "MPU6050.h"
#include "TinyWireM.h"
// Arduino Wire library is required if I2Cdev I2CDEV_ARDUINO_WIRE implementation
// is used in I2Cdev.h
//#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
//    #include "TinyWireM.h"
//#endif

// class default I2C address is 0x68
// specific I2C addresses may be passed as a parameter here
// AD0 low = 0x68 (default for InvenSense evaluation board)
// AD0 high = 0x69
MPU6050 accelgyro;
//MPU6050 accelgyro(0x69); // <-- use for AD0 high

int16_t ax, ay, az;
int16_t gx, gy, gz;


// uncomment "OUTPUT_READABLE_ACCELGYRO" if you want to see a tab-separated
// list of the accel X/Y/Z and then gyro X/Y/Z values in decimal. Easy to read,
// not so easy to parse, and slow(er) over UART.
//#define OUTPUT_READABLE_ACCELGYRO

// #define DEBUG

//#ifdef DEBUG
// #define DEBUG_PRINT(x)  //Serial.print (x)
// #define DEBUG_PRINTLN(x)  //Serial.println (x)
//#else
// #define DEBUG_PRINT(x)
// #define DEBUG_PRINTLN(x)
//#endif

#define USE_ACCEL 3
// #define USE_GYRO 3


#define LED_PIN 1
//bool blinkState = false;

uint16_t lastReport;

const int numReadings = 13;

//#if defined(USE_ACCEL) && defined(USE_GYRO)
//const int numAxis = USE_ACCEL + USE_GYRO;
//const int AX = 0;
//const int AY = 1;
//const int AZ = 2;
//const int GX = 3;
//const int GY = 4;
//const int GZ = 5;
#if defined(USE_ACCEL)
const int numAxis = USE_ACCEL;
const int AX = 0;
const int AY = 1;
const int AZ = 2;
//#elif defined(USE_GYRO)
//const int numAxis = USE_GYRO;
//const int GX = 0;
//const int GY = 1;
//const int GZ = 2;
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

    // verify connection
//    DEBUG_PRINTLN("Testing device connections...");
//    DEBUG_PRINTLN(accelgyro.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");

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
//    digitalWrite(LED_PIN, HIGH);
//    Serial.println("Setup complete");
}


void loop() {
    // read raw accel/gyro measurements from device
    accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    #ifdef USE_ACCEL
        smooth(AX, ax);
        smooth(AY, ay);
        smooth(AZ, az);
    #endif

//    #ifdef USE_GYRO
//        smooth(GX, gx);
//        smooth(GY, gy);
//        smooth(GZ, gz);
//    #endif
//
//    #ifdef OUTPUT_READABLE_ACCELGYRO
//        reportAcccelGyro();
//    #endif

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
//    if (bluetoothSerial.available()){
//      BluetoothData=bluetoothSerial.read();
//      if(BluetoothData=='1'){   
//        digitalWrite(LED_PIN,HIGH);
////        bluetoothSerial.println("LED  On D13 ON ! ");
//      }
//      if (BluetoothData=='0'){
//        digitalWrite(LED_PIN,LOW);
////        bluetoothSerial.println("LED  On D13 Off ! ");
//      }
//      delay(50);
//    }
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



//void reportStates(){
//    DEBUG_PRINT("flat: ");
//    DEBUG_PRINT(flat);
//    DEBUG_PRINT(" duration: ");
//    DEBUG_PRINT(flatDuration);
//    DEBUG_PRINT(" since last: ");
//    DEBUG_PRINT(millis() - flatLastEnded);
//
//    DEBUG_PRINT(" | vertical: ");
//    DEBUG_PRINT(vertical);
//    DEBUG_PRINT(" duration: ");
//    DEBUG_PRINT(verticalDuration);
//    DEBUG_PRINT(" since last: ");
//    DEBUG_PRINT(millis() - verticalLastEnded);
//
//    DEBUG_PRINT(" Glowing?: ");
//    DEBUG_PRINTLN(glowing);
//}


// display tab-separated accel/gyro x/y/z values
//void reportAcccelGyro(){
//
//    #ifdef USE_ACCEL
//        DEBUG_PRINT(average[AX]);
//        DEBUG_PRINT("\t");
//
//        DEBUG_PRINT(average[AY]);
//        DEBUG_PRINT("\t");
//
//        DEBUG_PRINT(average[AZ]);
//        DEBUG_PRINT("\t");
//    #endif
//
//    #ifdef USE_GYRO
//        DEBUG_PRINT(average[GX]);
//        DEBUG_PRINT("\t");
//
//        DEBUG_PRINT(average[GY]);
//        DEBUG_PRINT("\t");
//
//        DEBUG_PRINT(average[GZ]);
//        DEBUG_PRINT("\t");
//    #endif
//
//        DEBUG_PRINT(millis());
//        DEBUG_PRINT("\t");
//
//        DEBUG_PRINTLN(millis() - lastReport);
//        lastReport = millis();
//}

//
//
//
//void handleLights(){
//    if(glowing){
//        DEBUG_PRINTLN("GLOWING!");
//        if(glowEnd == -1)
//            glowEnd = millis() + glowDuration;
//
//        if(millis() > glowEnd)
//            glowing = false;
//
//        // run glow function here
//
//        blinkState = !blinkState;
//    }
//    if(!glowing){
//        // do some variable cleanup
//        glowEnd = -1;
//        blinkState = false;
//    }
//    digitalWrite(LED_PIN, blinkState);
//}


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