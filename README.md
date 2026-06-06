## UAS_mikrokontroller_K1
# SMART PARKING GATE


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
