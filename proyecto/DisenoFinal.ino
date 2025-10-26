#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------------- LCD ----------------
LiquidCrystal_I2C lcd(0x27, 16, 2); // Cambia a 0x3F si es necesario

// ---------------- MOTOR ----------------
const int IN1 = 7;
const int IN2 = 8;
const int ENA = 9;

// ---------------- FINALES DE CARRERA ----------------
const int finIzq = 2;
const int finDer = 3;

// ---------------- SENSORES IR ----------------
const int sensorAlto = A0;  // Para 1L
const int sensorMedio = A1; // Para 0.5L
const int sensorBajo = A3;  // Para 0.25L
const int UMBRAL_DETECCION = 500;

// ---------------- FOCO ----------------
const int pinFoco = 5;  // MOC3021
bool focoEncendido = true;
unsigned long tiempoInicioFoco = 0;
unsigned long duracionApagado = 0;
bool enApagado = false;

// ---------------- VARIABLES GLOBALES ----------------
int sentido = 1; // 1 = derecha, -1 = izquierda
unsigned long tiempoInicioRetardo = 0;
bool enRetardo = false;
bool retardoCompletado = false;

bool esperandoRecipiente = false;
bool recipienteDetectado = false;
int recipienteSeleccionado = 0; // 0=none, 250=pequeño, 500=mediano, 1000=grande
unsigned long tiempoRetardoRecipiente = 0;

bool sistemaInicializado = false; // NUEVA VARIABLE para controlar la inicialización

// ---------------- FUNCIONES ----------------
void mostrarLCD(String linea1, String linea2);
void detectarRecipiente();
void controlarFoco();
void rutinaInicializacion();

void setup() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(finIzq, INPUT_PULLUP);
  pinMode(finDer, INPUT_PULLUP);
  pinMode(pinFoco, OUTPUT);

  // Iniciar foco encendido
  digitalWrite(pinFoco, HIGH);
  focoEncendido = true;

  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  lcd.clear();

  mostrarLCD("Sistema iniciado", "Inicializando...");
  
  // RUTINA DE INICIALIZACIÓN SOLO UNA VEZ
  rutinaInicializacion();
  sistemaInicializado = true; // Marcar como inicializado
  
  mostrarLCD("Sistema listo", "Esperando...");
  Serial.println("Sistema inicializado y listo");
}

// ---------------- RUTINA DE INICIALIZACIÓN ----------------
void rutinaInicializacion() {
  Serial.println("Iniciando rutina de inicialización...");
  mostrarLCD("Inicializando", "motor...");
  
  bool izqPresionado = digitalRead(finIzq) == LOW;
  bool derPresionado = digitalRead(finDer) == LOW;
  
  // Si el motor está en el final izquierdo, moverlo a la derecha
  if (izqPresionado) {
    Serial.println("Motor en posición IZQUIERDA - Moviendo a DERECHA...");
    mostrarLCD("Moviendo a", "DERECHA...");
    
    // Activar motor hacia la derecha
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    analogWrite(ENA, 225);
    
    // Esperar hasta que llegue al final derecho o timeout
    unsigned long inicioMovimiento = millis();
    while (digitalRead(finDer) != LOW) {
      if (millis() - inicioMovimiento > 10000) { // Timeout de 10 segundos
        Serial.println("TIMEOUT: Motor no llegó al final derecho");
        break;
      }
      delay(50);
    }
    
    // Detener motor
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    Serial.println("Motor en posición DERECHA - Inicialización completada");
    delay(500);
  } 
  // Si el motor está en el final derecho, ya está en posición correcta
  else if (derPresionado) {
    Serial.println("Motor ya en posición DERECHA");
    delay(500);
  }
  // Si no está en ningún final, mover hacia la derecha hasta encontrar uno
  else {
    Serial.println("Motor en posición desconocida - Buscando referencia...");
    mostrarLCD("Buscando", "referencia...");
    
    // Primero intentar mover a la derecha
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    analogWrite(ENA, 225);
    
    unsigned long inicioBusqueda = millis();
    while (digitalRead(finDer) != LOW && digitalRead(finIzq) != LOW) {
      if (millis() - inicioBusqueda > 8000) { // Timeout de 8 segundos
        Serial.println("TIMEOUT: No se encontró referencia");
        break;
      }
      delay(50);
    }
    
    // Detener motor
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    
    // Si terminó en izquierdo, mover a derecho
    if (digitalRead(finIzq) == LOW) {
      Serial.println("Encontrado final IZQUIERDO - Moviendo a DERECHA...");
      delay(500);
      
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      analogWrite(ENA, 225);
      
      inicioBusqueda = millis();
      while (digitalRead(finDer) != LOW) {
        if (millis() - inicioBusqueda > 8000) {
          Serial.println("TIMEOUT: No se llegó al final derecho");
          break;
        }
        delay(50);
      }
      
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
    }
    
    Serial.println("Posición de referencia encontrada");
    delay(500);
  }
  
  // Configurar estado inicial del sistema
  sentido = -1; // Listo para moverse a izquierda cuando se detecte recipiente
  esperandoRecipiente = true;
  retardoCompletado = false;
  enRetardo = false;
  
  Serial.println("Rutina de inicialización COMPLETADA");
}

