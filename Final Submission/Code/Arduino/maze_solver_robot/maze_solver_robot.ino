// Arduino Maze Solver Robot Code (Left-Hand Rule with Path Simplification)

// === أداة التحكم في وضع الاختبار ===
// خليها true عشان تختبر الحساسات واللوجيك على السيريال مونيتور والمواتير واقفة
// ولما تتأكد إن كله تمام، خليها false عشان الروبوت يشتغل فعلياً
bool DRY_RUN_MODE = true; 
// ===================================

#define ENA_PIN 11 // Enable Pin for Left Motors (PWM)
#define IN1_PIN 9  // Input 1 for Left Motor
#define IN2_PIN 8  // Input 2 for Left Motor

#define ENB_PIN 5  // Enable Pin for Right Motors (PWM)
#define IN3_PIN 7  // Input 3 for Right Motor
#define IN4_PIN 6  // Input 4 for Right Motor

// Define Ultrasonic Sensor Pins (Arduino Mega Pins)
#define TRIG_RIGHT_PIN 26
#define ECHO_RIGHT_PIN 27

#define TRIG_FRONT_PIN 46
#define ECHO_FRONT_PIN 47

#define TRIG_LEFT_PIN 36  
#define ECHO_LEFT_PIN 37

// Define Button Pins
#define ON_BUTTON_PIN 42
#define OFF_BUTTON_PIN 43

// Motor Speed (0-255)
const int motorSpeed = 120; 

// Distance Thresholds (in cm)
const int WALL_THRESHOLD = 15; 
const int FRONT_CLEAR_THRESHOLD = 18; 

enum RobotState { STOPPED, EXPLORING, RETRACING };
RobotState currentState = STOPPED;

#define MAX_PATH_LENGTH 100
char path[MAX_PATH_LENGTH];
int pathIndex = 0;

// Function Prototypes
void moveForward();
void moveBackward();
void turnLeft();
void turnRight();
void stopMotors();
long readDistance(int trigPin, int echoPin);
void setupSensors();
void setupMotors();
void setupButtons();
void solveMaze();
void recordMove(char move);
void simplifyPath();

void setup() {
  Serial.begin(9600);
  setupMotors();
  setupSensors();
  setupButtons();
  
  Serial.println("=====================================");
  if(DRY_RUN_MODE) {
    Serial.println("  DRY RUN MODE: ON (Motors Disabled) ");
  } else {
    Serial.println("  LIVE MODE: ON (Motors Enabled)     ");
  }
  Serial.println("=====================================");
  Serial.println("Robot Ready. Press ON button to start.");
}

void loop() {
  if (digitalRead(ON_BUTTON_PIN) == LOW) { 
    delay(50); 
    if (digitalRead(ON_BUTTON_PIN) == LOW) {
      currentState = EXPLORING;
      Serial.println("-> System: Robot Started - Exploring Mode");
      pathIndex = 0; 
      for(int i=0; i<MAX_PATH_LENGTH; i++) path[i] = ' ';
      while(digitalRead(ON_BUTTON_PIN) == LOW); 
    }
  }

  if (digitalRead(OFF_BUTTON_PIN) == LOW) {
    delay(50); 
    if (digitalRead(OFF_BUTTON_PIN) == LOW) {
      currentState = STOPPED;
      stopMotors();
      Serial.println("-> System: Robot Stopped by User");
      while(digitalRead(OFF_BUTTON_PIN) == LOW); 
    }
  }

  if (currentState == EXPLORING) {
    solveMaze();
  } else if (currentState == RETRACING) {
    stopMotors();
    Serial.println("-> System: Maze Solved! Path recorded.");
    simplifyPath();
    currentState = STOPPED;
  }
}

// --- Motor Control Functions ---
void setupMotors() {
  pinMode(ENA_PIN, OUTPUT);
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);
  pinMode(ENB_PIN, OUTPUT);
  pinMode(IN3_PIN, OUTPUT);
  pinMode(IN4_PIN, OUTPUT);
  stopMotors();
}

void moveForward() {
  if (!DRY_RUN_MODE) {
    analogWrite(ENA_PIN, motorSpeed);
    digitalWrite(IN1_PIN, HIGH);
    digitalWrite(IN2_PIN, LOW);
    analogWrite(ENB_PIN, motorSpeed);
    digitalWrite(IN3_PIN, HIGH);
    digitalWrite(IN4_PIN, LOW);
  }
  Serial.println("Action: Moving Forward [F]");
}

