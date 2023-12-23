#include <LiquidCrystal_I2C.h>

// set the LCD number of columns and rows
int lcdColumns = 16;
int lcdRows = 2;

// set LCD address, number of columns and rows
// if you don't know your display address, run an I2C scanner sketch
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows); 

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

      // clears the display to print new message
      lcd.clear();
      // set cursor to first column, first row
      lcd.setCursor(0, 0);
      // print message
      lcd.print("Blockage");

      delay(2000);

      // Stop the DC motor
      Serial.println("Motor stopped");
      digitalWrite(motor1Pin1, LOW);
      digitalWrite(motor1Pin2, LOW);

      // clears the display to print new message
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("No Blockage");
    }
  }
}

void setup() {
  // sets the pins as outputs:
  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  pinMode(enable1Pin, OUTPUT);

  Serial.begin(115200);

  // initialize LCD
  lcd.init();
  // turn on LCD backlight                      
  lcd.backlight();

  pinMode(inttrptPin, INPUT);
	attachInterrupt(inttrptPin, isr, RISING);

  binarySemaphore = xSemaphoreCreateBinary();

  xTaskCreate(controlMotorTask, "controlMotor", 1024 * 2, NULL, 2, NULL);
}

void loop() {
  
}
