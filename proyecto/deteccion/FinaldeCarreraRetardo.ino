// Pines del motor
const int IN1 = 7;
const int IN2 = 8;
const int ENA = 9;

// Pines de los finales de carrera
const int finIzq = 2;
const int finDer = 3;

// Variable para guardar el sentido actual del motor
int sentido = 1; // 1 = derecha, -1 = izquierda

// Variables para el control del tiempo
unsigned long tiempoInicioRetardo = 0;
bool enRetardo = false;
bool retardoCompletado = false;

void setup() {
  // Configuración de pines
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);

  pinMode(finIzq, INPUT_PULLUP);
  pinMode(finDer, INPUT_PULLUP);

  Serial.begin(9600);
  Serial.println("Sistema iniciado - Retardo de 5 segundos en IZQUIERDO");
}

void loop() {
  // Leer los estados de los finales de carrera
  bool izqPresionado = digitalRead(finIzq) == LOW;
  bool derPresionado = digitalRead(finDer) == LOW;

  // Lógica del final derecho (sin retardo)
  if (derPresionado) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    sentido = 1; // Cambiar a derecha
    enRetardo = false;
    retardoCompletado = false;
    Serial.println("Final derecho presionado - Cambiando sentido a DERECHA");
    delay(500);
  }

  // Lógica del final izquierdo CON RETARDO DE 5 SEGUNDOS
  if (izqPresionado && !enRetardo && !retardoCompletado) {
    // Iniciar el retardo de 5 segundos
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    enRetardo = true;
    tiempoInicioRetardo = millis();
    Serial.println("Final izquierdo presionado - Iniciando retardo de 5 segundos...");
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
      // Mostrar tiempo restante
      unsigned long tiempoRestante = 5000 - (millis() - tiempoInicioRetardo);
      Serial.print("Tiempo restante: ");
      Serial.print(tiempoRestante / 1000);
      Serial.println(" segundos");
    }
  }

  // Reset del retardo cuando se suelta el final izquierdo
  if (!izqPresionado && retardoCompletado) {
    retardoCompletado = false;
    Serial.println("Final izquierdo liberado - Retardo resetado");
  }

  // Mover el motor según el sentido actual
  if (!enRetardo) {
    if (sentido == 1) {
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      if (!derPresionado) {
        Serial.println("Girando a la DERECHA");
      }
    } else {
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
      if (!izqPresionado) {
        Serial.println("Girando a la IZQUIERDA");
      }
    }
  }

  analogWrite(ENA, 200); // velocidad (0-255)
  delay(100);
}