void moveBackward() {
  if (!DRY_RUN_MODE) {
    analogWrite(ENA_PIN, motorSpeed);
    digitalWrite(IN1_PIN, LOW);
    digitalWrite(IN2_PIN, HIGH);
    analogWrite(ENB_PIN, motorSpeed);
    digitalWrite(IN3_PIN, LOW);
    digitalWrite(IN4_PIN, HIGH);
  }
  Serial.println("Action: Moving Backward [B]");
}

void turnLeft() {
  if (!DRY_RUN_MODE) {
    analogWrite(ENA_PIN, motorSpeed);
    digitalWrite(IN1_PIN, LOW);
    digitalWrite(IN2_PIN, HIGH);
    analogWrite(ENB_PIN, motorSpeed);
    digitalWrite(IN3_PIN, HIGH);
    digitalWrite(IN4_PIN, LOW); 
  }
  Serial.println("Action: Turning Left [L]");
  delay(400); 
  stopMotors();
  delay(100); 
}

void turnRight() {
  if (!DRY_RUN_MODE) {
    analogWrite(ENA_PIN, motorSpeed);
    digitalWrite(IN1_PIN, HIGH);
    digitalWrite(IN2_PIN, LOW);  
    analogWrite(ENB_PIN, motorSpeed);
    digitalWrite(IN3_PIN, LOW);
    digitalWrite(IN4_PIN, HIGH); 
  }
  Serial.println("Action: Turning Right [R]");
  delay(400); 
  stopMotors();
  delay(100); 
}

void stopMotors() {
  // We always want to be able to stop, even in dry run mode just to be safe
  analogWrite(ENA_PIN, 0);
  digitalWrite(IN1_PIN, LOW);
  digitalWrite(IN2_PIN, LOW);
  analogWrite(ENB_PIN, 0);
  digitalWrite(IN3_PIN, LOW);
  digitalWrite(IN4_PIN, LOW);
  // Only print if we are actually exploring to avoid spamming
  if (currentState == EXPLORING) Serial.println("Action: Stopping Motors");
}

// --- Sensor Control Functions ---
void setupSensors() {
  pinMode(TRIG_RIGHT_PIN, OUTPUT);
  pinMode(ECHO_RIGHT_PIN, INPUT);
  pinMode(TRIG_FRONT_PIN, OUTPUT);
  pinMode(ECHO_FRONT_PIN, INPUT);
  pinMode(TRIG_LEFT_PIN, OUTPUT);
  pinMode(ECHO_LEFT_PIN, INPUT);
}

long readDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH);
  long distance = duration * 0.034 / 2;
  
  // Filter out noisy extreme readings
  if(distance > 400 || distance <= 0) return 400; 
  return distance; 
}

// --- Button Setup ---
void setupButtons() {
  pinMode(ON_BUTTON_PIN, INPUT_PULLUP); 
  pinMode(OFF_BUTTON_PIN, INPUT_PULLUP);
}

// --- Maze Solving Logic (Left-Hand Rule) ---
void solveMaze() {
  long distanceLeft = readDistance(TRIG_LEFT_PIN, ECHO_LEFT_PIN);
  long distanceFront = readDistance(TRIG_FRONT_PIN, ECHO_FRONT_PIN);
  long distanceRight = readDistance(TRIG_RIGHT_PIN, ECHO_RIGHT_PIN);

  // تنسيق الطباعة عشان تكون واضحة وسهلة القراءة على الشاشة
  Serial.print("Sensors -> L: "); Serial.print(distanceLeft);
  Serial.print("cm | F: "); Serial.print(distanceFront);
  Serial.print("cm | R: "); Serial.print(distanceRight); 
  Serial.print("cm  ==>  ");

  if (distanceLeft > WALL_THRESHOLD) {
    turnLeft();
    moveForward();
    recordMove('L');
  } else if (distanceFront > FRONT_CLEAR_THRESHOLD) {
    moveForward();
    recordMove('F');
  } else if (distanceRight > WALL_THRESHOLD) {
    turnRight();
    moveForward();
    recordMove('R');
  } else {
    Serial.println("DEAD END: Turning Around");
    turnRight(); 
    turnRight(); 
    moveForward();
    recordMove('B'); 
  }
  
  // زيادة الـ Delay في الـ Dry Run عشان تلحق تقرأ المخرجات بعينك
  if(DRY_RUN_MODE) {
    delay(500); 
  } else {
    delay(50); 
  }
}

// --- Path Tracking and Simplification ---
void recordMove(char move) {
  if (pathIndex < MAX_PATH_LENGTH) {
    path[pathIndex++] = move;
  }
}

void simplifyPath() {
  Serial.print("Original Path: ");
  for (int i = 0; i < pathIndex; i++) {
    Serial.print(path[i]);
  }
  Serial.println();
  Serial.println("System: End of Maze. Robot Stopped.");
  stopMotors();
  currentState = STOPPED;
}