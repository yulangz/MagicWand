## 杂记
开关按下的时候是高电平，松开是低电平。
一个记录按键次数的实例程序，其中开关的 OUT 引脚接到 ESP32-C3 的 GPIO 0 引脚上了：
```c
#define KEY_IN 0

int count = 0;

void ScanKEY() {
  if (digitalRead(KEY_IN) == HIGH) {
    delay(10);
    if (digitalRead(KEY_IN) == HIGH) {
      count++;
      Serial.printf("pin count %d\n", count);
    }
  }
}


void setup() {
  Serial.begin(115200);
  pinMode(KEY_IN, INPUT_PULLUP);
}

void loop() {
  ScanKEY();
}
```

串口读写示例程序：
```c
void setup() {
  Serial.begin(115200);
}

void loop() {
  int data_len = Serial.available();
  if (data_len > 0) {
    char data = Serial.read();
    data ++;
    Serial.println(String(data));
  }
}
```

[https://www.waveshare.net/wiki/ESP32-C3-Zero](https://www.waveshare.net/wiki/ESP32-C3-Zero) 记得打开 USB CDC 选项，不然默认的串口没法用
一个基于串口控制红外收发器的示例程序：
```c
#include <HardwareSerial.h>

#define RGB_BRIGHTNESS 10 // Change white brightness (max 255)

// the setup function runs once when you press reset or power the board
#ifdef RGB_BUILTIN
#undef RGB_BUILTIN
#endif
#define RGB_BUILTIN 10

byte trg = 0, cont = 0;
byte buf[32], len;

HardwareSerial SerialPort(0); // use UART2


//求校验和
byte getSum(byte *data, byte len) {
  byte i, sum = 0;
  for (i = 0; i < len; i++) {
    sum += data[i];
  }
  return sum;
}

//红外内码学习
byte IrStudy(byte *data, byte group) {
  byte *offset = data, cs;
  //帧头
  *offset++ = 0x68;
  //帧长度
  *offset++ = 0x08;
  *offset++ = 0x00;
  //模块地址
  *offset++ = 0xff;
  //功能码
  *offset++ = 0x10;
  //内码索引号，代表第几组
  *offset++ = group;
  cs = getSum(&data[3], offset - data - 3);
  *offset++ = cs;
  *offset++ = 0x16;
  return offset - data; 
}

//红外内码发送
byte IrSend(byte *data, byte group) {
  byte *offset = data, cs;
  //帧头
  *offset++ = 0x68;
  //帧长度
  *offset++ = 0x08;
  *offset++ = 0x00;
  //模块地址
  *offset++ = 0xff;
  //功能码
  *offset++ = 0x12;
  //内码索引号，代表第几组
  *offset++ = group;
  cs = getSum(&data[3], offset - data - 3);
  *offset++ = cs;
  *offset++ = 0x16;
  return offset - data; 
}

void setup() {
  neopixelWrite(RGB_BUILTIN,0,0,RGB_BRIGHTNESS); // BLUE
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, 20, 21);
}

void loop() {
  int data_len = Serial.available();

  if (data_len > 0) {
    char data = Serial.read();
    Serial.println(data);

    if (data == '1') {
      len = IrStudy(buf, 0);
      Serial1.write(buf, len);
      Serial.println("1");
    } else if (data == '2') {
      len = IrSend(buf, 0);
      Serial1.write(buf, len);
      Serial.println("2");
    }
  }
}
```

