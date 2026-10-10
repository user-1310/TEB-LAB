/*
 * =========================================================================================
 * UNIVERSIDAD NACIONAL MAYOR DE SAN MARCOS
 * Facultad de Ingeniería Electrónica y Eléctrica
 * Escuela Profesional de Ingeniería Biomédica
 * Taller de Equipos Biomédicos II - Laboratorio N° 2
 * 
 * Proyecto: Caracterización de Sensor de Presión Manométrica (MPS20N0040D-D + HX710B)
 * Microcontrolador: ESP32 DevKit
 * =========================================================================================
 * Protocolo HX710B:
 *   - 24 pulsos de reloj para leer los 24 bits de datos (complemento a 2).
 *   - Pulso 25: fija salida diferencial a 10 Hz y deja DOUT en nivel alto.
 * =========================================================================================
 */

#include <Arduino.h>

// --- ASIGNACIÓN DE PINES ESP32 (Ajustables según tu montaje) ---
const int PIN_DOUT = 21;  // Pin de datos digitales del HX710B
const int PIN_SCK  = 22;  // Pin de reloj serie del HX710B

// --- PARÁMETROS CONFIGURABLES SEGÚN REQUISITOS DE GUÍA (Sección 6) ---
const int NUM_LECTURAS_PROMEDIO = 20;     // N >= 20 constante durante la sesión

float P_atm = 101.325; // Presión atmosférica en kPa (se actualiza con dato SENAMHI)
float V0    = 1.00;    // Volumen inicial jeringa en mL
float Vm    = 0.62;    // Volumen muerto medido con agua en mL (menor de Va1 y Va2)
const float P_MAX_SEGURA = 35.0; // Margen de seguridad (kPa)
const float P_MAX_SENSOR = 40.0; // Rango máximo del sensor (kPa)

// Variables de estado
float Vj_min = 0.0;
float offset_cero = 0.0;
float ruido_cero  = 0.0;

// =========================================================================================
// Lectura de 24 bits del convertidor HX710B
// =========================================================================================
long leerHX710B() {
  unsigned long timeout = millis();
  while (digitalRead(PIN_DOUT) == HIGH) {
    if (millis() - timeout > 500) {
      Serial.println(F("[ALERTA] Timeout: HX710B no responde. Revisa conexiones."));
      return 0;
    }
  }

  unsigned long rawData = 0;

  for (int i = 0; i < 24; i++) {
    digitalWrite(PIN_SCK, HIGH);
    delayMicroseconds(2);
    rawData = (rawData << 1);
    digitalWrite(PIN_SCK, LOW);
    delayMicroseconds(2);
    if (digitalRead(PIN_DOUT) == HIGH) {
      rawData++;
    }
  }

  // Pulso 25 para fijar tasa a 10 Hz
  digitalWrite(PIN_SCK, HIGH);
  delayMicroseconds(2);
  digitalWrite(PIN_SCK, LOW);
  delayMicroseconds(2);

  // Extensión de signo (complemento a 2 de 24 bits a long con signo)
  long signedData = 0;
  if (rawData & 0x800000) {
    signedData = (long)(rawData | 0xFF000000);
  } else {
    signedData = (long)(rawData & 0x00FFFFFF);
  }

  return signedData;
}

// =========================================================================================
// Adquisición de N muestras y cálculo de Media y Desviación Estándar
// =========================================================================================
void tomarEstadisticas(int n, float &media, float &desvStd) {
  long lecturas[n];
  double suma = 0.0;

  for (int i = 0; i < n; i++) {
    lecturas[i] = leerHX710B();
    suma += lecturas[i];
    delay(100); // 100 ms acorde a 10 Hz
  }

  media = suma / n;

  double sumaVarianza = 0.0;
  for (int i = 0; i < n; i++) {
    sumaVarianza += pow(lecturas[i] - media, 2);
  }
  desvStd = sqrt(sumaVarianza / (n - 1));
}

