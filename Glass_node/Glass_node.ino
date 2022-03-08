
#include <esp_now.h>
#include <WiFi.h>

#define HostCom   Serial
#define HOST_BAUDRATE 115200

#define TRIGGER_PIN 21
#define TRIGGER_HIGH_INTERVAL_MS 500

typedef struct{
  const uint8_t PIN;
  uint16_t event_number;
  bool trigger_received;
}trigger_t;

trigger_t trigger_signal = {TRIGGER_PIN, 0, false};
bool enable_trigger = false;

void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len) {
  uint16_t *pbuff;

  if(enable_trigger == false){
    // Load data to the buffer //
    pbuff = (uint16_t*)incomingData;
    
    // Load data to the official data structure //
    memcpy(&trigger_signal.event_number, pbuff, sizeof(uint16_t));
    trigger_signal.trigger_received = true;
  }
  // Print, for debugging //
//  HostCom.print(">> rcv: ");
//  HostCom.print(len);
//  HostCom.print(" -> Node: ");
//  HostCom.print(pwifi_buff->id);
//  HostCom.print("; temp: ");
//  HostCom.print(pwifi_buff->temp_intgr);
//  HostCom.print(".");
//  HostCom.print(pwifi_buff->temp_fractnl);
//  HostCom.print("; Enc: ");
//  HostCom.print(pwifi_buff->encdr_abslt_val);
//  HostCom.println(); 
  
}

void setup() {
  
  HostCom.begin(HOST_BAUDRATE);

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    HostCom.println("Error initializing ESP-NOW"); 
//    error=1;
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);

  // Trigger pin //
  pinMode(trigger_signal.PIN, OUTPUT);
  digitalWrite(trigger_signal.PIN, LOW);
  enable_trigger = false;
  
}

void loop() {
  static unsigned long trigger_high_period=0;
  unsigned long mtime = millis();
  
  if(trigger_signal.trigger_received == true){
      // set output pin to HIGH //
      digitalWrite(trigger_signal.PIN, HIGH);
      enable_trigger = true;
      
      trigger_high_period = mtime+TRIGGER_HIGH_INTERVAL_MS;
      trigger_signal.trigger_received = false;
  }

  if( (mtime >= trigger_high_period) && (enable_trigger == true) ){
    // set output pin to LOW //
    digitalWrite(trigger_signal.PIN, LOW);
    enable_trigger = false;
  }
    
}
