#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------------- LCD ----------------
LiquidCrystal_I2C lcd(0x27, 16, 2); // Cambia a 0x3F si es necesario

// ---------------- MOTOR ----------------
const int IN1 = 7;
const int IN2 = 8;
const int ENA = 9;

// ---------------- FINALES DE CARRERA ----------------
const int finIzq = 2;
const int finDer = 3;

// ---------------- SENSORES IR ----------------
const int sensorAlto = A0;  // Para 1L
const int sensorMedio = A1; // Para 0.5L
const int sensorBajo = A3;  // Para 0.25L
const int UMBRAL_DETECCION = 500;

// ---------------- FOCO ----------------
const int pinFoco = 5;  // MOC3021
bool focoEncendido = true;
unsigned long tiempoInicioFoco = 0;
unsigned long duracionApagado = 0;
bool enApagado = false;

// ---------------- VARIABLES GLOBALES ----------------
int sentido = 1; // 1 = derecha, -1 = izquierda
unsigned long tiempoInicioRetardo = 0;
bool enRetardo = false;
bool retardoCompletado = false;

bool esperandoRecipiente = false;
bool recipienteDetectado = false;
int recipienteSeleccionado = 0; // 0=none, 250=pequeño, 500=mediano, 1000=grande
unsigned long tiempoRetardoRecipiente = 0;

// ---------------- FUNCIONES ----------------
void mostrarLCD(String linea1, String linea2);
void detectarRecipiente();
void controlarFoco();

void setup() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(finIzq, INPUT_PULLUP);
  pinMode(finDer, INPUT_PULLUP);
  pinMode(pinFoco, OUTPUT);

  // Iniciar foco encendido
  digitalWrite(pinFoco, HIGH);
  focoEncendido = true;

  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  lcd.clear();

  mostrarLCD("Sistema iniciado", "Calibrando...");
  delay(2000);
  mostrarLCD("Esperando", "Final Izquierdo");
}

// ---------------- FUNCIONES AUXILIARES ----------------

void mostrarLCD(String linea1, String linea2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(linea1);
  if (linea2 != "") {
    lcd.setCursor(0, 1);
    lcd.print(linea2);
  }
}

// Detectar tipo de recipiente y ajustar tiempos
void detectarRecipiente() {
  int valorAlto = analogRead(sensorAlto);
  int valorMedio = analogRead(sensorMedio);
  int valorBajo = analogRead(sensorBajo);
  
  bool detectaAlto = (valorAlto < UMBRAL_DETECCION);
  bool detectaMedio = (valorMedio < UMBRAL_DETECCION);
  bool detectaBajo = (valorBajo < UMBRAL_DETECCION);

  if (detectaBajo && !detectaMedio && !detectaAlto) {
    recipienteSeleccionado = 250;
    tiempoRetardoRecipiente = 300; // 0.3 s para motor
    duracionApagado = 4000;         // 4 s para foco
    recipienteDetectado = true;
    Serial.println("Recipiente PEQUEÑO detectado (250 ml)");
    mostrarLCD("Recipiente", "PEQUEÑO 250ml");
  } 
  else if (detectaBajo && detectaMedio && !detectaAlto) {
    recipienteSeleccionado = 500;
    tiempoRetardoRecipiente = 5000; // 0.8 s para motor
    duracionApagado = 4000;         // 0.8 s para foco
    recipienteDetectado = true;
    Serial.println("Recipiente MEDIANO detectado (500 ml)");
    mostrarLCD("Recipiente", "MEDIANO 500ml");
  } 
  else if (detectaBajo && detectaMedio && detectaAlto) {
    recipienteSeleccionado = 1000;
    tiempoRetardoRecipiente = 1500; // 1.5 s para motor
    duracionApagado = 6000;         // 1.5 s para foco
    recipienteDetectado = true;
    Serial.println("Recipiente GRANDE detectado (1000 ml)");
    mostrarLCD("Recipiente", "GRANDE 1000ml");
  } 
  else {
    recipienteDetectado = false;
    if (esperandoRecipiente) {
      mostrarLCD("Esperando:", "Coloque recipiente");
    }
  }
}

