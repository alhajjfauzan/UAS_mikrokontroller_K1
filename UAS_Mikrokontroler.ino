/*
 * =====================================================
 * SMART PARKING GATE v4.0 — CLEAN REWRITE
 * Bluino ESP32 IoT Starter Kit
 * =====================================================
 *
 * Konsep: Potentiometer = switch 3 posisi
 *
 *   KIRI  (0-30%)   → MODE MANUAL TUTUP (servo 0°)
 *   TENGAH (30-70%)  → MODE AUTO (sensor kontrol semua)
 *   KANAN (70-100%)  → MODE MANUAL BUKA (servo 90°)
 *
 * Mode AUTO alur:
 *   Ultrasonik < 20cm → konfirmasi 800ms → buka palang
 *   → tunggu mobil lewat (>50cm) → jeda 2s → tutup
 *
 * Mode MANUAL:
 *   Langsung buka/tutup sesuai posisi potensio.
 *   Sensor tetap terbaca & tampil di OLED, tapi
 *   TIDAK mengontrol servo.
 *
 * Pin:
 *   PIR         → GPIO 27     (on-board)
 *   Buzzer      → GPIO 13     (on-board)
 *   LED Merah    → GPIO 25     (on-board)
 *   LED Hijau    → GPIO 26     (on-board)
 *   Potensio    → GPIO 34     (on-board)
 *   OLED SDA    → GPIO 21     (on-board)
 *   OLED SCL    → GPIO 22     (on-board)
 *   Trig        → GPIO 5      (kabel)
 *   Echo        → GPIO 16     (kabel)
 *   Servo       → GPIO 17     (kabel)
 *
 * Library: Adafruit SSD1306, Adafruit GFX, ESP32Servo
 * =====================================================
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>

// =====================================================
// PIN
// =====================================================
#define PIN_PIR        27
#define PIN_BUZZER     13
#define PIN_LED_RED    25
#define PIN_LED_GREEN  26
#define PIN_POT        34
#define PIN_SDA        21
#define PIN_SCL        22
#define PIN_TRIG        5
#define PIN_ECHO       16
#define PIN_SERVO      17

// =====================================================
// SETTING
// =====================================================
#define JARAK_DEKAT    20      // cm
#define JARAK_JAUH     50      // cm
#define SUDUT_TUTUP     0
#define SUDUT_BUKA     90
#define WAKTU_KONFIRM  800     // ms
#define WAKTU_JEDA     2000    // ms sebelum tutup
#define WAKTU_TIMEOUT  15000   // ms auto-tutup

// Potensio zona (dari 4095)
#define POT_BATAS_KIRI  1230   // 0-30% = manual tutup
#define POT_BATAS_KANAN 2865   // 70-100% = manual buka

// =====================================================
// OBJEK
// =====================================================
Adafruit_SSD1306 oled(128, 64, &Wire, -1);
Servo servo;

// =====================================================
// MODE
// =====================================================
enum Mode { MODE_MANUAL_TUTUP, MODE_AUTO, MODE_MANUAL_BUKA };
Mode mode = MODE_AUTO;
Mode modeLama = MODE_AUTO;

// State untuk mode AUTO saja
enum AutoState {
  AUTO_IDLE,
  AUTO_KONFIRMASI,
  AUTO_BUKA,
  AUTO_TERBUKA,
  AUTO_JEDA,
  AUTO_TUTUP
};
AutoState autoState = AUTO_IDLE;

// =====================================================
// VARIABEL SENSOR
// =====================================================
float jarak       = 999.0;
bool  pir         = false;
int   pot         = 2048;

// Servo
int   sudutNow    = SUDUT_TUTUP;
int   sudutTarget = SUDUT_TUTUP;

// Counter
int   totalMobil  = 0;

// Timing
unsigned long tState      = 0;
unsigned long tServo      = 0;
unsigned long tDisplay    = 0;
unsigned long tSensor     = 0;
unsigned long tDebug      = 0;

// =====================================================
// SETUP
// =====================================================
void setup() {
  Serial.begin(115200);
  Serial.println("=== SMART PARKING v4.0 ===");

  pinMode(PIN_PIR, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

  digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_LED_RED, LOW);
  digitalWrite(PIN_LED_GREEN, LOW);

  servo.attach(PIN_SERVO);
  servo.write(SUDUT_TUTUP);

  Wire.begin(PIN_SDA, PIN_SCL);
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED ERROR!");
    while (1) { digitalWrite(PIN_LED_RED, !digitalRead(PIN_LED_RED)); delay(100); }
  }

  // Splash
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(10, 10);
  oled.println("SMART PARKING v4");
  oled.setCursor(10, 30);
  oled.println("Potensio = 3 mode");
  oled.setCursor(10, 45);
  oled.println("KIRI|AUTO|KANAN");
  oled.display();
  delay(2000);

  // Beep siap
  buzz(2, 100);

  Serial.println("Siap!\n");
}

// =====================================================
// LOOP
// =====================================================
void loop() {
  unsigned long now = millis();

  // --- Baca semua sensor (SELALU, 100ms) ---
  if (now - tSensor >= 100) {
    tSensor = now;
    jarak = bacaJarak();
    pir   = digitalRead(PIN_PIR) == HIGH;
    pot   = analogRead(PIN_POT);
  }

  // --- Tentukan mode dari potensio ---
  if (pot <= POT_BATAS_KIRI) {
    mode = MODE_MANUAL_TUTUP;
  } else if (pot >= POT_BATAS_KANAN) {
    mode = MODE_MANUAL_BUKA;
  } else {
    mode = MODE_AUTO;
  }

  // --- Deteksi pergantian mode ---
  if (mode != modeLama) {
    Serial.print("[MODE] ");
    if (mode == MODE_MANUAL_TUTUP) Serial.println("MANUAL TUTUP");
    else if (mode == MODE_MANUAL_BUKA) Serial.println("MANUAL BUKA");
    else {
      Serial.println("AUTO");
      autoState = AUTO_IDLE;  // Reset state auto saat masuk
    }
    modeLama = mode;
  }

  // --- Jalankan sesuai mode ---
  if (mode == MODE_AUTO) {
    jalankanAuto();
  } else if (mode == MODE_MANUAL_BUKA) {
    jalankanManualBuka();
  } else {
    jalankanManualTutup();
  }

  // --- Update servo (non-blocking) ---
  gerakServo();

  // --- Update display (200ms) ---
  if (now - tDisplay >= 200) {
    tDisplay = now;
    tampilOLED();
  }

  // --- Debug serial (1 detik) ---
  if (now - tDebug >= 1000) {
    tDebug = now;
    Serial.print("Jrk:");
    Serial.print(jarak, 1);
    Serial.print(" PIR:");
    Serial.print(pir ? "Y" : "N");
    Serial.print(" Pot:");
    Serial.print(pot);
    Serial.print(" Sudut:");
    Serial.print(sudutNow);
    Serial.print(" Mode:");
    if (mode == MODE_AUTO) {
      Serial.print("AUTO(");
      Serial.print(autoState);
      Serial.print(")");
    } else if (mode == MODE_MANUAL_BUKA) {
      Serial.print("M-BUKA");
    } else {
      Serial.print("M-TUTUP");
    }
    Serial.println();
  }
}

// =====================================================
// MODE AUTO — State machine lengkap
// =====================================================
void jalankanAuto() {
  switch (autoState) {

    case AUTO_IDLE:
      sudutTarget = SUDUT_TUTUP;
      digitalWrite(PIN_LED_RED, HIGH);
      digitalWrite(PIN_LED_GREEN, LOW);

      if (jarak < JARAK_DEKAT && jarak > 2) {
        autoState = AUTO_KONFIRMASI;
        tState = millis();
        Serial.print("[AUTO] Objek terdeteksi: ");
        Serial.print(jarak, 1);
        Serial.println("cm");
      }
      break;

    case AUTO_KONFIRMASI: {
      // Kedip LED merah
      bool blink = ((millis() / 200) % 2) == 0;
      digitalWrite(PIN_LED_RED, blink);

      if (millis() - tState >= WAKTU_KONFIRM) {
        if (jarak < JARAK_DEKAT && jarak > 2) {
          totalMobil++;
          Serial.print("[AUTO] Terkonfirmasi! Mobil ke-");
          Serial.print(totalMobil);
          Serial.print(" PIR:");
          Serial.println(pir ? "YA" : "TIDAK");
          buzz(1, 150);
          autoState = AUTO_BUKA;
        } else {
          Serial.println("[AUTO] Batal, objek pergi.");
          autoState = AUTO_IDLE;
        }
      }
      break;
    }

    case AUTO_BUKA:
      sudutTarget = SUDUT_BUKA;
      digitalWrite(PIN_LED_GREEN, HIGH);
      digitalWrite(PIN_LED_RED, LOW);

      if (sudutNow == SUDUT_BUKA) {
        Serial.println("[AUTO] Palang terbuka.");
        buzz(2, 80);
        autoState = AUTO_TERBUKA;
        tState = millis();
      }
      break;

    case AUTO_TERBUKA:
      digitalWrite(PIN_LED_GREEN, HIGH);
      digitalWrite(PIN_LED_RED, LOW);

      // Mobil sudah lewat?
      if (jarak > JARAK_JAUH) {
        Serial.println("[AUTO] Mobil lewat.");
        autoState = AUTO_JEDA;
        tState = millis();
      }

      // Timeout
      if (millis() - tState > WAKTU_TIMEOUT) {
        Serial.println("[AUTO] Timeout, tutup.");
        autoState = AUTO_JEDA;
        tState = millis();
      }
      break;

    case AUTO_JEDA: {
      // Kedip hijau
      bool blink = ((millis() / 300) % 2) == 0;
      digitalWrite(PIN_LED_GREEN, blink);

      if (millis() - tState >= WAKTU_JEDA) {
        // Safety: cek lagi ada objek nggak
        if (jarak < JARAK_DEKAT && jarak > 2) {
          Serial.println("[SAFETY] Objek baru! Batal tutup.");
          buzz(3, 50);
          autoState = AUTO_TERBUKA;
          tState = millis();
        } else {
          autoState = AUTO_TUTUP;
        }
      }
      break;
    }

    case AUTO_TUTUP:
      // Safety selama menutup
      if (jarak < JARAK_DEKAT && jarak > 2) {
        Serial.println("[SAFETY] Objek saat menutup! Buka lagi!");
        buzz(3, 50);
        sudutTarget = SUDUT_BUKA;
        autoState = AUTO_TERBUKA;
        tState = millis();
        break;
      }

      sudutTarget = SUDUT_TUTUP;
      digitalWrite(PIN_LED_RED, HIGH);
      digitalWrite(PIN_LED_GREEN, LOW);

      if (sudutNow == SUDUT_TUTUP) {
        Serial.println("[AUTO] Palang tertutup.");
        buzz(1, 200);
        autoState = AUTO_IDLE;
      }
      break;
  }
}

// =====================================================
// MODE MANUAL BUKA — Simpel, langsung buka
// =====================================================
void jalankanManualBuka() {
  sudutTarget = SUDUT_BUKA;
  digitalWrite(PIN_LED_GREEN, HIGH);
  digitalWrite(PIN_LED_RED, LOW);
}

// =====================================================
// MODE MANUAL TUTUP — Simpel, langsung tutup
// =====================================================
void jalankanManualTutup() {
  // Safety: jangan tutup kalau ada objek dekat
  if (jarak < JARAK_DEKAT && jarak > 2) {
    // Kedip merah = warning
    bool blink = ((millis() / 150) % 2) == 0;
    digitalWrite(PIN_LED_RED, blink);
    digitalWrite(PIN_LED_GREEN, LOW);
    // Jangan gerakkan servo ke tutup
    return;
  }

  sudutTarget = SUDUT_TUTUP;
  digitalWrite(PIN_LED_RED, HIGH);
  digitalWrite(PIN_LED_GREEN, LOW);
}

// =====================================================
// SERVO: Non-blocking, 1 derajat per 15ms
// =====================================================
void gerakServo() {
  if (sudutNow == sudutTarget) return;
  if (millis() - tServo < 15) return;
  tServo = millis();

  if (sudutNow < sudutTarget) sudutNow++;
  else sudutNow--;

  servo.write(sudutNow);
}

// =====================================================
// ULTRASONIK
// =====================================================
float bacaJarak() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  long d = pulseIn(PIN_ECHO, HIGH, 25000);
  if (d == 0) return 999.0;
  return (d * 0.0343) / 2.0;
}

// =====================================================
// BUZZER (blocking singkat — max 300ms, acceptable)
// =====================================================
void buzz(int kali, int ms) {
  for (int i = 0; i < kali; i++) {
    digitalWrite(PIN_BUZZER, HIGH);
    delay(ms);
    digitalWrite(PIN_BUZZER, LOW);
    if (i < kali - 1) delay(ms / 2);
  }
}

// =====================================================
// OLED DISPLAY
// =====================================================
void tampilOLED() {
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);

  // === Baris 1: Mode ===
  oled.setCursor(0, 0);
  if (mode == MODE_MANUAL_TUTUP) {
    oled.print("MODE: MANUAL TUTUP");
  } else if (mode == MODE_MANUAL_BUKA) {
    oled.print("MODE: MANUAL BUKA");
  } else {
    oled.print("MODE: AUTO - ");
    switch (autoState) {
      case AUTO_IDLE:       oled.print("IDLE");   break;
      case AUTO_KONFIRMASI: oled.print("CEK..");   break;
      case AUTO_BUKA:       oled.print("BUKA");    break;
      case AUTO_TERBUKA:    oled.print("OPEN");    break;
      case AUTO_JEDA:       oled.print("JEDA");    break;
      case AUTO_TUTUP:      oled.print("TUTUP");   break;
    }
  }

  oled.drawLine(0, 10, 128, 10, SSD1306_WHITE);

  // === Baris 2: Sensor ===
  oled.setCursor(0, 14);
  oled.print("Jrk:");
  if (jarak > 400) oled.print("---");
  else { oled.print(jarak, 0); oled.print("cm"); }

  oled.setCursor(72, 14);
  oled.print("PIR:");
  oled.print(pir ? "YA!" : " -- ");

  // === Baris 3: Servo + counter ===
  oled.setCursor(0, 26);
  oled.print("Palang:");
  oled.print(sudutNow);
  oled.print((char)247);

  oled.setCursor(72, 26);
  oled.print("Mbl:");
  oled.print(totalMobil);

  // === Baris 4: Potensio visual ===
  oled.drawRect(0, 38, 128, 10, SSD1306_WHITE);

  // Bar fill
  int barW = map(pot, 0, 4095, 0, 126);
  oled.fillRect(1, 39, barW, 8, SSD1306_WHITE);

  // Garis batas zona
  int lineKiri = map(POT_BATAS_KIRI, 0, 4095, 0, 126);
  int lineKanan = map(POT_BATAS_KANAN, 0, 4095, 0, 126);
  oled.drawLine(lineKiri, 38, lineKiri, 48, SSD1306_WHITE);
  oled.drawLine(lineKanan, 38, lineKanan, 48, SSD1306_WHITE);

  // Label zona
  oled.setCursor(2, 50);
  oled.print("TUTUP");
  oled.setCursor(48, 50);
  oled.print("AUTO");
  oled.setCursor(96, 50);
  oled.print("BUKA");

  // === Baris 5: Pesan ===
  oled.drawLine(0, 58, 128, 58, SSD1306_WHITE);
  oled.setCursor(0, 59);

  if (mode == MODE_MANUAL_BUKA) {
    oled.print("Palang dibuka manual");
  } else if (mode == MODE_MANUAL_TUTUP) {
    if (jarak < JARAK_DEKAT && jarak > 2) {
      oled.print("OBJEK! Tdk bisa tutup");
    } else {
      oled.print("Palang ditutup manual");
    }
  } else {
    switch (autoState) {
      case AUTO_IDLE:       oled.print("Menunggu kendaraan"); break;
      case AUTO_KONFIRMASI: oled.print("Konfirmasi...");      break;
      case AUTO_BUKA:       oled.print("Silakan masuk >>");   break;
      case AUTO_TERBUKA: {
        unsigned long e = millis() - tState;
        unsigned long r = (e < WAKTU_TIMEOUT) ? (WAKTU_TIMEOUT - e) / 1000 : 0;
        oled.print("Lewat dlm ");
        oled.print(r);
        oled.print("s");
        break;
      }
      case AUTO_JEDA:  oled.print("Jeda keamanan...");  break;
      case AUTO_TUTUP: oled.print("Menutup palang...");  break;
    }
  }

  oled.display();
}
