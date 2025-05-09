// #include <esp_now.h>
// #include <WiFi.h>
// #include "esp_now_master.h"
// add do loop sendData(temperatura, umidadeAR, umidadeSolo, valorChuva)
// add to setup setupEspNow()


uint8_t broadcastAddress1[] = {0xCC, 0xDB, 0xA7, 0x63, 0x96, 0x38};
                              //  substituir aqui
typedef struct SensorsData {
  int temperature;
  int humidity;
  int rainStatus;
  int soilMoisture;
} SensorsData;

SensorsData sensor;

esp_now_peer_info_t peerInfo;

// callback when data is sent

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  char macStr[18];
  Serial.print("Packet to: ");
  // Copies the sender mac address to a string
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print(macStr);
  Serial.print(" send status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}
 
void setupEspNow() {
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  esp_now_register_send_cb(OnDataSent);
   
  // register peer
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
   
  memcpy(peerInfo.peer_addr, broadcastAddress1, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
}

void sendData(float temp, float hum, float s_moist, float rain) {
  sensor.temperature = temp;
  sensor.humidity = hum;
  sensor.rainStatus = rain;
  sensor.soilMoisture = s_moist;
 
  esp_err_t result = esp_now_send(0, (uint8_t *) &sensor, sizeof(SensorsData));
   
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
}
