/*
  ===========================================
  =====CARACTERIZADOR SENSOR DE FOTONES======
  ===========================================

  =======================
  ========NEMA 17========
  =======================
  =  Pasos= 1.8°/paso   =
  =  Pasos por Rev: 200 =
  =======================

  =========================
  =======SERVOMOTOR========
  =========================
  = °/paso = 90/muestras  =
  = Variable de estado    =
  = muestras = pasos/iter =
  =========================

  =========================
  ==========Menu===========
  =========================
  = var estados
  = Menu principal
   >Inicio >Config

   ----Inicio
   ------Error o rutinaCaracterización

   ----Configuracion
   ------ # de muestras
   ------ #iteraciones
   ------ tiempo LED

   =================================

   =============================
   ===========ENCODER===========
   =============================
   = Girar izquierda
   = Girar Derecha
   = Enter
   =============================

   =============================
   =========A4988===============
   =7===========================
*/

//------LIBRERIAS--------

#include "encoder.h"
#include<Wire.h>
#include<LiquidCrystal_I2C.h>
#include<Servo.h>

//-----------------------

//------Servo------
#define pinServo 9
Servo servoBasculante;

//-----NEMA 17-------
#define dirPin 2
#define stepPin 3
#define stepsPerRev 200

//------LCD---------
LiquidCrystal_I2C pantalla(0x27, 20, 4);

//------encoder------
encoder control(4, 5, 6);

Contador contadorMenuPrincipal (4, 5, 6, 0, 1);
Contador contadorMuestras(4, 5, 6, 0, 11);
Contador contadorIteraciones(4, 5, 6, 0, 11);
Contador contadorTiempoLED (4, 5, 6, 1, 99);

//===========================================================
//--------------------V A R I A B L E S----------------------
//===========================================================

//=====================MAQUINA DE ESTADOS MENU=======================

int estadoPrimeraCapa = 0, estadoSegundaCapa, estadoTerceraCapa;

int varConfig = LOW; //Habilita o deshabilita el inicio de la rutina

//===================================================================

//=======INTERFAZ GRAFICA MENU========

const char separator_row[] PROGMEM = "====================";
const char blank_row[] PROGMEM = "                    ";
char buffer[21];

//menuPrincipal
const char main_row_1[] PROGMEM = "   Menu Principal   ";
const char main_row_3[] PROGMEM = " >Iniciar   Config  ";
const char main_row_3_[] PROGMEM = "  Iniciar  >Config  ";

const char *const menuPrincipalString[][4] PROGMEM = {
  { main_row_1,
    separator_row,
    blank_row,
    main_row_3
  },

  { main_row_1,
    separator_row,
    blank_row,
    main_row_3_
  }
};

//mensaje Error
const char error_row_0 [] PROGMEM = "    Configura los   ";
const char error_row_1 [] PROGMEM = "    parametros de   ";
const char error_row_2 [] PROGMEM = "   operacion antes  ";
const char error_row_3 [] PROGMEM = "     de iniciar     ";

const char *const mensajeErrorString[][4] PROGMEM =
{
  { error_row_0,
    error_row_1,
    error_row_2,
    error_row_3
  }
};

//menu Configuracion

const char config_row_1 [] PROGMEM = "Cantidad de Muestras";
const char config_row_3 [] PROGMEM = "        [00]        ";
const char config_row_1_ [] PROGMEM = "Cantidad Iteraciones";
const char config_row_3_ [] PROGMEM = "       [000]        ";
const char _config_row_1_ [] PROGMEM = " Tiempo por Muestra ";


const char *const menuConfiguracionString[][4] PROGMEM = {
  { config_row_1,
    separator_row,
    blank_row,
    config_row_3
  },
  { config_row_1_,
    separator_row,
    blank_row,
    config_row_3_
  },
  { _config_row_1_,
    separator_row,
    blank_row,
    config_row_3
  }
};

//menuProgreso
const char prog_row_0 [] PROGMEM = " hrs:min      Iter  ";
const char prog_row_1 [] PROGMEM = " 000:00     000/000 ";
const char prog_row_3 [] PROGMEM = " 000%  [          ] ";

const char *const menuProgresoString[][4] PROGMEM =
{
  { prog_row_0,
    prog_row_1,
    blank_row,
    prog_row_3
  }
};

