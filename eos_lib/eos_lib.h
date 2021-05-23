#ifndef _eos_lib_h_
#define _eos_lib_h_

/*
	versao 1.0.1
	versao 1.0.2 	js?mvrms=0 (0 a 5 ou gpio)
	versao 1.0.3 	js?fields=analogRead 
	versao 1.0.4 	js?fields=an_pins
	versao 1.0.5 	String wifiscan()
	versao 1.0.6 	RFRX
	versao 1.0.7 	rftx
	versao 1.0.8 	String http_req(String url);
	versao 1.0.9  	oled SSD1306
	versao 1.0.10 	oled SH1106
	versao 1.0.11 	tft drawBmp("/bmp01.bmp", 0, 0);
	versao 1.0.12 	tft drawJpg("/jpg01.jpg", 0, 0);
	versao 1.0.13 	#define ds18_pin | float ds18b20();
	versao 1.0.14	incluir oled flip no config.txt
	versao 1.0.15	incluir versao no config.txt
	versao 1.0.16	wifi.htm interno
	versao 1.0.17	defined default sda scl for esp32
	versao 1.0.18	float read_dist(float temperature=25.0); HC-SR04 Ultrasonic Distance Sensor
	versao 1.0.19	eliminado:	esp8266 #include <WiFiUdp.h>
	versao 1.0.20	adicionado: uint32_t ESP_getChipId();
	versao 1.0.21	mqtt #include <PubSubClient.h>
	versao 1.0.22	ESP32 edit.htm ok
	versao 1.0.23	/24h.txt, /30d.txt
	versao 1.0.24
	versao 1.0.26
	versao 1.0.27	mqtt json
	versao 1.1.1	eos_setup(); eos_loop();
	versao 1.1.2	max_loop_wdt_cnt, reset if loop_wdt_cnt>1000
*/

#if defined (ESP32)
	#include 	<SPIFFS.h>
	#include 	<WiFi.h>
	#include 	<WebServer.h>
	#include 	<ESPmDNS.h>
	#include 	<ESP32Ping.h>
	#include 	<Update.h>
	WebServer	server(80);
	#include 	<HTTPClient.h>
	HTTPClient 	http;
	#include <TimeLib.h>
#else	// (ESP8266)
	#include <Arduino.h>
	#include <TimeLib.h>
	#include <FS.h>
	#include <ESP8266WiFi.h>
	#include <WiFiClient.h>
	#include <ESP8266mDNS.h>
	#include <ESP8266WebServer.h>
	#include <ESP8266Ping.h>
	ESP8266WebServer server(80);
	#include <ESP8266HTTPClient.h>
	HTTPClient http;
	WiFiClient client2;
#endif

WiFiUDP 	Udp;
File 		uploadFile;

String  	essid = "";
String  	epass = "";
uint8_t 	ipMode= 0; 	// 0=auto
String 		DeviceID;
String 		apPass = "";
uint16_t  	APmode=5;	// 0 minutos
uint8_t   	prev_sec=60, prev_min=60, prev_hour=24, prev_day=32, prev_month=13;
uint16_t  	prev_year=0;
const int 	timeZone = -3;
uint16_t	timeNTP_interval = 60;
uint16_t   	timeNTP_cnt=2, timeNTP_errCnt=0;
String    	ntpServer[3]={"132.163.97.1","132.163.97.2","200.160.7.186"};
bool 		fsok=false;
uint16_t  	ap_disconn_cnt;
uint8_t 	station_num;
uint32_t 	loop100_prevmillis;
uint32_t 	loop100_cnt;
uint8_t 	rst_cnt=0;
String  	log_recent;
String 		log_buffer;
String		homePage="";
IPAddress 	prev_localIP(0,0,0,0);
#ifdef ESP32 	// an_pins
	uint8_t 	an_pins[]={36,39,34,35,32,33}; // ESP32S
#else			// ESP8266
	uint8_t 	an_pins[]={A0}; // ESP8266
