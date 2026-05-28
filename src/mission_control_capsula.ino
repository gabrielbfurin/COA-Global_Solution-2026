#include <LiquidCrystal.h>

// LCD: RS, E, D4, D5, D6, D7
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// Entradas analógicas
const int PIN_TEMP = A0;   // TMP36
const int PIN_LUZ  = A1;   // LDR (divisor)
const int PIN_VIB  = A2;   // Pot (simula vibração)

// LED de alerta
const int PIN_LED_ALERTA = 8;

// Limites calibrados
const float TEMP_ALERTA_C = 30.0;
const int   LUZ_ALERTA    = 150;  
const int   VIB_ALERTA    = 700;  

// Troca de tela
const unsigned long SCREEN_INTERVAL_MS = 3000;
unsigned long lastScreenChange = 0;
int screenIndex = 0;

float lerTemperaturaC_TMP36(int leituraAnalogica) {
  float tensao = leituraAnalogica * (5.0 / 1023.0);
  return (tensao - 0.5) * 100.0;
}

// Abreviação para caber no LCD 16x2
const char* statusAbrev(bool alerta) {
  return alerta ? "ALRT" : "OK";
}

void setup() {
  pinMode(PIN_LED_ALERTA, OUTPUT);
  digitalWrite(PIN_LED_ALERTA, LOW);

  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Mission Control");
  lcd.setCursor(0, 1);
  lcd.print("Iniciando...");
  delay(1200);

  Serial.begin(9600);
}

void loop() {
  // Leituras brutas
  int rawTemp = analogRead(PIN_TEMP);
  int rawLuz  = analogRead(PIN_LUZ);
  int rawVib  = analogRead(PIN_VIB);

  // Conversões
  float tempC = lerTemperaturaC_TMP36(rawTemp);

  int vibNivel = 1023 - rawVib; 

  // Alertas por variável
  bool alertaTemp = tempC >= TEMP_ALERTA_C;
  bool alertaLuz  = rawLuz <= LUZ_ALERTA;
  bool alertaVib  = vibNivel >= VIB_ALERTA;

  bool alertaGeral = alertaTemp || alertaLuz || alertaVib;

  // LED geral
  digitalWrite(PIN_LED_ALERTA, alertaGeral ? HIGH : LOW);

  // Troca de telas sem travar
  unsigned long now = millis();
  if (now - lastScreenChange >= SCREEN_INTERVAL_MS) {
    screenIndex = (screenIndex + 1) % 2;
    lastScreenChange = now;
  }

  // LCD
  lcd.clear();

  if (screenIndex == 0) {
    // Tela 1: valores + status geral abreviado
    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.print(tempC, 1);
    lcd.print("C");

    lcd.setCursor(10, 0);
    lcd.print("L:");
    lcd.print(rawLuz);

    lcd.setCursor(0, 1);
    lcd.print("V:");
    lcd.print(vibNivel);

    lcd.setCursor(10, 1);
    lcd.print("G:");
    lcd.print(statusAbrev(alertaGeral));
  } else {
    // Tela 2: diagnóstico compacto (cabe 100%)
    // Linha 1: "T:OK  L:ALRT"
    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.print(statusAbrev(alertaTemp));
    lcd.print("  L:");
    lcd.print(statusAbrev(alertaLuz));

    // Linha 2: "V:OK  G:OK"
    lcd.setCursor(0, 1);
    lcd.print("V:");
    lcd.print(statusAbrev(alertaVib));
    lcd.print("  G:");
    lcd.print(statusAbrev(alertaGeral));
  }

  // Serial (debug)
  Serial.print("TempC=");
  Serial.print(tempC, 2);
  Serial.print(" Luz=");
  Serial.print(rawLuz);
  Serial.print(" rawVib=");
  Serial.print(rawVib);
  Serial.print(" vibNivel=");
  Serial.print(vibNivel);
  Serial.print(" | Temp=");
  Serial.print(statusAbrev(alertaTemp));
  Serial.print(" Luz=");
  Serial.print(statusAbrev(alertaLuz));
  Serial.print(" Vib=");
  Serial.print(statusAbrev(alertaVib));
  Serial.print(" Geral=");
  Serial.println(statusAbrev(alertaGeral));

  delay(200);
}