## UAS_mikrokontroller_K1
# SMART PARKING GATE
 Rifqi Ramadhan - 2309106007;
 Al Hajj Fauzan - 2309106019;
 Muhammad Guntur - Adyatma 2309106023;
 Muhammad Shandy - Alfarizal 2309106106
  Sebuah prototipe Smart Parking Gate System berbasis IoT menggunakan mikrokontroler ESP32 DevKit V1 beserta sensor Ultrasonik HC SR04 dan PIR AM312 untuk mendeteksi kendaraan secara real time. Sistem ini menggunakan motor servo sebagai penggerak palang, layar OLED sebagai pemantau lokal, serta potensiometer untuk beralih instan di antara tiga mode operasional yaitu Manual TUTUP pada kisaran 0% sampai 30% dengan fitur anti jepit, AUTO pada 30% sampai 70% secara otomatis lewat sensor, dan Manual BUKA pada 70% sampai 100% tanpa batas waktu.

# Konfigurasi Pin (Pin Mapping)
* **Sensor PIR** ➡️ GPIO 27
* **Buzzer** ➡️ GPIO 13
* **LED Merah** ➡️ GPIO 25
* **LED Hijau** ➡️ GPIO 26
* **Potensiometer** ➡️ GPIO 34 (Analog Input)
* **Layar OLED (SDA)** ➡️ GPIO 21 (I2C Data)
* **Layar OLED (SCL)** ➡️ GPIO 22 (I2C Clock)
* **Sensor Ultrasonik (TRIG)** ➡️ GPIO 5
* **Sensor Ultrasonik (ECHO)** ➡️ GPIO 16
* **Motor Servo** ➡️ GPIO 17

# Foto Prototipe
<img width="1365" height="767" alt="image" src="https://github.com/user-attachments/assets/91db9d40-e01a-4686-8fe9-7cc83c000d30" />
