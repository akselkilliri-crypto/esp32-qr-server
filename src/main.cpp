#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <qrcode.h>

// Правильная инициализация GyverOLED для I2C (128x64, с буфером)
#include <GyverOLED.h>
GyverOLED<SSD1306_128x64> oled;  // Автоматически использует I2C на GPIO21/22

#define SERVICE_UUID        "a2b3c4d5-e6f7-8901-2345-6789abcdef01"
#define CHARACTERISTIC_UUID "b3c4d5e6-f7a8-9012-3456-789abcdef01"

String currentText = "Hello";
bool qrNeedsUpdate = true;

class TextReceiverCallback : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pChar) {
        std::string value = pChar->getValue();
        if (!value.empty()) {
            if (value.length() > 150) value.resize(150);
            currentText = value.c_str();
            qrNeedsUpdate = true;
            Serial.println("Received: " + currentText);
        }
    }
};

void drawQRToOLED() {
    if (!qrNeedsUpdate) return;
    oled.clear();
    
    QRCode qrcode;
    uint8_t qrcodeBytes[qrcode_getBufferSize(3)];
    qrcode_initText(&qrcode, qrcodeBytes, 3, ECC_LOW, currentText.c_str());
    
    uint8_t scale = 2;
    uint8_t offset_x = (128 - qrcode.size * scale) / 2;
    uint8_t offset_y = (64 - qrcode.size * scale) / 2;
    
    for (uint8_t y = 0; y < qrcode.size; y++) {
        for (uint8_t x = 0; x < qrcode.size; x++) {
            if (qrcode_getModule(&qrcode, x, y)) {
                for (uint8_t dy = 0; dy < scale; dy++) {
                    for (uint8_t dx = 0; dx < scale; dx++) {
                        oled.dot(offset_x + x * scale + dx, offset_y + y * scale + dy);
                    }
                }
            }
        }
    }
    
    oled.update();
    qrNeedsUpdate = false;
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // Инициализация OLED (автоматически на I2C: SDA=21, SCL=22)
    if (!oled.init()) {
        Serial.println("OLED init failed!");
        while (1) delay(10);
    }
    
    // Приветствие
    oled.clear();
    oled.setCursor(20, 25);
    oled.print("Привет!");
    oled.update();
    delay(2000);

    // BLE-сервер
    NimBLEDevice::init("ESP32 QR Server");
    NimBLEServer* server = NimBLEDevice::createServer();
    NimBLEService* service = server->createService(SERVICE_UUID);
    NimBLECharacteristic* textChar = service->createCharacteristic(
        CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ
    );
    textChar->setCallbacks(new TextReceiverCallback());
    textChar->setValue(currentText.c_str());
    service->start();
    
    NimBLEDevice::getAdvertising()->addServiceUUID(SERVICE_UUID);
    NimBLEDevice::getAdvertising()->start();
    
    Serial.println("BLE server ready");
}

void loop() {
    drawQRToOLED();
    delay(50);
}
