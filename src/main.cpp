#include <Arduino.h>
#include <GyverOLED.h>

// Используем аппаратный I2C: SDA=21, SCL=22
GyverOLED<SSD1306_128x64, OLED_BUFFER> oled;

void setup() {
    oled.init();          // Инициализация OLED
    oled.clear();         // Очистка экрана
    oled.setCursor(0, 25); // Позиция текста
    oled.print("Hello World"); // Вывод текста
    oled.update();        // Обновление дисплея
}

void loop() {
    // Ничего не делаем — текст остаётся на экране
}
