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
