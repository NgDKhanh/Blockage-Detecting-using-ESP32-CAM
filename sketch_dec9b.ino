#define MAX_RESOLUTION_VGA 1

/**
 * Run Edge Impulse FOMO model on the Esp32 camera
 */

#include <FS.h>
#include <SD_MMC.h>
#include <SPI.h>
// #include <Detect_blokage_inferencing.h>
#include <blockage_inferencing.h>
#include "esp32cam.h"
#include "esp32cam/tinyml/edgeimpulse/FOMO.h"

#define MACHINE_PIN 3


using namespace Eloquent::Esp32cam;

Cam cam;
TinyML::EdgeImpulse::FOMO fomo;

uint32_t counter = 1;

//Semaphore for 2 tasks to communicate with each other
static SemaphoreHandle_t binarySemaphore;     

/**
 * Task that handle image processing and saving images to SD card
 */
void imageProcessingTask(void *pvParameter)
{
  while (1)
  {
    if (!cam.capture()) {
        Serial.println(cam.getErrorMessage());
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        return;
    }

    // run FOMO model
    if (!fomo.detectObjects(cam)) {
        Serial.println(fomo.getErrorMessage());
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        return;
    }

    // print found bounding boxes
    if (fomo.hasObjects()) {
        Serial.printf("Found %d objects in %d millis\n", fomo.count(), fomo.getExecutionTimeInMillis());

        fomo.forEach([](size_t ix, ei_impulse_result_bounding_box_t bbox) {
            if (bbox.label == "blockage")
            {
              String filename = String("/blockage_") + counter + ".jpg";

              if (cam.saveTo(SD_MMC, filename)) {
                  Serial.println(filename + " saved to disk");
                  counter += 1;
              }
              else {
                  Serial.println(cam.getErrorMessage());
              }

              // Release the binary semaphore
              xSemaphoreGive(binarySemaphore);
            }
        });
    }
    else {
        Serial.println("No objects detected");
    }
  }
}

/**
 * Task that handle machine controller
 */
void machineControllerTask(void *pvParameter)
{
  while (1)
  {
    // It wait for signal from image processing task
    if (xSemaphoreTake(binarySemaphore, 100))
    {
      // If it takes the signal, the task will turn on the machine
      Serial.println("MACHINE ON!");
      digitalWrite(MACHINE_PIN, HIGH);
      // Wait for 1s then turn off
      vTaskDelay(500);
      digitalWrite(MACHINE_PIN, LOW);
    }
  }
}

void setup() {
    Serial.begin(115200);
    delay(3000);
    Serial.println("Init");

    // Begin camera configuration
    cam.aithinker();
    cam.highQuality();
    cam.highestSaturation();
    cam.vga();

    while (!cam.begin())
        Serial.println(cam.getErrorMessage());

    while (!SD_MMC.begin() || SD_MMC.cardType() == CARD_NONE)
        Serial.println("Cannot init SD Card");

    Serial.println("Init cam and sd card successful!");

    binarySemaphore = xSemaphoreCreateBinary();
    if(binarySemaphore == NULL)
    {
      Serial.println("Semaphore create failed!");
    }
    else
    {
      Serial.println("Semaphore create OK!");
    }

    pinMode(MACHINE_PIN, OUTPUT);

    xTaskCreate(            // Use xTaskCreate() in vanilla FreeRTOS
      imageProcessingTask,  // Function to be called
      "imageProcessing",    // Name of task
      1024 * 10,            // Stack size (bytes in ESP32, words in FreeRTOS)
      NULL,                 // Parameter to pass to function
      1,                    // Task priority (0 to configMAX_PRIORITIES - 1)
      NULL);                // Task handle   

    xTaskCreate(            // Use xTaskCreate() in vanilla FreeRTOS
      machineControllerTask,  // Function to be called
      "machineControl",     // Name of task
      1024,                 // Stack size (bytes in ESP32, words in FreeRTOS)
      NULL,                 // Parameter to pass to function
      1,                    // Task priority (0 to configMAX_PRIORITIES - 1)
      NULL);                // Task handle  
}

void loop() {
  // Do nothing
}