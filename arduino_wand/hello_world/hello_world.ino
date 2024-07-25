void setup() {
  Serial.begin(115200);
}

void loop() {
  int data_len = Serial.available();
  if (data_len > 0) {
    char data = Serial.read();
    if (data == '1') {
      Serial.println("good");
    } else {
      Serial.println("bad");
    }
    Serial.println(String(data));
  }
  // Serial.println("000");
  // Serial1.println("111");

  delay(100);
}