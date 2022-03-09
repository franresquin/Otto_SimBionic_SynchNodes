
#include <esp_now.h>
#include <WiFi.h>

#define HostCom Serial
#define HOST_BAUDRATE 115200

#define UPDATING_INTERVAL_MS 100
#define WARNING_PERIOD_MS 5000

#define INTERRUPT_PIN 21
#define FREEZE_INTERVAL_MS 1000

// Host MAC-Address //
uint8_t Esp_Host_MacAddress[] = {0xA8, 0x03, 0x2A, 0xEA, 0x92, 0x0C};

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
  HostCom.println("[S] Trigger sent");
}

void interrupt_handler(void){

  //HostCom.print("+ [I] Trigger Received -> ");
  if(trigger_input.freeze_receiving == false){
    trigger_input.freeze_receiving = true;
    trigger_input.event_number++;
    flag_send_trigger = true;
    //HostCom.print("Valid");
  }else{
    //HostCom.print("Discarded");
  }

  //HostCom.println();
  
}

void setup() {
  
  //Serial Port to host //
  HostCom.begin(HOST_BAUDRATE);
  while(!HostCom);

  HostCom.println();
  HostCom.println("*** SimBionic Application - Trigger Node ***");

  HostCom.print("* 1. Settting Wifi communication ");
  // ESP NOW Communication //
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    HostCom.println("[WIFI] Error initializing ESP-NOW");
    return;
  }
  esp_now_register_send_cb(OnDataSent);
  memcpy(peerInfo.peer_addr, Esp_Host_MacAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    HostCom.println("** [WIFI] Failed to add peer **");
    return;
  }
  HostCom.println(" -> [ok]");

  HostCom.print("-* My MACAddress: ");
  HostCom.print(WiFi.macAddress());
  HostCom.println(" *-");
  
  // Reset Values //
  HostCom.print("* 2. Settting input PIN ");
  pinMode(trigger_input.PIN, INPUT);
  attachInterrupt(trigger_input.PIN, interrupt_handler, RISING);
  HostCom.println(" -> [ok]");

  HostCom.println("--- Runnning Application ---");
  HostCom.println();
  HostCom.flush();
  
}

void loop() {
  static unsigned long freeze_period=0;
  unsigned long mtime = millis();
  esp_err_t result;
  static bool send_warning = true;
  static unsigned long warning_period = millis();
  
  if(trigger_input.freeze_receiving == true){
    // Send the message to the glass node //
    if(flag_send_trigger == true){
      // Send data to host by WiFi //
      esp_err_t result = esp_now_send(Esp_Host_MacAddress, 
                                     (uint8_t *)&trigger_input.event_number,
                                      sizeof(uint16_t) );
      flag_send_trigger = false;
      freeze_period = mtime+FREEZE_INTERVAL_MS;
      send_warning = false;
      HostCom.println("[L] -> Pin High");
    }

    // Check the freezing time //
    if(mtime >= freeze_period){
      trigger_input.freeze_receiving = false;
      HostCom.println("[L] -> Pin low");
    }
  }

  if(mtime >= warning_period){
    if(send_warning == true){
      HostCom.println("...Waiting for the trigger...");
    }
    send_warning = true;
    warning_period += WARNING_PERIOD_MS;
  }
  
}
