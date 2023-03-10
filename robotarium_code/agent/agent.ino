// Robotarium-UCM - Agent firmware
// Copyright (C) 2022 UCM-237
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
#include <MeanFilterLib.h>

using namespace std;

// H-BRIDGE - Uncomment only one option
#define H_BRIDGE_RED 
// #define H_BRIDGE_BLACK

// -----------------------------------------------------------------------------
// PIN layout
// -----------------------------------------------------------------------------
const int pin_left_encoder          = 2;
const int pin_right_encoder         = 3;

#ifdef H_BRIDGE_BLACK
  const int pin_left_motor_direction  = 4;
  const int pin_left_motor_enable     = 5;
  const int pin_right_motor_direction = 8;
  const int pin_right_motor_enable    = 9;
#endif

#ifdef H_BRIDGE_RED
  const int pin_left_motor_dir_1   = 4;
  const int pin_left_motor_dir_2   = 5;
  const int pin_left_motor_enable  = 6;
  const int pin_right_motor_dir_1  = 7;
  const int pin_right_motor_dir_2  = 8;
  const int pin_right_motor_enable = 9;
#endif

// -----------------------------------------------------------------------------
// Motors
// -----------------------------------------------------------------------------
const int LEFT_WHEEL = 0;
const int RIGHT_WHEEL = 1;
#define FORWARD LOW
#define BACKWARD HIGH
#define MINPWM 100
#define MAXPWM 255

void set_wheel_speed(int wheel, int direction, int speed) {
#ifdef H_BRIDGE_BLACK
  int pin_direction = (wheel == LEFT_WHEEL) ? pin_left_motor_direction : pin_right_motor_direction;
  int dir = (wheel == LEFT_WHEEL) ? direction : !direction;
  digitalWrite(pin_direction, dir);
#endif

#ifdef H_BRIDGE_RED
  int pin_dir_1  = (wheel == LEFT_WHEEL) ? pin_left_motor_dir_1 : pin_right_motor_dir_2;
  int pin_dir_2  = (wheel == LEFT_WHEEL) ? pin_left_motor_dir_2 : pin_right_motor_dir_1;
  digitalWrite(pin_dir_1, (direction == FORWARD && speed != 0) ? HIGH : LOW);
  digitalWrite(pin_dir_2, (direction == BACKWARD && speed != 0) ? HIGH : LOW);
#endif

  int pin_enable = (wheel == LEFT_WHEEL) ? pin_left_motor_enable : pin_right_motor_enable;
  analogWrite(pin_enable, speed);
}

// -----------------------------------------------------------------------------
// Encoders
// -----------------------------------------------------------------------------
const int encoder_resolution_ppt = 20*2;
int filter_window_size = 10;
unsigned long encoder_count[2] = {0, 0};
MeanFilter<double> encoder_w_estimated[2] = {
  MeanFilter<double>(filter_window_size),
  MeanFilter<double>(filter_window_size)
};

void isr_left_encoder_count() {
  isr_encoder_count(LEFT_WHEEL);
}

void isr_right_encoder_count() {
  isr_encoder_count(RIGHT_WHEEL);
}

void isr_encoder_count(int pin) {
    encoder_count[pin] ++;
}

// -----------------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------------
void setup() {
  setup_motors();
  setup_encoders();
  setup_comms();
}

void setup_motors() {
  pinMode(pin_left_motor_enable, OUTPUT);
  pinMode(pin_right_motor_enable, OUTPUT);
  #ifdef H_BRIDGE_RED
    pinMode(pin_left_motor_dir_1, OUTPUT);
    pinMode(pin_left_motor_dir_2, OUTPUT);
    pinMode(pin_right_motor_dir_1, OUTPUT);
    pinMode(pin_right_motor_dir_2, OUTPUT);
  #endif
  #ifdef H_BRIDGE_BLACK
    pinMode(pin_left_motor_direction, OUTPUT);
    pinMode(pin_right_motor_direction, OUTPUT);
  #endif
  set_wheel_speed(LEFT_WHEEL, FORWARD, 0);
  set_wheel_speed(RIGHT_WHEEL, FORWARD, 0);
}

void setup_encoders() {
  pinMode(pin_left_encoder, INPUT_PULLUP);
  pinMode(pin_right_encoder, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(2), isr_left_encoder_count, CHANGE);
  attachInterrupt(digitalPinToInterrupt(3), isr_right_encoder_count, CHANGE);
}

