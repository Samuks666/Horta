
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>

typedef struct SensorsData {
  int temperature;
  int humidity;
  int rainStatus;
  int soilMoisture;
} SensorsData;

SensorsData myData;

// print mac address to use on the master
void get_MAC_address(){
  
  Serial.print("ESP Board MAC Address: ");
 
  WiFi.STA.begin();
  uint8_t baseMac[6];
  esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, baseMac);
  if (ret == ESP_OK) {
    Serial.printf("%02x:%02x:%02x:%02x:%02x:%02x\n",
                  baseMac[0], baseMac[1], baseMac[2],
                  baseMac[3], baseMac[4], baseMac[5]);
  } else {
    Serial.println("Failed to read MAC address");
  }

  Serial.print("Endereço MAC com wifi.h: ");
  Serial.println(WiFi.macAddress()); // retorna o endereço MAC do dispositivo
}
 
//callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("x: ");
  Serial.println(myData.temperature);
  Serial.print("y: ");
  Serial.println(myData.humidity);
  Serial.println();
}
 
void setup() {
  //Initialize Serial Monitor
  Serial.begin(115200);
  
  //Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  //Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  get_MAC_address();

  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {

}
