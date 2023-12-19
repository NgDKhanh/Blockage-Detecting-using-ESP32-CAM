// Motor A
int motor1Pin1 = 27; 
int motor1Pin2 = 26; 
int enable1Pin = 14; 
int inttrptPin = 25;

// semaphore for intterupt
SemaphoreHandle_t binarySemaphore = NULL;

void IRAM_ATTR isr() 
{
	xSemaphoreGiveFromISR(binarySemaphore, NULL);
}

void controlMotorTask(void *pvPeripheral)
{
  while (1)
  {
    if (xSemaphoreTake(binarySemaphore, 100))
    {
      // Move the DC motor forward at maximum speed
      Serial.println("Moving Forward");
      digitalWrite(motor1Pin1, LOW);
      digitalWrite(motor1Pin2, HIGH); 
      delay(2000);

      // Stop the DC motor
      Serial.println("Motor stopped");
      digitalWrite(motor1Pin1, LOW);
      digitalWrite(motor1Pin2, LOW);
    }
  }
}

void setup() {
  // sets the pins as outputs:
  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  pinMode(enable1Pin, OUTPUT);

  Serial.begin(115200);

  pinMode(inttrptPin, INPUT);
	attachInterrupt(inttrptPin, isr, RISING);

  binarySemaphore = xSemaphoreCreateBinary();

  xTaskCreate(controlMotorTask, "controlMotor", 1024 * 2, NULL, 2, NULL);
  xTaskCreate(testIntTask, "testInt", 1024, NULL, 2, NULL);
}

void loop() {
  
}
