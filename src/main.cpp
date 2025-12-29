#include <Arduino.h>
#include <Wire.h>
#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <qrcode.h>

// OLED I2C
#define OLED_ADDR 0x3C
#define OLED_WIDTH 128
#define OLED_HEIGHT 64

bool oled_found = false;

void oled_init() {
    Wire.begin();
    Wire.beginTransmission(OLED_ADDR);
    oled_found = (Wire.endTransmission() == 0);
}

void oled_command(uint8_t cmd) {
    if (!oled_found) return;
    Wire.beginTransmission(OLED_ADDR);
    Wire.write(0x00); // control byte for command
    Wire.write(cmd);
    Wire.endTransmission();
}

void oled_data_start() {
    if (!oled_found) return;
    Wire.beginTransmission(OLED_ADDR);
    Wire.write(0x40); // control byte for data
}

void oled_data_send(uint8_t data) {
    if (!oled_found) return;
    Wire.write(data);
}

void oled_data_end() {
    if (!oled_found) return;
    Wire.endTransmission();
}

void oled_clear() {
    if (!oled_found) return;
    for (uint8_t page = 0; page < 8; page++) {
        oled_command(0xB0 + page);
        oled_command(0x00);
        oled_command(0x10);
        oled_data_start();
        for (int i = 0; i < 128; i++) {
            oled_data_send(0x00);
        }
        oled_data_end();
    }
}

void oled_init_display() {
    if (!oled_found) return;
    oled_command(0xAE); // Display OFF
    oled_command(0xD5); oled_command(0x80); // Clock
    oled_command(0xA8); oled_command(0x3F); // MUX
    oled_command(0xD3); oled_command(0x00); // Offset
    oled_command(0x40); // Start line
    oled_command(0x8D); oled_command(0x14); // Charge pump
    oled_command(0x20); oled_command(0x00); // Memory mode
    oled_command(0xA1); // Segment remap
    oled_command(0xC8); // COM scan
    oled_command(0xDA); oled_command(0x12); // COM pins
    oled_command(0x81); oled_command(0xCF); // Contrast
    oled_command(0xD9); oled_command(0xF1); // Precharge
    oled_command(0xDB); oled_command(0x40); // VCOMH
    oled_command(0xA4); // Entire display ON
    oled_command(0xA6); // Normal display
    oled_command(0xAF); // Display ON
}

void oled_print_center(const char* text) {
    if (!oled_found) return;
    oled_clear();
    uint8_t len = strlen(text);
    uint8_t start_col = (16 - len) / 2; // 16 chars per line

    oled_command(0xB0 + 3); // page 3
    oled_command(0x00 + (start_col * 8) % 16);
    oled_command(0x10 + (start_col * 8) / 16);

    oled_data_start();
    for (uint8_t i = 0; i < len; i++) {
        // Simple 5x7 font (subset for ASCII 32-95)
        const uint8_t font[] = {
            0x00,0x00,0x00,0x00,0x00, // space
            0x00,0x00,0x5F,0x00,0x00, // !
            0x00,0x07,0x00,0x07,0x00, // "
            0x14,0x7F,0x14,0x7F,0x14, // #
            0x24,0x2A,0x7F,0x2A,0x12, // $
            0x23,0x13,0x08,0x64,0x62, // %
            0x36,0x49,0x55,0x22,0x50, // &
            0x00,0x05,0x03,0x00,0x00, // '
            0x00,0x1C,0x22,0x41,0x00, // (
            0x00,0x41,0x22,0x1C,0x00, // )
            0x14,0x08,0x3E,0x08,0x14, // *
            0x08,0x08,0x3E,0x08,0x08, // +
            0x00,0x50,0x30,0x00,0x00, // ,
            0x08,0x08,0x08,0x08,0x08, // -
            0x00,0x60,0x60,0x00,0x00, // .
            0x20,0x10,0x08,0x04,0x02, // /
            0x3E,0x51,0x49,0x45,0x3E, // 0
            0x00,0x42,0x7F,0x40,0x00, // 1
            0x42,0x61,0x51,0x49,0x46, // 2
            0x21,0x41,0x45,0x4B,0x31, // 3
            0x18,0x14,0x12,0x7F,0x10, // 4
            0x27,0x45,0x45,0x45,0x39, // 5
            0x3C,0x4A,0x49,0x49,0x30, // 6
            0x01,0x71,0x09,0x05,0x03, // 7
            0x36,0x49,0x49,0x49,0x36, // 8
            0x06,0x49,0x49,0x29,0x1E, // 9
            0x00,0x36,0x36,0x00,0x00, // :
            0x00,0x56,0x36,0x00,0x00, // ;
            0x08,0x14,0x22,0x41,0x00, // <
            0x14,0x14,0x14,0x14,0x14, // =
            0x00,0x41,0x22,0x14,0x08, // >
            0x02,0x01,0x51,0x09,0x06, // ?
            0x32,0x49,0x79,0x41,0x3E, // @
            0x7E,0x11,0x11,0x11,0x7E, // A
            0x7F,0x49,0x49,0x49,0x36, // B
            0x3E,0x41,0x41,0x41,0x22, // C
            0x7F,0x41,0x41,0x22,0x1C, // D
            0x7F,0x49,0x49,0x49,0x41, // E
            0x7F,0x09,0x09,0x09,0x01, // F
            0x3E,0x41,0x49,0x49,0x7A, // G
            0x7F,0x08,0x08,0x08,0x7F, // H
            0x00,0x41,0x7F,0x41,0x00, // I
            0x20,0x40,0x41,0x3F,0x01, // J
            0x7F,0x08,0x14,0x22,0x41, // K
            0x7F,0x40,0x40,0x40,0x40, // L
            0x7F,0x02,0x0C,0x02,0x7F, // M
            0x7F,0x04,0x08,0x10,0x7F, // N
            0x3E,0x41,0x41,0x41,0x3E, // O
            0x7F,0x09,0x09,0x09,0x06, // P
            0x3E,0x41,0x51,0x21,0x5E, // Q
            0x7F,0x09,0x19,0x29,0x46, // R
            0x46,0x49,0x49,0x49,0x31, // S
            0x01,0x01,0x7F,0x01,0x01, // T
            0x3F,0x40,0x40,0x40,0x3F, // U
            0x1F,0x20,0x40,0x20,0x1F, // V
            0x3F,0x40,0x38,0x40,0x3F, // W
            0x63,0x14,0x08,0x14,0x63, // X
            0x07,0x08,0x70,0x08,0x07, // Y
            0x61,0x51,0x49,0x45,0x43, // Z
        };

        uint8_t c = text[i];
        if (c < 32 || c > 90) c = 32; // fallback to space
        uint8_t offset = (c - 32) * 5;
        for (uint8_t j = 0; j < 5; j++) {
            oled_data_send(font[offset + j]);
        }
        oled_data_send(0x00); // spacing
    }
    oled_data_end();
}

