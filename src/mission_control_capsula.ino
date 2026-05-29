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
const unsigned long SAMPLE_INTERVAL_MS  = 200;  // taxa de amostragem (antes era delay(200))

unsigned long lastScreenChange = 0;
unsigned long lastSampleTime   = 0;

int screenIndex = 0;

// Filtro (média móvel simples)
// Mantém uma janela pequena pra reduzir oscilação perto do limite.
const int FILTER_WINDOW = 8;
int luzBuffer[FILTER_WINDOW];
int vibBuffer[FILTER_WINDOW];
int filterIndex = 0;
long luzSum = 0;
long vibSum = 0;
bool filterInitialized = false;

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

void updateFilter(int rawLuz, int vibNivel) {
  // remove o valor antigo do somatório
  luzSum -= luzBuffer[filterIndex];
  vibSum -= vibBuffer[filterIndex];

  // coloca o novo valor
  luzBuffer[filterIndex] = rawLuz;
  vibBuffer[filterIndex] = vibNivel;

  // adiciona ao somatório
  luzSum += rawLuz;
  vibSum += vibNivel;

  // avança o índice circular
  filterIndex = (filterIndex + 1) % FILTER_WINDOW;
}

int getLuzFiltrada() {
  return (int)(luzSum / FILTER_WINDOW);
}

int getVibFiltrada() {
  return (int)(vibSum / FILTER_WINDOW);
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

  // Inicializa o filtro com as primeiras leituras
  int rawLuz0 = analogRead(PIN_LUZ);
  int rawVib0 = analogRead(PIN_VIB);
  int vibNivel0 = 1023 - rawVib0;
  initFilterWith(rawLuz0, vibNivel0);

  delay(1200); // só splash screen (opcional). O loop principal é não-bloqueante.
}

void loop() {
  unsigned long now = millis();

  // 1) Amostragem NÃO-bloqueante
  static int rawTemp = 0;
  static int rawLuz  = 0;
  static int rawVib  = 0;

  static float tempC = 0.0;
  static int vibNivel = 0;

  if (now - lastSampleTime >= SAMPLE_INTERVAL_MS) {
    lastSampleTime = now;

    rawTemp = analogRead(PIN_TEMP);
    rawLuz  = analogRead(PIN_LUZ);
    rawVib  = analogRead(PIN_VIB);

    tempC = lerTemperaturaC_TMP36(rawTemp);

    // Corrige vibração invertida no circuito
    vibNivel = 1023 - rawVib;

    // Atualiza filtro (média móvel) para luz e vibração
    if (!filterInitialized) {
      initFilterWith(rawLuz, vibNivel);
    } else {
      updateFilter(rawLuz, vibNivel);
    }

    // Debug no Serial apenas na mesma taxa de amostragem
    Serial.print("TempC=");
    Serial.print(tempC, 2);
    Serial.print(" LuzRaw=");
    Serial.print(rawLuz);
    Serial.print(" LuzFil=");
    Serial.print(getLuzFiltrada());
    Serial.print(" VibRaw=");
    Serial.print(rawVib);
    Serial.print(" VibNivel=");
    Serial.print(vibNivel);
    Serial.print(" VibFil=");
    Serial.print(getVibFiltrada());
    Serial.println();
  }

  // 2) Troca de telas NÃO-bloqueante
  if (now - lastScreenChange >= SCREEN_INTERVAL_MS) {
    screenIndex = (screenIndex + 1) % 2;
    lastScreenChange = now;
  }

  // 3) Usa valores filtrados para evitar “pisca-pisca” perto do limite
  int luzFiltrada = getLuzFiltrada();
  int vibFiltrada = getVibFiltrada();

  bool alertaTemp = tempC >= TEMP_ALERTA_C;
  bool alertaLuz  = luzFiltrada <= LUZ_ALERTA;
  bool alertaVib  = vibFiltrada >= VIB_ALERTA;

  bool alertaGeral = alertaTemp || alertaLuz || alertaVib;

  // LED geral
  digitalWrite(PIN_LED_ALERTA, alertaGeral ? HIGH : LOW);

  // 4) LCD (atualiza sempre; se quiser, dá pra colocar um intervalo próprio)
  lcd.clear();

  if (screenIndex == 0) {
    // Tela 1: valores + status geral
    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.print(tempC, 1);
    lcd.print("C");

    lcd.setCursor(10, 0);
    lcd.print("L:");
    lcd.print(luzFiltrada);

    lcd.setCursor(0, 1);
    lcd.print("V:");
    lcd.print(vibFiltrada);

    lcd.setCursor(10, 1);
    lcd.print("G:");
    lcd.print(statusAbrev(alertaGeral));
  } else {
    // Tela 2: diagnóstico compacto
    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.print(statusAbrev(alertaTemp));
    lcd.print("  L:");
    lcd.print(statusAbrev(alertaLuz));

    lcd.setCursor(0, 1);
    lcd.print("V:");
    lcd.print(statusAbrev(alertaVib));
    lcd.print("  G:");
    lcd.print(statusAbrev(alertaGeral));
  }
}