byte progreso_0[8] = {
  B00000,
  B10000,
  B10000,
  B10000,
  B10000,
  B10000,
  B00000,
  B00000
};

byte progreso_1[8] = {
  B00000,
  B11000,
  B11000,
  B11000,
  B11000,
  B11000,
  B00000,
  B00000
};

byte progreso_2[8] = {
  B00000,
  B11100,
  B11100,
  B11100,
  B11100,
  B11100,
  B00000,
  B00000
};

byte progreso_3[8] = {
  B00000,
  B11110,
  B11110,
  B11110,
  B11110,
  B11110,
  B00000,
  B00000
};

byte progreso_4[8] = {
  B00000,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B00000,
  B00000
};

char muestrasPorIter[2], cantidadIter[3], tiempoPorMuestra[2]; //Dan formato a los valores mostrados en pantalla
char tiempoEstimadoChar[6], porcentajeProgresoChar[5], iterFaltantes[8];

int posBarra = 8;

//=====================LISTA NUMEROS MUESTRAS======================

int listaMuestras[12] = {1, 2, 3, 5, 6, 9, 10, 15, 18, 30, 45, 90};
int listaIteraciones[12] = {1, 2, 4, 5, 8, 10, 20, 25, 40, 50, 100, 200};


//=================FUNCION GET TIEMPO RESTANTE=====================

double tiempoEstimadoMin;
int muestras, iteraciones_, tiempoLED; //variables donde se almacenan los valores escogidos en el menu
int minutos = 0, horas = 0;
int minutosRestantes, horasRestantes;
unsigned long tiempoIteracion, tiempoEstimadoHrs, tiempoEjecucion;

//===========RUTINA DE CARACTERIZACION===========

int iteraciones = 0;
bool estadoDir = 1;
unsigned long int timeStamp = 0;

//=================================================
//=================================================


void setup() {
  //--PANTALLA--
  pantalla.init();
  pantalla.backlight();
  pantalla.clear();

  //---Menu---
  pantalla.createChar(1, progreso_0);
  pantalla.createChar(2, progreso_1);
  pantalla.createChar(3, progreso_2);
  pantalla.createChar(4, progreso_3);
  pantalla.createChar(5, progreso_4);

  //--encoder--
  control.init();

  //--SERVO--
  servoBasculante.write(0);
  servoBasculante.attach(pinServo);

  //--Stepper--
  pinMode(dirPin, OUTPUT);
  pinMode(stepPin, OUTPUT);

  //---LED----
  pinMode(LED_BUILTIN, OUTPUT);

  //--------------------------

  mensajeBienvenida();

}

void loop() {
  estadosMenu();
  //Serial.println(muestras);Serial.println(iteraciones_);Serial.println(tiempoLED);
}

//==========================================
//------------F U N C I O N E S-------------
//==========================================

void mensajeBienvenida() {

  pantalla.setCursor(0, 1);
  pantalla.print("     Bienvenido     ");
  delay(650);
  pantalla.clear();

  pantalla.setCursor(0, 1);
  pantalla.print("     Iniciando      ");
  pantalla.setCursor(0, 2);
  pantalla.print("      Sistema       ");
  delay(1500);
  pantalla.clear();

}


void displayMenuPrincipal() {

  for (int i = 0; i <= 3; i++) {

    pantalla.setCursor(0, i);
    strcpy_P(buffer, (char *)pgm_read_word(&(menuPrincipalString[0][i])));
    pantalla.print(buffer);

  }
}


void menuPrincipal(int estado) {

  switch (estado) {
    case 0:
      pantalla.setCursor(0, 3);
      strcpy_P(buffer, (char *)pgm_read_word(&(menuPrincipalString[estado][3])));
      pantalla.print(buffer);
      break;

    case 1:
      pantalla.setCursor(0, 3);
      strcpy_P(buffer, (char *)pgm_read_word(&(menuPrincipalString[estado][3])));
      pantalla.print(buffer);
      break;

    default: break;
  }
}

void mensajeError() {

  for (int i = 0; i <= 3; i++) {

    pantalla.setCursor(0, i);
    strcpy_P(buffer, (char *)pgm_read_word(&(mensajeErrorString[0][i])));
    pantalla.print(buffer);

  }
}

