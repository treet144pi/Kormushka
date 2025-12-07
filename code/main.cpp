#include <Wire.h>
#include <RTClib.h>

RTC_DS3231 rtc;

// Пины A4988
const int STEP_PIN = 3;
const int DIR_PIN = 4;
const int ENABLE_PIN = 5;

// Пины насосов
const int DRAIN_PUMP_PIN = 6;
const int REFILL_PUMP_PIN = 7;

// Датчик воды
const int WATER_SENSOR_PIN = A0;
const int LOW_WATER_LEVEL = 200;
const int TARGET_WATER_LEVEL = 700;

// Расписание кормлений (часы, минуты)
int feedSchedule[][2] = {
  {8, 0},
  {12, 0},
  {18, 0},
  {23, 53}
};
const int numFeedings = sizeof(feedSchedule) / sizeof(feedSchedule[0]);

// Последнее кормление
int lastFeedDay = -1;
int lastFeedHour = -1;
int lastFeedMinute = -1;
bool waterChangeDone = false;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  rtc.begin();

  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);
  pinMode(DRAIN_PUMP_PIN, OUTPUT);
  pinMode(REFILL_PUMP_PIN, OUTPUT);

  digitalWrite(ENABLE_PIN, HIGH); // Отключаем драйвер по умолчанию
  digitalWrite(DRAIN_PUMP_PIN, LOW);
  digitalWrite(REFILL_PUMP_PIN, LOW);

  Serial.println("Система запущена");
}

void loop() {
  DateTime now = rtc.now();
  Serial.print("Текущее время: ");
  Serial.print(now.hour()); Serial.print(":");
  Serial.print(now.minute()); Serial.print(" | ");
  Serial.print("Уровень воды: ");
  Serial.println(analogRead(WATER_SENSOR_PIN));

  for (int i = 0; i < numFeedings; i++) {
    if (now.hour() == feedSchedule[i][0] && now.minute() == feedSchedule[i][1]) {
      if (lastFeedDay != now.day() ||
          lastFeedHour != now.hour() ||
          lastFeedMinute != now.minute()) {

        Serial.println("➡ Кормление начинается");

        // Вращаем мотор (например, 1000 шагов)
        digitalWrite(DIR_PIN, HIGH);       // Направление вращения
        digitalWrite(ENABLE_PIN, LOW);     // Включаем драйвер

        for (int i = 0; i < 1000; i++) {
          digitalWrite(STEP_PIN, HIGH);
          delayMicroseconds(800);
          digitalWrite(STEP_PIN, LOW);
          delayMicroseconds(800);
        }

        digitalWrite(ENABLE_PIN, HIGH);    // Выключаем драйвер

        // Сохраняем время кормления
        lastFeedDay = now.day();
        lastFeedHour = now.hour();
        lastFeedMinute = now.minute();

        waterChangeDone = false;
      }
    }
  }

  // Если не было смены воды после последнего кормления
  if (!waterChangeDone) {
    int waterLevel = analogRead(WATER_SENSOR_PIN);
    if (waterLevel < LOW_WATER_LEVEL) {
      Serial.println("🚰 Меняем воду");

      // Сливаем старую воду
      digitalWrite(DRAIN_PUMP_PIN, HIGH);
      delay(5000);
      digitalWrite(DRAIN_PUMP_PIN, LOW);

      // Наливаем до нужного уровня
      digitalWrite(REFILL_PUMP_PIN, HIGH);
      while (analogRead(WATER_SENSOR_PIN) < TARGET_WATER_LEVEL) {
        delay(200);
      }
      digitalWrite(REFILL_PUMP_PIN, LOW);

      Serial.println("✅ Вода обновлена");
      waterChangeDone = true;
    }
  }

  delay(1000); // Пауза между итерациями
}
