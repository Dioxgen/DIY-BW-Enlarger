#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal.h>

// LCD引脚配置
/* LCD RS pin to digital pin 12
 * LCD Enable pin to digital pin 11
 * LCD D4 pin to digital pin 5
 * LCD D5 pin to digital pin 4
 * LCD D6 pin to digital pin 3
 * LCD D7 pin to digital pin 2
 * LCD R/W pin to ground
 * LCD VSS pin to ground
 * LCD VDD pin to 5V
 * LCD A pin to D9
 * LCD K pin to ground
 */
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
#define LCD_BKL 9

// LED灯带配置
#define LED_PIN 6     // The pin on the Arduino connected to the NeoPixels
#define LED_COUNT 64  // Popular NeoPixel size
Adafruit_NeoPixel pixels(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// 电位器引脚定义
#define R_POT_PIN A0
#define G_POT_PIN A1
#define B_POT_PIN A2
#define BRI_POT_PIN A3

#define MODE_SELECT_PIN 7
#define START_PIN 8         // 在功能1中承担选择与开始曝光功能
#define EXPOSURE_ADJUST 0   // 调整曝光时间状态
#define EXPOSURE_WAITING 1  // 等待开始曝光状态

int ExposureState = EXPOSURE_ADJUST;
int Mode = 0;  // 0为调光模式，1为设置曝光模式
int ExpTime_level = 0;
int ExpTime = 1;  // 曝光时间

unsigned long pressStartTime = 0;                // 按键按下开始时间
bool isPressing = false;                         // 按键是否正在被按下
const unsigned long LONG_PRESS_THRESHOLD = 500;  // 长按阈值(ms)

// 等级数量和校准参数
const int LEVELS = 10;           // 0-9共10个等级
const int DEADZONE_LOW = 30;     // 低端死区阈值
const int DEADZONE_HIGH = 1000;  // 高端死区阈值

// 颜色查找表 (10级)
const uint8_t colorLevels[LEVELS] = { 0, 28, 56, 85, 113, 141, 170, 198, 226, 255 };
float ExpTimeLevels[8] = { 500, 1000, 2000, 4000, 8000, 16000, 32000, 64000 };  // 曝光时间，单位ms

uint8_t R_val;  // 保存在全局
uint8_t G_val;
uint8_t B_val;
uint8_t brightness;

unsigned long PRESSED_TIME = millis();  // 区分按键短按长按

void setup() {
  pinMode(MODE_SELECT_PIN, INPUT_PULLUP);
  pinMode(START_PIN, INPUT_PULLUP);
  pinMode(LCD_BKL, OUTPUT);

  // 初始化LED灯带
  pixels.begin();
  pixels.clear();
  pixels.setBrightness(0);  // 初始亮度为0
  pixels.show();

  // 初始化LCD
  digitalWrite(LCD_BKL, HIGH);
  lcd.begin(16, 2);
  lcd.print("B/W Enlarger Sys");
  delay(1500);
  lcd.clear();
}

void loop() {
  if (Mode == 0) {
    Dimming();
  } else if (Mode == 1) {
    Exposure();
  }
}

void Dimming() {
  // 读取并校准电位器值
  int rRaw = calibratedRead(R_POT_PIN);
  int gRaw = calibratedRead(G_POT_PIN);
  int bRaw = calibratedRead(B_POT_PIN);
  int briRaw = calibratedRead(BRI_POT_PIN);
  // 映射到0-9等级
  int R_level = map(rRaw, DEADZONE_LOW, DEADZONE_HIGH, 0, LEVELS - 1);
  int G_level = map(gRaw, DEADZONE_LOW, DEADZONE_HIGH, 0, LEVELS - 1);
  int B_level = map(bRaw, DEADZONE_LOW, DEADZONE_HIGH, 0, LEVELS - 1);
  int Bri_level = map(briRaw, DEADZONE_LOW, DEADZONE_HIGH, 0, LEVELS - 1);
  /*
    int R_level = map(rRaw, DEADZONE_HIGH, DEADZONE_LOW, 0, LEVELS - 1);  // 电位器反向适配
    int G_level = map(gRaw, DEADZONE_HIGH, DEADZONE_LOW, 0, LEVELS - 1);
    int B_level = map(bRaw, DEADZONE_HIGH, DEADZONE_LOW, 0, LEVELS - 1);
    int Bri_level = map(briRaw, DEADZONE_HIGH, DEADZONE_LOW, 0, LEVELS - 1);
    */
  // 获取实际颜色值
  R_val = colorLevels[R_level];
  G_val = colorLevels[G_level];
  B_val = colorLevels[B_level];
  brightness = colorLevels[Bri_level];
  // 计算亮度百分比
  int percent = map(Bri_level, 0, LEVELS - 1, 0, 100);

  if (G_level == 0 && B_level == 0) {  //安全灯
    digitalWrite(LCD_BKL, LOW);
  } else {
    digitalWrite(LCD_BKL, HIGH);
  }

  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("Dimming");
  delay(1000);
  lcd.clear();

  while (1) {
    rRaw = calibratedRead(R_POT_PIN);
    gRaw = calibratedRead(G_POT_PIN);
    bRaw = calibratedRead(B_POT_PIN);
    briRaw = calibratedRead(BRI_POT_PIN);
    R_level = map(rRaw, DEADZONE_LOW, DEADZONE_HIGH, 0, LEVELS - 1);
    G_level = map(gRaw, DEADZONE_LOW, DEADZONE_HIGH, 0, LEVELS - 1);
    B_level = map(bRaw, DEADZONE_LOW, DEADZONE_HIGH, 0, LEVELS - 1);
    Bri_level = map(briRaw, DEADZONE_LOW, DEADZONE_HIGH, 0, LEVELS - 1);
    /*
    R_level = map(rRaw, DEADZONE_HIGH, DEADZONE_LOW, 0, LEVELS - 1);  // 电位器反向适配
    G_level = map(gRaw, DEADZONE_HIGH, DEADZONE_LOW, 0, LEVELS - 1);
    B_level = map(bRaw, DEADZONE_HIGH, DEADZONE_LOW, 0, LEVELS - 1);
    Bri_level = map(briRaw, DEADZONE_HIGH, DEADZONE_LOW, 0, LEVELS - 1);
    */
    R_val = colorLevels[R_level];
    G_val = colorLevels[G_level];
    B_val = colorLevels[B_level];
    brightness = colorLevels[Bri_level];
    percent = map(Bri_level, 0, LEVELS - 1, 0, 100);

    // 设置LED - 只有在亮度>0时才设置颜色
    pixels.setBrightness(brightness);
    if (brightness > 0) {
      for (int i = 0; i < LED_COUNT; i++) {
        pixels.setPixelColor(i, pixels.Color(R_val, G_val, B_val));
      }
    } else {
      pixels.clear();  // 亮度为0时清除所有LED
    }
    pixels.show();

    if (G_level == 0 && B_level == 0) {  //安全灯
      digitalWrite(LCD_BKL, LOW);
    } else {
      digitalWrite(LCD_BKL, HIGH);
    }

    // 更新LCD显示
    updateLCD(R_level, G_level, B_level, percent);

    delay(10);
    if (digitalRead(MODE_SELECT_PIN) == LOW) {
      delay(50);  // 消抖
      if (digitalRead(MODE_SELECT_PIN) == LOW) {
        Mode = 1;
        delay(300);  // 防止快速切换
        lcd.clear();
        break;
      }
    }
  }
}

void Exposure() {
  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("Exposure");
  digitalWrite(LCD_BKL, HIGH);
  delay(1000);
  lcd.clear();

  ExposureState = EXPOSURE_ADJUST;  // 重置状态
  updateExposureLCD();

  while (Mode == 1) {
    // 处理开始按键
    int startPinState = digitalRead(START_PIN);

    // 按键按下
    if (startPinState == LOW && !isPressing) {
      isPressing = true;
      pressStartTime = millis();
    }
    // 按键释放
    else if (startPinState == HIGH && isPressing) {
      isPressing = false;
      unsigned long pressDuration = millis() - pressStartTime;
      // 短按处理
      if (pressDuration < LONG_PRESS_THRESHOLD) {
        if (ExposureState == EXPOSURE_ADJUST) {
          // 调整曝光时间等级
          ExpTime_level++;
          if (ExpTime_level >= 8) {
            ExpTime_level = 0;
          }
          updateExposureLCD();
        } else if (ExposureState == EXPOSURE_WAITING) {
          // 从等待状态返回调整状态
          ExposureState = EXPOSURE_ADJUST;
          updateExposureLCD();
        }
      }
    }
    // 长按处理(按键仍在按下且超过阈值)
    else if (startPinState == LOW && isPressing && millis() - pressStartTime >= LONG_PRESS_THRESHOLD) {
      isPressing = false;  // 防止重复触发
      ExposureState = EXPOSURE_WAITING;
      pixels.clear();  // 关闭光源防止误曝光
      pixels.show();

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Waiting to START");

      pixels.setBrightness(225);
      for (int i = 0; i < LED_COUNT; i++) {
        pixels.setPixelColor(i, pixels.Color(255, 0, 0));
      }
      pixels.show();

      delay(3000);
      digitalWrite(LCD_BKL, LOW);  // 关闭LCD背光防止漏光
      while (1) {
        startPinState = digitalRead(START_PIN);
        if (startPinState == LOW) {
          delay(2500);  //等待机震消除
          pixels.clear();
          pixels.show();
          pixels.setBrightness(0);
          delay(2500);
          // 开始曝光
          lcd.clear();
          lcd.print("Exposing...");
          START_ENLARGE(ExpTimeLevels[ExpTime_level], R_val, G_val, B_val, brightness);
          ExposureState = EXPOSURE_ADJUST;
          updateExposureLCD();
          break;
        }
      }
    }

    // 处理模式切换按键
    if (digitalRead(MODE_SELECT_PIN) == LOW) {
      delay(50);  // 消抖
      if (digitalRead(MODE_SELECT_PIN) == LOW) {
        Mode = 0;
        delay(300);  // 防止快速切换
        lcd.clear();
        break;
      }
    }
    delay(10);
  }
}

// 校准读取函数 - 解决电位器极限值问题
int calibratedRead(int pin) {
  int raw = analogRead(pin);

  // 应用死区处理
  if (raw < DEADZONE_LOW) return 0;
  if (raw > DEADZONE_HIGH) return 1023;

  // 映射到有效范围
  return map(raw, DEADZONE_LOW, DEADZONE_HIGH, 0, 1023);
}

// 调光模式下的LCD显示
void updateLCD(int R_lvl, int G_lvl, int B_lvl, int percent) {
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate < 200) return;  // 200ms刷新间隔
  lastUpdate = millis();
  // 第一行显示RGB等级
  lcd.setCursor(0, 0);
  lcd.print("R: ");
  lcd.print(R_lvl);
  lcd.print(" G: ");
  lcd.print(G_lvl);
  lcd.print(" B: ");
  lcd.print(B_lvl);

  // 第二行显示亮度信息和百分比
  lcd.setCursor(0, 1);
  lcd.print("Bri: ");
  lcd.print(percent);
  lcd.print("% ");
}

// 曝光模式下的LCD显示
void updateExposureLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Exposure Time:");
  lcd.setCursor(0, 1);
  lcd.print(ExpTimeLevels[ExpTime_level] / 1000);
  lcd.print("s");
}

// 开始曝光函数
void START_ENLARGE(int EXPOSURE_TIME, int R, int G, int B, int L) {  // L为Luminance
  // 开启LED
  pixels.setBrightness(L);
  for (int i = 0; i < LED_COUNT; i++) {
    pixels.setPixelColor(i, pixels.Color(R, G, B));
  }
  pixels.show();

  // 曝光计时
  delay(EXPOSURE_TIME);

  // 关闭LED
  pixels.clear();
  pixels.show();
  pixels.setBrightness(0);

  // 显示曝光完成信息
  //digitalWrite(LCD_BKL, HIGH);
  lcd.clear();
  lcd.print("Exposure Done");
  delay(3000);
}