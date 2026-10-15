#include <Wire.h>
#include <math.h>

// ========== PIN CONFIGURATION ==========
#define ADXL_ADDR  0x53
#define SDA_PIN    21
#define SCL_PIN    22

#define RED_LED    26
#define GREEN_LED  25
#define BOOT_BTN   0

#define SENSE_PIN  34

// ========== SETTINGS ==========
const float TILT_THRESHOLD = 15.0;
const float FAULT_A = 0.60;

// ACS712-5A sensitivity: approximately 185 mV/A
const float ACS_SENSITIVITY_MV = 185.0;

// Your voltage divider:
// ACS OUT -> 10k -> GPIO34
// GPIO34 -> 20k -> GND
const float DIVIDER_RATIO = 1.5;

float ZERO_mV = 0.0;

float baseX = 0.0;
float baseY = 0.0;
float baseZ = 1.0;

bool adxlOK = false;

// ========== ADXL345 REGISTER WRITE ==========
void writeReg(byte reg, byte val) {
  Wire.beginTransmission(ADXL_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

// ========== READ ACCELEROMETER ==========
bool readAccel(float &ax, float &ay, float &az) {
  Wire.beginTransmission(ADXL_ADDR);
  Wire.write(0x32);

  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  if (Wire.requestFrom(ADXL_ADDR, 6) != 6) {
    return false;
  }

  byte x0 = Wire.read();
  byte x1 = Wire.read();
  byte y0 = Wire.read();
  byte y1 = Wire.read();
  byte z0 = Wire.read();
  byte z1 = Wire.read();

  ax = (int16_t)((x1 << 8) | x0) / 256.0;
  ay = (int16_t)((y1 << 8) | y0) / 256.0;
  az = (int16_t)((z1 << 8) | z0) / 256.0;

  return true;
}

// ========== READ ACS712 ==========
float readSensor_mV() {
  uint32_t sum = 0;

  // Discard initial ADC reading
  analogReadMilliVolts(SENSE_PIN);

  for (int i = 0; i < 200; i++) {
    sum += analogReadMilliVolts(SENSE_PIN);
    delayMicroseconds(500);
  }

  float adc_mV = sum / 200.0;

  // Convert divider voltage back to ACS712 output voltage
  return adc_mV * DIVIDER_RATIO;
}

// ========== ZERO CURRENT CALIBRATION ==========
void calibrate() {
  Serial.println();
  Serial.println("ZERO CURRENT CALIBRATION");
  Serial.println("Keep the load OFF.");
  Serial.println("Keep the pole straight and still.");

  for (int i = 3; i > 0; i--) {
    Serial.println(i);
    delay(1000);
  }

  float sx = 0;
  float sy = 0;
  float sz = 0;

  float ax, ay, az;
  int count = 0;

  // Calibrate pole position
  for (int i = 0; i < 50; i++) {
    if (readAccel(ax, ay, az)) {
      sx += ax;
      sy += ay;
      sz += az;
      count++;
    }

    delay(20);
  }

  if (count == 0) {
    Serial.println("ERROR: ADXL345 read failed.");
    adxlOK = false;
    return;
  }

  baseX = sx / count;
  baseY = sy / count;
  baseZ = sz / count;

  // Allow the sensor supply to settle
  delay(500);

  // Average the ACS712 zero-current output
  float total = 0;

  for (int i = 0; i < 20; i++) {
    total += readSensor_mV();
    delay(20);
  }

  ZERO_mV = total / 20.0;

  adxlOK = true;

  Serial.println("Calibration complete.");

  Serial.print("Zero reference: ");
  Serial.print(ZERO_mV, 1);
  Serial.println(" mV");

  Serial.println("---------------");
}

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);

  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BOOT_BTN, INPUT_PULLUP);

  digitalWrite(RED_LED, LOW);
  digitalWrite(GREEN_LED, LOW);

  analogReadResolution(12);
  analogSetPinAttenuation(SENSE_PIN, ADC_11db);

  Wire.begin(SDA_PIN, SCL_PIN);

  delay(500);

  Wire.beginTransmission(ADXL_ADDR);

  if (Wire.endTransmission() != 0) {
    Serial.println("ADXL345 NOT FOUND.");
    Serial.println("Check SDA, SCL, VCC and GND.");

    while (true) {
      digitalWrite(RED_LED, HIGH);
      delay(300);
      digitalWrite(RED_LED, LOW);
      delay(300);
    }
  }

  Serial.println("ADXL345 found.");

  // Measurement mode
  writeReg(0x2D, 0x08);

  // Full resolution, +/-2g
  writeReg(0x31, 0x08);

  delay(300);

  calibrate();
}

// ========== MAIN LOOP ==========
void loop() {

  // BOOT button recalibrates the zero reference.
  // The load must be OFF before pressing BOOT.
  if (digitalRead(BOOT_BTN) == LOW) {
    delay(30);

    if (digitalRead(BOOT_BTN) == LOW) {
      calibrate();

      while (digitalRead(BOOT_BTN) == LOW) {
        delay(10);
      }
    }
  }

  if (!adxlOK) {
    digitalWrite(RED_LED, HIGH);
    digitalWrite(GREEN_LED, LOW);
    delay(500);
    return;
  }

  // ========== TILT MEASUREMENT ==========
  float ax, ay, az;

  if (!readAccel(ax, ay, az)) {
    Serial.println("ADXL345 read error.");

    digitalWrite(RED_LED, HIGH);
    digitalWrite(GREEN_LED, LOW);

    delay(500);
    return;
  }

  float dot = ax * baseX + ay * baseY + az * baseZ;

  float magA = sqrt(ax * ax + ay * ay + az * az);
  float magB = sqrt(baseX * baseX +
                    baseY *
