#include <Wire.h>
#include <Adafruit_VL53L0X.h>
#include <Adafruit_DRV2605.h>
#include <Adafruit_SSD1306.h>
#include <BH1750.h>
#include <Servo.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

// ==== Sensors ====
BH1750 lightSensor;
Adafruit_VL53L0X lox = Adafruit_VL53L0X();

// ==== Actuators ====
Adafruit_DRV2605 drv;
Servo servoMotor;

// ==== Core dynamic variables ====
float sigma = 0.5;
String state = "idle";

float gamma_leak = 0.02;
float k_m = 0.05;
float k_a = 0.04;

float noise_min = -0.005;
float noise_max =  0.005;

// ==== Baseline ====
float LUX_baseline = 0;
float dist_baseline = 0;
float cam_baseline = 0;

// Utility
float clamp(float x, float a, float b) {
  if (x < a) return a;
  if (x > b) return b;
  return x;
}

float normalize_clamp(float x, float min_val, float max_val) {
  x = clamp(x, min_val, max_val);
  return (x - min_val) / (max_val - min_val);
}

float readCameraBrightness() {
  return analogRead(34);   // placeholder pin
}

// ------------------------------------
// Baseline calibration
// ------------------------------------
void calibrate_baseline() {

  long sumL = 0, sumD = 0, sumC = 0;
  int count = 0;

  unsigned long start = millis();

  while (millis() - start < 3000) {
    sumL += lightSensor.readLightLevel();

    VL53L0X_RangingMeasurementData_t measure;
    lox.rangingTest(&measure, false);
    if (!measure.RangeStatus) sumD += measure.RangeMilliMeter;

    sumC += readCameraBrightness();
    count++;

    delay(20);
  }

  LUX_baseline  = sumL / (float)count;
  dist_baseline = sumD / (float)count;
  cam_baseline  = sumC / (float)count;

  Serial.println("Baseline calibrated.");
  Serial.println(LUX_baseline);
  Serial.println(dist_baseline);
  Serial.println(cam_baseline);
}

// ------------------------------------
// State machine with hysteresis
// ------------------------------------
void updateState() {
  if (state == "collapse") {
    if (sigma > 0.30) state = "idle";
    return;
  }
  if (state == "rephase") {
    if (sigma < 0.60) state = "idle";
    return;
  }

  if (sigma < 0.20) state = "collapse";
  else if (sigma > 0.70) state = "rephase";
  else state = "idle";
}

// ------------------------------------
// Actuator control
// ------------------------------------
void driveActuators() {

  if (state == "collapse") {
    float intensity = 0.4 + 0.4*(1 - sigma);
    drv.setWaveform(0, int(intensity * 100));
    drv.go();
    servoMotor.write(80);
  }

  else if (state == "idle") {
    drv.setWaveform(0, 50);
    drv.go();
    servoMotor.write(90);
  }

  else if (state == "rephase") {
    drv.setWaveform(0, 80);
    drv.go();
    servoMotor.write(110);
  }
}

// ------------------------------------
// OLED Render
// ------------------------------------
void renderOLED() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  if (state == "collapse") {
    display.setCursor(0,0);
    display.println("C O L L A P S E");
  }
  else if (state == "idle") {
    display.setCursor(0,0);
    display.println("I D L E");
  }
  else {
    display.setCursor(0,0);
    display.println("R E P H A S E");
  }

  display.display();
}

// ------------------------------------
// SETUP
// ------------------------------------
void setup() {
  Serial.begin(115200);
  Wire.begin();

  lightSensor.begin();
  lox.begin();

  drv.begin();
  drv.selectLibrary(1);
  drv.setMode(DRV2605_MODE_INTTRIG);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  servoMotor.attach(14);  // update later

  calibrate_baseline();
}

// ------------------------------------
// LOOP
// ------------------------------------
void loop() {

  // 1. Read sensors
  float L_raw = lightSensor.readLightLevel();

  VL53L0X_RangingMeasurementData_t measure;
  lox.rangingTest(&measure, false);
  float D_raw = measure.RangeMilliMeter;

  float C_raw = readCameraBrightness();
  float A_raw = analogRead(32) / 4095.0;

  // 2. normalized
  float L = normalize_clamp(L_raw - LUX_baseline,   -300, 700);
  float D = normalize_clamp(dist_baseline - D_raw,  -300, 700);
  float C = normalize_clamp(C_raw - cam_baseline,   -50, 200);
  float A = clamp(A_raw, 0, 1);

  
  float lambda_m = clamp(0.2*L + 0.5*D + 0.3*C, 0, 1);
  float lambda_a = A;

  float noise = noise_min + (random(10000)/10000.0)*(noise_max - noise_min);
  sigma = (1 - gamma_leak)*sigma - k_m*lambda_m + k_a*lambda_a + noise;
  sigma = clamp(sigma, 0, 1);

  // 5. Update state
  updateState();

  // 6. Drive outputs
  driveActuators();

  // 7. OLED
  renderOLED();

  // 8. Logs
  Serial.printf(
    "{\"sigma\":%.3f, \"lambda_m\":%.3f, \"lambda_a\":%.3f, \"state\":\"%s\"}\n",
    sigma, lambda_m, lambda_a, state.c_str()
  );

  delay(20);
}