// ---------------- FUNCIONES AUXILIARES ----------------

void mostrarLCD(String linea1, String linea2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(linea1);
  if (linea2 != "") {
    lcd.setCursor(0, 1);
    lcd.print(linea2);
  }
}

// Detectar tipo de recipiente y ajustar tiempos
void detectarRecipiente() {
  int valorAlto = analogRead(sensorAlto);
  int valorMedio = analogRead(sensorMedio);
  int valorBajo = analogRead(sensorBajo);
  
  bool detectaAlto = (valorAlto < UMBRAL_DETECCION);
  bool detectaMedio = (valorMedio < UMBRAL_DETECCION);
  bool detectaBajo = (valorBajo < UMBRAL_DETECCION);

  if (detectaBajo && !detectaMedio && !detectaAlto) {
    recipienteSeleccionado = 250;
    tiempoRetardoRecipiente = 500; // 0.5 s para motor ---valor correcto
    duracionApagado = 3000;         // 0.3 s para foco (CORREGIDO)
    recipienteDetectado = true;
    Serial.println("Recipiente PEQUEÑO detectado (250 ml)");
    mostrarLCD("Recipiente", "PEQUEÑO 250ml");
  } 
  else if (detectaBajo && detectaMedio && !detectaAlto) {
    recipienteSeleccionado = 500;
    tiempoRetardoRecipiente = 1200; // 0.8 s para motor (CORREGIDO)
    duracionApagado = 5000;         // 4 s para foco (CORREGIDO)
    recipienteDetectado = true;
    Serial.println("Recipiente MEDIANO detectado (500 ml)");
    mostrarLCD("Recipiente", "MEDIANO 500ml");
  } 
  else if (detectaBajo && detectaMedio && detectaAlto) {
    recipienteSeleccionado = 1000;
    tiempoRetardoRecipiente = 1900; // 1.9 s para motor
    duracionApagado = 6000;         // 1.5 s para foco (CORREGIDO)
    recipienteDetectado = true;
    Serial.println("Recipiente GRANDE detectado (1000 ml)");
    mostrarLCD("Recipiente", "GRANDE 1000ml");
  } 
  else {
    recipienteDetectado = false;
    if (esperandoRecipiente) {
      mostrarLCD("Esperando:", "Coloque recipiente");
    }
  }
}

