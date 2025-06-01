#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Fonksiyon prototipleri
void drawMenu();
void handleMenuInput();
void startSnake();
void readSnakeButtons();
void moveSnake();
void drawSnake();
void spawnFood();
void startFlappy();
void readFlappyInput();
void updateFlappy();
void drawFlappy();
void startBounce();
void readBounceInput();
void updateBounce();
void drawBounce();
void gameOver();

// OLED ekran ayarları
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Buton pinleri
#define BTN_UP     14
#define BTN_DOWN   12
#define BTN_LEFT   13
#define BTN_RIGHT  27

// Oyun türleri
enum GameType { SNAKE, FLAPPY, BOUNCE, GAME_COUNT };
const char* gameNames[GAME_COUNT] = {"Snake", "Flappy Bird", "Bounce Ball"};
GameType selectedGame = SNAKE;
bool inMenu = true;

// === SNAKE ===
#define CELL_SIZE 4
#define MAX_LENGTH 100
struct Point { int x; int y; };
Point snake[MAX_LENGTH];
int length = 3, dx = 1, dy = 0;
Point food;
unsigned long lastMove = 0;
int speed = 150;
bool playingSnake = false;

// === FLAPPY ===
int birdY = SCREEN_HEIGHT / 2;
int velocity = 0;
const int gravity = 1;
int pipeX = SCREEN_WIDTH;
int gapY;
const int gapHeight = 20;
int flappyScore = 0;
bool playingFlappy = false;

// === BOUNCE BALL ===
int ballX = SCREEN_WIDTH / 2;
int ballY = SCREEN_HEIGHT / 2;
int ballDX = 2;
int ballDY = 2;
bool playingBounce = false;

unsigned long lastUpdate = 0;
const int frameDelay = 40;

void setup() {
  Serial.begin(115200);
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);

  Wire.begin(25, 26); // SDA, SCL
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
    for (;;);
  }

  display.clearDisplay();
  display.display();
  randomSeed(analogRead(0));
}

void loop() {
  if (inMenu) {
    drawMenu();
    handleMenuInput();
  } else if (playingSnake) {
    if (millis() - lastMove > speed) {
      readSnakeButtons();
      moveSnake();
      drawSnake();
      lastMove = millis();
    }
  } else if (playingFlappy) {
    if (millis() - lastUpdate < frameDelay) return;
    lastUpdate = millis();
    readFlappyInput();
    updateFlappy();
    drawFlappy();
  } else if (playingBounce) {
    if (millis() - lastUpdate < frameDelay) return;
    lastUpdate = millis();
    readBounceInput();
    updateBounce();
    drawBounce();
  }
}

// ===== MENU =====
void drawMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  for (int i = 0; i < GAME_COUNT; i++) {
    display.setCursor(0, i * 10);
    if (i == selectedGame) display.print("> ");
    else display.print("  ");
    display.println(gameNames[i]);
  }
  display.display();
}

void handleMenuInput() {
  if (digitalRead(BTN_UP) == LOW) {
    selectedGame = (GameType)((selectedGame - 1 + GAME_COUNT) % GAME_COUNT);
    delay(200);
  }
  if (digitalRead(BTN_DOWN) == LOW) {
    selectedGame = (GameType)((selectedGame + 1) % GAME_COUNT);
    delay(200);
  }
  if (digitalRead(BTN_LEFT) == LOW || digitalRead(BTN_RIGHT) == LOW) {
    switch (selectedGame) {
      case SNAKE: startSnake(); break;
      case FLAPPY: startFlappy(); break;
      case BOUNCE: startBounce(); break;
    }
    delay(300);
  }
}

// ===== SNAKE =====
void startSnake() {
  inMenu = false;
  playingSnake = true;
  playingFlappy = false;
  playingBounce = false;

  length = 3; dx = 1; dy = 0;
  snake[0] = {10, 10};
  snake[1] = {9, 10};
  snake[2] = {8, 10};
  spawnFood();
}

void readSnakeButtons() {
  if (digitalRead(BTN_UP) == LOW && dy == 0)   { dx = 0; dy = -1; }
  if (digitalRead(BTN_DOWN) == LOW && dy == 0) { dx = 0; dy = 1; }
  if (digitalRead(BTN_LEFT) == LOW && dx == 0) { dx = -1; dy = 0; }
  if (digitalRead(BTN_RIGHT) == LOW && dx == 0){ dx = 1; dy = 0; }
}

