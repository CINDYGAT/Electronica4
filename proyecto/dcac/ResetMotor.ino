// Pines de control del L298N
const int IN1 = 7;  // Dirección 1
const int IN2 = 8;  // Dirección 2
const int ENA = 9;  // PWM para velocidad

// Tiempo de espera entre cambios de dirección (en milisegundos)
const unsigned long intervalo = 4000;

void setup() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);

  // Opcional: iniciar comunicación serial
  Serial.begin(9600);
}

void loop() {
  // Girar a la derecha
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, 200);  // Velocidad (0–255)
  Serial.println("Girando a la DERECHA");
  delay(intervalo);

  // Girar a la izquierda
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  analogWrite(ENA, 200);  // Velocidad constante
  Serial.println("Girando a la IZQUIERDA");
  delay(intervalo);
}