// Control del foco según el recipiente - CORREGIDO
void controlarFoco() {
  // Si el foco está encendido y se detectó un recipiente → apagarlo temporalmente
  if (!enApagado && recipienteDetectado && sistemaInicializado) {
    recipienteDetectado = false; // solo ejecutar una vez por detección

    Serial.println("Foco: Esperando 1 segundo antes de apagarse...");
    delay(9400); // espera 9 segundo antes de apagar (CORREGIDO)

    Serial.print("Apagando foco por ");
    Serial.print(duracionApagado);
    Serial.println(" ms...");
    digitalWrite(pinFoco, LOW);
    focoEncendido = false;

    tiempoInicioFoco = millis(); // guarda cuándo se apagó
    enApagado = true;
  }

  // Si el foco está apagado y ya pasó el tiempo → volver a encender
  if (enApagado && (millis() - tiempoInicioFoco >= duracionApagado)) {
    Serial.println("Reencendiendo foco...");
    digitalWrite(pinFoco, HIGH);
    focoEncendido = true;
    enApagado = false;

    // NO resetear esperandoRecipiente aquí - eso causa el ciclo infinito
    // El sistema debe esperar a que se presione el final de carrera izquierdo
    // para volver a esperar un recipiente
    Serial.println("Foco reencendido - Esperando siguiente ciclo");
  }
}

void loop() {
  bool izqPresionado = digitalRead(finIzq) == LOW;
  bool derPresionado = digitalRead(finDer) == LOW;

  // FINAL IZQUIERDO - Solo entra en espera de recipiente cuando se presiona físicamente
  if (izqPresionado && !esperandoRecipiente) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    esperandoRecipiente = true;
    recipienteDetectado = false;
    recipienteSeleccionado = 0;
    sentido = -1;

    Serial.println("Final IZQUIERDO presionado - Esperando recipiente...");
    mostrarLCD("Esperando:", "Coloque recipiente");
    delay(500);
  }

  if (esperandoRecipiente) {
    detectarRecipiente();
    if (recipienteSeleccionado > 0) {
      sentido = 1;
      esperandoRecipiente = false;

      switch (recipienteSeleccionado) {
        case 250: 
          Serial.println("Iniciando llenado PEQUENO..."); 
          break;
        case 500: 
          Serial.println("Iniciando llenado MEDIANO..."); 
          break;
        case 1000: 
          Serial.println("Iniciando llenado GRANDE..."); 
          break;
      }
    }
  }

  // Control del foco
  controlarFoco();

  // FINAL DERECHO
  if (derPresionado && !enRetardo && !retardoCompletado) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    enRetardo = true;
    tiempoInicioRetardo = millis();
    esperandoRecipiente = false;

    Serial.print("Final DERECHO presionado - Retardo de ");
    Serial.print(tiempoRetardoRecipiente);
    Serial.println(" ms...");
    mostrarLCD("Final DER presionado", "Esperando...");
  }

  if (enRetardo) {
    if (millis() - tiempoInicioRetardo >= tiempoRetardoRecipiente) {
      sentido = -1;
      enRetardo = false;
      retardoCompletado = true;
      Serial.println("Retardo completado - Cambiando a IZQUIERDA");
      mostrarLCD("Retardo completado", "Mov. IZQUIERDA");
    }
  }

  if (!derPresionado && retardoCompletado) {
    retardoCompletado = false;
    Serial.println("Final DERECHO liberado - Sistema listo");
    mostrarLCD("Sistema listo", "Esperando...");
  }

  // CONTROL MOTOR
  if (!enRetardo && !esperandoRecipiente) {
    if (sentido == 1) {
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      if (!derPresionado) {
        Serial.println("Girando DERECHA - LLENANDO");
        mostrarLCD("Llenando:", String(recipienteSeleccionado) + "ml");
      }
    } else {
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
      if (!izqPresionado) {
        Serial.println("Girando IZQUIERDA - CERRANDO");
        mostrarLCD("Cerrando tolva", "Mov. IZQUIERDO");
      }
    }
  } else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
  }

  analogWrite(ENA, 225);
  delay(100);
}
