#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

const char* ssid = "pukimak";
const char* password = "kimakkau";

// Inisialisasi bot token
#define BOTtoken "7226021988:AAFPemI-gFrt_akK_9w0NmGU8FOkPjJW460"

// Chat ID dari MyIDBot
#define CHAT_ID "1183092826"

WiFiClientSecure client; 
UniversalTelegramBot bot(BOTtoken, client);

int botRequestDelay = 1000;
unsigned long lastTimeBotRan;

LiquidCrystal_I2C lcd(0x27, 16, 2);  

Servo servoPemilah;
Servo servoPenutup;

const int inductiveSensorPin = 2;  

int ultrasonicTriggerPin = 12;  // Pin trigger sensor ultrasonik untuk mendeteksi sampah non-logam
int ultrasonicEchoPin = 14;  // Pin echo sensor ultrasonik untuk mendeteksi sampah non-logam

int ultrasonicTriggerPinKapasitasLogam = 33;  // Pin trigger sensor ultrasonik untuk kapasitas sampah logam
int ultrasonicEchoPinKapasitasLogam = 32;  // Pin echo sensor ultrasonik untuk kapasitas sampah logam

int ultrasonicTriggerPinKapasitasNonLogam = 18;  // Pin trigger sensor ultrasonik untuk kapasitas sampah non-logam
int ultrasonicEchoPinKapasitasNonLogam = 19;  // Pin echo sensor ultrasonik untuk kapasitas sampah non-logam

int sensorValue = 0;

const int servoPemilahPin = 5;
const int servoPenutupPin = 23;

void setup() {
    Serial.begin(115200); 

    Wire.begin(21, 22);  // Inisialisasi I2C dengan pin SDA di GPIO 21 dan pin SCL di GPIO 22
    lcd.init();  // Inisialisasi LCD
    lcd.backlight();  // Nyalakan backlight LCD
    pinMode(inductiveSensorPin, INPUT);
    pinMode(ultrasonicTriggerPin, OUTPUT);  
    pinMode(ultrasonicEchoPin, INPUT); 
    pinMode(ultrasonicTriggerPinKapasitasLogam, OUTPUT);  
    pinMode(ultrasonicEchoPinKapasitasLogam, INPUT); 
    pinMode(ultrasonicTriggerPinKapasitasNonLogam, OUTPUT);  
    pinMode(ultrasonicEchoPinKapasitasNonLogam, INPUT); 

    // Inisialisasi koneksi WiFi
    WiFi.begin(ssid, password);

    // Tunggu hingga ESP32 terhubung ke WiFi
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }

    // Jika berhasil terhubung
    Serial.println("Connected to WiFi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    client.setInsecure();  

    // Menampilkan pesan selamat datang
    lcd.setCursor(0, 0);
    lcd.print(" Pemilah Sampah ");
    lcd.setCursor(0, 1);
    lcd.print("Logam & NonLogam");
    delay(2000);
    lcd.clear();  

    // Menampilkan pesan "Masukkan Sampah" ketika IDLE
    lcd.setCursor(0, 0);
    lcd.print(" Masukkan Sampah");
    lcd.setCursor(0, 1);
    lcd.print("      Anda      ");

    // Servo
    servoPemilah.attach(servoPemilahPin);
    servoPemilah.write(90);  // Posisi default servo
    
    servoPenutup.attach(servoPenutupPin);
    servoPenutup.write(0);  // Posisi default servo
}

void loop() {
    // Cek koneksi WiFi
    if (WiFi.status() != WL_CONNECTED) {
        while (WiFi.status() != WL_CONNECTED) {
            delay(1000);
            Serial.println("Reconnecting to WiFi...");
        }
        Serial.println("Reconnected to WiFi");
    }

    // Ambil pesan baru dari bot
    if (millis() > lastTimeBotRan + botRequestDelay) {
        int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
        handleNewMessages(numNewMessages);
        lastTimeBotRan = millis();
    }

    // Membaca nilai dari sensor proximity induktif
    sensorValue = digitalRead(inductiveSensorPin);

    // Cek jarak menggunakan sensor ultrasonik untuk sampah non-logam
    float nonLogamDistance = getUltrasonicDistance(ultrasonicTriggerPin, ultrasonicEchoPin);
    float kapasitasDistanceLogam = getUltrasonicDistance(ultrasonicTriggerPinKapasitasLogam, ultrasonicEchoPinKapasitasLogam);
    float kapasitasDistanceNonLogam = getUltrasonicDistance(ultrasonicTriggerPinKapasitasNonLogam, ultrasonicEchoPinKapasitasNonLogam);

    // Reset detection flags
    bool metalDetected = (sensorValue == HIGH);
    bool nonMetalDetected = (nonLogamDistance <= 5);
    bool kapasitasPenuhLogam = (kapasitasDistanceLogam <= 27);
    bool kapasitasPenuhNonLogam = (kapasitasDistanceNonLogam <= 27);
    bool kapasitasSetengahLogam = (kapasitasDistanceLogam >= 27 || kapasitasDistanceLogam <= 35);
    bool kapasitasSetengahNonLogam = (kapasitasDistanceNonLogam >= 27 || kapasitasDistanceNonLogam <= 35);
    
    // Prioritize metal detection over non-metal  
    if (!metalDetected && nonMetalDetected) {
        // Jika mendeteksi logam
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("  Jenis Sampah  ");  
        lcd.setCursor(0, 1);
        lcd.print("     Logam!     ");
        
        // Buka servo logam
        delay(100);  
        servoPemilah.write(180);
        delay(2000);  
        servoPemilah.write(90);
        delay(2000);  
        
    } else if (!metalDetected) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("  Jenis Sampah  ");
        lcd.setCursor(0, 1);
        lcd.print("     Logam!     ");

        // Buka servo logam
        delay(100); 
        servoPemilah.write(180);
        delay(2000); 
        servoPemilah.write(90);
        delay(2000); 

    } else if (nonMetalDetected) {
        // Jika mendeteksi non-logam
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("  Jenis Sampah  ");
        lcd.setCursor(0, 1);
        lcd.print("   Non Logam!   ");

        // Buka servo non-logam
        delay(100); 
        servoPemilah.write(0);
        delay(2000); 
        servoPemilah.write(90);
        delay(2000); 

    } else if (kapasitasPenuhLogam || kapasitasPenuhNonLogam) {
        // Jika mendeteksi non-logam
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Kapasitas Sampah");
        lcd.setCursor(0, 1);
        lcd.print("     Penuh!     ");

        delay(100); 
        servoPenutup.write(90);
        delay(2000); 

    } else {
        // Menampilkan pesan "Masukkan Sampah" ketika tidak ada deteksi
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(" Masukkan Sampah");
        lcd.setCursor(0, 1);
        lcd.print("      Anda      ");
        servoPemilah.write(90);
        servoPenutup.write(0);
    }
    delay(3000);  // Delay untuk stabilisasi
}