#endif

////////////////////////////

// user
void 	eos_setup();
void 	eos_loop();
void 	a_loop100(uint32_t cont);
void 	a_second(time_t t);
void 	a_minute(time_t t);
void	a_hour(time_t t);
void 	a_day(time_t t);
void 	a_month(time_t t);
void 	a_read_log(String line);	// leitura do log no inicio (para recurerar dados do log)
String 	a_json(String str);	// resposta json
void 	a_config_write();	// gravar user config (config2.txt)
void 	a_config_read();	// ler user config (config2.txt)
void 	on_reset();
void 	on_update();
void 	on_wifi_status();
// eos
void 		loop100();
void 		fs_config_write();
void 		fs_config_read();
void 		DeviceID_gen();
uint32_t 	ESP_getChipId();
uint32_t	get_mvrms(uint8_t pin);
// file system
bool 	exists(String path);
void 	file_write(String path, String text);
String 	file_read(String path);
void 	handleFSformat();
void 	handleUpload();
void 	fs_minute(time_t t);
void 	fs_auto_del(String path, uint8_t maxFiles);
String 	formatBytes(size_t bytes);
String 	getContentType(String filename);
bool 	handleFileRead(String path);
void 	handleFileUpload();
void 	handleFileDelete();
void 	handleFileCreate();
void 	handleFileList();
void 	handleStatus();
// json
void 	handleJson();
String 	query_resp(String str);
String 	req_value(String str, String sa);
// log
void 	addLog(String text);
void 	log_second(time_t t);
void 	fs_gravar_log(uint32_t t, String text);
void 	fs_gravar_log_dia(String text, uint16_t ano, uint16_t mes, uint16_t dia);
void 	handle24H();
void 	handle30D();
// time
void 	time_setup();
void 	time_log();
void 	get_ntp_time();
time_t 	getNtpTime(String ntpServerIPstr);
void 	sendNTPpacket(IPAddress &address);
String 	hhmmss(uint32_t t);
String 	str00(uint16_t n);
String 	dd_mm_aaaa(time_t t);
uint8_t days_of_month(time_t t);
// update
void 	update_setup();
// WiFi
uint8_t 	prev_wifiStatus=255;
uint16_t	wifi_err_cnt	= 0;
uint16_t	wifi_con_wait	= 0; // minutos
void 	wf_setup();
void 	softAP_conect();
void 	handleReset();
String 	wifiStatusText(byte x);
void 	set_ip();
String 	wifiscan();
void 	handleRoot();
void 	wifi_minute();
String 		http_req(String url);
uint32_t	minFreeHeap;
void 		minFreeHeap_chk();
// rf rx
#ifdef rfrx_pin
	uint32_t 	RFcode_millis=0;
	String 		RFcode,lastRFcode;
	void rfrx_loop();
	void rf433rx3();
	void rfrx_executar_codigo(String code);
	#endif
// rf tx
#ifdef rftx_pin
	uint8_t 	rftx_wait;
	String 		rftxBuffer2="";
	void rftx(String sa);
	void rftxLoop100();
	void EV1527TX(uint32_t evcode);
	void HT6P20BTX3(uint32_t htcode);
	#endif
// if rx
#ifdef pinIR_RX
	unsigned long ir_Code, last_irCode;
	void ICACHE_RAM_ATTR IRread();
	void irrx_loop100();
	void on_irrx(unsigned long value);
	#endif
