#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------------- Pines del motor ----------------
const int IN1 = 7;
const int IN2 = 8;
const int ENA = 9;

// ---------------- Pines finales de carrera ----------------
const int finIzq = 2;
const int finDer = 3;

// ---------------- Pines sensores IR ----------------
const int sensorAlto = A0;   // 1L
const int sensorMedio = A1;  // 0.5L
const int sensorBajo = A3;   // 0.25L

// ---------------- Variables ----------------
const int UMBRAL_DETECCION = 500;
int sentido = 1;  // 1 = derecha, -1 = izquierda

unsigned long tiempoInicioRetardo = 0;
bool enRetardo = false;
bool retardoCompletado = false;
bool esperandoRecipiente = false;

// ---------------- Setup ----------------
void setup() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);

  pinMode(finIzq, INPUT_PULLUP);
  pinMode(finDer, INPUT_PULLUP);
  pinMode(sensorAlto, INPUT);
  pinMode(sensorMedio, INPUT);
  pinMode(sensorBajo, INPUT);

  Serial.begin(9600);
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Sistema iniciado");
  lcd.setCursor(0, 1);
  lcd.print("Esperando IR...");
  delay(2000);
  lcd.clear();
}

void mostrarLCD(String linea1, String linea2 = "") {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(linea1);
  lcd.setCursor(0, 1);
  lcd.print(linea2);
}

// ---------------- Loop ----------------
void loop() {
  bool izqPresionado = digitalRead(finIzq) == LOW;
  bool derPresionado = digitalRead(finDer) == LOW;

  int valAlto = analogRead(sensorAlto);
  int valMedio = analogRead(sensorMedio);
  int valBajo = analogRead(sensorBajo);

  bool alto = (valAlto < UMBRAL_DETECCION);
  bool medio = (valMedio < UMBRAL_DETECCION);
  bool bajo = (valBajo < UMBRAL_DETECCION);

  String tipo = "Sin recipiente";
  int tipoRecipiente = 0; // 0 = ninguno, 1 = pequeño, 2 = mediano, 3 = grande

  if (bajo && !medio && !alto) {
    tipo = "Recipiente pequeno";
    tipoRecipiente = 1;
  } 
  else if (bajo && medio && !alto) {
    tipo = "Recipiente mediano";
    tipoRecipiente = 2;
  } 
  else if (bajo && medio && alto) {
    tipo = "Recipiente grande";
    tipoRecipiente = 3;
  }

  // Mostrar lectura en serial
  Serial.print("Tipo: "); Serial.print(tipo);
  Serial.print(" | Alto: "); Serial.print(valAlto);
  Serial.print(" Medio: "); Serial.print(valMedio);
  Serial.print(" Bajo: "); Serial.println(valBajo);

  // ---------------- FINAL IZQUIERDO ----------------
  if (izqPresionado && !esperandoRecipiente) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    esperandoRecipiente = true;
    enRetardo = false;
    retardoCompletado = false;
    mostrarLCD("Final IZQ", "Esperando IR...");
    Serial.println("Final IZQ presionado - Esperando recipiente...");
  }

  // ---------------- FINAL DERECHO ----------------
  if (derPresionado && !enRetardo && !retardoCompletado) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    enRetardo = true;
    tiempoInicioRetardo = millis();
    mostrarLCD("Final DER", "Retardo 5s...");
    Serial.println("Final DER presionado - Retardo iniciado...");
  }

  // ---------------- CONTROL RETARDO ----------------
  if (enRetardo) {
    if (millis() - tiempoInicioRetardo >= 5000) {
      sentido = -1;
      enRetardo = false;
      retardoCompletado = true;
      mostrarLCD("Retardo", "Moviendo IZQ");
      Serial.println("Retardo completado - Girando IZQ");
    } else {
      unsigned long restante = 5000 - (millis() - tiempoInicioRetardo);
      lcd.setCursor(0, 1);
      lcd.print("Tiempo: ");
      lcd.print(restante / 1000);
      lcd.print("s ");
    }
  }

  // ---------------- CASOS DE RECIPIENTES ----------------
  if (esperandoRecipiente) {
    if (tipoRecipiente == 1) {
      // Recipiente pequeño
      sentido = 1;
      esperandoRecipiente = false;
      mostrarLCD("Pequeno detectado", "Moviendo DERECHA");
      Serial.println("IR pequeño activado - Mueve DERECHA");
      delay(300);
    }
    else if (tipoRecipiente == 2) {
      // Recipiente mediano
      sentido = 1;
      esperandoRecipiente = false;
      mostrarLCD("Mediano detectado", "Moviendo DERECHA");
      Serial.println("IR mediano activado - Mueve DERECHA");
      delay(300);
    }
    else if (tipoRecipiente == 3) {
      // Recipiente grande
      sentido = 1;
      esperandoRecipiente = false;
      mostrarLCD("Grande detectado", "Moviendo DERECHA");
      Serial.println("IR grande activado - Mueve DERECHA");
      delay(300);
    }
  }

  // ---------------- CONTROL MOTOR ----------------
  if (!enRetardo && !esperandoRecipiente) {
    if (sentido == 1 && !derPresionado) {
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      mostrarLCD("Girando DERECHA", tipo);
    } 
    else if (sentido == -1 && !izqPresionado) {
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
      mostrarLCD("Girando IZQUIERDA", tipo);
    } 
    else {
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
    }
  }

  analogWrite(ENA, 200);
  delay(200);
}