// Fungsi untuk mendapatkan jarak dari sensor ultrasonik
float getUltrasonicDistance(int triggerPin, int echoPin) {
    digitalWrite(triggerPin, LOW);
    delayMicroseconds(2);
    digitalWrite(triggerPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(triggerPin, LOW);
    
    return pulseIn(echoPin, HIGH) / 58.0; // Convert pulse width to centimeters
}

void handleNewMessages(int numNewMessages) {

    // Membaca nilai dari sensor proximity induktif
    sensorValue = digitalRead(inductiveSensorPin);

    float TelenonLogamDistance = getUltrasonicDistance(ultrasonicTriggerPin, ultrasonicEchoPin);
    float TelekapasitasDistanceLogam = getUltrasonicDistance(ultrasonicTriggerPinKapasitasLogam, ultrasonicEchoPinKapasitasLogam);
    float TelekapasitasDistanceNonLogam = getUltrasonicDistance(ultrasonicTriggerPinKapasitasNonLogam, ultrasonicEchoPinKapasitasNonLogam);

    // Reset detection flags
    bool TelemetalDetected = (sensorValue == HIGH);
    bool TelenonMetalDetected = (TelenonLogamDistance <= 5);
    bool TelekapasitasPenuhLogam = (TelekapasitasDistanceLogam <= 27);
    bool TelekapasitasPenuhNonLogam = (TelekapasitasDistanceNonLogam <= 27);
    bool TelekapasitasSetengahLogam = (TelekapasitasDistanceLogam >= 27 || TelekapasitasDistanceLogam <= 35);
    bool TelekapasitasSetengahNonLogam = (TelekapasitasDistanceNonLogam >= 27 || TelekapasitasDistanceNonLogam <= 35 );

    Serial.println("HandleNewMessages");
    Serial.println(String(numNewMessages));
    for (int i = 0; i < numNewMessages; i++) {
        String chat_id = String(bot.messages[i].chat_id);
        if (chat_id != CHAT_ID) {
            bot.sendMessage(chat_id, "Unauthorized user", "");
            continue;
        }
        String text = bot.messages[i].text;
        Serial.println(text);

        String from_name = bot.messages[i].from_name;
        if (text == "/start") {
            String control = "Selamat Datang, " + from_name + ".\n";
            control += "Gunakan Commands Di Bawah Untuk Control Pemilah Sampah Logam dan Non Logam.\n\n";
            control += "/buka Untuk Membuka penutup tempat Sampah\n";
            control += "/tutup Untuk Menutup penutup tempat Sampah\n";
            control += "/Status Untuk Cek Status Led Saat Ini \n";
            bot.sendMessage(chat_id, control, "");
            
        }
        if (text == "/buka") {
          if (TelekapasitasPenuhLogam || TelekapasitasPenuhNonLogam) {
            bot.sendMessage(chat_id, "Penutup tempat sampah dibuka", "");
            delay(100); 
            servoPenutup.write(0);
            delay(2000); 
          } else {
            bot.sendMessage(chat_id, "Penutup sudah terbuka", "");
          }
            
        }
        if (text == "/tutup") {
          if (!TelekapasitasPenuhLogam || !TelekapasitasPenuhNonLogam) {
            bot.sendMessage(chat_id, "Penutup tempat sampah ditutup", "");
            delay(100); 
            servoPenutup.write(90);
            delay(2000); 
          } else {
            bot.sendMessage(chat_id, "Penutup sudah tertutup", "");
          }
        }
        if (text == "/status") {
            if (TelekapasitasSetengahLogam && !TelekapasitasSetengahNonLogam) {
              bot.sendMessage(chat_id, "Kapasitas sampah berisi 50%", "");
          } else if (TelekapasitasPenuhLogam || TelekapasitasPenuhNonLogam) {
              bot.sendMessage(chat_id, "Kapasitas sampah penuh 100%", "");
          } else {
              bot.sendMessage(chat_id, "Kapasitas sampah masih aman", "");
          }
        }
    }
}
