#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------------- LCD ----------------
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------------- MOTOR DC (TOLVA) ----------------
const int IN1 = 7;
const int IN2 = 8;
const int ENA = 9;

// ---------------- FINALES DE CARRERA ----------------
const int finIzq = 2;
const int finDer = 3;

// ---------------- SENSORES IR ----------------
const int sensorAlto = A0;  
const int sensorMedio = A1; 
const int sensorBajo = A3;  
const int UMBRAL_DETECCION = 500;

// ---------------- MOTOR AC ----------------
const int pinFoco = 5;  
bool focoEncendido = true;
unsigned long tiempoInicioFoco = 0;
unsigned long duracionApagado = 0;
bool enApagado = false;

// ---------------- VARIABLES GLOBALES ----------------
int sentido = 1; 
unsigned long tiempoInicioRetardo = 0;
bool enRetardo = false;
bool retardoCompletado = false;

bool esperandoRecipiente = false;
bool recipienteDetectado = false;
int recipienteSeleccionado = 0;
unsigned long tiempoRetardoRecipiente = 0;

bool sistemaInicializado = false;

// --- NUEVAS VARIABLES DE BLOQUEO ---
bool bloqueoSensores = false;
unsigned long tiempoInicioBloqueo = 0;
const unsigned long TIEMPO_BLOQUEO_SENSORES = 30000; // 30 segundos

// ---------------- FUNCIONES ----------------
void mostrarLCD(String linea1, String linea2);
void detectarRecipiente();
void controlarFoco();
void rutinaInicializacion();

void setup() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(finIzq, INPUT_PULLUP);
  pinMode(finDer, INPUT_PULLUP);
  pinMode(pinFoco, OUTPUT);

  digitalWrite(pinFoco, HIGH);
  focoEncendido = true;

  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  lcd.clear();

  mostrarLCD("Sistema iniciado", "Inicializando...");
  rutinaInicializacion();
  sistemaInicializado = true;

  mostrarLCD("Sistema listo", "Esperando...");
  Serial.println("Sistema inicializado y listo");
}

void rutinaInicializacion() {
  Serial.println("Iniciando rutina de inicialización...");
  mostrarLCD("Inicializando", "motor...");
  
  bool izqPresionado = digitalRead(finIzq) == LOW;
  bool derPresionado = digitalRead(finDer) == LOW;

  if (izqPresionado) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    analogWrite(ENA, 225);
    unsigned long inicioMovimiento = millis();
    while (digitalRead(finDer) != LOW) {
      if (millis() - inicioMovimiento > 10000) break;
      delay(50);
    }
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
  } 
  else if (!derPresionado) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    analogWrite(ENA, 225);
    unsigned long inicioBusqueda = millis();
    while (digitalRead(finDer) != LOW && digitalRead(finIzq) != LOW) {
      if (millis() - inicioBusqueda > 8000) break;
      delay(50);
    }
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
  }

  sentido = -1;
  esperandoRecipiente = true;
  Serial.println("Inicialización completada");
}

void mostrarLCD(String linea1, String linea2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(linea1);
  if (linea2 != "") {
    lcd.setCursor(0, 1);
    lcd.print(linea2);
  }
}

// ---------------- DETECCIÓN DE RECIPIENTE ----------------
void detectarRecipiente() {
  // Si el sistema está bloqueado, no leer sensores
  if (bloqueoSensores) {
    if (millis() - tiempoInicioBloqueo >= TIEMPO_BLOQUEO_SENSORES) {
      bloqueoSensores = false;
      Serial.println("Bloqueo sensores finalizado. Sensores activos nuevamente.");
    } else {
      return; // Ignora lecturas mientras esté bloqueado
    }
  }

  int valorAlto = analogRead(sensorAlto);
  int valorMedio = analogRead(sensorMedio);
  int valorBajo = analogRead(sensorBajo);

  bool detectaAlto = (valorAlto < UMBRAL_DETECCION);
  bool detectaMedio = (valorMedio < UMBRAL_DETECCION);
  bool detectaBajo = (valorBajo < UMBRAL_DETECCION);

  if (detectaBajo && !detectaMedio && !detectaAlto) {
    recipienteSeleccionado = 250;
    tiempoRetardoRecipiente = 500;
    duracionApagado = 3000;
    recipienteDetectado = true;
    Serial.println("Recipiente PEQUEÑO detectado (250 ml)");
    mostrarLCD("Recipiente", "PEQUEÑO 250ml");
  } 
  else if (detectaBajo && detectaMedio && !detectaAlto) {
    recipienteSeleccionado = 500;
    tiempoRetardoRecipiente = 1200;
    duracionApagado = 5000;
    recipienteDetectado = true;
    Serial.println("Recipiente MEDIANO detectado (500 ml)");
    mostrarLCD("Recipiente", "MEDIANO 500ml");
  } 
  else if (detectaBajo && detectaMedio && detectaAlto) {
    recipienteSeleccionado = 1000;
    tiempoRetardoRecipiente = 1900;
    duracionApagado = 6000;
    recipienteDetectado = true;
    Serial.println("Recipiente GRANDE detectado (1000 ml)");
    mostrarLCD("Recipiente", "GRANDE 1000ml");
  } 
  else {
    recipienteDetectado = false;
    if (esperandoRecipiente)
      mostrarLCD("Esperando:", "Coloque recipiente");
  }

  // ✅ Si se detectó un recipiente, activar el bloqueo de sensores
  if (recipienteDetectado) {
    bloqueoSensores = true;
    tiempoInicioBloqueo = millis();
    Serial.println("Sensores IR bloqueados durante 30 segundos.");
  }
}

// ---------------- CONTROL DEL MOTOR AC ----------------
void controlarFoco() {
  if (!enApagado && recipienteDetectado && sistemaInicializado) {
    recipienteDetectado = false;
    Serial.println("Foco: Esperando antes de apagarse...");
    delay(9400);
    Serial.println("Apagando foco...");
    digitalWrite(pinFoco, LOW);
    focoEncendido = false;
    tiempoInicioFoco = millis();
    enApagado = true;
  }

  if (enApagado && (millis() - tiempoInicioFoco >= duracionApagado)) {
    Serial.println("Reencendiendo foco...");
    digitalWrite(pinFoco, HIGH);
    focoEncendido = true;
    enApagado = false;
  }
}

// ---------------- LOOP PRINCIPAL ----------------
void loop() {
  bool izqPresionado = digitalRead(finIzq) == LOW;
  bool derPresionado = digitalRead(finDer) == LOW;

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

  controlarFoco();

  if (derPresionado && !enRetardo && !retardoCompletado) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    enRetardo = true;
    tiempoInicioRetardo = millis();
    esperandoRecipiente = false;
  }

  if (enRetardo && (millis() - tiempoInicioRetardo >= tiempoRetardoRecipiente)) {
    sentido = -1;
    enRetardo = false;
    retardoCompletado = true;
  }

  if (!derPresionado && retardoCompletado) {
    retardoCompletado = false;
  }

  if (!enRetardo && !esperandoRecipiente) {
    if (sentido == 1) {
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      if (!derPresionado)
        mostrarLCD("Llenando:", String(recipienteSeleccionado) + "ml");
    } else {
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
      if (!izqPresionado)
        mostrarLCD("Cerrando tolva", "Mov. IZQUIERDO");
    }
  } else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
  }

  analogWrite(ENA, 225);
  delay(100);
}
