#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// === Configuración del LCD ===
// Dirección I2C típica: 0x27 o 0x3F (ajústala según tu módulo)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// === Pines del L298N ===
const int IN1 = 8;
const int IN2 = 9;
const int ENA = 10;  // PWM para velocidad (si quieres ajustar)
 
// === Tiempo de giro ===
const int tiempo = 4000; // 4 segundos

void setup() {
  // Configuración de pines
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);

  // Inicializar LCD
  lcd.init();
  lcd.backlight();

  // Mensaje de inicio
  lcd.setCursor(0,0);
  lcd.print("Motor Puente H");
  lcd.setCursor(0,1);
  lcd.print("Listo!");
  delay(2000);
  lcd.clear();
}

void loop() {
  // --- Giro en una dirección ---
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, 200); // Velocidad (0-255)

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Girando Derecha");
  delay(tiempo);

  // --- Detener motor brevemente ---
  detenerMotor();
  delay(1000);

  // --- Giro en la otra dirección ---
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  analogWrite(ENA, 200);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Girando Izquierda");
  delay(tiempo);

  // --- Detener motor ---
  detenerMotor();
  delay(1000);
}

// === Función para parar motor ===
void detenerMotor() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, 0);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Motor Detenido");
}
