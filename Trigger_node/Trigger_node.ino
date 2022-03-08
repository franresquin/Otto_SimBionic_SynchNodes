  
#include <esp_now.h>
#include <WiFi.h>

#define HostCom Serial
#define HOST_BAUDRATE 115200

#define UPDATING_INTERVAL_MS 100

#define INTERRUPT_PIN 21
#define FREEZE_INTERVAL_MS 1000

uint8_t Esp_Host_MacAddress[] = {0xC4, 0xDD, 0x57, 0x9C, 0xBD, 0x2C}; // Tagged as Host

esp_now_peer_info_t peerInfo;

typedef struct{
  const uint8_t PIN;
  uint16_t event_number;
  bool freeze_receiving;
}trigger_t;

trigger_t trigger_input = {INTERRUPT_PIN, 0, false};
unsigned long active_period=0;
bool flag_send_trigger;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  
  //HostCom.print("\r\nLast Packet Send Status: ");
  //HostCom.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  
}

void interrupt_handler(void){
  
  if(trigger_input.freeze_receiving == false){
    trigger_input.freeze_receiving = true;
    trigger_input.event_number++;
    flag_send_trigger = true;
  }
  
}

void setup() {
  
  //Serial Port to host //
  HostCom.begin(HOST_BAUDRATE);
  
  // ESP NOW Communication //
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    HostCom.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_send_cb(OnDataSent);
  memcpy(peerInfo.peer_addr, Esp_Host_MacAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    HostCom.println("Failed to add peer");
    return;
  }

  // Reset Values //
  pinMode(trigger_input.PIN, INPUT);
  attachInterrupt(trigger_input.PIN, interrupt_handler, RISING);
  
}

void loop() {
  static unsigned long freeze_period=0;
  unsigned long mtime = millis();
  esp_err_t result;
  
  if(trigger_input.freeze_receiving == true){
      
    // Send the message to the glass node //
    if(flag_send_trigger == true){
      // Send data to host by WiFi //
      esp_err_t result = esp_now_send(Esp_Host_MacAddress, 
                                     (uint8_t *)&trigger_input.event_number,
                                      sizeof(uint16_t) );
      flag_send_trigger = false;
      freeze_period = mtime+FREEZE_INTERVAL_MS;
    }

    // Check the freezing time //
    if(mtime >= freeze_period){
      trigger_input.freeze_receiving = false;
    }
    
  }
  
}
