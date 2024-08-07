#include <TensorFlowLite_ESP32.h>
#include <Adafruit_NeoPixel.h>
#include "model.h"
#include "tensorflow/lite/micro/kernels/micro_ops.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include <Wire.h>
#include <MPU6050.h>

////////////////////////////// 针脚定义 ////////////////////////////////
#define MPU_ADDR0 0
#define MPU_ADDR1 1
const int buttonPin = 2
;  // 定义按钮引脚

////////////////////////////// 红外模块 /////////////////////////////////
#define IR_STUDY_CODE 0xE0
#define IR_SEND_CODE 0xE3
#define IR_CANCEL_CODE 0xE2

byte infrared_buf[8];
HardwareSerial SerialPort(0);  // use UART2

// 开始学习编码，如果不通过 IrCancel 取消，不会自动退出
// group 必须 >= 1，不能是 0
void Ir_study(byte group) {
  byte* offset = infrared_buf;
  *offset++ = IR_STUDY_CODE;
  *offset++ = group;
  // Serial.write(infrared_buf, offset - infrared_buf);
  Serial1.write(infrared_buf, offset - infrared_buf);
}

// 发送已学习编码
void Ir_send(byte group) {
  byte* offset = infrared_buf;
  *offset++ = IR_SEND_CODE;
  *offset++ = group;
  // Serial.write(infrared_buf, offset - infrared_buf);
  Serial1.write(infrared_buf, offset - infrared_buf);
}

// 取消学习
void Ir_cancel() {
  byte* offset = infrared_buf;
  *offset++ = IR_CANCEL_CODE;
  Serial.write(infrared_buf, offset - infrared_buf);
  Serial1.write(infrared_buf, offset - infrared_buf);
}

// 获取红外串口返回
byte Ir_recv() {
  while (!Serial1.available()) {
    delay(5);
  }
  return Serial1.read();
}


////////////////////////////// 灯带 /////////////////////////////////
#define NUM_LEDS 40
#define DATA_PIN 10  // 定义led引脚

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, DATA_PIN, NEO_GRB + NEO_KHZ800);
void LED_setup() {
  strip.begin();
  strip.show();
}

uint8_t RGB_ALL[16][3] = { { 255, 0, 0 },
                           { 0, 255, 0 },
                           { 0, 0, 255 },
                           { 255, 255, 0 },
                           { 128, 0, 128 },
                           { 0, 255, 255 },
                           { 255, 165, 0 },
                           { 255, 192, 203 },
                           { 165, 42, 42 },
                           { 128, 128, 128 },
                           { 0, 255, 0 },
                           { 0, 0, 128 },
                           { 128, 128, 0 },
                           { 128, 0, 0 },
                           { 255, 215, 0 },
                           { 192, 192, 192 } };

// 聚集效果 - 底部LED逐渐变亮
// gatherDelay 20 是一个不错的选择
void RGB_converge(uint8_t gatherDelay, uint8_t rgb[3]) {
  int baseLeds = 3;  // 底部亮起的LED数量

  strip.clear();

  for (int brightness = 0; brightness <= 255; brightness += 5) {
    for (int i = 0; i < baseLeds; i++) {
      int r = rgb[0] * brightness / 255;
      int g = rgb[1] * brightness / 255;
      int b = rgb[2] * brightness / 255;
      strip.setPixelColor(i, strip.Color(r, g, b));
    }
    strip.show();
    delay(gatherDelay);
  }
}

// 爆发效果 - LED从底部快速移动到顶部
void RGB_erupt(uint8_t burstDelay, uint8_t rgb[3]) {
  int baseLeds = 3;
  strip.clear();
  for (int i = 0; i < baseLeds; i++) {
    strip.setPixelColor(i, strip.Color(rgb[0], rgb[1], rgb[2]));
  }
  strip.show();
  delay(burstDelay);

  for (int i = 1; i <= strip.numPixels(); i++) {
    strip.clear();
    for (int j = 0; j < baseLeds; j++) {
      strip.setPixelColor(i + j, strip.Color(rgb[0], rgb[1], rgb[2]));
    }
    strip.show();
    delay(burstDelay);
  }

  // 清除顶部亮光
  strip.clear();
  strip.show();
}

// 常亮效果
void RGB_light(uint8_t rgb[3]) {
  int baseLeds = 3;  // 底部亮起的LED数量

  strip.clear();
  for (int i = 0; i < baseLeds; i++) {
    strip.setPixelColor(i, strip.Color(rgb[0], rgb[1], rgb[2]));
  }
  strip.show();
}

// 关闭LED
void RGB_off() {
  strip.clear();
  strip.show();
}


////////////////////////////// 模型 /////////////////////////////////
namespace {
tflite::ErrorReporter* error_reporter = nullptr;
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* model_input = nullptr;

const int num_classes = 9;
const int input_dim = 3;

constexpr int kTensorArenaSize = 80 * 1024;
uint8_t tensor_arena[kTensorArenaSize];
}