// Control del foco según el recipiente
// Control del foco según el recipiente
void controlarFoco() {
  // Si el foco está encendido y se detectó un recipiente → apagarlo temporalmente
  if (!enApagado && recipienteDetectado) {
    recipienteDetectado = false; // solo ejecutar una vez por detección

    Serial.println("Foco: Esperando 1 segundo antes de apagarse...");
    delay(2000); // espera antes de apagar

    Serial.print("Apagando foco por ");
    Serial.print(duracionApagado);
    Serial.println(" ms...");
    digitalWrite(pinFoco, LOW);
    focoEncendido = false;

    tiempoInicioFoco = millis(); // guarda cuándo se apagó
    enApagado = true;
  }

  // Si el foco está apagado y ya pasó el tiempo → volver a encender
  if (enApagado && (millis() - tiempoInicioFoco >= duracionApagado)) {
    Serial.println("Reencendiendo foco...");
    digitalWrite(pinFoco, HIGH);
    focoEncendido = true;
    enApagado = false;

    // 🔁 volver a estado de espera para el siguiente recipiente
    esperandoRecipiente = true;
    mostrarLCD("Esperando:", "Coloque recipiente");
  }
}

void loop() {
  bool izqPresionado = digitalRead(finIzq) == LOW;
  bool derPresionado = digitalRead(finDer) == LOW;

  // FINAL IZQUIERDO
  if (izqPresionado && !esperandoRecipiente) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    esperandoRecipiente = true;
    recipienteDetectado = false;
    recipienteSeleccionado = 0;
    sentido = -1;

    Serial.println("Final IZQUIERDO presionado - Esperando recipiente...");
    mostrarLCD("Esperando:", "Coloque recipiente");
    delay(500);
  }

if (esperandoRecipiente) {
  detectarRecipiente();
  if (recipienteSeleccionado > 0) {
    sentido = 1;
    esperandoRecipiente = false;

    switch (recipienteSeleccionado) {
      case 250: Serial.println("Iniciando llenado PEQUEÑO..."); break;
      case 500: Serial.println("Iniciando llenado MEDIANO..."); break;
      case 1000: Serial.println("Iniciando llenado GRANDE..."); break;
    }
  }
}

// 👇 mover esto fuera del if para que se ejecute siempre
controlarFoco(); // controla encendido/apagado del foco según el tiempo


  // FINAL DERECHO
  if (derPresionado && !enRetardo && !retardoCompletado) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    enRetardo = true;
    tiempoInicioRetardo = millis();
    esperandoRecipiente = false;

    Serial.print("Final DERECHO presionado - Retardo de ");
    Serial.print(tiempoRetardoRecipiente);
    Serial.println(" ms...");
    mostrarLCD("Final DER presionado", "Esperando...");
  }

  if (enRetardo) {
    if (millis() - tiempoInicioRetardo >= tiempoRetardoRecipiente) {
      sentido = -1;
      enRetardo = false;
      retardoCompletado = true;
      Serial.println("Retardo completado - Cambiando a IZQUIERDA");
      mostrarLCD("Retardo completado", "Mov. IZQUIERDA");
    }
  }

  if (!derPresionado && retardoCompletado) {
    retardoCompletado = false;
    Serial.println("Final DERECHO liberado - Sistema listo");
    mostrarLCD("Sistema listo", "Esperando...");
  }

  // CONTROL MOTOR
  if (!enRetardo && !esperandoRecipiente) {
    if (sentido == 1) {
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      if (!derPresionado) {
        Serial.println("Girando DERECHA - LLENANDO");
        mostrarLCD("Llenando:", String(recipienteSeleccionado) + "ml");
      }
    } else {
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
      if (!izqPresionado) {
        Serial.println("Girando IZQUIERDA - CERRANDO");
        mostrarLCD("Cerrando tolva", "Mov. IZQUIERDO");
      }
    }
  } else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
  }

  analogWrite(ENA, 225);
  delay(100);
}
