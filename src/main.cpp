#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <qrcode.h>
#include <Wire.h>

// Используем стандартный I2C (GPIO21=SDA, GPIO22=SCL)
#define OLED_I2C_ADDR 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Проверка наличия OLED
bool oledDetected = false;

void oled_init() {
    Wire.begin();
    Wire.beginTransmission(OLED_I2C_ADDR);
    oledDetected = (Wire.endTransmission() == 0);
}

void oled_clear() {
    if (!oledDetected) return;
    Wire.beginTransmission(OLED_I2C_ADDR);
    Wire.write(0x00); // Команда
    Wire.write(0xAE); // Display OFF
    Wire.write(0xD5); Wire.write(0x80); // Clock
    Wire.write(0xA8); Wire.write(0x3F); // MUX
    Wire.write(0xD3); Wire.write(0x00); // Offset
    Wire.write(0x40); // Start line
    Wire.write(0x8D); Wire.write(0x14); // Charge pump
    Wire.write(0x20); Wire.write(0x00); // Memory mode
    Wire.write(0xA1); // Segment remap
    Wire.write(0xC8); // COM scan
    Wire.write(0xDA); Wire.write(0x12); // COM pins
    Wire.write(0x81); Wire.write(0xCF); // Contrast
    Wire.write(0xD9); Wire.write(0xF1); // Precharge
    Wire.write(0xDB); Wire.write(0x40); // VCOMH
    Wire.write(0xA4); // Entire display ON
    Wire.write(0xA6); // Normal display
    Wire.write(0xAF); // Display ON
    Wire.endTransmission();
}

void oled_fill(bool black) {
    if (!oledDetected) return;
    for (int page = 0; page < 8; page++) {
        Wire.beginTransmission(OLED_I2C_ADDR);
        Wire.write(0x40);
        for (int i = 0; i < 128; i++) {
            Wire.write(black ? 0x00 : 0xFF);
        }
        Wire.endTransmission();
    }
}

void oled_draw_char(int x, int y, char c) {
    // Упрощённый вывод (только для "Privet!")
    // В реальности лучше использовать готовую библиотеку, но для совместимости — минимальный код
}

void oled_print(const char* text) {
    if (!oledDetected) return;
    oled_fill(true); // Очистка

    // Вывод текста "Privet!" по центру
    Wire.beginTransmission(OLED_I2C_ADDR);
    Wire.write(0x00);
    Wire.write(0xB0 + 3); // Страница 3
    Wire.write(0x00 + 32); // Низкий байт колонки
    Wire.write(0x10 + 0);  // Высокий байт колонки
    Wire.endTransmission();

    Wire.beginTransmission(OLED_I2C_ADDR);
    Wire.write(0x40);
    const char* msg = text;
    while (*msg) {
        // Простая замена: выводим прямоугольник вместо букв
        for (int i = 0; i < 6; i++) Wire.write(0x7E); // "P"
        for (int i = 0; i < 6; i++) Wire.write(0x7F); // "r"
        for (int i = 0; i < 6; i++) Wire.write(0x7F); // "i"
        for (int i = 0; i < 6; i++) Wire.write(0x76); // "v"
        for (int i = 0; i < 6; i++) Wire.write(0x65); // "e"
        for (int i = 0; i < 6; i++) Wire.write(0x74); // "t"
        for (int i = 0; i < 6; i++) Wire.write(0x21); // "!"
        break;
    }
    Wire.endTransmission();
}

void oled_draw_qr(const uint8_t* qrcode, int size) {
    if (!oledDetected) return;
    int scale = 2;
    int offset_x = (128 - size * scale) / 2;
    int offset_y = (64 - size * scale) / 2;

    for (int page = 0; page < 8; page++) {
        Wire.beginTransmission(OLED_I2C_ADDR);
        Wire.write(0x00);
        Wire.write(0xB0 + page);
        Wire.write(offset_x & 0x0F);
        Wire.write(0x10 | (offset_x >> 4));
        Wire.endTransmission();

        Wire.beginTransmission(OLED_I2C_ADDR);
        Wire.write(0x40);
        for (int x = 0; x < 128; x++) {
            uint8_t byte = 0;
            for (int bit = 0; bit < 8; bit++) {
                int y = page * 8 + bit;
                int qr_x = (x - offset_x) / scale;
                int qr_y = (y - offset_y) / scale;
                if (qr_x >= 0 && qr_x < size && qr_y >= 0 && qr_y < size) {
                    if (qrcode_getModule(qr, qr_x, qr_y)) {
                        byte |= (0x80 >> bit);
                    }
                }
            }
            Wire.write(byte);
        }
        Wire.endTransmission();
    }
}

// BLE
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

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Starting QR Server...");

    // Инициализация OLED
    oled_init();
    if (oledDetected) {
        oled_clear();
        oled_print("Privet!");
        delay(2000);
        oled_fill(true); // Очистка перед QR
    } else {
        Serial.println("OLED not found!");
    }

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
    if (qrNeedsUpdate && oledDetected) {
        QRCode qrcode;
        uint8_t qrcodeBytes[qrcode_getBufferSize(3)];
        qrcode_initText(&qrcode, qrcodeBytes, 3, ECC_LOW, currentText.c_str());
        oled_draw_qr(qrcodeBytes, qrcode.size);
        qrNeedsUpdate = false;
    }
    delay(50);
}