////////////////////////////// 陀螺仪 /////////////////////////////////
MPU6050 mpu;

// 定义每秒采样次数
const int freq = 100;
const int second = 2;

// 重力分量
float gravity_x;
float gravity_y;
float gravity_z;

// 换算到x,y轴上的角速度
float roll_v, pitch_v;

// 上次更新时间
unsigned long prevTime;

// 三个状态，先验状态，观测状态，最优估计状态
float gyro_roll, gyro_pitch;  //陀螺仪积分计算出的角度，先验状态
float acc_roll, acc_pitch;    //加速度计观测出的角度，观测状态
float k_roll, k_pitch;        //卡尔曼滤波后估计出最优角度，最优估计状态

// 误差协方差矩阵P
float e_P[2][2];  //误差协方差矩阵，这里的e_P既是先验估计的P，也是最后更新的P

// 卡尔曼增益K
float k_k[2][2];  //这里的卡尔曼增益矩阵K是一个2X2的方阵

void MPU_resetState() {
  // 读取加速度计数据
  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);

  // 转换加速度计数据为g值
  float Ax = ax / 16384.0;
  float Ay = ay / 16384.0;
  float Az = az / 16384.0;

  // 计算Pitch和Roll
  k_pitch = -atan2(Ax, sqrt(Ay * Ay + Az * Az));
  k_roll = atan2(Ay, Az);

  // 误差协方差矩阵P
  e_P[0][0] = 1;
  e_P[0][1] = 0;
  e_P[1][0] = 0;
  e_P[1][1] = 1;

  // 卡尔曼增益K
  k_k[0][0] = 0;
  k_k[0][0] = 0;
  k_k[0][0] = 0;
  k_k[0][0] = 0;

  prevTime = millis();
}

void MPU_get_kalman_mpu_data(int i, float* input) {
  // 计算微分时间
  unsigned long currentTime = millis();
  float dt = (currentTime - prevTime) / 1000.0;  // 时间间隔（秒）
  prevTime = currentTime;

  // 获取角速度和加速度
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  // 转换加速度计数据为g值
  float Ax = ax / 16384.0;
  float Ay = ay / 16384.0;
  float Az = az / 16384.0;
  float Ox, Oy, Oz;

  // 转换陀螺仪数据为弧度/秒
  float Gx = gx / 131.0 / 180 * PI;
  float Gy = gy / 131.0 / 180 * PI;
  float Gz = gz / 131.0 / 180 * PI;

  // step1:计算先验状态
  // 计算roll, pitch, yaw轴上的角速度
  roll_v = Gx + ((sin(k_pitch) * sin(k_roll)) / cos(k_pitch)) * Gy + ((sin(k_pitch) * cos(k_roll)) / cos(k_pitch)) * Gz;  //roll轴的角速度
  pitch_v = cos(k_roll) * Gy - sin(k_roll) * Gz;                                                                          //pitch轴的角速度
  gyro_roll = k_roll + dt * roll_v;                                                                                       //先验roll角度
  gyro_pitch = k_pitch + dt * pitch_v;                                                                                    //先验pitch角度

  // step2:计算先验误差协方差矩阵
  e_P[0][0] = e_P[0][0] + 0.0025;  //这里的Q矩阵是一个对角阵且对角元均为0.0025
  e_P[0][1] = e_P[0][1] + 0;
  e_P[1][0] = e_P[1][0] + 0;
  e_P[1][1] = e_P[1][1] + 0.0025;

  // step3:更新卡尔曼增益
  k_k[0][0] = e_P[0][0] / (e_P[0][0] + 0.3);
  k_k[0][1] = 0;
  k_k[1][0] = 0;
  k_k[1][1] = e_P[1][1] / (e_P[1][1] + 0.3);

  // step4:计算最优估计状态
  // 观测状态
  // roll角度
  acc_roll = atan2(Ay, Az);
  //pitch角度
  acc_pitch = -atan2(Ax, sqrt(Ay * Ay + Az * Az));
  // 最优估计状态
  k_roll = gyro_roll + k_k[0][0] * (acc_roll - gyro_roll);
  k_pitch = gyro_pitch + k_k[1][1] * (acc_pitch - gyro_pitch);

  // step5:更新协方差矩阵
  e_P[0][0] = (1 - k_k[0][0]) * e_P[0][0];
  e_P[0][1] = 0;
  e_P[1][0] = 0;
  e_P[1][1] = (1 - k_k[1][1]) * e_P[1][1];

  // 计算重力加速度方向
  gravity_x = -sin(k_pitch);
  gravity_y = sin(k_roll) * cos(k_pitch);
  gravity_z = cos(k_roll) * cos(k_pitch);

  // 重力消除
  Ax = Ax - gravity_x;
  Ay = Ay - gravity_y;
  Az = Az - gravity_z;

  // 得到全局空间坐标系中的相对加速度
  Ox = cos(k_pitch) * Ax + sin(k_pitch) * sin(k_roll) * Ay + sin(k_pitch) * cos(k_roll) * Az;
  Oy = cos(k_roll) * Ay - sin(k_roll) * Az;
  Oz = -sin(k_pitch) * Ax + cos(k_pitch) * sin(k_roll) * Ay + cos(k_pitch) * cos(k_roll) * Az;

  input[i * input_dim] = Ox;
  input[i * input_dim + 1] = Oy;
  input[i * input_dim + 2] = Oz;

  delay(1000 / freq);  // 短暂延迟，避免过高的循环频率
}