void displayConfiguracion(int estado) {

  for (int i = 0; i <= 3; i++) {

    pantalla.setCursor(0, i);
    strcpy_P(buffer, (char *)pgm_read_word(&(menuConfiguracionString[estado][i])));
    pantalla.print(buffer);

  }
}

void menuConfiguracion(int estado) {

  switch (estado) {

    case 0:
      sprintf(muestrasPorIter, "%02d", listaMuestras[contadorMuestras.cuenta()]);
      pantalla.setCursor(9, 3);
      pantalla.print(muestrasPorIter);
      break;

    case 1:
      sprintf(cantidadIter, "%03d%", listaIteraciones[contadorIteraciones.cuenta()]);
      pantalla.setCursor(8, 3);
      pantalla.print(cantidadIter);
      break;

    case 2:
      sprintf(tiempoPorMuestra, "%02d", contadorTiempoLED.cuenta());
      pantalla.setCursor(9, 3);
      pantalla.print(tiempoPorMuestra);
      break;

  }

}

void displayProgreso() {

  for ( int i = 0; i <= 3; i++) {
    pantalla.setCursor(0, i);
    strcpy_P(buffer, (char *)pgm_read_word(&(menuProgresoString[0][i])));
    pantalla.print(buffer);
  }
}

void setTimeParameters() {
  tiempoEjecucion = millis();
  tiempoIteracion = (tiempoLED * muestras);
  tiempoEstimadoHrs = (tiempoIteracion * iteraciones_) / 3600;
  tiempoEstimadoMin = ((tiempoIteracion * iteraciones_) / 60) - (tiempoEstimadoHrs * 60);
}

void getTiempoRestante() {

  minutosRestantes = tiempoEstimadoMin - minutos;
  horasRestantes = tiempoEstimadoHrs - horas;


  if (minutosRestantes > 0 or horasRestantes > 0) {

    if (millis() - tiempoEjecucion >= 60000) {

      minutos++;

      if (minutos >= 60) {
        horas++;
        minutos = 0;
      }

      tiempoEjecucion = millis();
    }
  }

  if (minutosRestantes <= 0) {

    if (horasRestantes > 0) {

      tiempoEstimadoHrs--;
      tiempoEstimadoMin = 59;
    }
  }
}

void displayTiempo() {

  getTiempoRestante();

  sprintf(tiempoEstimadoChar, "%03d:%02d", horasRestantes, minutosRestantes );
  pantalla.setCursor(1, 1);
  pantalla.print(tiempoEstimadoChar);

}

void displayIteraciones() {

  sprintf(iterFaltantes, "%03d/%03d", iteraciones, iteraciones_);
  pantalla.setCursor(12, 1);
  pantalla.print(iterFaltantes);

}

int getPorcentaje() {
  double iter = iteraciones;
  double iter_ = iteraciones_;

  double porcentaje = (iter / iter_) * 100;
  return int(porcentaje);
}

void displayPorcentajeProgreso() {

  sprintf(porcentajeProgresoChar, "%03d", getPorcentaje());
  pantalla.setCursor(1, 3);
  pantalla.print(porcentajeProgresoChar);
}

void buildProgressBar() {

  float cantidadBarrasTotal = float(getPorcentaje()) / 2;
  float cantidadBarrasFracc = fmod(cantidadBarrasTotal, 5.0);
  float cantidadRecuadros = (cantidadBarrasTotal - cantidadBarrasFracc) / 5;
  int cantidadBarrasRest = int(cantidadBarrasFracc);

  pantalla.setCursor(8, 3);


  if (cantidadRecuadros > 0) {
    for (int i = 0; i < cantidadRecuadros; i++) {
      pantalla.write(5);
    }
  }
  if (cantidadBarrasRest > 0 and cantidadBarrasRest < 5) {
    pantalla.write(cantidadBarrasRest);
  }

}

void mensajeFinalizacion() {
  pantalla.clear();
  pantalla.setCursor(0, 1);
  pantalla.print(" Rutina  Finalizada ");
  delay(1000);
}

void estadosDefault(bool primera, bool segunda, bool tercera) {

  if (primera) {
    estadoPrimeraCapa = 100;
  }

  if (segunda) {
    estadoSegundaCapa = 100;
  }

  if (tercera) {
    estadoTerceraCapa = 100;
  }

}

