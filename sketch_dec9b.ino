// 27_EdgeImpulse_FOMO.ino
#define MAX_RESOLUTION_VGA 1

/**
 * Run Edge Impulse FOMO model on the Esp32 camera
 */

// replace with the name of your library
#include <FS.h>
#include <SD_MMC.h>
#include <SPI.h>
#include <khanhdtvt192934-project-1_inferencing.h>
#include "esp32cam.h"
#include "esp32cam/tinyml/edgeimpulse/FOMO.h"


using namespace Eloquent::Esp32cam;

Cam cam;
TinyML::EdgeImpulse::FOMO fomo;

uint32_t counter = 1;

static SemaphoreHandle_t binarySemaphore;     

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
            if (bbox.label == "nivia")
            {
              String filename = String("/nivia_") + counter + ".jpg";

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

void machineControllerTask(void *pvParameter)
{
  while (1)
  {
    if (xSemaphoreTake(binarySemaphore, 100))
    {
      Serial.println("LED ON!");
    }
  }
}

void setup() {
    Serial.begin(115200);
    delay(3000);
    Serial.println("Init");


    /**
     * Replace with your camera model.
     * Available: aithinker, m5, m5wide, wrover, eye, ttgoLCD
     */
    cam.aithinker();
    cam.highQuality();
    cam.highestSaturation();
    cam.vga();

    while (!cam.begin())
        Serial.println(cam.getErrorMessage());

    while (!SD_MMC.begin() || SD_MMC.cardType() == CARD_NONE)
        Serial.println("Cannot init SD Card");

    Serial.println("Init successful!");

    binarySemaphore = xSemaphoreCreateBinary();
    if(binarySemaphore == NULL)
    {
      Serial.println("Semaphore create failed!");
    }
    else
    {
      Serial.println("Semaphore create OK!");
    }

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
  
}