
#include <esp_now.h>
#include <WiFi.h>

#define HostCom   Serial
#define HOST_BAUDRATE 115200

#define TRIGGER_PIN A2
#define TRIGGER_HIGH_INTERVAL_MS 500
#define WARNING_PERIOD_MS 5000

typedef struct{
  const uint8_t PIN;
  uint16_t event_number;
  bool trigger_received;
}trigger_t;

trigger_t trigger_signal = {TRIGGER_PIN, 0, false};
bool enable_trigger = false;

void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len) {
  uint16_t *pbuff;

  //HostCom.print("+ [I] Trigger Received -> ");
  if(enable_trigger == false){
    // Load data to the buffer //
    pbuff = (uint16_t*)incomingData;
    
    // Load data to the official data structure //
    memcpy(&trigger_signal.event_number, pbuff, sizeof(uint16_t));
    trigger_signal.trigger_received = true;

    //HostCom.print("Valid");
  }else{
    HostCom.print("Discarded");  
  }
  //HostCom.println();
}

void setup() {
  
  HostCom.begin(HOST_BAUDRATE);
  
  HostCom.println();
  HostCom.println("*** SimBionic Application - Glass Node ***");

  HostCom.print("* 1. Settting Wifi communication ");
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    HostCom.println("Error initializing ESP-NOW"); 
//    error=1;
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);
  HostCom.println(" -> [ok]");

  HostCom.print("-* My MACAddress: ");
  HostCom.print(WiFi.macAddress());
  HostCom.println(" *-");

  
  // Trigger pin //
  HostCom.print("* 2. Settting input PIN ");
  pinMode(trigger_signal.PIN, OUTPUT);
  digitalWrite(trigger_signal.PIN, LOW);
  enable_trigger = false;
  HostCom.println(" -> [ok]");

  // Running the loop application //
  HostCom.println("--- Runnning Application ---");
  HostCom.println();
  HostCom.flush();
  
}

void loop() {
  static unsigned long trigger_high_period=0;
  unsigned long mtime = millis();
  static bool send_warning = true;
  static unsigned long warning_period = millis();
  
  if(trigger_signal.trigger_received == true){
      // set output pin to HIGH //
      digitalWrite(trigger_signal.PIN, HIGH);
      enable_trigger = true;
      
      trigger_high_period = mtime+TRIGGER_HIGH_INTERVAL_MS;
      trigger_signal.trigger_received = false;

      send_warning = false;
      
      HostCom.println("[L] -> Pin High");
  }

  if( (mtime >= trigger_high_period) && (enable_trigger == true) ){
    // set output pin to LOW //
    digitalWrite(trigger_signal.PIN, LOW);
    enable_trigger = false;

    HostCom.println("[L] -> Pin Low");
  }

  if(mtime >= warning_period){
    if(send_warning == true){
      HostCom.println("...Waiting for the trigger...");
    }
    send_warning = true;
    warning_period += WARNING_PERIOD_MS;
  }
    
}
