#include <iarduino_RTC.h>
#include <Servo.h>

// Кнопка внеочередной выдачи порции
#define BUTTON 2

// Контакты часов
#define CLK 3 // SCLK
#define DAT 4 // I/O
#define RST 5 //CE

// Управление сервоприводом
#define SERVO 6

// Переменные для настройки
uint32_t feedingFrequencySeconds = 43200;  // периодичность в часах, с которой надо кормить собаку - примерно 1 минута
uint32_t feedingPeriodSeconds = 2;              // время в секундах, которое нужно подавать корм для собаки
int servoSpeed = 50;                            // скорость вращения сервопривода
float delayFromStartToFeedingHours = 0;         // Время в часах, которое надо выждать с момента запуска Ардуино до момента первого кормления
int timeShowingPeriod = 1;

// Служебные переменные
Servo servo;                        // Объект, который представляет сервопривод MG 996R
int servoStopValue = 90;            // Значение, которое надо передать сервоприводу, чтобы он остановился
unsigned long periodOfRestShowing;  // periodOfRestShowi6ngSeconds в миллисекундах
iarduino_RTC clock(RTC_DS1302, RST, CLK, DAT);
uint32_t stateChangingLastTime;
unsigned long timeToShowTime;

volatile bool buttonClicked;
volatile uint32_t debounceTime;  // Время предыдущего срабатывания прерывания - от дребезга контактов
bool servoOn = false;            // Текущее состояние сервопривода
int buttonClickedTimes = 0;

void setup() {
  Serial.begin(9600);
  clock.begin();
  pinMode(BUTTON, INPUT_PULLUP);
  servo.attach(SERVO);

  buttonClicked = false;
  attachInterrupt(0, interrupt, RISING);
  timeToShowTime = millis() + timeShowingPeriod * 1000;

  stateChangingLastTime = clock.gettimeUnix();
}

void loop() {
  uint32_t nowTime = clock.gettimeUnix();

  if(buttonClicked) {
    buttonClickedTimes++;
  }

 if (millis() >= timeToShowTime) {
    Serial.print(clock.gettime("d m Y, H:i:s, D"));
    timeToShowTime = timeToShowTime + timeShowingPeriod * 1000;
    Serial.print(" ");
    Serial.print(nowTime);
    Serial.print(" Next running: ");
    Serial.print(stateChangingLastTime + feedingFrequencySeconds);
    Serial.println(" button clicked " + String(buttonClickedTimes) + " times.");
  }

  bool itsTimeToStartFeeding = !servoOn && ((nowTime >= (stateChangingLastTime + feedingFrequencySeconds)) || buttonClicked);
  bool itsTimeToStopFeeding = servoOn && ((nowTime >= (stateChangingLastTime + feedingPeriodSeconds)) || buttonClicked);

  if (itsTimeToStartFeeding) {
    servoOn = true;
    Serial.println("Motor is on, it'll be stop in " + String(feedingPeriodSeconds) + " seconds.");

    stateChangingLastTime = clock.gettimeUnix();
  } else if (itsTimeToStopFeeding) {
    servoOn = false;

    Serial.println("Motor is off, it'll be start in " + String(feedingFrequencySeconds) + " seconds");

    stateChangingLastTime = clock.gettimeUnix();
  } else {
    return;
  }

  // Работа двигателя, пока идет время кормежки, и простой в остальное время
  if (servoOn) {
    servo.write(servoSpeed);
  } else {
    servo.write(servoStopValue);
  }

  buttonClicked = false;
}

void interrupt() {
  if (millis() - debounceTime >= 500 && digitalRead(BUTTON)) {
    Serial.println("You've clicked the button.");
    debounceTime = millis();
    buttonClicked = true;
  }
}