void moveSnake() {
  Point newHead = {snake[0].x + dx, snake[0].y + dy};
  if (newHead.x < 0 || newHead.x >= SCREEN_WIDTH / CELL_SIZE || newHead.y < 0 || newHead.y >= SCREEN_HEIGHT / CELL_SIZE) {
    gameOver();
    return;
  }
  for (int i = 0; i < length; i++) {
    if (snake[i].x == newHead.x && snake[i].y == newHead.y) {
      gameOver();
      return;
    }
  }
  for (int i = length; i > 0; i--) snake[i] = snake[i - 1];
  snake[0] = newHead;
  if (newHead.x == food.x && newHead.y == food.y) {
    length++;
    if (length > MAX_LENGTH) length = MAX_LENGTH;
    spawnFood();
  }
}

void drawSnake() {
  display.clearDisplay();
  display.fillRect(food.x * CELL_SIZE, food.y * CELL_SIZE, CELL_SIZE, CELL_SIZE, WHITE);
  for (int i = 0; i < length; i++) {
    display.fillRect(snake[i].x * CELL_SIZE, snake[i].y * CELL_SIZE, CELL_SIZE, CELL_SIZE, WHITE);
  }
  display.display();
}

void spawnFood() {
  food.x = random(0, SCREEN_WIDTH / CELL_SIZE);
  food.y = random(0, SCREEN_HEIGHT / CELL_SIZE);
}

// ===== FLAPPY BIRD =====
void startFlappy() {
  inMenu = false;
  playingSnake = false;
  playingFlappy = true;
  playingBounce = false;

  birdY = SCREEN_HEIGHT / 2;
  velocity = 0;
  pipeX = SCREEN_WIDTH;
  gapY = random(10, SCREEN_HEIGHT - gapHeight - 10);
  flappyScore = 0;
}

void readFlappyInput() {
  if (digitalRead(BTN_UP) == LOW) {
    velocity = -4;
  }
}

void updateFlappy() {
  velocity += gravity;
  birdY += velocity;
  pipeX -= 2;

  if (pipeX < -10) {
    pipeX = SCREEN_WIDTH;
    gapY = random(10, SCREEN_HEIGHT - gapHeight - 10);
    flappyScore++;
  }

  if (birdY <= 0 || birdY >= SCREEN_HEIGHT) gameOver();
  if (pipeX < 20 && (birdY < gapY || birdY > gapY + gapHeight)) gameOver();
}

void drawFlappy() {
  display.clearDisplay();
  display.fillRect(15, birdY, 5, 5, WHITE);
  display.fillRect(pipeX, 0, 10, gapY, WHITE);
  display.fillRect(pipeX, gapY + gapHeight, 10, SCREEN_HEIGHT - (gapY + gapHeight), WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Score: ");
  display.print(flappyScore);
  display.display();
}

// ===== BOUNCE BALL =====
void startBounce() {
  inMenu = false;
  playingSnake = false;
  playingFlappy = false;
  playingBounce = true;

  ballX = SCREEN_WIDTH / 2;
  ballY = SCREEN_HEIGHT / 2;
  ballDX = 2;
  ballDY = 2;
}

void readBounceInput() {
  if (digitalRead(BTN_LEFT) == LOW) ballDX = -2;
  else if (digitalRead(BTN_RIGHT) == LOW) ballDX = 2;
}

void updateBounce() {
  ballX += ballDX;
  ballY += ballDY;
  if (ballY <= 0 || ballY >= SCREEN_HEIGHT - 5) ballDY = -ballDY;
  if (ballX <= 0 || ballX >= SCREEN_WIDTH - 5) gameOver();
}

void drawBounce() {
  display.clearDisplay();
  display.fillRect(ballX, ballY, 5, 5, WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Bounce Ball!");
  display.display();
}

// ===== GAME OVER =====
void gameOver() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(20, 25);
  display.println("GAME OVER!");
  display.setCursor(10, 40);
  display.println("Press LEFT or RIGHT");
  display.display();

  playingSnake = false;
  playingFlappy = false;
  playingBounce = false;
  inMenu = false;

  while (true) {
    if (digitalRead(BTN_LEFT) == LOW || digitalRead(BTN_RIGHT) == LOW) {
      delay(300);
      inMenu = true;
      break;
    }
  }
}
// End of file
// This code implements a simple game menu and three games (Snake, Flappy Bird, Bounce Ball) on an ESP32 with an OLED display.