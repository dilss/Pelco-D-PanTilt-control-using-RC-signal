
/*Author: Edilson Fernandes Pereira
 *Date: 2018/10/24
 *Initial revision
 *
 *This software allows to control PTZ cam using RC signals, it takes the signal by interruption subroutines and makes the corresponding PelcoD comands 
 *WARNING!Do not use SoftwareSerial library to write the output, use the default arduino serial
 *Set the correct baud rate of you cam, mine was configurable and i've set maximun available(for better performance)
 *It is needed a MAX485 module to convert the RS232 arduino serial output on a RS485 serial output(default on most commercially available PTZ cams), read your camera's manual
 *I tested on PanTilt YP3040 by Axis, use the code on your own risc
 */
 
//Pelco D bytes {sync, adress, comandOne, comandoTwo, dataOne, dataTwo, checksum}
//comandOne(not used in this case therefore is always zero)
//comandTwo bits {focusFar = 0, zoomWide = 0, zoomTele= 0, tiltDown, tiltUp, panLeft, panRight, FixedToZero = 0}
//DataOne - PanSpeed from 0x00(stop) to 0x3F(highSpeed), 0xFF(turbo)
//DataTwo- TiltSpeed from 0x00(stop) to 0x3F(Maximun Speed)
//CheckSum - sum of bytes, excluding the synchronization byte), then modulo 100 (decimal code: 256)

#define LED_PIN                                     13
#define PAN_INTERRUPT_PIN                           2
#define TILT_INTERRUPT_PIN                          3
#define PWM_MIN                                     1100
#define PWM_CENTER                                  1500
#define PWM_MAX                                     1920
#define DEAD_ZONE                                   200
#define PAN_LEFT                                    0x04
#define PAN_RIGTH                                   0x02
#define PAN_STOP                                    0x00
#define PAN_SPEED                                   0xFF  
#define TILT_UP                                     0x08
#define TILT_DOWN                                   0x10
#define TILT_STOP                                   0x00
#define TILT_SPEED                                  0x3F
#define MIN_SPEED                                   0
#define MAX_SPEED                                   63
#define CAM_ADRESS                                  0x01
#define COMAND_ONE                                  0x00
#define SYNC                                        0xFF
#define PORT_TEST                                   12
#define PORT_TEST_2                                 11

volatile unsigned long panPulse = 0;
volatile unsigned long panPreviousTime = 0;
volatile unsigned long tiltPulse = 0;
volatile unsigned long tiltPreviousTime = 0;
volatile unsigned long panPWM, tiltPWM;
byte panDirection, panSpeed, tiltDirection, tiltSpeed;
byte comandTwo = 0x00;
byte checkSum;
byte panTiltMovement[7] = {SYNC, CAM_ADRESS, COMAND_ONE, 0x00, PAN_SPEED, TILT_SPEED, 0x00};

void setup()
{
  Serial.begin(19200);
  pinMode(PORT_TEST, OUTPUT);
  pinMode(PORT_TEST_2, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(PAN_INTERRUPT_PIN), panRising, RISING);
  attachInterrupt(digitalPinToInterrupt(TILT_INTERRUPT_PIN), tiltRising, RISING);
}

void loop()
{

  if (panPWM < (PWM_CENTER - DEAD_ZONE))
    panDirection = PAN_LEFT;
  else if (panPWM > (PWM_CENTER + DEAD_ZONE))
    panDirection = PAN_RIGTH;
  else
    panDirection = PAN_STOP;
     

  if (tiltPWM < (PWM_CENTER - DEAD_ZONE))
    tiltDirection = TILT_DOWN;
  else if (tiltPWM > (PWM_CENTER + DEAD_ZONE))
    tiltDirection  = TILT_UP;
  else
    tiltDirection = TILT_STOP;

  panPWM = constrain(panPWM, PWM_MIN, PWM_MAX);    //Limiting the pwm values
  tiltPWM = constrain(tiltPWM, PWM_MIN, PWM_MAX);

  panSpeed = map(panPWM, (PWM_MIN, PWM_MAX, MIN_SPEED, MAX_SPEED);  //Speed calculated according to the pwm entry
  tiltSpeed = map(tiltPWM, PWM_MIN, PWM_MAX, MIN_SPEED, MAX_SPEED);
  
  comandTwo = panDirection + tiltDirection;

  //If a pwm proporcional speed is desired use in line below panSpeed and tiltSpeed values intead of the pre defined macros
  checkSum = (byte)((CAM_ADRESS + COMAND_ONE + comandTwo + PAN_SPEED + TILT_SPEED) % 256);
  

  panTiltMovement[3] = comandTwo;
  //Uncomment the two lines below two use pwm proportional speeds for pan and tilt movements, do not forget to change as well on cheksum computation
  //panTiltMovement[4] = panSpeed; 
  //panTiltMovement[5] = tiltSpeed;
  panTiltMovement[6] = checkSum;
  Serial.write(panTiltMovement, 7); //Send values by serial using the native arduino serial, 
                                    //do not use SoftwareSerial, its is not hardware implemented and results on interruptions failure
}

/*****************************************************************************************************************************************************/
void panRising()
{
  attachInterrupt(digitalPinToInterrupt(PAN_INTERRUPT_PIN), panFalling, FALLING);
  panPreviousTime = micros();
  //digitalWrite(PORT_TEST, HIGH); //Just for checking on osciloscope if the interruption is working properly
}

void panFalling()
{
  attachInterrupt(digitalPinToInterrupt(PAN_INTERRUPT_PIN), panRising, RISING);
  panPWM = micros() - panPreviousTime;
  //digitalWrite(PORT_TEST,LOW);
}

void tiltRising()
{
  attachInterrupt(digitalPinToInterrupt(TILT_INTERRUPT_PIN), tiltFalling, FALLING);
  tiltPreviousTime = micros();
  //digitalWrite(PORT_TEST_2, HIGH);
}

void tiltFalling()
{
  attachInterrupt(digitalPinToInterrupt(TILT_INTERRUPT_PIN), tiltRising, RISING);
  tiltPWM = micros() - tiltPreviousTime;
  //digitalWrite(PORT_TEST_2, LOW);
}
