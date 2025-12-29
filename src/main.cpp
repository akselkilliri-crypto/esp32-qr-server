#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <qrcode.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define SERVICE_UUID        "a2b3c4d5-e6f7-8901-2345-6789abcdef01"
#define CHARACTERISTIC_UUID "b3c4d5e6-f7a8-9012-3456-789abcdef01"

String currentText = "Scan me";
bool qrNeedsUpdate = true;

class TextReceiverCallback : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pChar) {
        std::string value = pChar->getValue();
        if (!value.empty()) {
            if (value.length() > 200) value.resize(200);
            currentText = value.c_str();
            qrNeedsUpdate = true;
            Serial.println("Received: " + currentText);
        }
    }
};

void updateQRDisplay() {
    if (!qrNeedsUpdate) return;
    display.clearDisplay();
    QRCode qrcode;
    uint8_t qrcodeBytes[qrcode_getBufferSize(3)];
    qrcode_initText(&qrcode, qrcodeBytes, 3, ECC_LOW, currentText.c_str());
    uint8_t scale = 3;
    uint8_t offset = (SCREEN_WIDTH - qrcode.size * scale) / 2;
    for (uint8_t y = 0; y < qrcode.size; y++) {
        for (uint8_t x = 0; x < qrcode.size; x++) {
            if (qrcode_getModule(&qrcode, x, y)) {
                display.fillRect(offset + x * scale, 10 + y * scale, scale, scale, WHITE);
            }
        }
    }
    display.display();
    qrNeedsUpdate = false;
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        while (1) delay(10);
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 25);
    display.println("QR Server Ready");
    display.display();

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
}

void loop() {
    updateQRDisplay();
    delay(50);
}
