#include <HardwareSerial.h>

#define RGB_BRIGHTNESS 10 // Change white brightness (max 255)

// the setup function runs once when you press reset or power the board
#ifdef RGB_BUILTIN
#undef RGB_BUILTIN
#endif
#define RGB_BUILTIN 10

byte trg = 0, cont = 0;
byte buf[8], len;

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
  byte *offset = data;
  //指令
  *offset++ = 0xE0;
  //内码索引号，代表第几组
  *offset++ = group;
  return offset - data; 
}

//红外内码发送
byte IrSend(byte *data, byte group) {
  byte *offset = data;
  //帧头
  *offset++ = 0xE3;
  //内码索引号，代表第几组
  *offset++ = group;
  return offset - data; 
}

//红外内码发送
byte IrCancel(byte *data) {
  byte *offset = data;
  //帧头
  *offset++ = 0xE2;
  return offset - data; 
}

void setup() {
  neopixelWrite(RGB_BUILTIN,0,0,RGB_BRIGHTNESS); // BLUE
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, 20, 21);
}

void loop() {
  int data_len = Serial.available();

  if (data_len > 0) {
    char data = Serial.read();
    Serial.println(data);

    if (data == '1') {
      len = IrStudy(buf, 1);
      Serial1.write(buf, len);
      Serial.println("1");
    } else if (data == '2') {
      len = IrSend(buf, 1);
      Serial1.write(buf, len);
      Serial.println("2");
    } else if (data == '3') {
      len = IrCancel(buf);
      Serial1.write(buf, len);
      Serial.println("2");
    }
  }
}
