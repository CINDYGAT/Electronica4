#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Dirección I2C del módulo LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Pin del sensor IR
const int sensorIR = A0;

void setup() {
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0,0);
  lcd.print("Sensor IR listo");
  delay(1500);
  lcd.clear();
}

void loop() {
  int valor = analogRead(sensorIR);

  // ---- Calibración básica ----
  // Los sensores Sharp NO son lineales, pero a modo de ejemplo:
  // si estás usando un GP2Y0A21YK, valores ~500-600 suelen ser cerca de 5 cm.
  // Esto debes ajustarlo probando en tu prototipo.
  if (valor < 500) {  
    lcd.setCursor(0,0);
    lcd.print("Objeto detectado ");
    lcd.setCursor(0,1);
    lcd.print("Recipiente 0.25L          ");
  } else {
    lcd.setCursor(0,0);
    lcd.print("No detectado     ");
    lcd.setCursor(0,1);
    lcd.print("                "); 
  }

  delay(300);
}
