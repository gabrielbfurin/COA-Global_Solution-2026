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

// Temporização (não-bloqueante)
const unsigned long SCREEN_INTERVAL_MS  = 3000; // troca de tela
const unsigned long SAMPLE_INTERVAL_MS  = 200;  // taxa de amostragem
const unsigned long LCD_REFRESH_MS      = 500;  // atualização do LCD (mais lento = mais estável)

unsigned long lastScreenChange = 0;
unsigned long lastSampleTime   = 0;
unsigned long lastLcdRefresh   = 0;

int screenIndex = 0;

// Filtro (média móvel simples)
const int FILTER_WINDOW = 8;
int luzBuffer[FILTER_WINDOW];
int vibBuffer[FILTER_WINDOW];
int filterIndex = 0;
long luzSum = 0;
long vibSum = 0;
bool filterInitialized = false;

// Leituras atuais (mantidas em memória)
int rawTemp = 0, rawLuz = 0, rawVib = 0;
float tempC = 0.0;
int vibNivel = 0;

// Valores filtrados
int luzFiltrada = 0;
int vibFiltrada = 0;

// Status
bool alertaTemp = false;
bool alertaLuz  = false;
bool alertaVib  = false;
bool alertaGeral = false;

float lerTemperaturaC_TMP36(int leituraAnalogica) {
  float tensao = leituraAnalogica * (5.0 / 1023.0);
  return (tensao - 0.5) * 100.0;
}

// Abreviação para caber no LCD 16x2
const char* statusAbrev(bool alerta) {
  return alerta ? "ALRT" : "OK";
}

void initFilterWith(int initialLuz, int initialVib) {
  luzSum = 0;
  vibSum = 0;

  for (int i = 0; i < FILTER_WINDOW; i++) {
    luzBuffer[i] = initialLuz;
    vibBuffer[i] = initialVib;
    luzSum += initialLuz;
    vibSum += initialVib;
  }

  filterIndex = 0;
  filterInitialized = true;
}

void updateFilter(int newLuz, int newVib) {
  luzSum -= luzBuffer[filterIndex];
  vibSum -= vibBuffer[filterIndex];

  luzBuffer[filterIndex] = newLuz;
  vibBuffer[filterIndex] = newVib;

  luzSum += newLuz;
  vibSum += newVib;

  filterIndex = (filterIndex + 1) % FILTER_WINDOW;
}

int getLuzFiltrada() { return (int)(luzSum / FILTER_WINDOW); }
int getVibFiltrada() { return (int)(vibSum / FILTER_WINDOW); }

void printPadded(const char* s, int width) {
  int n = 0;
  while (s[n] != '\0') n++;
  lcd.print(s);
  for (int i = n; i < width; i++) lcd.print(' ');
}

void renderLcd() {
  // Para não sobrar caractere da tela anterior: sempre imprimir exatamente 16 caracteres por linha.
  char line0[17];
  char line1[17];

  if (screenIndex == 0) {
    // Layout "B" (profissional): valores com largura fixa
    // Linha 0: T:24.8C L:0054
    // Linha 1: V:0000  G:ALRT

    // tempC com largura fixa e sempre com sinal/dígito: %+05.1f
    // Ex.: +24.8, +7.2 (evita "virar" pra direita quando tem 1 dígito só)
    snprintf(line0, sizeof(line0), "T:%+05.1fC L:%04d", tempC, luzFiltrada);
    snprintf(line1, sizeof(line1), "V:%04d  G:%-4s", vibFiltrada, statusAbrev(alertaGeral));
  } else {
    // Diagnóstico compacto (também fixo em 16 via padding)
    snprintf(line0, sizeof(line0), "T:%-4s L:%-4s", statusAbrev(alertaTemp), statusAbrev(alertaLuz));
    snprintf(line1, sizeof(line1), "V:%-4s G:%-4s", statusAbrev(alertaVib), statusAbrev(alertaGeral));
  }

  line0[16] = '\0';
  line1[16] = '\0';

  lcd.setCursor(0, 0);
  printPadded(line0, 16);

  lcd.setCursor(0, 1);
  printPadded(line1, 16);
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

  Serial.begin(9600);

  // Inicializa filtro com as primeiras leituras
  rawLuz = analogRead(PIN_LUZ);
  rawVib = analogRead(PIN_VIB);
  vibNivel = 1023 - rawVib;
  initFilterWith(rawLuz, vibNivel);

  unsigned long now = millis();
  lastSampleTime = now;
  lastScreenChange = now;
  lastLcdRefresh = now;

  delay(1200); // splash
  lcd.clear();
}

void loop() {
  unsigned long now = millis();

  // 1) Amostragem NÃO-bloqueante (sensores + filtro + status + serial)
  if (now - lastSampleTime >= SAMPLE_INTERVAL_MS) {
    lastSampleTime = now;

    rawTemp = analogRead(PIN_TEMP);
    rawLuz  = analogRead(PIN_LUZ);
    rawVib  = analogRead(PIN_VIB);

    tempC = lerTemperaturaC_TMP36(rawTemp);

    // Corrige vibração invertida no circuito
    vibNivel = 1023 - rawVib;

    if (!filterInitialized) {
      initFilterWith(rawLuz, vibNivel);
    } else {
      updateFilter(rawLuz, vibNivel);
    }

    luzFiltrada = getLuzFiltrada();
    vibFiltrada = getVibFiltrada();

    alertaTemp = tempC >= TEMP_ALERTA_C;
    alertaLuz  = luzFiltrada <= LUZ_ALERTA;
    alertaVib  = vibFiltrada >= VIB_ALERTA;
    alertaGeral = alertaTemp || alertaLuz || alertaVib;

    digitalWrite(PIN_LED_ALERTA, alertaGeral ? HIGH : LOW);

    // Serial com telemetria + status
    Serial.print("TempC=");
    Serial.print(tempC, 2);
    Serial.print(" Luz=");
    Serial.print(luzFiltrada);
    Serial.print(" Vib=");
    Serial.print(vibFiltrada);
    Serial.print(" | T=");
    Serial.print(statusAbrev(alertaTemp));
    Serial.print(" L=");
    Serial.print(statusAbrev(alertaLuz));
    Serial.print(" V=");
    Serial.print(statusAbrev(alertaVib));
    Serial.print(" G=");
    Serial.println(statusAbrev(alertaGeral));
  }

  // 2) Troca de telas NÃO-bloqueante
  if (now - lastScreenChange >= SCREEN_INTERVAL_MS) {
    screenIndex = (screenIndex + 1) % 2;
    lastScreenChange = now;

    // Ao trocar de tela, força redesenho imediato
    lastLcdRefresh = 0;
  }

  // 3) Atualização do LCD controlada (evita flicker)
  if (now - lastLcdRefresh >= LCD_REFRESH_MS) {
    lastLcdRefresh = now;
    renderLcd();
  }
}
