/*
    25/12/15 CHRISTMAS! Initial commit.
    29 Dec rectified timer_init()
------------------------------------------------------------------------------------------------
    Tested with gs-control.py, incompatible with kalman*.py as they don't expect quaternions
        Build and flash (via IDE).
        Launch gs-control.py

MPU_CONFIGURATION
    * external fifo_frame sync: TEMP_OUT_L[0] {by default (26/12/15)}
    * Accel and gyro offsets are taken from eeprom/calibrated
    * Clock Source:        Z-GYRO
    * Sample rate:         200Hz (5ms)
    * DLPF bandwidth:      42Hz
    * gyro sensitivity:    2000 dps
    * accel sensitivity:   2g
*/
#include "I2Cdev.h"
#include "Wire.h"
#include "mydmp.h"
#include "mytimer.h"

#define _TIMER_PERIOD 5
//#define READABLE_Q
#define BINARY_Q

MPU6050 mpu;
bool dmpReady = false;  // set true if DMP init was successful
uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
uint16_t fifoCount;     // count of all bytes currently in FIFO
uint8_t fifoBuffer[64]; // FIFO storage buffer

// orientation/motion vars
    Quaternion q;           // [w, x, y, z]         quaternion container
// control and comms
    uint8_t script_ready=0, cmd_in;
    volatile uint8_t IntVector = 0;     // Interrupt Vector and also shows if any interrupt is outstanding.
    enum _interrupts {TIMER_INT=1};     // Types of interrupts
// function decl
     void timer_int_fn(void);


void setup(){
    Wire.begin();
    TWBR = 24; // 400kHz I2C clock (200kHz if CPU is 8MHz)
    Serial.begin(115200);
    mpu_init(0);
    devStatus = mpu.dmpInitialize();
    if (devStatus == 0) {
        cfgr_mpu_off();
        mpu.setDMPEnabled(true);
        dmpReady = true;
        //Serial.println("SETUP");
    }
    pinMode(13, OUTPUT);
    timer_init(_TIMER_PERIOD, &timer_int_fn);

}

void loop(){
    if (!dmpReady) return;
    if (IntVector == 0) {
        // Control-Program behavior stuff here
        if (Serial.available() > 0){
          cmd_in = Serial.read();
          Serial.write(cmd_in+1);
          script_ready = !script_ready;
          digitalWrite(13, !script_ready);
        }
    }
    //handle interrupts
    if (IntVector & TIMER_INT){
        fifoCount = mpu.getFIFOCount();
        if (fifoCount == 1024){
            mpu.resetFIFO(); // if overflow happened
        }
        else if (fifoCount >= dmpPacketSize){
            // wait for correct available data length
            mpu.getFIFOBytes(fifoBuffer, dmpPacketSize);       
            // track FIFO count here in case there is > 1 packet available
            // (this lets us immediately read more without waiting for an interrupt)
            fifoCount -= dmpPacketSize;
            dmpGetQuaternion(&q, fifoBuffer);
        }
        // Even if fifo has been reset or incomplete data in fifo,
        // send "Most Current" (previous) quaternion.
        // Arduino consistently sends data, whether it is true or not.
        if (script_ready){
            #ifdef READABLE_Q
                Serial.print("quat\t");
                Serial.print(q.w);
                Serial.print("\t");
                Serial.print(q.x);
                Serial.print("\t");
                Serial.print(q.y);
                Serial.print("\t");
                Serial.println(q.z);
            #endif
            #ifdef BINARY_Q
                mpu_send_quat_packet(fifoBuffer);
            #endif
        }

        IntVector &= ~TIMER_INT;
    } // other interrupts
}

void timer_int_fn(void){
    IntVector |= TIMER_INT;
}
