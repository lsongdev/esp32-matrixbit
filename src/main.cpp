#include <Arduino.h>
#include <Wire.h>

#include "matrixbit.h"

namespace {

void scanI2C()
{
  Serial.println("Scanning I2C bus...");

  unsigned int found = 0;

  for (uint8_t address = 1; address < 0x7f; ++address) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      Serial.printf("  found 0x%02X\n", address);
      ++found;
    }
  }

  Serial.printf("I2C scan complete: %u device(s) found\n", found);
}

void printButtons(int a, int b)
{
  Serial.printf(
    "buttons: A=%s B=%s\n",
    a == LOW ? "pressed" : "released",
    b == LOW ? "pressed" : "released"
  );
}

} // namespace

void setup()
{
  Serial.begin(115200);
  delay(300);

  pinMode(matrixbit::pin::button_a, INPUT_PULLUP);
  pinMode(matrixbit::pin::button_b, INPUT_PULLUP);

  Wire.begin(matrixbit::pin::i2c_sda, matrixbit::pin::i2c_scl);

  Serial.println();
  Serial.println("Matrix:bit / PlatformIO");
  Serial.printf(
    "I2C: SDA=%u SCL=%u\n",
    matrixbit::pin::i2c_sda,
    matrixbit::pin::i2c_scl
  );

  scanI2C();
  printButtons(
    digitalRead(matrixbit::pin::button_a),
    digitalRead(matrixbit::pin::button_b)
  );
}

void loop()
{
  static int lastA = HIGH;
  static int lastB = HIGH;

  const int a = digitalRead(matrixbit::pin::button_a);
  const int b = digitalRead(matrixbit::pin::button_b);

  if (a != lastA || b != lastB) {
    printButtons(a, b);
    lastA = a;
    lastB = b;
  }

  delay(10);
}
