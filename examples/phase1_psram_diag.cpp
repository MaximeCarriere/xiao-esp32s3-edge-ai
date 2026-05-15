#include <Arduino.h>

void printInventory()
{
  Serial.println("\n=== XIAO ESP32-S3 Sense :: Board Inventory ===\n");

  Serial.printf("Chip model:        %s\n", ESP.getChipModel());
  Serial.printf("Chip revision:     %d\n", ESP.getChipRevision());
  Serial.printf("CPU cores:         %d\n", ESP.getChipCores());
  Serial.printf("CPU frequency:     %d MHz\n", ESP.getCpuFreqMHz());
  Serial.printf("SDK version:       %s\n", ESP.getSdkVersion());

  Serial.println();

  Serial.printf("Flash size:        %d bytes (%.1f MB)\n",
                ESP.getFlashChipSize(), ESP.getFlashChipSize() / 1048576.0);
  Serial.printf("Flash speed:       %d MHz\n", ESP.getFlashChipSpeed() / 1000000);

  Serial.println();

  Serial.printf("Internal heap:     %d bytes free of %d total\n",
                ESP.getFreeHeap(), ESP.getHeapSize());

  if (psramFound())
  {
    Serial.println("PSRAM:             FOUND");
    Serial.printf("PSRAM size:        %d bytes (%.1f MB)\n",
                  ESP.getPsramSize(), ESP.getPsramSize() / 1048576.0);
    Serial.printf("PSRAM free:        %d bytes (%.1f MB)\n",
                  ESP.getFreePsram(), ESP.getFreePsram() / 1048576.0);
  }
  else
  {
    Serial.println("PSRAM:             NOT FOUND  <-- check platformio.ini");
  }

  Serial.println("\n=== Done. Will repeat in 5s. ===");
}

void setup()
{
  Serial.begin(115200);
  delay(1000);
}

void loop()
{
  printInventory();
  delay(5000);
}