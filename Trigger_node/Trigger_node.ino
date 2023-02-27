
#include <esp_now.h>
#include <WiFi.h>

#define HostCom Serial
#define HOST_BAUDRATE 115200

#define WARNING_PERIOD_MS 10000
#define BUTTON_UPDATE_MS 100

#define INTERRUPT_PIN 21
#define BUTTON_PIN A2
#define LED_PIN A0

#define NUM_TRIGGERS_TO_TRANSMIT 2
#define TRIGGER_BLOCKING_TIME_MS 2000

// Host MAC-Address //
//uint8_t Esp_Host_MacAddress[] = {0xA8, 0x03, 0x2A, 0xEA, 0x92, 0x0C};
const uint8_t Esp_Host_MacAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Tagged as Host

esp_now_peer_info_t peerInfo;

typedef struct{
	const uint8_t PIN;
	uint16_t event_number;
	bool listen_trigger;
  unsigned int block_time;
}trigger_t;

trigger_t trigger_input = {INTERRUPT_PIN, 0, false, 0};
bool flag_send_trigger;
unsigned int tgg_counter=0;

//- Button Variable -//
bool buttonState;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  //HostCom.println("[S] Trigger sent - Confirmed");
}

//- Callback executed when a trigger is received -//
void trigger_interrupt_handler(void){
  unsigned int mtime = millis();

  //HostCom.print("+ [I] Trigger Received -> ");
  if( (trigger_input.listen_trigger == true) && (mtime >= trigger_input.block_time) ){
    if( trigger_input.event_number++ >= (NUM_TRIGGERS_TO_TRANSMIT-1) ){
      trigger_input.listen_trigger = false;
    }
    flag_send_trigger = true;
    trigger_input.block_time = mtime+TRIGGER_BLOCKING_TIME_MS;
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
  //- Setting up ESP NOW Communication -//
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

  //- TRigger PIN -//
  HostCom.print("* 2. Settting input Trigger PIN ");
  pinMode(trigger_input.PIN, INPUT);
  attachInterrupt(trigger_input.PIN, trigger_interrupt_handler, RISING);
  HostCom.println(" -> [ok]");

  //- Check button state -> Pin to enable trigger transmission -//
  pinMode(BUTTON_PIN, INPUT);
  buttonState = digitalRead(BUTTON_PIN);

  //- Feedback Led Pin -> it is on when trigger is transmited -//
  pinMode(LED_PIN, OUTPUT);    // sets the digital pin 13 as output
  digitalWrite(LED_PIN, LOW);

  //- Flag and globals -//
  flag_send_trigger = false;
  trigger_input.block_time = 0;

  HostCom.println("--- Runnning Application ---");
  HostCom.println();
  HostCom.flush();

  tgg_counter = 0;
  
}

void loop() {
  unsigned long mtime = millis();
  esp_err_t result;
  static unsigned long warning_period = millis();
  bool curr_button=buttonState;
  static unsigned long time_read_button = millis();

  // Check the button state high or low every "BUTTON_UPDATE_MS" milliseconds //
  // Does not need to check the button to fast -> avoid wrong reading do to bouncing effects //
  if(mtime >= time_read_button){
    curr_button = check_buttonState();
    time_read_button += BUTTON_UPDATE_MS;
  }

  //- Check button PIN state -> if it changes then enable trigger transmision -//
  if(curr_button != buttonState){
    
    buttonState = curr_button;
    // HostCom.print("[B] Button Changed: ");
    // HostCom.print(buttonState);
    HostCom.print(" >> ");
    
    if(trigger_input.listen_trigger == false){
      trigger_input.listen_trigger = true;
      trigger_input.event_number = 0;
      digitalWrite(LED_PIN, HIGH);
      HostCom.print("Listen On <<");
    }else{
      trigger_input.listen_trigger = false;
      trigger_input.event_number = 0;
      digitalWrite(LED_PIN, LOW);
      HostCom.print("Listen Off <<");
    }
    HostCom.println();
  }

  //- Check if trigger has been received -//
  if(flag_send_trigger == true){
    flag_send_trigger = false;
    esp_err_t result = esp_now_send(Esp_Host_MacAddress, 
                                    (uint8_t *)&trigger_input.event_number,
                                    sizeof(uint16_t) );
    if(!trigger_input.listen_trigger){
      digitalWrite(LED_PIN, LOW);
    }
    tgg_counter++;
    HostCom.print("[T] Trigger -> C: ");
    HostCom.print(tgg_counter);
    HostCom.println("; <<");
  }

  if(mtime >= warning_period){
    HostCom.print("...Waiting for the trigger; C: ");
    HostCom.print(tgg_counter);
    HostCom.println("; ...");
    warning_period += WARNING_PERIOD_MS;
  }
  
}

bool check_buttonState(void){
  uint8_t bstate = digitalRead(BUTTON_PIN);
  bool ret_val = false;
  
  // check if the pushbutton is pressed. If it is, the button is HIGH:
  if (bstate == HIGH) {
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
    ret_val = true;
  }
  
  return ret_val;
  
}