int MPU_process_classification(float* output) {
  int max_index = 0;
  float max_value = output[0];

  for (int i = 1; i < num_classes; i++) {
    if (output[i] >= max_value) {
      max_value = output[i];
      max_index = i;
    }
  }

  if (max_index == 4) {
    max_index = 2;
  }
  // Serial.printf("max index is %d, max value is %f\n", max_index, max_value);
  return max_index;
}

int MPU_get_gasture() {
  MPU_resetState();

  for (int i = 0; i < freq * second; i++) {
    MPU_get_kalman_mpu_data(i, model_input->data.f);
  }

  TfLiteStatus invoke_status = interpreter->Invoke();
  if (invoke_status != kTfLiteOk) {
    error_reporter->Report("Invoke failed");
    return -1;
  }
  // Serial.println("got mpu data.");
  return MPU_process_classification(interpreter->output(0)->data.f);
}


////////////////////////////// 按钮 /////////////////////////////////
volatile bool buttonPressed = false;
volatile bool buttonLongPressed = false;
volatile unsigned long lastInterruptTime = 0;  // 上一次中断时间
#define DEBOUNCE_DELAY 5                  // 去抖动延时
#define LONG_PRESS_TIME 500               // 长按时间

void check_button_pressed() {
  
  if (digitalRead(buttonPin) == HIGH) {
    Serial.println("check_button_pressed HIGH");
    lastInterruptTime = millis();
    delay(DEBOUNCE_DELAY);
    if (digitalRead(buttonPin) == HIGH) {
      buttonPressed = true;
    }
  }
}

void clearButtonPressed() {
  buttonPressed = false;
  buttonLongPressed = false;
}

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, 20, 21);

  LED_setup();

  static tflite::MicroErrorReporter micro_error_reporter;  // NOLINT
  error_reporter = &micro_error_reporter;

  model = tflite::GetModel(model_tflite);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    error_reporter->Report(
      "Model provided is schema version %d not equal "
      "to supported version %d.",
      model->version(), TFLITE_SCHEMA_VERSION);
    return;
  }

  static tflite::AllOpsResolver resolver;
  resolver.AddConv2D();
  resolver.AddRelu();
  resolver.AddFullyConnected();
  resolver.AddSoftmax();
  resolver.AddReshape();
  resolver.AddTranspose();
  resolver.AddExpandDims();

  static tflite::MicroInterpreter static_interpreter(
    model, resolver, tensor_arena, kTensorArenaSize,
    error_reporter);
  interpreter = &static_interpreter;

  interpreter->AllocateTensors();
  model_input = interpreter->input(0);

  Wire.begin(MPU_ADDR0, MPU_ADDR1);
  mpu.initialize();

  if (!mpu.testConnection()) {
    Serial.println("MPU6050连接失败");
    while (1)
      ;
  }

  pinMode(buttonPin, INPUT_PULLUP);                                           // 将按钮引脚设置为输入模式，并启用内部上拉电阻
  // attachInterrupt(digitalPinToInterrupt(buttonPin), button_pressed, RISING);  // 设置中断
}

void loop() {

  check_button_pressed();

  if (buttonPressed == true) {
    while (digitalRead(buttonPin) == HIGH) {
      unsigned long currentTime = millis();
      if ((currentTime - lastInterruptTime) > LONG_PRESS_TIME) {
        buttonLongPressed = true;
        break;
      }
    }

    if (buttonLongPressed == true) {
      Serial.println("long pressed");
      RGB_light(RGB_ALL[15]);
      int gasture = MPU_get_gasture();
      Serial.printf("get_gasture is: %d\n", gasture);
      RGB_converge(4, RGB_ALL[gasture]);
      RGB_light(RGB_ALL[gasture]);
      Serial.println("rgb_light");
      Ir_study(gasture + 1);
      Serial.println("ir_study");
      Ir_recv();
      Serial.println("ir_recv");
      clearButtonPressed();
      RGB_off();

      
      // TODO: 添加再次长按取消学习的逻辑
    } else {
      Serial.println("short pressed");
      int gasture = MPU_get_gasture();
      Serial.printf("get_gasture is: %d\n", gasture);
      RGB_converge(20, RGB_ALL[gasture]);
      RGB_erupt(5, RGB_ALL[gasture]);
      Serial.println("rgb_erupt");
      Ir_send(gasture + 1);
      Serial.println("ir_send");
      Ir_recv();
      Serial.println("ir_recv");
      clearButtonPressed();
    }
  }
}
