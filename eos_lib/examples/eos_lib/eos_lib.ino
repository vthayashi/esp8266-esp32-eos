
/*  eos_lib.h         1.1.2
    Arduino           14
    ESP32             1.0.6   https://github.com/espressif/arduino-esp32  https://dl.espressif.com/dl/package_esp32_index.json 
    ESP8266           2.7.4   https://github.com/esp8266/Arduino          http://arduino.esp8266.com/stable/package_esp8266com_index.json 
    TimeLib.h         1.6     by Michael Margolis   https://github.com/PaulStoffregen/Time
    qrcode.h          0.0.1   by Richard Moore      https://github.com/ricmoo/QRCode
    SSD1306Wire.h     4.0.2   by ThingPulse         https://github.com/ThingPulse/esp8266-oled-ssd1306
    ESP32Ping.h       1.5     by Daniele Colanardi  https://github.com/marian-craciunescu/ESP32Ping
    ESP8266Ping.h     1.0     by Daniele Colanardi  https://github.com/dancol90/ESP8266Ping
    TFT_eSPI.h                by Bodmer             https://github.com/Bodmer/TFT_eSPI
    OneWire           2.3.5   by Jim Studt          https://github.com/PaulStoffregen/OneWire
    PubSubClient.h    2.8     by Nick O'Leary       https://github.com/knolleary/pubsubclient
*/

#define Versao "eos_lib"

#ifdef ESP8266  // D1 mini
  #define   blink_led   D4
#endif
#ifdef ESP32    // ESP32 Dev Module
  #define   blink_led   2
#endif

#include <PubSubClient.h>
#include <eos_lib.h>

void setup() {
  eos_setup();
  // put your setup code here, to run once:

}

void loop() {
  eos_loop();
  // put your main code here, to run repeatedly:

}

///// timers

void ticker50(){
    
}

void a_loop100(uint32_t cont){
    
}

void a_second(time_t t){
  
}

void a_minute(time_t t){
    
}

void a_hour(time_t t){
  
}

void a_day(time_t t){
}

void a_month(time_t t){
}

///// events

void on_loop_start(){
    
}

void on_reset(){
  
}

void on_update(){
  
}

void on_wifi_status(){
    
}

/////

void a_read_log(String line){ // Recuperar dados do log
    
}

String a_json(String str){
  String pg="";
  return pg;
}

//

void a_config_write(){
  
}

void a_config_read(){
  
}

///// MQTT

void mqtt_on_connect(){
  
}

void mqtt_callback(String topic, String msg){
  
}

//
