#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Configuración LCD - Si 0x27 no funciona, prueba con 0x3F
LiquidCrystal_I2C lcd(0x27, 16, 2); // Cambia a 0x3F si es necesario

// Pines del motor
const int IN1 = 7;
const int IN2 = 8;
const int ENA = 9;

// Pines de los finales de carrera
const int finIzq = 2;
const int finDer = 3;

// Pines para los 3 sensores IR
const int sensorAlto = A0;    // Para 1L
const int sensorMedio = A1;   // Para 0.5L  
const int sensorBajo = A3;    // Para 0.25L

// Umbral para detección (ajustar según calibración)
const int UMBRAL_DETECCION = 500;

// Variable para guardar el sentido actual del motor
int sentido = 1; // 1 = derecha, -1 = izquierda

// Variables para el control del tiempo
unsigned long tiempoInicioRetardo = 0;
bool enRetardo = false;
bool retardoCompletado = false;

// Variables para el control de recipientes
bool esperandoRecipiente = false;
int recipienteSeleccionado = 0; // 0=none, 250=pequeño, 500=mediano, 1000=grande
bool recipienteDetectado = false;
unsigned long tiempoRetardoRecipiente = 0; // tiempo de espera según el tipo

// DECLARACIÓN DE FUNCIONES
void mostrarLCD(String linea1, String linea2);
void detectarRecipiente();

void setup() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(finIzq, INPUT_PULLUP);
  pinMode(finDer, INPUT_PULLUP);

  Serial.begin(9600);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  
  mostrarLCD("Sistema iniciado", "Calibrando...");
  delay(2000);
  mostrarLCD("Esperando", "Final Izquierdo");
}

// Función para mostrar texto en LCD
void mostrarLCD(String linea1, String linea2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(linea1);
  if (linea2 != "") {
    lcd.setCursor(0, 1);
    lcd.print(linea2);
  }
}

// Función para detectar el tipo de recipiente
void detectarRecipiente() {
  int valorAlto = analogRead(sensorAlto);
  int valorMedio = analogRead(sensorMedio);
  int valorBajo = analogRead(sensorBajo);
  
  bool detectaAlto = (valorAlto < UMBRAL_DETECCION);
  bool detectaMedio = (valorMedio < UMBRAL_DETECCION);
  bool detectaBajo = (valorBajo < UMBRAL_DETECCION);

  if (detectaBajo && !detectaMedio && !detectaAlto) {
    recipienteSeleccionado = 250;
    tiempoRetardoRecipiente = 300;   // 0.2 segundos
    recipienteDetectado = true;
    Serial.println("RECIPIENTE PEQUEÑO detectado (250ml)");
    mostrarLCD("Recipiente", "PEQUEÑO 250ml");
  } 
  else if (detectaBajo && detectaMedio && !detectaAlto) {
    recipienteSeleccionado = 500;
    tiempoRetardoRecipiente = 800;  // 0.8 segundo
    recipienteDetectado = true;
    Serial.println("RECIPIENTE MEDIANO detectado (500ml)");
    mostrarLCD("Recipiente", "MEDIANO 500ml");
  } 
  else if (detectaBajo && detectaMedio && detectaAlto) {
    recipienteSeleccionado = 1000;
    tiempoRetardoRecipiente = 1500;  // 1.5 segundos
    recipienteDetectado = true;
    Serial.println("RECIPIENTE GRANDE detectado (1000ml)");
    mostrarLCD("Recipiente", "GRANDE 1000ml");
  }
  else {
    recipienteDetectado = false;
    if (esperandoRecipiente) {
      mostrarLCD("Esperando:", "Coloque recipiente");
    }
  }
}

void loop() {
  bool izqPresionado = digitalRead(finIzq) == LOW;
  bool derPresionado = digitalRead(finDer) == LOW;

  // ---------------------------
  // FINAL IZQUIERDO: espera recipiente
  // ---------------------------
  if (izqPresionado && !esperandoRecipiente) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    esperandoRecipiente = true;
    recipienteDetectado = false;
    recipienteSeleccionado = 0;
    sentido = -1;
    
    Serial.println("Final IZQUIERDO presionado - Esperando detección de recipiente...");
    mostrarLCD("Esperando:", "Coloque recipiente");
    delay(500);
  }

  if (esperandoRecipiente) {
    detectarRecipiente();
    if (recipienteDetectado) {
      sentido = 1;
      esperandoRecipiente = false;
      
      switch(recipienteSeleccionado) {
        case 250:
          Serial.println("INICIANDO LLENADO - PEQUEÑO (0.5s)");
          break;
        case 500:
          Serial.println("INICIANDO LLENADO - MEDIANO (1s)");
          break;
        case 1000:
          Serial.println("INICIANDO LLENADO - GRANDE (2s)");
          break;
      }
      delay(300);
    }
  }

  // ---------------------------
  // FINAL DERECHO: retardo dependiente del recipiente
  // ---------------------------
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

  // Control del retardo personalizado
  if (enRetardo) {
    if (millis() - tiempoInicioRetardo >= tiempoRetardoRecipiente) {
      sentido = -1;
      enRetardo = false;
      retardoCompletado = true;
      Serial.println("Retardo completado - Cambiando a IZQUIERDA");
      mostrarLCD("Retardo completado", "Moviendo IZQUIERDA");
    }
  }

  // Reset del retardo cuando se libera el final derecho
  if (!derPresionado && retardoCompletado) {
    retardoCompletado = false;
    Serial.println("Final DERECHO liberado - Sistema listo");
    mostrarLCD("Sistema listo", "Esperando...");
  }

  // ---------------------------
  // CONTROL DEL MOTOR
  // ---------------------------
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
        mostrarLCD("Cerrando tolva", "Mov IZQUIERDO");
      }
    }
  } else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
  }

  analogWrite(ENA, 225);
  delay(100);
}