void estadosMenu() {

  switch (estadoPrimeraCapa) {
    case 0:
      displayMenuPrincipal();
      do {
        menuPrincipal(contadorMenuPrincipal.cuenta());

      } while ( !control.switchWasPressed() );

      estadoSegundaCapa = contadorMenuPrincipal.cuenta();
      break;


    case 1:

      setTimeParameters();
      displayProgreso();

      do {
        displayTiempo();
        displayIteraciones();
        rutinaCaracterizacion();
      } while (iteraciones < iteraciones_);

      servoBasculante.write(0);
      backStepper();
      mensajeFinalizacion();
      varConfig = false;
      iteraciones_ = 0;
      iteraciones = 0;
      tiempoLED = 0;
      muestras = 0;

      estadoPrimeraCapa = 0;



      break;

    default: break;
  }

  switch (estadoSegundaCapa) {
    case 0:
      if (!varConfig) {

        mensajeError();
        delay(2500);

        estadoPrimeraCapa = 0;
        estadosDefault(0, 0, 1);
      }

      else {

        estadoPrimeraCapa = 1;
        estadosDefault(0, 1, 1);
      }
      break;

    case 1:
      estadoTerceraCapa = 0;
      break;

    default: break;
  }


  switch (estadoTerceraCapa) {
    case 0:
      displayConfiguracion(estadoTerceraCapa);

      do {
        menuConfiguracion(estadoTerceraCapa);
      } while (!control.switchWasPressed());

      muestras = listaMuestras[contadorMuestras.cuenta()];
      estadosDefault(1, 1, 0);
      estadoTerceraCapa = 1;
      break;

    case 1:
      displayConfiguracion(estadoTerceraCapa);

      do {
        menuConfiguracion(estadoTerceraCapa);
      } while (!control.switchWasPressed());

      iteraciones_ = listaIteraciones[contadorIteraciones.cuenta()];
      estadoTerceraCapa = 2;

      break;

    case 2:
      displayConfiguracion(estadoTerceraCapa);

      do {
        menuConfiguracion(estadoTerceraCapa);
      } while (!control.switchWasPressed());

      tiempoLED = contadorTiempoLED.cuenta();
      varConfig = true;
      estadoPrimeraCapa = 0;
      break;

    default: break;
  }
}

bool stepState = HIGH;

void moveStepper() {
  //unsigned long TimeStamp = micros();
  //unsigned long TimeStamp2;
  digitalWrite(dirPin, HIGH);

  for (int s = 0; s <= (stepsPerRev / iteraciones_); s ++) {
    /*if ((micros() - TimeStamp) >= 8000) {
      stepState = !stepState;
      TimeStamp = micros();
      }
      digitalWrite(stepPin, stepState);*/

    digitalWrite(stepPin, HIGH);
    delayMicroseconds(8000);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(8000);
  }
}

void backStepper() {
  //unsigned long TimeStamp = millis();
  digitalWrite(dirPin, LOW);
  for (int s = 0; s <= 200; s ++) {

    /*if ((millis() - TimeStamp) >= 8) {
      stepState = !stepState;
      timeStamp = millis();
      }
      digitalWrite(stepPin, stepState);*/

    digitalWrite(stepPin, HIGH);
    delayMicroseconds(8000);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(8000);
  }
}

void encenderLed(int seg) {
  timeStamp = millis();
  do {
    digitalWrite(13, true);
  } while ((millis() - timeStamp) <= (seg * 1000));
  digitalWrite(13, false);
  //timeStamp = millis();
}

void moveServo(bool dir) {
  unsigned long timeStamp = millis();
  switch (dir) {

    case 0:
      for (int p = 90; p >= 0; p -= 90 / muestras) {
        servoBasculante.write(p);
        //delay(200);
        encenderLed(tiempoLED);
      }
      break;

    case 1:
      for (int p = 0; p <= 90; p += 90 / muestras) {
        servoBasculante.write(p);
        //delay(200);
        encenderLed(tiempoLED);
      }
      break;
  }
}

void rutinaCaracterizacion() {
  moveServo(estadoDir);
  moveStepper();
  estadoDir = !estadoDir;
  iteraciones++;

  displayPorcentajeProgreso();
  buildProgressBar();
}