// =========================================================================================
// Cálculo de Marca de Parada (Ecuación 2)
// Vj_min = (V0 - r * Vm) / (1 + r)
// =========================================================================================
void calcularMarcaParada() {
  float r = P_MAX_SEGURA / P_atm;
  Vj_min = (V0 - (r * Vm)) / (1.0 + r);
  
  Serial.println(F("\n========================================================="));
  Serial.println(F("         CONFIGURACIÓN Y MARCA DE PARADA SEGURA          "));
  Serial.println(F("========================================================="));
  Serial.print(F("Patm (Presión Atmosférica)  : ")); Serial.print(P_atm, 3); Serial.println(F(" kPa"));
  Serial.print(F("V0 (Volumen Inicial Jeringa): ")); Serial.print(V0, 2);    Serial.println(F(" mL"));
  Serial.print(F("Vm (Volumen Muerto Medido)  : ")); Serial.print(Vm, 3);    Serial.println(F(" mL"));
  Serial.print(F("Pmax de seguridad           : ")); Serial.print(P_MAX_SEGURA, 1); Serial.println(F(" kPa"));
  Serial.println(F("---------------------------------------------------------"));
  Serial.print(F(">>> MARCA DE PARADA (Vj_min): "));
  Serial.print(Vj_min, 3);
  Serial.println(F(" mL <<<"));
  Serial.println(F("(!) NUNCA comprimir por debajo de este volumen."));
  Serial.println(F("=========================================================\n"));
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(PIN_DOUT, INPUT);
  pinMode(PIN_SCK, OUTPUT);
  digitalWrite(PIN_SCK, LOW);

  Serial.println(F("\n========================================================"));
  Serial.println(F("  UNMSM - FIEE - TALLER DE EQUIPOS BIOMÉDICOS II       "));
  Serial.println(F("  LAB 2: Caracterización MPS20N0040D-D con HX710B       "));
  Serial.println(F("========================================================"));

  calcularMarcaParada();

  Serial.println(F("\n--- ETAPA A: CARACTERIZACIÓN DE CERO Y RUIDO ---"));
  Serial.println(F("Sensor abierto al aire (presión manométrica 0)."));
  Serial.print(F("Midiendo ")); Serial.print(NUM_LECTURAS_PROMEDIO); Serial.println(F(" lecturas..."));

  tomarEstadisticas(NUM_LECTURAS_PROMEDIO, offset_cero, ruido_cero);

  Serial.print(F("-> Cero Inicial (C0,i - Media)     : ")); Serial.println(offset_cero, 2);
  Serial.print(F("-> Ruido Inicial (sigma0 - DesvStd): ")); Serial.println(ruido_cero, 2);
  Serial.println(F("\n[CHECKPOINT 1]: Mostrar valores y osciloscopio a la docente."));
  
  Serial.println(F("\n--- MODO BARRIDO (ETAPA D) ---"));
  Serial.println(F("Ingresa: <ciclo> <sentido: C/D> <Vj>  (ejemplo: 1 C 0.88)"));
  Serial.println(F("O comandos:"));
  Serial.println(F("  SET_VM 0.64       -> actualiza Vm con el agua"));
  Serial.println(F("  SET_PATM 101.1    -> actualiza Patm"));
  Serial.println(F("  CERO_FINAL        -> mide C0,f al terminar la sesión"));
  Serial.println(F("-------------------------------------------------------------"));
  Serial.println(F("FORMATO CSV: ciclo,sentido,Vj_mL,Pref_kPa,media_cuentas,desvStd,N"));
  Serial.println(F("-------------------------------------------------------------"));
}

void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.length() == 0) return;

    if (input.startsWith("SET_VM")) {
      float nuevoVm = input.substring(6).toFloat();
      if (nuevoVm > 0.05 && nuevoVm < 2.0) {
        Vm = nuevoVm;
        Serial.print(F("[OK] Vm actualizado: ")); Serial.println(Vm, 3);
        calcularMarcaParada();
      }
      return;
    }

    if (input.startsWith("SET_PATM")) {
      float nuevaPatm = input.substring(8).toFloat();
      if (nuevaPatm > 80.0 && nuevaPatm < 110.0) {
        P_atm = nuevaPatm;
        Serial.print(F("[OK] Patm actualizada: ")); Serial.println(P_atm, 3);
        calcularMarcaParada();
      }
      return;
    }

    if (input.equalsIgnoreCase("CERO_FINAL")) {
      Serial.println(F("\n--- MIDIENDO CERO FINAL (C0,f) ---"));
      float media_f = 0.0, ruido_f = 0.0;
      tomarEstadisticas(NUM_LECTURAS_PROMEDIO, media_f, ruido_f);
      Serial.print(F("-> Cero Final (C0,f - Media)      : ")); Serial.println(media_f, 2);
      Serial.print(F("-> Ruido Final (sigma0,f - DesvStd): ")); Serial.println(ruido_f, 2);
      Serial.print(F("-> Deriva de Offset (C0,f - C0,i) : ")); Serial.println(media_f - offset_cero, 2);
      Serial.println(F("[CHECKPOINT 4]: Listo para presentar.\n"));
      return;
    }

    int sp1 = input.indexOf(' ');
    int sp2 = input.indexOf(' ', sp1 + 1);

    if (sp1 == -1 || sp2 == -1) {
      Serial.println(F("[ERROR] Usa formato: <ciclo> <C/D> <Vj>"));
      return;
    }

    int ciclo = input.substring(0, sp1).toInt();
    String sentido = input.substring(sp1 + 1, sp2);
    sentido.toUpperCase();
    float Vj = input.substring(sp2 + 1).toFloat();

    // Validaciones de seguridad de la Guía
    if (Vj > V0) {
      Serial.println(F("[RECHAZADO] Vj > V0 (1.00 mL)."));
      return;
    }
    if (Vj <= 0.0) {
      Serial.println(F("[RECHAZADO] Vj debe ser mayor que 0."));
      return;
    }

    // Pref = Patm * (V0 - Vj) / (Vj + Vm)
    float Pref = P_atm * (V0 - Vj) / (Vj + Vm);

    if (Pref > P_MAX_SENSOR) {
      Serial.print(F("[PELIGRO] Pref = ")); Serial.print(Pref, 2);
      Serial.println(F(" kPa supera el límite del sensor (40 kPa)."));
      return;
    }

    float media = 0.0;
    float desvStd = 0.0;
    tomarEstadisticas(NUM_LECTURAS_PROMEDIO, media, desvStd);

    // Salida CSV exacta requerida: ciclo, sentido, Vj, Pref, media, desvStd, N
    Serial.print(ciclo);
    Serial.print(F(","));
    Serial.print(sentido);
    Serial.print(F(","));
    Serial.print(Vj, 2);
    Serial.print(F(","));
    Serial.print(Pref, 3);
    Serial.print(F(","));
    Serial.print(media, 2);
    Serial.print(F(","));
    Serial.print(desvStd, 2);
    Serial.print(F(","));
    Serial.println(NUM_LECTURAS_PROMEDIO);
  }
}
