#include <Wire.h>
#include <Adafruit_VL53L0X.h>
#include <Adafruit_SSD1306.h>
#include <BH1750.h>
#include <ESP32Servo.h>


#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);


BH1750 lightSensor;
Adafruit_VL53L0X lox = Adafruit_VL53L0X();


Servo servoMotor;

#define FAN_IN1 25   
#define FAN_IN2 26   


#define VIB_IN1 27   
#define VIB_IN2 33   


float lambda_m_cam = 0.0;   
float sigma = 0.5;
String state = "idle";


unsigned long last_cam_update = 0;
const unsigned long CAM_TIMEOUT = 120; 

// Dynamics coefficients
float gamma_leak = 0.01;
float k_m = 0.06;
float k_a = 0.03;


float LUX_baseline  = 0;
float dist_baseline = 0;
float cam_baseline  = 0;


float gaze_hold = 0.0;
float gaze_hold_rate = 0.01;
float gaze_release_rate = 0.02;


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
  return analogRead(34);
}


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

  Serial.println("Baseline calibrated:");
  Serial.println(LUX_baseline);
  Serial.println(dist_baseline);
  Serial.println(cam_baseline);
}


void updateState() {

  if (state == "collapse") {
    if (sigma > 0.30) state = "idle";
    return;
  }

  if (state == "rephase") {
    if (sigma < 0.60) state = "idle";
    return;
  }

  if (sigma < 0.20)      state = "collapse";
  else if (sigma > 0.70) state = "rephase";
  else                   state = "idle";
}


void driveActuators() {


  digitalWrite(FAN_IN1, LOW);
  digitalWrite(FAN_IN2, LOW);
  digitalWrite(VIB_IN1, LOW);
  digitalWrite(VIB_IN2, LOW);

  if (state == "collapse") {

    digitalWrite(VIB_IN1, HIGH);
    digitalWrite(VIB_IN2, LOW);

    servoMotor.write(80);
  }

  else if (state == "idle") {

    servoMotor.write(90);
  }

  else if (state == "rephase") {

    digitalWrite(FAN_IN1, HIGH);
    digitalWrite(FAN_IN2, LOW);


    digitalWrite(VIB_IN1, HIGH);
    digitalWrite(VIB_IN2, LOW);

    servoMotor.write(110);
  }
}


void renderOLED() {

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  if (state == "collapse")      display.println("C O L L A P S E");
  else if (state == "idle")     display.println("I D L E");
  else                          display.println("R E P H A S E");

  display.display();
}


void setup() {

  Serial.begin(115200);
  Wire.begin();

  lightSensor.begin();
  lox.begin();


  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  servoMotor.attach(14);

  pinMode(FAN_IN1, OUTPUT);
  pinMode(FAN_IN2, OUTPUT);
  pinMode(VIB_IN1, OUTPUT);
  pinMode(VIB_IN2, OUTPUT);


  digitalWrite(FAN_IN1, LOW);
  digitalWrite(FAN_IN2, LOW);
  digitalWrite(VIB_IN1, LOW);
  digitalWrite(VIB_IN2, LOW);


  calibrate_baseline();
}


void loop() {

  if (lambda_m_cam > 0.01) {
    servoMotor.write(130);
  } else {
    servoMotor.write(50);
  }
  

  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    float v = line.toFloat();

    Serial.print("[ARDUINO RX] ");
    Serial.println(line);

    if (v >= 0.0 && v <= 1.0) {
      lambda_m_cam = v;
      last_cam_update = millis();
    }
  }
 

  float L_raw = lightSensor.readLightLevel();

  VL53L0X_RangingMeasurementData_t measure;
  lox.rangingTest(&measure, false);
  float D_raw = measure.RangeMilliMeter;

  float A_raw = analogRead(32) / 4095.0;
  float A = clamp(A_raw, 0, 1);

 
  float lambda_m_raw;
  if (millis() - last_cam_update < CAM_TIMEOUT) {
    lambda_m_raw = lambda_m_cam;
  } else {
    lambda_m_raw = 0.0;
  }

  static float lambda_m_smooth = 0.0;
  lambda_m_smooth = 0.9 * lambda_m_smooth + 0.1 * lambda_m_raw;



  float Dn = normalize_clamp(dist_baseline - D_raw, -300, 700);
  float Ln = normalize_clamp(L_raw - LUX_baseline, -300, 700);

  if (lambda_m_smooth > 0.05) gaze_hold += 0.01;
  else                        gaze_hold -= 0.02;
  gaze_hold = clamp(gaze_hold, 0, 1);

  float lambda_dist  = gaze_hold * pow(Dn, 1.3);
  float lambda_light = 0.1 * Ln;

  float lambda_m =
      0.65 * lambda_m_smooth +
      0.25 * lambda_dist +
      0.10 * lambda_light;

  lambda_m = clamp(lambda_m, 0, 1);


  sigma = (1 - gamma_leak) * sigma
        - k_m * lambda_m
        + k_a * A
        + 0.02 * (1.0 - Dn)      
        + 0.015 * Ln; 

  sigma = clamp(sigma, 0, 1);


  updateState();
  driveActuators();
  renderOLED();

  Serial.printf(
    "{\"sigma\":%.3f,\"lambda_m\":%.3f,\"lambda_m_cam\":%.3f,\"state\":\"%s\"}\n",
    sigma, lambda_m, lambda_m_cam, state.c_str()
  );

  delay(20);
}