// tft
#ifdef tft_eSPI
	#include  	<TFT_eSPI.h>      // https://github.com/Bodmer/TFT_eSPI
	TFT_eSPI	tft = TFT_eSPI(); // Invoke custom library
	bool      	was_pressed=false;
	int8_t    	tft_mode=0;
	void		eSPI_setup();
	void 		tft_draw_btn(String text, uint16_t lef, uint16_t top, uint16_t wid, uint16_t hei, bool pressed );
	#ifndef tft_rotation
		#define tft_rotation 2
	#endif
	#if defined(TOUCH_CS) && !defined(tft_no_touch)
	//#ifdef TOUCH_CS
	//#ifndef tft_no_touch
		void touch_calibrate();
		void on_tft_touch(bool pressed, uint16_t x, uint16_t y);
	#endif
	void drawBmp(const char *filename, int16_t x, int16_t y);
	uint16_t 	read16(File f);
	uint32_t 	read32(File f);
	// JPEGDecoder
	#include <JPEGDecoder.h> // by Bodmer
	#ifdef JPEGDECODER_H
		void drawJpg(const char *filename, int xpos=0, int ypos=0 );
		void jpegRender(int xpos, int ypos);
	#endif
#endif
// QR code
#include "qrcode.h" // 0.0.1 https://github.com/ricmoo/QRCode
#ifdef __QRCODE_H_
	void tft_qrcode(String text, uint16_t top=0);
#endif
// I2C
#if defined(SCL_pin) && defined(SDA_pin)
	 #include <Wire.h>
#endif
// oled ssd1306
#if defined(i2c_SSD1306) || defined(i2c_SH1106)
	#ifndef SCL_pin
		#ifdef ESP8266
			#define SCL_pin     D1 	// gpio 4
		#endif
		#ifdef ESP32
			#define SCL_pin     22 	// gpio 22
		#endif
	#endif
	#ifndef SDA_pin
		#ifdef ESP8266
			#define SDA_pin     D2 	// gpio 5
		#endif
		#ifdef ESP32
			#define SDA_pin     21 	// gpio 21
		#endif
	#endif
	#ifdef i2c_SSD1306
		#include "SSD1306Wire.h"  	// 4.2.0
		SSD1306Wire  display(0x3c, SDA_pin, SCL_pin);
	#endif
	#ifdef i2c_SH1106
		#include "SH1106Wire.h" 
		SH1106Wire display(0x3c, SDA_pin, SCL_pin);
	#endif
	bool 	oled_err=true;
	//bool	oled_flip=false;
	void oled_setup0();
	void oled_qrcode( String text, uint8_t top=3,  uint8_t lef=39, uint8_t zoom=2 );
#endif
// ticker
#include <Ticker.h>
#ifdef TICKER_H
	Ticker 		tk50;
	uint32_t 	tkr50_cnt		=	0;
	uint32_t	loop_wdt_cnt	=	0;
	uint32_t	max_loop_wdt_cnt	=	0;
	void 		tkr50();
	void 		ticker50();
#endif
// DS18B20
#ifdef ds18_pin
	#include 	<OneWire.h>
	OneWire  	ds(ds18_pin);
	bool		ds18_ok	=	false;
	float 		ds18b20();
#endif
// HC-SR04 Ultrasonic Distance Sensor
#if defined(sr04_trigPin) && defined(sr04_echoPin)
	float read_dist(float temperature=25.0);
#endif
// MQTT
// #include <PubSubClient.h>
#ifdef PubSubClient_h
	WiFiClient    	espClient;        // Cria o objeto espClient
	PubSubClient  	MQTT(espClient);  // Instancia o Cliente MQTT passando o objeto espClient
	String  		MQTT_BROKER   = "";
	int     		MQTT_PORT     = 1883;
	String  		MQTT_ID		= "";
	bool			mqtt_ok				= true;
	uint16_t		mqtt_connect_wait 	= 2;
	uint16_t  		mqtt_err 			= 0;
	uint8_t 		mqtt_con_cnt  		= 0;
	bool    		mqtt_status			= false;
	uint16_t		mqtt_mode			= 0; // 0=sempre ligado, 1=desligado
	void 	initMQTT();
	void 	mqtt_loop();
	void 	mqtt_second();
	void 	mqtt_on_connect();
	void 	reconnectMQTT();
#endif
//

#include "src/esp-os_lib.c"
#include "src/esp-os_tft.c"

#endif
