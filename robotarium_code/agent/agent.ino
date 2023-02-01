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
struct appdata operation_send;
struct appdata *server_operation;

//prototypes//
void do_operation(int operation);
void op_saludo();
void op_message();
void op_moveWheel();
void op_StopWheel();
void op_vel_robot();
void motorSetup();
void moveForward(const int pinMotor[3], int speed);
void moveBackward(const int pinMotor[3], int speed);
void fullStop(const int pinMotor[3]);
void moveWheel(int pwm,double w, const int pinMotor[3],bool back);
void isrD();
void isrI();
int pidD(double wD);
int pidI(double wI);

void feedForwardD();
void feedForwardI();
//void tokenize(const string s, char c,vector<string>& v);
int option;
int led = 13;

void setup() {
  motorSetup();
  // Interrupciones para contar pulsos de encoder
  pinMode(encoderD, INPUT_PULLUP);
  pinMode(encoderI, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(encoderI), isrI, RISING);//prepara la entrada del encoder como interrupcion
  attachInterrupt(digitalPinToInterrupt(encoderD), isrD, RISING);

  // Se empieza con los motores parados
  fullStop(pinMotorD);
  fullStop(pinMotorI);

}
  

unsigned char *read_ptr; 

void loop() {
  currentTime = micros();
  delay(100);
  op_moveWheel();
  digitalWrite(led, HIGH);

  // TO DO: Refactor
  if(currentTime - timeAfter >= SAMPLINGTIME) {
    int auxPWMD = 0, auxPWMI = 0;
    double fD, fI;
    timeStopD = timeStopI = millis();
    //condition for know when the wheel is stoped
    //se define un contador de tiempo para comprobar que las ruedas estan paradas
    deltaTimeStopD = timeStopD - timeAfterDebounceD;
    deltaTimeStopI = timeStopI - timeAfterDebounceI;
    meanFilterD.AddValue(deltaTimeD);
    meanFilterI.AddValue(deltaTimeI);
    fI = deltaTimeStopI >= 200 ? 0 : (double)1 / (meanFilterI.GetFiltered() * N) * 1000000;
    fD = deltaTimeStopD >= 200 ? 0 : (double)1 / (meanFilterD.GetFiltered() * N) * 1000000;
    //condicion para que no supere linealidad y se sature.
    //es un filtro para que no de valores ridiculos
    if(fD < 15/2/3.14) { wD = 2*3.14*fD; }
    if(fI < 15/2/3.14) { wI = 2*3.14*fI; }
    //fin filtro
    if(wD !=0.0 && setpointWD !=0.0) {   
      PWM_D = constrain(PWM_D + pidD(wD), MINPWM, MAXPWM);
    }
    if(wI !=0.0 && setpointWI !=0.0) {
      PWM_I = constrain(PWM_I + pidI(wI), MINPWM, MAXPWM);
    }
    if(auxPWMI != PWM_I){
      moveWheel(PWM_I, setpointWI, pinMotorI, backI);
      auxPWMI = PWM_I;
    }
    if(auxPWMD != PWM_D){
      moveWheel(PWM_D, setpointWD, pinMotorD, backD);
      auxPWMD = PWM_D;
    }

  }
  timeAfter = currentTime;
}

void do_operation(int operation) {
  switch (operation) {
    case OP_SALUDO:
      op_saludo();
      break;
    case OP_MOVE_WHEEL:
      op_moveWheel();
      digitalWrite(led, HIGH);
      break;
    case OP_STOP_WHEEL:
      op_StopWheel();
      break;
    case OP_VEL_ROBOT:
       break;
    default:
      break;
  }
}


void op_message() { }

void op_moveWheel() {
  digitalWrite(led, LOW);
  setpointWD = bytesToDouble(&server_operation->data[0]);
  setpointWI = bytesToDouble(&server_operation->data[8]);
  if(setpointWD < 1 && setpointWD > -1) {
    setpointWD = 0;
  }
  if(setpointWI < 1 && setpointWI > -1) {
    setpointWI = 0;
  }
  if(setpointWD < 0) {
    //setPointGWD = setpointWD;
    setpointWD = setpointWD*(-1);
    backD = true;
  } else if(setpointWD > 0) {
    backD = false;
  }
  if(setpointWI < 0) {
    //setPointGWI = setpointWI;
    setpointWI = setpointWI*(-1);
    backI = true;
  }
  else if(setpointWI>0) {
    backI = false;
  }
  feedForwardD();
  feedForwardI();  
  moveWheel(PWM_I, setpointWI, pinMotorI, backI);
  moveWheel(PWM_D, setpointWD, pinMotorD, backD);
  for(int i=0; i<20; i++){
    meanFilterD.AddValue(deltaTimeD);
    meanFilterI.AddValue(deltaTimeI);
  }
}

void op_StopWheel() {
  setpointWD=0;
  setpointWI=0;
  fullStop(pinMotorI);
  fullStop(pinMotorD);
}

void op_vel_robot() {
  operation_send.op = OP_VEL_ROBOT;
  operation_send.id = ID;
  short int a=1;
  doubleToBytes(wD, &operation_send.data[0]);
  doubleToBytes(wI, &operation_send.data[8]);
  if(backD) {
    shortToBytes(a, &operation_send.data[16]);
  }
  if(backI) {
    shortToBytes(a, &operation_send.data[18]);
  }
  operation_send.len = 20; //strlen((char*)operation_send.data);
}


void motorSetup() {
  pinMode(pinIN1, OUTPUT);
  pinMode(pinIN2, OUTPUT);
  pinMode(pinENA, OUTPUT);
  pinMode(pinIN3, OUTPUT);
  pinMode(pinIN4, OUTPUT);
  pinMode(pinENB, OUTPUT);
}

void moveForward(const int pinMotor[3], int speed) {
  digitalWrite(pinMotor[1], HIGH);
  digitalWrite(pinMotor[2], LOW);
  analogWrite(pinMotor[0], speed);
}

void moveBackward(const int pinMotor[3], int speed) {
  digitalWrite(pinMotor[1], LOW);
  digitalWrite(pinMotor[2], HIGH);
  analogWrite(pinMotor[0], speed);
}

void fullStop(const int pinMotor[3]) {
  digitalWrite(pinMotor[1], HIGH);
  digitalWrite(pinMotor[2], HIGH);
  analogWrite(pinMotor[0], 0);
}

void moveWheel(int pwm, double w, const int pinMotor[3], bool back) {
  if(pwm == 0 || ((int)w) == 0){
    fullStop(pinMotor);
  } else {
    if(back) {
      moveBackward(pinMotor, pwm);
    } else if(!back) {
      moveForward(pinMotor, pwm);
    }
  }
  //se espera un tiempo antes de cambiar  PWM
  //no se usa delay opara evitar interferir con las interruociones.
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
