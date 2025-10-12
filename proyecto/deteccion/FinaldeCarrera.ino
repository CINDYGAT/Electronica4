// Pines del motor
const int IN1 = 7;
const int IN2 = 8;
const int ENA = 9;

// Pines de los finales de carrera
const int finIzq = 2;
const int finDer = 3;

// Variable para guardar el sentido actual del motor
int sentido = 1; // 1 = derecha, -1 = izquierda

void setup() {
  // Configuración de pines
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);

  pinMode(finIzq, INPUT_PULLUP);
  pinMode(finDer, INPUT_PULLUP);

  Serial.begin(9600);
}

void loop() {
  // Leer los estados de los finales de carrera
  bool izqPresionado = digitalRead(finIzq) == LOW;
  bool derPresionado = digitalRead(finDer) == LOW;

  // Si se presiona algún final de carrera, DETENER el motor
  if (izqPresionado || derPresionado) {
    // Detener motor
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    
    // Cambiar sentido según qué final se presionó
    if (izqPresionado) {
      sentido = 1; // Cambiar a derecha
      Serial.println("Final izquierdo - Cambiando sentido a DERECHA");
    }
    if (derPresionado) {
      sentido = -1; // Cambiar a izquierda
      Serial.println("Final derecho - Cambiando sentido a IZQUIERDA");
    }
    
    delay(500); // Esperar a que se suelte el final de carrera
  } else {
    // Si no hay final de carrera presionado, mover el motor
    if (sentido == 1) {
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      Serial.println("Girando a la DERECHA");
    } else {
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
      Serial.println("Girando a la IZQUIERDA");
    }
  }

  analogWrite(ENA, 200); // velocidad (0-255)
  delay(100); // Pequeño retardo para estabilidad
}
