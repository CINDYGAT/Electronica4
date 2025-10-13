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
  Serial.println("Sistema iniciado - Retardo de 5 segundos en DERECHO");
}

void loop() {
  // Leer los estados de los finales de carrera y el sensor IR
  bool izqPresionado = digitalRead(finIzq) == LOW;
  bool derPresionado = digitalRead(finDer) == LOW;
  bool irActivo = digitalRead(sensorIR) == LOW; // AHORA EL IR ESTÁ ACTIVO EN BAJO

  // ---------------------------
  // LÓGICA DEL FINAL IZQUIERDO (ahora hace lo que hacía el derecho)
  // ---------------------------
  if (izqPresionado && !esperandoIR) {
    // Detener el motor
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);

    // Indicar que estamos esperando señal del sensor IR
    esperandoIR = true;
    sentido = -1; // Estado actual: en extremo izquierdo
    enRetardo = false;
    retardoCompletado = false;

    Serial.println("Final IZQUIERDO presionado - Esperando señal del sensor IR (en BAJO) para moverse a DERECHA...");
  }

  // Si estamos esperando la señal IR y el sensor se activa (en bajo)
  if (esperandoIR && irActivo) {
    sentido = 1; // Cambiar el sentido hacia la derecha
    esperandoIR = false; // Ya no esperamos señal
    Serial.println("Señal IR detectada (BAJO) - Iniciando movimiento a la DERECHA");
    delay(300); // pequeña pausa para estabilidad
  }

  // ---------------------------
  // LÓGICA DEL FINAL DERECHO (ahora hace lo que hacía el izquierdo)
  // ---------------------------
  if (derPresionado && !enRetardo && !retardoCompletado) {
    // Iniciar el retardo de 5 segundos
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    enRetardo = true;
    tiempoInicioRetardo = millis();
    Serial.println("Final DERECHO presionado - Iniciando retardo de 5 segundos...");
  }

  // Control del retardo de 5 segundos
  if (enRetardo) {
    if (millis() - tiempoInicioRetardo >= 5000) {
      // Retardo completado, cambiar sentido a izquierda
      sentido = -1;
      enRetardo = false;
      retardoCompletado = true;
      Serial.println("Retardo de 5 segundos completado - Cambiando sentido a IZQUIERDA");
    } else {
      unsigned long tiempoRestante = 5000 - (millis() - tiempoInicioRetardo);
      Serial.print("Tiempo restante: ");
      Serial.print(tiempoRestante / 1000);
      Serial.println(" segundos");
    }
  }

  // Reset del retardo cuando se suelta el final derecho
  if (!derPresionado && retardoCompletado) {
    retardoCompletado = false;
    Serial.println("Final DERECHO liberado - Retardo resetado");
  }

  // ---------------------------
  // CONTROL DE MOVIMIENTO DEL MOTOR
  // ---------------------------
  if (!enRetardo && !esperandoIR) {
    if (sentido == 1) {
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      if (!derPresionado) Serial.println("Girando a la DERECHA");
    } else {
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
      if (!izqPresionado) Serial.println("Girando a la IZQUIERDA");
    }
  } else {
    // Si estamos en retardo o esperando IR, el motor se mantiene detenido
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
  }

  analogWrite(ENA, 200); // velocidad (0-255)
  delay(100);
}
