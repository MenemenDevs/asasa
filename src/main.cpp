#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// === Pin Tanımları ===
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define JOY_X_PIN    34
#define JOY_Y_PIN    35
#define JOY_BTN_PIN   4
#define BUZZER_PIN   21
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// === Oyun Türü ===
enum GameType { SNAKE, FLAPPY, BOUNCE, GAME_COUNT };
const char* gameNames[GAME_COUNT] = {"Snake", "Flappy Bird", "Bounce Ball"};
GameType selectedGame = SNAKE;
bool inMenu = true;

// === Snake ===
#define CELL_SIZE 4
#define MAX_LENGTH 100
struct Point { int x; int y; };
Point snake[MAX_LENGTH];
int length = 3, dx = 1, dy = 0;
Point food;
unsigned long lastMove = 0;
int speed = 150;
bool playingSnake = false;

// === Flappy ===
int birdY = SCREEN_HEIGHT / 2;
int velocity = 0;
const int gravity = 1;
int pipeX = SCREEN_WIDTH;
int gapY;
const int gapHeight = 20;
int flappyScore = 0;
bool playingFlappy = false;

// === Bounce ===
int ballX = SCREEN_WIDTH / 2;
int ballY = SCREEN_HEIGHT / 2;
int ballDX = 2;
int ballDY = 2;
bool playingBounce = false;

unsigned long lastUpdate = 0;
const int frameDelay = 40;

// === Fonksiyon Prototipleri ===
void drawMenu();
void handleMenuInput();
void startSnake();
void readSnakeJoystick();
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
void beep();

void setup() {
  Serial.begin(115200);
  pinMode(JOY_BTN_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  Wire.begin(25, 26);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 init failed");
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
      readSnakeJoystick();
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

// === Menü ===
void drawMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  for (int i = 0; i < GAME_COUNT; i++) {
    display.setCursor(0, i * 10);
    display.print(i == selectedGame ? "> " : "  ");
    display.println(gameNames[i]);
  }
  display.display();
}

void handleMenuInput() {
  int y = analogRead(JOY_Y_PIN);
  if (y < 1000) {
    selectedGame = (GameType)((selectedGame - 1 + GAME_COUNT) % GAME_COUNT);
    beep();
    delay(200);
  }
  if (y > 3000) {
    selectedGame = (GameType)((selectedGame + 1) % GAME_COUNT);
    beep();
    delay(200);
  }
  if (digitalRead(JOY_BTN_PIN) == LOW) {
    beep();
    switch (selectedGame) {
      case SNAKE: startSnake(); break;
      case FLAPPY: startFlappy(); break;
      case BOUNCE: startBounce(); break;
    }
    delay(300);
  }
}

// === Snake ===
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

void readSnakeJoystick() {
  int x = analogRead(JOY_X_PIN);
  int y = analogRead(JOY_Y_PIN);
  if (x < 1000 && dx == 0) { dx = -1; dy = 0; }
  if (x > 3000 && dx == 0) { dx = 1; dy = 0; }
  if (y < 1000 && dy == 0) { dx = 0; dy = -1; }
  if (y > 3000 && dy == 0) { dx = 0; dy = 1; }
}

void moveSnake() {
  Point newHead = {snake[0].x + dx, snake[0].y + dy};
  if (newHead.x < 0 || newHead.x >= SCREEN_WIDTH / CELL_SIZE || newHead.y < 0 || newHead.y >= SCREEN_HEIGHT / CELL_SIZE) {
    gameOver(); return;
  }
  for (int i = 0; i < length; i++) {
    if (snake[i].x == newHead.x && snake[i].y == newHead.y) {
      gameOver(); return;
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

// === Flappy ===
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
  if (digitalRead(JOY_BTN_PIN) == LOW) {
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

// === Bounce ===
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
  int x = analogRead(JOY_X_PIN);
  if (x < 1000) ballDX = -2;
  else if (x > 3000) ballDX = 2;
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

// === GAME OVER ===
void gameOver() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(20, 25);
  display.println("GAME OVER!");
  display.setCursor(10, 40);
  display.println("Press Btn...");
  display.display();

  playingSnake = false;
  playingFlappy = false;
  playingBounce = false;
  inMenu = false;

  while (true) {
    if (digitalRead(JOY_BTN_PIN) == LOW) {
      beep();
      delay(300);
      inMenu = true;
      break;
    }
  }
}

// === Buzzer ===
void beep() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(80);
  digitalWrite(BUZZER_PIN, LOW);
}
