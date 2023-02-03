//robot2
#include "common.h"
#include <SimpleKalmanFilter.h>

using namespace std;
char packetBuffer[256]; //buffer to hold incoming packet

//--------------------------------------------filtro de media movil simple para estabilizar la lectura de rad/s---------------------------------------
MeanFilter<long> meanFilterD(10);
MeanFilter<long> meanFilterI(10);
//Time variables
unsigned long previous_timer;
unsigned long timer=10000;


//operation variables
/*struct appdata operation_send;
struct appdata *server_operation;
*/

//prototypes//

void motorSetup();
void moveForward(const int pinMotor[3], int speed);
void moveBackward(const int pinMotor[3], int speed);
void fullStop(const int pinMotor[3]);

void isrD();
void isrI();
int pidD(double wD);
int pidI(double wI);

void feedForwardD();
void feedForwardI();

int option;
int led = LED_BUILTIN;

void setup() {
  motorSetup();
  // Interrupciones para contar pulsos de encoder
  pinMode(encoderD, INPUT_PULLUP);
  pinMode(encoderI, INPUT_PULLUP);
  pinMode(led,OUTPUT);
  attachInterrupt(digitalPinToInterrupt(encoderI), isrI, RISING);//prepara la entrada del encoder como interrupcion
  attachInterrupt(digitalPinToInterrupt(encoderD), isrD, RISING);

  // Se empieza con los motores parados
  fullStop(pinMotorD);
  fullStop(pinMotorI);

}
  


void loop() {
/*
 * VUESTRO CODIGO AQUI
 */
}



// Funcion que inicializa los pines de control de los motores de Arduino en modo salida
void motorSetup() {
  pinMode(pinIN1, OUTPUT);
  pinMode(pinIN2, OUTPUT);
  pinMode(pinENA, OUTPUT);
  pinMode(pinIN3, OUTPUT);
  pinMode(pinENB, OUTPUT);
  pinMode(pinIN4, OUTPUT);
}

// Mueve hacia adelante el motor conectado a los pines pinMotor[2] con un valor de pwm indicado por speed
// speed vale entre 0 y 255

void moveForward(const int pinMotor[3], int speed) {
  digitalWrite(pinMotor[1], HIGH);
  digitalWrite(pinMotor[2], LOW);
  analogWrite(pinMotor[0], speed);
}

// Mueve hacia atrás el motor conectado a los pines pinMotor[2] con un valor de pwm indicado por speed
// speed vale entre 0 y 255

void moveBackward(const int pinMotor[3], int speed) {
  digitalWrite(pinMotor[1], LOW);
  digitalWrite(pinMotor[2],HIGH);
  analogWrite(pinMotor[0], speed);
}

// Para el motor conectado a los pines pinMotor[2]

void fullStop(const int pinMotor[3]) {
  digitalWrite(pinMotor[1], LOW);
  digitalWrite(pinMotor[2],LOW);
  analogWrite(pinMotor[0], 0);
}

 
void isrD() {
  timeBeforeDebounceD = millis();//tiempo para evitar rebotes
  deltaDebounceD = timeBeforeDebounceD - timeAfterDebounceD;// tiempo que ha pasdo entre interrupcion e interrupcion
  if(deltaDebounceD > TIMEDEBOUNCE) {//condicion para evitar rebotes
    //se empieza a contar el tiempo que ha pasado entre una interrupcion "valida" y otra.    
    startTimeD=micros();
    encoder_countD++;
      deltaTimeD = startTimeD - timeAfterD;
      encoder_countD = 0;
      timeAfterD = micros();
  }
  timeAfterDebounceD = millis();  
}

void isrI() {
  timeBeforeDebounceI = millis();//tiempo para evitar rebotes
  deltaDebounceI = timeBeforeDebounceI - timeAfterDebounceI;// tiempo que ha pasdo entre interrupcion e interrupcion
  if(deltaDebounceI > TIMEDEBOUNCE) {//condicion para evitar rebotes
    //se empieza a contar el tiempo que ha pasado entre una interrupcion "valida" y otra.    
    startTimeI = micros();
    encoder_countI++;//se cuenta los pasos de encoder
    deltaTimeI = startTimeI - timeAfterI;
    encoder_countI = 0;
    timeAfterI = micros();
  }
  timeAfterDebounceI = millis();
}

int pidD(double wD) {
  currentTimeD = millis();
  elapsedTimeD = currentTimeD - previousTimeD;
  int outputD = 0;
  errorD = setpointWD - wD;
  double aux = abs(errorD);
  if(aux >= 0.30) {
    cumErrorD += errorD * elapsedTimeD;                     // calcular la integral del error
    if(lastErrorD > 0 && errorD < 0) { 
       cumErrorD = errorD * elapsedTimeD;
    }
    if(lastErrorD < 0 && errorD > 0) {
       cumErrorD = errorD * elapsedTimeD/1000;
    }
    constrain(cumErrorD, -MAXCUMERROR, MAXCUMERROR);
    rateErrorD = (errorD - lastErrorD) / elapsedTimeD; // calcular la derivada del error
    outputD = static_cast<int> (round(KD_p*errorD + KD_i*cumErrorD + KD_d*rateErrorD ));     // calcular la salida del PID    0.1*errorD +0.0065*cumErrorD + 0.0*rateErrorD
    lastErrorD = errorD;                               // almacenar error anterior
  }
  previousTimeD=currentTimeD;
  //Serial.println(errorD);//0.7*errorD + 0.06*cumErrorD + 0.0*rateErrorD 
  //Serial.println(cumErrorD);
  return outputD;
}

int pidI(double wI) {
  currentTimeI = millis();
  elapsedTimeI = currentTimeI - previousTimeI;
  int outputI = 0;
  errorI = setpointWI - wI;   
  double aux = abs(errorI);
  if(aux >= 0.30) {
    cumErrorI += errorI * elapsedTimeI; 
    //se resetea el error acumulativo cuando se cambia de signo
    if(lastErrorI>0 && errorI<0) {
      cumErrorI = errorI * elapsedTimeI;
    }
    if(lastErrorI<0 && errorI>0) {
      cumErrorI = errorI* elapsedTimeI/1000;
    }
    constrain(cumErrorI, -MAXCUMERROR, MAXCUMERROR);
    rateErrorI = (errorI - lastErrorI) / elapsedTimeI; // calcular la derivada del error    
    outputI = static_cast<int> (round(KI_p*errorI + KI_i*cumErrorI + KI_d*rateErrorI));     // calcular la salida del PID 0.42*errorI  + 0.006*cumErrorI + 0.00*rateErrorI
    lastErrorI = errorI;
  }
  previousTimeI = currentTimeI;
  return outputI;
}

void feedForwardD() {
  PWM_D = (setpointWD != 0.0) ?
    constrain(round((setpointWD - 0.0825) / 0.0707), MINPWM, MAXPWM) : 0;
}

void feedForwardI(){
  PWM_I = (setpointWI != 0.0) ? 
    constrain(round((setpointWI + 1.656) / 0.0720), MINPWM, MAXPWM) : 0;
}
