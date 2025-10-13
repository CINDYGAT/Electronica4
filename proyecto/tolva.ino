#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Configura la dirección I2C de tu LCD (comúnmente 0x27 o 0x3F)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Pines del motor
const int IN1 = 7;
const int IN2 = 8;
const int ENA = 9;

// Pines de los finales de carrera
const int finIzq = 2;
const int finDer = 3;

// Pin del sensor infrarrojo
const int sensorIR = A0;

// Variable para guardar el sentido actual del motor
int sentido = 1; // 1 = derecha, -1 = izquierda

// Variables para el control del tiempo (retardo del final derecho)
unsigned long tiempoInicioRetardo = 0;
bool enRetardo = false;
bool retardoCompletado = false;

// Variable para saber si se espera la señal del sensor IR
bool esperandoIR = false;

void setup() {
  // Configuración de pines
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);

  pinMode(finIzq, INPUT_PULLUP);
  pinMode(finDer, INPUT_PULLUP);
  pinMode(sensorIR, INPUT);

  Serial.begin(9600);

  // Inicializar LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Sistema iniciado");
  lcd.setCursor(0, 1);
  lcd.print("Esperando IR LOW");
  delay(2000);
  lcd.clear();

  Serial.println("Sistema iniciado - Retardo de 5 segundos en DERECHO");
}

void mostrarLCD(String linea1, String linea2 = "") {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(linea1);
  lcd.setCursor(0, 1);
  lcd.print(linea2);
}

void loop() {
  // Leer los estados de los finales de carrera y el sensor IR
  bool izqPresionado = digitalRead(finIzq) == LOW;
  bool derPresionado = digitalRead(finDer) == LOW;
  bool irActivo = digitalRead(sensorIR) == LOW; // Sensor IR activo en BAJO

  // ---------------------------
  // LÓGICA DEL FINAL IZQUIERDO (espera IR para moverse a derecha)
  // ---------------------------
  if (izqPresionado && !esperandoIR) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);

    esperandoIR = true;
    sentido = -1;
    enRetardo = false;
    retardoCompletado = false;

    Serial.println("Final IZQUIERDO presionado - Esperando señal IR (LOW) para moverse a DERECHA...");
    mostrarLCD("Final IZQ presionado", "Esperando IR LOW");
  }

  // Si estamos esperando la señal IR y el sensor se activa (en bajo)
  if (esperandoIR && irActivo) {
    sentido = 1; // Cambiar a derecha
    esperandoIR = false;
    Serial.println("Señal IR detectada (LOW) - Iniciando movimiento a la DERECHA");
    mostrarLCD("Sensor IR LOW", "Moviendo DERECHA");
    delay(300);
  }

  // ---------------------------
  // LÓGICA DEL FINAL DERECHO (retardo 5s)
  // ---------------------------
  if (derPresionado && !enRetardo && !retardoCompletado) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    enRetardo = true;
    tiempoInicioRetardo = millis();
    Serial.println("Final DERECHO presionado - Iniciando retardo de 5 segundos...");
    mostrarLCD("Final DER presionado", "Retardo 5s...");
  }

  // Control del retardo de 5 segundos
  if (enRetardo) {
    if (millis() - tiempoInicioRetardo >= 5000) {
      sentido = -1;
      enRetardo = false;
      retardoCompletado = true;
      Serial.println("Retardo completado - Cambiando a IZQUIERDA");
      mostrarLCD("Retardo completado", "Moviendo IZQUIERDA");
    } else {
      unsigned long restante = 5000 - (millis() - tiempoInicioRetardo);
      Serial.print("Tiempo restante: ");
      Serial.print(restante / 1000);
      Serial.println(" s");
      lcd.setCursor(0, 1);
      lcd.print("Tiempo: ");
      lcd.print(restante / 1000);
      lcd.print("s  ");
    }
  }

  // Reset del retardo cuando se suelta el final derecho
  if (!derPresionado && retardoCompletado) {
    retardoCompletado = false;
    Serial.println("Final DERECHO liberado - Retardo resetado");
    mostrarLCD("Final DER liberado", "Retardo resetado");
  }

  // ---------------------------
  // CONTROL DE MOVIMIENTO DEL MOTOR
  // ---------------------------
  if (!enRetardo && !esperandoIR) {
    if (sentido == 1) {
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      if (!derPresionado) {
        Serial.println("Girando a la DERECHA");
        mostrarLCD("Abriendo Tolva", "MOV DERECHA");
      }
    } else {
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
      if (!izqPresionado) {
        Serial.println("Girando a la IZQUIERDA");
        mostrarLCD("Cerrando Tolva", "MOV IZQUIERDO");
      }
    }
  } else {
    // Si está detenido
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
  }

  analogWrite(ENA, 200);
  delay(100);
}