void oled_draw_qr(QRCode *qrcode) {
    if (!oled_found) return;
    int size = qrcode->size;
    int scale = 2;
    int offset_x = (OLED_WIDTH - size * scale) / 2;
    int offset_y = (OLED_HEIGHT - size * scale) / 2;

    for (uint8_t page = 0; page < 8; page++) {
        oled_command(0xB0 + page);
        oled_command(offset_x & 0x0F);
        oled_command(0x10 | (offset_x >> 4));
        oled_data_start();
        for (int x = 0; x < OLED_WIDTH; x++) {
            uint8_t byte = 0;
            for (int bit = 0; bit < 8; bit++) {
                int y = page * 8 + bit;
                int qr_x = (x - offset_x) / scale;
                int qr_y = (y - offset_y) / scale;
                if (qr_x >= 0 && qr_x < size && qr_y >= 0 && qr_y < size) {
                    if (qrcode_getModule(qrcode, qr_x, qr_y)) {
                        byte |= (0x80 >> bit);
                    }
                }
            }
            oled_data_send(byte);
        }
        oled_data_end();
    }
}

#define SERVICE_UUID        "a2b3c4d5-e6f7-8901-2345-6789abcdef01"
#define CHARACTERISTIC_UUID "b3c4d5e6-f7a8-9012-3456-789abcdef01"

String currentText = "Hello";
bool qrUpdate = true;

class WriteCallback : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* c) {
        std::string v = c->getValue();
        if (!v.empty()) {
            if (v.length() > 100) v.resize(100);
            currentText = v.c_str();
            qrUpdate = true;
            Serial.println("RX: " + currentText);
        }
    }
};

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("QR Server starting...");

    oled_init();
    if (oled_found) {
        oled_init_display();
        oled_print_center("Privet!");
        delay(2000);
        oled_clear();
    } else {
        Serial.println("OLED not found");
    }

    NimBLEDevice::init("ESP32 QR Server");
    NimBLEServer* server = NimBLEDevice::createServer();
    NimBLEService* service = server->createService(SERVICE_UUID);
    NimBLECharacteristic* chr = service->createCharacteristic(
        CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ
    );
    chr->setCallbacks(new WriteCallback());
    chr->setValue("OK");
    service->start();
    NimBLEDevice::startAdvertising();
    NimBLEDevice::getAdvertising()->addServiceUUID(SERVICE_UUID);
    NimBLEDevice::getAdvertising()->start();

    Serial.println("BLE ready");
}

void loop() {
    if (qrUpdate && oled_found) {
        QRCode qrcode;
        uint8_t qrbuff[qrcode_getBufferSize(3)];
        qrcode_initText(&qrcode, qrbuff, 3, ECC_LOW, currentText.c_str());
        oled_draw_qr(&qrcode);
        qrUpdate = false;
    }
    delay(50);
}