void setup_comms() {
  Serial.begin(9600);
}
// -----------------------------------------------------------------------------
// Control Loop
// -----------------------------------------------------------------------------
const unsigned long sampling_time_us = 100000;
unsigned long last_time_us = 0;

void loop() {
  unsigned long current_time_us = micros();  
  unsigned long dt_us = current_time_us - last_time_us; 
  if(dt_us >= sampling_time_us) {
    double w_left = encoder_w_estimated[LEFT_WHEEL].AddValue(encoder_count[LEFT_WHEEL]);
    double w_right = encoder_w_estimated[RIGHT_WHEEL].AddValue(encoder_count[RIGHT_WHEEL]);
    noInterrupts();
    encoder_count[LEFT_WHEEL] = encoder_count[RIGHT_WHEEL] = 0;
    interrupts();
    update_control(w_left, w_right, dt_us*1e-6);
    last_time_us = current_time_us;
  }
}

// -----------------------------------------------------------------------------
// PID Controller
// -----------------------------------------------------------------------------
unsigned long previous_time_ms[2] = {0, 0};
double K_p[2] = {100, 100};
double K_i[2] = {75, 75};
double K_d[2] = {0.0, 0.0};
double a_ff[2] = {1.8, 1.8/(MAXPWM - MINPWM)};
double b_ff[2] = {1.8, 1.8/(MAXPWM - MINPWM)};
double setpointW[2] = {3.0, 2.5}; // setpoint in { 0 U (1.8, 3.6) } 
double setpoint_left_motor = 0;
double setpoint_right_motor = 0;
double previous_error[2] = {0, 0};
double I_prev[2] = {0, 0};

int pid_right_motor(double w) {
  return pid(RIGHT_WHEEL, w);
}

int pid_left_motor(double w) {
  return pid(LEFT_WHEEL, w);
}

int pid(int motor, double w) {
  unsigned long current_time = millis();
  double elapsed_time = current_time - previous_time_ms[motor];
  double error = setpointW[motor] - w;
  double u0 = feedforward(motor); 
  double P = K_p[motor]*error;
  double I = I_prev[motor] + K_i[motor]*error*elapsed_time*1e-3;
  double D = K_d[motor]*(error - previous_error[motor]) / elapsed_time;
  double u = u0 + P+I+D;
  double v = constrain(u, MINPWM, MAXPWM);
  if((u - v)*error <= 0) {
    I_prev[motor] = I;
  }
  previous_error[motor] = error;
  previous_time_ms[motor] = current_time;
  return static_cast<int> (round(v));
}

int feedforward(int motor) {
  return (setpointW[motor] != 0.0) ? constrain(
    round((setpointW[motor] + a_ff[motor]) / b_ff[motor]), MINPWM, MAXPWM
  ) : 0;    
}

// -----------------------------------------------------------------------------
// INSTRUCCIONES
// -----------------------------------------------------------------------------
//   Inserta tu c??digo en la funci??n "update_control". Est?? funci??n se invoca
// per????dicamente, pas??ndole informaci??n actualizada sobre el estado del robot.
//
// En ella puedes realizar llamadas a las funciones de la API para acceder a las
// funciones b??sicas del robot:
//   - set_wheel_speed(int wheel, int direction, int speed) 
//
// Si necesitas que el estado de una variable persista entre llamadas a
// update_control, deber??s utilizar una variable global. Utiliza la secci??n
// comentada unas l??neas m??s abajo. Para evitar conflictos de nombre con 
// variables del firmware, utiliza el prefijo "user_". 

// -----------------------------------------------------------------------------
// Define aqu?? tus variables globales
// -----------------------------------------------------------------------------
// (Ej.: const int user_time = 0;)

// -----------------------------------------------------------------------------
// update_control
// -----------------------------------------------------------------------------
//   Bucle de control principal. Ejecuta el c??digo peri??dicamente, con un
// peri??do nominal especificado por el valor sampling_time_us.
//
// params
// ------
//   double count_left_wheel  Incremento de cuentas de encoder de la rueda izquierda. 
//   double count_right_wheel Incremento de cuentas de encoder de la rueda derecha. 
//   double dt_s              Indica el tiempo real, en segundos, transcurrido
//                            desde la ??ltima invocaci??n.
// -----------------------------------------------------------------------------
void update_control(double count_left_wheel, double count_right_wheel, double dt_s) {
  // Pon aqu?? tu c??digo
}