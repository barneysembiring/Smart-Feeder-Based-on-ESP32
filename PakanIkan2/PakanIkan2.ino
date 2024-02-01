#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <ESP32_Servo.h>
#include <BlynkSimpleEsp32.h>

//Template Blynk
#define BLYNK_TEMPLATE_ID "TMPL6iBvzeFnM"
#define BLYNK_TEMPLATE_NAME "Pakan Ikan2"
#define BLYNK_AUTH_TOKEN "hD8ugWIPyTpW-nWr_TrDguh8bTNOP-Sl"
//Mendefenisikan LCD pada Blynk
#define BLYNK_PRINT Serial
//Mendefenisikan Pin Trig dan Echo sensor Ultrasonik
#define trig 19
#define echo 18

long duration;
int distance;

BlynkTimer timer;
WidgetLCD lcd1(V4);

char auth[] = "hD8ugWIPyTpW-nWr_TrDguh8bTNOP-Sl";
char ssid[] = "Hello world";
char pass[] = "gaktausoryya";

// Inisialisasi objek servo dengan pin yang digunakan
Servo servo;

// Sebuah variabel untuk menampung status pakan
int Statuspakan;
bool isFeeding = false;
String previousText = "Fish Feeder";

int servoPin = 2; // Ganti nomor pin sesuai dengan yang digunakan

RTC_DS1307 rtc;
LiquidCrystal_I2C lcd(0x27, 16, 2); // Set the LCD pins

// Feed schedule
const int feedTimes[] = {6, 11, 16};      // Feed times in hours
const float feedMinutes[] = {0, 47, 0};   // Feed minutes
const int numFeedTimes = sizeof(feedTimes) / sizeof(feedTimes[0]);

// Ultrasonic sensor pins
const int trigPin = 19;
const int echoPin = 18;

// Servo operation duration
const int feedDuration = 60000;    // 1 minute

void setup() {
  Serial.begin(9600);
  Wire.begin();

  // RTC setup
  rtc.begin();

  if (!rtc.isrunning()) {
    Serial.println("RTC is not running!");
    // Set the RTC time to the laptop's time if not running
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Synchronize RTC with laptop's time
  rtc.adjust(DateTime(__DATE__, __TIME__));

  // LCD display setup
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Fish Feeder");

  // Ultrasonic sensor setup
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  timer.setInterval(1000L, sendSensor);

  Serial.println("Fish Feeder started");

  // Wifi
  WiFi.begin("Hello world", "gaktausoryya");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("WiFi Connected");

  Blynk.begin("hD8ugWIPyTpW-nWr_TrDguh8bTNOP-Sl", "Hello world", "gaktausoryya");

  Serial.println("Blynk Connected");

  // Attach servo pada pin yang digunakan
  servo.attach(servoPin);
  servo.write(90);

  Blynk.begin(auth, ssid, pass, "blynk.cloud", 80);

  Statuspakan = 0;
}
//Fungsi untuk pergerakan pakan
//Dimana pakan akan bergerak secara posisi 90 ke 0
//Dengan perulangan 5 kali
void Beripakan() {
  for (int i = 0; i < 5; i++) {
    for (int posisi = 90; posisi >= 0; posisi--) {
      servo.write(posisi);
      delay(10);
    }

    // proses pemutaran t4 kembali dari 0 ke 90
    for (int posisi = 0; posisi <= 90; posisi++) {
      servo.write(posisi);
      delay(10);
    }
  }
}

//Fungsi untuk terus mengupdate lcd menampilkan Waktu sesuai Real Time
void updateLCDTime(DateTime time) {
  lcd.setCursor(0, 1);
  lcd.print(time.day(), DEC);
  lcd.print("/");
  lcd.print(time.month(), DEC);
  lcd.print("/");
  lcd.print(time.year() % 100, DEC);
  lcd.print(" ");
  lcd.print(time.hour(), DEC);
  lcd.print(":");
  lcd.print(time.minute(), DEC);
  lcd.print(":");
  if (time.second() < 10) {
    lcd.print("0");  // Add leading zero for seconds less than 10
  }
  lcd.print(time.second(), DEC);
}

void feedFish() {
  // Display message on LCD display
  lcd.setCursor(0, 0);
  lcd.print("Feeding fish    "); // Clear the line

  delay(feedDuration); // Delay for the feeding duration

  // Update LCD display with current time after feeding
  updateLCDTime(rtc.now());
}

void sendSensor() {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);

  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  duration = pulseIn(echo, HIGH);
  distance = duration * 0.034 / 2;

  Serial.print("Distance = ");
  Serial.println(distance);

  Blynk.virtualWrite(V4, distance);

  lcd1.print(0, 0, "Kapasitas Pakan");
  lcd1.print(0, 1, "Distance: " + String(distance) + "cm");
  delay(1000);
}

int measureFoodLevel() {
  // Send ultrasonic pulse
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Read echo duration
  unsigned long duration = pulseIn(echoPin, HIGH);

  // Calculate distance in cm
  int distance = duration * 0.034 / 2;

  return distance;
}

BLYNK_WRITE(V0) {
  // Membaca nilai datastream V0 (status pakan)
  int value = param.asInt();

  // Memperbarui nilai Statuspakan sesuai dengan nilai datastream yang diterima
  Statuspakan = value;
}

void loop() {
  // Get current time from RTC
  DateTime now = rtc.now();
  int currentHour = now.hour();
  int currentMinute = now.minute();

  // Check if it's time to feed
  for (int i = 0; i < numFeedTimes; i++) {
    if (currentHour == feedTimes[i] && currentMinute == feedMinutes[i] && now.second() == 0) {
      updateLCDTime(now);
      feedFish();
      updateLCDTime(now);
    }
  }

  // Update LCD display with RTC time every second
  if (!isFeeding) {
    updateLCDTime(now);
  }

  Blynk.run();

  timer.run();

  // Tampilkan status pakan di Serial monitor
  Serial.println("Status : " + String(Statuspakan));

  // Jika status pakan = 1 dan tidak sedang dalam proses pemberian pakan
 if (Statuspakan == 1 && !isFeeding) {
    isFeeding = true; // Set the feeding status to true
    lcd.setCursor(0, 0);
    lcd.print("Feeding Fish "); // Display "Feeding fish" on LCD
    Beripakan();
    Blynk.virtualWrite(V0, 0);
    Statuspakan = 0;
    isFeeding = false; // Set the feeding status to false after feeding
    delay(10);
    
  }else if (Statuspakan == 0 && !isFeeding) {
    lcd.setCursor(0, 0);
    lcd.print("Fish Feeder"); // Display "Fish Feeder" on LCD
  }

  // Delay for 1 second
  delay(1000);
}
