#include <Adafruit_NeoPixel.h>

#define NUM_LEDS 40
#define DATA_PIN 0

uint8_t RGB_ALL[16][3] = {{255, 0, 0},
 {0, 255, 0},
 {0, 0, 255},
 {255, 255, 0},
 {128, 0, 128},
 {0, 255, 255},
 {255, 165, 0},
 {255, 192, 203},
 {165, 42, 42},
 {128, 128, 128},
 {0, 255, 0},
 {0, 0, 128},
 {128, 128, 0},
 {128, 0, 0},
 {255, 215, 0},
 {192, 192, 192}
};


Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, DATA_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  strip.begin();
  strip.show();
}

void loop() {
  for (int i = 0;i < 16; i++) {
    energyGatherAndBurst(20, 5, RGB_ALL[i]); // 调整这些值来控制效果速度

    delay(2000);
  }
  
}

void energyGatherAndBurst(uint8_t gatherDelay, uint8_t burstDelay, uint8_t rgb[3]) {
  int baseLeds = 3; // 底部亮起的LED数量

  // 聚集效果 - 底部LED逐渐变亮
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

  // 爆发效果 - LED从底部快速移动到顶部
  strip.clear();
  for (int i = 0; i < baseLeds; i++) {
    strip.setPixelColor(i, strip.Color(rgb[0], rgb[1], rgb[2]));
  }
  strip.show();
  delay(burstDelay);

  for (int i = 1; i <= strip.numPixels() - baseLeds; i++) {
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
