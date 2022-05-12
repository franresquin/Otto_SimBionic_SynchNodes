
#include <esp_now.h>
#include <WiFi.h>

#define HostCom Serial
#define HOST_BAUDRATE 115200

#define WARNING_PERIOD_MS 10000
#define BUTTON_UPDATE_MS 100

#define INTERRUPT_PIN 21
#define BUTTON_PIN A2
#define LED_PIN A0

// Host MAC-Address //
//uint8_t Esp_Host_MacAddress[] = {0xA8, 0x03, 0x2A, 0xEA, 0x92, 0x0C};
const uint8_t Esp_Host_MacAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Tagged as Host

esp_now_peer_info_t peerInfo;

typedef struct{
  const uint8_t PIN;
  uint16_t event_number;
  bool listen_trigger;
}trigger_t;

trigger_t trigger_input = {INTERRUPT_PIN, 0, false};
bool flag_send_trigger;

// Button Variable //
bool buttonState;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  HostCom.println("[S] Trigger sent - Confirmed");
}

void interrupt_handler(void){

  //HostCom.print("+ [I] Trigger Received -> ");
  if(trigger_input.listen_trigger == true){
    trigger_input.listen_trigger = false;
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
  //while(!HostCom);

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

  // Check button state //
  pinMode(BUTTON_PIN, INPUT);
  buttonState = digitalRead(BUTTON_PIN);

  // Led Pin //
  pinMode(LED_PIN, OUTPUT);    // sets the digital pin 13 as output
  digitalWrite(LED_PIN, LOW);

  // Flag and globals //
  flag_send_trigger = false;
  
  HostCom.println("--- Runnning Application ---");
  HostCom.println();
  HostCom.flush();
  
}

void loop() {
  unsigned long mtime = millis();
  esp_err_t result;
  static unsigned long warning_period = millis();
  bool curr_button=buttonState;
  static unsigned long time_read_button = millis();

  if(mtime >= time_read_button){
    curr_button = check_buttonState();
    time_read_button += BUTTON_UPDATE_MS;
  }

  // Check button PIN state -> if it changes then enable trigger interrupt //
  if(curr_button != buttonState){
    
    buttonState = curr_button;
    HostCom.print("[B] Button Changed: ");
    HostCom.print(buttonState);
    HostCom.print(" >> ");
    
    if(trigger_input.listen_trigger == false){
      trigger_input.listen_trigger = true;
      digitalWrite(LED_PIN, HIGH);
      HostCom.print("Listen On <<");
    }else{
      trigger_input.listen_trigger = false;  
      digitalWrite(LED_PIN, LOW);
      HostCom.print("Listen Off <<");
    }
    HostCom.println();
  }

  // Check if trigger has been received //
  if(flag_send_trigger == true){
    flag_send_trigger = false;
    digitalWrite(LED_PIN, LOW);
    esp_err_t result = esp_now_send(Esp_Host_MacAddress, 
                                    (uint8_t *)&trigger_input.event_number,
                                    sizeof(uint16_t) );
    HostCom.println("[T] Trigger send >>");
  }

  if(mtime >= warning_period){
    HostCom.println("...Waiting for the trigger...");
    warning_period += WARNING_PERIOD_MS;
  }
  
}

bool check_buttonState(void){
  uint8_t buttonState = digitalRead(BUTTON_PIN);
  bool ret_val = false;
  
  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  if (buttonState == HIGH) {
    // turn LED on:
    //digitalWrite(LED_PIN, HIGH);
    ret_val = true;
  } //else {
    // turn LED off:
    //digitalWrite(LED_PIN, LOW);
  //}

  return ret_val;
}

bool disable_trigger_listen(){
  bool ret_val=false;
  bool but_state = check_buttonState();
  
  if( (but_state != buttonState) || 
      (trigger_input.listen_trigger == false) ){
    //buttonState = but_state;
    ret_val = true;
  }
  
  return ret_val;
  
}
