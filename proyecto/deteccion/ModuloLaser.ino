// Módulo Láser KY-008 con Arduino UNO
const int laserPin = 9;  // Pin digital para controlar el láser

void setup() {
  pinMode(laserPin, OUTPUT);
  Serial.begin(9600);
  
  Serial.println("KY-008 Laser Module");
  Serial.println("Listo para usar");
  
  // Apagar láser al inicio (seguridad)
  digitalWrite(laserPin, LOW);
}

void loop() {
  // Ejemplo 1: Láser continuo
  Serial.println("Laser ON - Continuo");
  digitalWrite(laserPin, HIGH);
  delay(3000);  // Encendido por 3 segundos
  
  Serial.println("Laser OFF");
  digitalWrite(laserPin, LOW);
  delay(1000);
  
  // Ejemplo 2: Láser intermitente
  Serial.println("Modo intermitente...");
  for(int i = 0; i < 10; i++) {
    digitalWrite(laserPin, HIGH);
    delay(200);  // 200ms ON
    digitalWrite(laserPin, LOW);
    delay(200);  // 200ms OFF
  }
  
  delay(2000);
}
