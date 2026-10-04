#include <matrixbit.h>

bool displayReady = false;
bool imuReady = false;
bool magnetometerReady = false;

void setup()
{
  Serial.begin(115200);
  matrixbit::begin();
  matrixbit::beginRGB();
  matrixbit::beginBuzzer();
  displayReady = matrixbit::beginDisplay();
  imuReady = matrixbit::imu().begin();
  magnetometerReady = matrixbit::magnetometer().begin();
  Serial.printf("OLED=%d IMU=%d MAG=%d (%s)\n", displayReady,
                imuReady, magnetometerReady, matrixbit::magnetometer().name());
  matrixbit::setRGB(0, 255, 0);
  if (displayReady) {
    matrixbit::display().println("Matrix:bit ready\nA=red B=blue/beep");
    matrixbit::display().display();
  }
}

void loop()
{
  static bool lastA = false, lastB = false;
  static uint32_t sampledAt = 0;
  const bool a = matrixbit::buttonA(), b = matrixbit::buttonB();
  if (a != lastA || b != lastB) {
    matrixbit::setRGB(a ? 255 : 0, !a && !b ? 255 : 0, b ? 255 : 0);
    if (b && !lastB) matrixbit::beep();
    Serial.printf("A=%d B=%d\n", a, b);
    lastA = a; lastB = b;
  }
  if (millis() - sampledAt >= 1000) {
    sampledAt = millis();
    matrixbit::ImuReading motion;
    matrixbit::Vector3 field;
    const bool motionValid = imuReady && matrixbit::imu().read(motion);
    const bool fieldValid = magnetometerReady && matrixbit::magnetometer().read(field);
    const int light = matrixbit::light(), sound = matrixbit::soundLevel();
    if (motionValid) Serial.printf("accel_g=(%.3f,%.3f,%.3f) gyro_dps=(%.2f,%.2f,%.2f) temp_C=%.2f\n",
      motion.acceleration.x, motion.acceleration.y, motion.acceleration.z,
      motion.gyroscope.x, motion.gyroscope.y, motion.gyroscope.z, motion.temperature);
    if (fieldValid) Serial.printf("mag_uT=(%.2f,%.2f,%.2f)\n", field.x, field.y, field.z);
    Serial.printf("light=%d sound_p_p=%d I2C_errors=%lu\n", light, sound,
                  static_cast<unsigned long>(matrixbit::i2cErrorCount()));
    if (displayReady) {
      auto &screen = matrixbit::display();
      screen.clearDisplay(); screen.setCursor(0, 0);
      screen.println("Matrix:bit\nA=red B=blue/beep");
      if (motionValid) screen.printf("g %.2f %.2f %.2f\n", motion.acceleration.x, motion.acceleration.y, motion.acceleration.z);
      else screen.println("IMU: no new data");
      if (fieldValid) screen.printf("uT %.1f %.1f %.1f\n", field.x, field.y, field.z);
      else screen.println("MAG: no data");
      screen.printf("Light:%d Mic:%d", light, sound); screen.display();
    }
  }
  delay(25);
}
