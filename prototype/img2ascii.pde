PImage img;
String asciiChars = " .:-=+*#%@";
int cols, rows;
float charWidth = 6;
float charHeight = 10;

void setup() {
  size(1200, 800);

  img = loadImage("test.jpg");
  if (img == null) {
    img = createTestImage();
  }

  cols = width / (int)charWidth;
  rows = height / (int)charHeight;

  background(0);
  textFont(createFont("Courier", 8));
  textSize(8);
  fill(255);

  convertToAscii();
}

PImage createTestImage() {
  PImage testImg = createImage(200, 150, RGB);
  testImg.loadPixels();

  for (int y = 0; y < testImg.height; y++) {
    for (int x = 0; x < testImg.width; x++) {
      float brightness = map(sin(x * 0.1) * cos(y * 0.1), -1, 1, 0, 255);
      testImg.pixels[y * testImg.width + x] = color(brightness);
    }
  }
  testImg.updatePixels();
  return testImg;
}

void convertToAscii() {
  img.resize(cols, rows);
  img.loadPixels();

  for (int y = 0; y < rows; y++) {
    for (int x = 0; x < cols; x++) {
      if (x < img.width && y < img.height) {
        color pixelColor = img.pixels[y * img.width + x];
        float grayValue = brightness(pixelColor);
        int charIndex = (int)map(grayValue, 0, 255, 0, asciiChars.length() - 1);
        char asciiChar = asciiChars.charAt(charIndex);

        text(asciiChar, x * charWidth, y * charHeight);
      }
    }
  }
}

void convertRGBBuffer(int[] rgbBuffer, int bufferWidth, int bufferHeight) {
  background(0);

  int targetCols = width / (int)charWidth;
  int targetRows = height / (int)charHeight;

  float scaleX = (float)bufferWidth / targetCols;
  float scaleY = (float)bufferHeight / targetRows;

  for (int y = 0; y < targetRows; y++) {
    for (int x = 0; x < targetCols; x++) {
      int srcX = (int)(x * scaleX);
      int srcY = (int)(y * scaleY);

      if (srcX < bufferWidth && srcY < bufferHeight) {
        int pixelColor = rgbBuffer[srcY * bufferWidth + srcX];

        int r = (pixelColor >> 16) & 0xFF;
        int g = (pixelColor >> 8) & 0xFF;
        int b = pixelColor & 0xFF;

        float grayValue = 0.299 * r + 0.587 * g + 0.114 * b;

        int charIndex = (int)map(grayValue, 0, 255, 0, asciiChars.length() - 1);
        char asciiChar = asciiChars.charAt(charIndex);

        text(asciiChar, x * charWidth, y * charHeight);
      }
    }
  }
}

void keyPressed() {
  if (key == 'r') {
    background(0);
    convertToAscii();
  }
  if (key == 't') {
    int[] testBuffer = createTestRGBBuffer(320, 240);
    convertRGBBuffer(testBuffer, 320, 240);
  }
}

int[] createTestRGBBuffer(int w, int h) {
  int[] buffer = new int[w * h];

  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      float brightness = map(sin(x * 0.05) * cos(y * 0.05), -1, 1, 0, 255);
      int gray = (int)brightness;
      buffer[y * w + x] = (gray << 16) | (gray << 8) | gray;
    }
  }

  return buffer;
}

void draw() {
}