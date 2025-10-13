#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Configuración LCD - Si 0x27 no funciona, prueba con 0x3F
LiquidCrystal_I2C lcd(0x27, 16, 2); // Cambia a 0x3F si es necesario

// Pines del motor
const int IN1 = 7;
const int IN2 = 8;
const int ENA = 9;

// Pines de los finales de carrera
const int finIzq = 2;
const int finDer = 3;

// Pines para los 3 sensores IR
const int sensorAlto = A0;    // Para 1L
const int sensorMedio = A1;   // Para 0.5L  
const int sensorBajo = A3;    // Para 0.25L

// Umbral para detección (ajustar según calibración)
const int UMBRAL_DETECCION = 500;

// Variable para guardar el sentido actual del motor
int sentido = 1; // 1 = derecha, -1 = izquierda

// Variables para el control del tiempo
unsigned long tiempoInicioRetardo = 0;
bool enRetardo = false;
bool retardoCompletado = false;

// Variables para el control de recipientes
bool esperandoRecipiente = false;
int recipienteSeleccionado = 0; // 0=none, 250=pequeño, 500=mediano, 1000=grande
bool recipienteDetectado = false;

// DECLARACIÓN DE FUNCIONES (para evitar el error)
void mostrarLCD(String linea1, String linea2);
void detectarRecipiente();

void setup() {
  // Configuración de pines
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(finIzq, INPUT_PULLUP);
  pinMode(finDer, INPUT_PULLUP);

  Serial.begin(9600);

  // Inicializar LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  
  // Usar la función mostrarLCD correctamente
  mostrarLCD("Sistema iniciado", "Calibrando...");
  delay(2000);
  mostrarLCD("Esperando", "Final Izquierdo");
}

// DEFINICIÓN de la función mostrarLCD
void mostrarLCD(String linea1, String linea2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(linea1);
  if (linea2 != "") {
    lcd.setCursor(0, 1);
    lcd.print(linea2);
  }
}

// DEFINICIÓN de la función detectarRecipiente
void detectarRecipiente() {
  // Leer los tres sensores
  int valorAlto = analogRead(sensorAlto);
  int valorMedio = analogRead(sensorMedio);
  int valorBajo = analogRead(sensorBajo);
  
  // Convertir a detección binaria
  bool detectaAlto = (valorAlto < UMBRAL_DETECCION);
  bool detectaMedio = (valorMedio < UMBRAL_DETECCION);
  bool detectaBajo = (valorBajo < UMBRAL_DETECCION);

  // CASO 1: Recipiente Pequeño (solo sensor bajo)
  if (detectaBajo && !detectaMedio && !detectaAlto) {
    recipienteSeleccionado = 250;
    recipienteDetectado = true;
    Serial.println("RECIPIENTE PEQUEÑO detectado (250ml)");
    mostrarLCD("Recipiente", "PEQUEÑO 250ml");
  } 
  // CASO 2: Recipiente Mediano (sensores bajo + medio)
  else if (detectaBajo && detectaMedio && !detectaAlto) {
    recipienteSeleccionado = 500;
    recipienteDetectado = true;
    Serial.println("RECIPIENTE MEDIANO detectado (500ml)");
    mostrarLCD("Recipiente", "MEDIANO 500ml");
  } 
  // CASO 3: Recipiente Grande (todos los sensores)
  else if (detectaBajo && detectaMedio && detectaAlto) {
    recipienteSeleccionado = 1000;
    recipienteDetectado = true;
    Serial.println("RECIPIENTE GRANDE detectado (1000ml)");
    mostrarLCD("Recipiente", "GRANDE 1000ml");
  }
  else {
    // No hay recipiente válido detectado
    recipienteDetectado = false;
    
    // Mostrar estado de sensores para debug
    Serial.print("Sensores - Alto:");
    Serial.print(detectaAlto);
    Serial.print(" Medio:");
    Serial.print(detectaMedio);
    Serial.print(" Bajo:");
    Serial.println(detectaBajo);
    
    // Mostrar en LCD qué se espera
    if (esperandoRecipiente) {
      mostrarLCD("Esperando:", "Coloque recipiente");
    }
  }
}

void loop() {
  // Leer los estados de los finales de carrera
  bool izqPresionado = digitalRead(finIzq) == LOW;
  bool derPresionado = digitalRead(finDer) == LOW;

  // ---------------------------
  // LÓGICA DEL FINAL IZQUIERDO (espera detección de recipiente)
  // ---------------------------
  if (izqPresionado && !esperandoRecipiente) {
    // Detener motor y comenzar espera de recipiente
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    esperandoRecipiente = true;
    recipienteDetectado = false;
    recipienteSeleccionado = 0;
    sentido = -1; // Estado actual: en extremo izquierdo
    
    Serial.println("Final IZQUIERDO presionado - Esperando detección de recipiente...");
    mostrarLCD("Esperando:", "Coloque recipiente");
    delay(500); // Pequeña pausa anti-rebote
  }

  // Si estamos esperando un recipiente, detectar continuamente
  if (esperandoRecipiente) {
    detectarRecipiente();
    
    // Si se detecta un recipiente válido, iniciar movimiento a derecha
    if (recipienteDetectado) {
      sentido = 1; // Cambiar a derecha
      esperandoRecipiente = false;
      
      // Aquí puedes agregar lógica específica para cada recipiente
      switch(recipienteSeleccionado) {
        case 250:
          Serial.println("INICIANDO LLENADO - Recipiente PEQUEÑO");
          // Puedes agregar tiempos específicos aquí
          break;
        case 500:
          Serial.println("INICIANDO LLENADO - Recipiente MEDIANO");
          // Puedes agregar tiempos específicos aquí
          break;
        case 1000:
          Serial.println("INICIANDO LLENADO - Recipiente GRANDE");
          // Puedes agregar tiempos específicos aquí
          break;
      }
      
      delay(300);
    }
  }

  // ---------------------------
  // LÓGICA DEL FINAL DERECHO (retardo 5 segundos)
  // ---------------------------
  if (derPresionado && !enRetardo && !retardoCompletado) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    enRetardo = true;
    tiempoInicioRetardo = millis();
    esperandoRecipiente = false; // Cancelar espera de recipiente si estaba activa
    
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
    mostrarLCD("Sistema listo", "Esperando...");
  }

  // ---------------------------
  // CONTROL DE MOVIMIENTO DEL MOTOR
  // ---------------------------
  if (!enRetardo && !esperandoRecipiente) {
    if (sentido == 1) {
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      if (!derPresionado) {
        Serial.println("Girando a la DERECHA - LLENANDO");
        mostrarLCD("Llenando:", String(recipienteSeleccionado) + "ml");
      }
    } else {
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
      if (!izqPresionado) {
        Serial.println("Girando a la IZQUIERDA - CERRANDO");
        mostrarLCD("Cerrando tolva", "Mov IZQUIERDO");
      }
    }
  } else {
    // Motor detenido si está en retardo o esperando recipiente
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
  }

  analogWrite(ENA, 200);
  delay(100);
}
