/*
	esp-os_lib.h ver 1.1.1
	//	ver 1.0.1	WiFiUDP::stopAll(); para ESP8266
	versao 1.0.2	ESP32 led
	210505a	DeviceID_gen()
	versao 1.1.1	eos_setup(); eos_loop();
*/

////////// eos

void eos_setup(){
	#ifdef blink_led
		pinMode(blink_led,OUTPUT);
	#endif
	#if defined (ESP32)
		analogReadResolution(10);
	#endif
	addLog("Versao "+String(Versao));
	fsok=SPIFFS.begin();
	#ifdef tft_eSPI
		eSPI_setup();
	#endif
	DeviceID_gen();
	if(!fsok){ addLog("SPIFFS ERR"); }else{ addLog("SPIFFS OK"); }
	
	//Serial.println("timeNTP_interval 1 "+String(timeNTP_interval));
	
	fs_config_read();
	
	//Serial.println("timeNTP_interval 2 "+String(timeNTP_interval));
	
	a_config_read();
	wf_setup();
	time_setup();
	#if defined(SCL_pin) && defined(SDA_pin)
		Wire.begin(SDA_pin,SCL_pin);
		String pg=F("i2c scan:");
		for(uint8_t i=0;i<127;i++){
			Wire.beginTransmission(i);
			if(!Wire.endTransmission()){
				pg+=" 0x"; if(i<15) pg+="0";
				pg+=String(i,HEX);
			}
		}
		if(pg.length()==9){ pg+=F(" not found"); }
		addLog(pg);
	#endif
	#if defined(i2c_SSD1306) || defined(i2c_SH1106)
		oled_setup0();
	#endif
	#ifdef ds18_pin
		byte addr[8];
		if ( ds.search(addr) ){
		  ds18_ok=true;
		  addLog(F("ds18b20 ok"));
		}else{
		  ds.reset_search();
		  addLog(F("ds18b20 err"));
		}
		ds.reset();
	#endif
	//addLog("loop");
	//a_setup();
	tk50.attach(0.05, tkr50);
	#ifdef pinIR_RX
		pinMode(pinIR_RX, INPUT_PULLUP);
		attachInterrupt(digitalPinToInterrupt(pinIR_RX), IRread, CHANGE);
	#endif
	#if defined(sr04_trigPin) && defined(sr04_echoPin)
		pinMode(sr04_trigPin, OUTPUT);
		pinMode(sr04_echoPin, INPUT);
		digitalWrite(sr04_trigPin, LOW);
	#endif
	#ifdef PubSubClient_h
		initMQTT();
	#endif
	minFreeHeap_chk();
}

bool loop_ini=false;
void on_loop_start();

void eos_loop(){
	time_t t=now();
	if(!loop_ini){
		loop_ini=true;
		addLog("loop");
		on_loop_start();
	}
	server.handleClient();
	#if defined (ESP8266)
		MDNS.update();
	#endif
	if( (millis()-loop100_prevmillis)>=100 ){ // a cada 100ms
		loop100_prevmillis=millis();
		loop100_cnt++;
		loop100();
		///
	}
	server.handleClient();
	////
	if(prev_sec!=second(t)) 	{
		a_second(t);
		log_second(t);
		minFreeHeap_chk();
		#ifdef PubSubClient_h
			mqtt_second();
		#endif
	}
	if(prev_min!=minute(t)){
		wifi_minute();
		a_minute(t);
		fs_minute(t);
		if( (timeNTP_interval>0) && (timeNTP_cnt>0) && (WiFi.status()==WL_CONNECTED) ){
			timeNTP_cnt--;
			if(timeNTP_cnt==0){
				get_ntp_time();
				//WiFiUDP::stopAll(); addLog("WiFiUDP::stopAll()");
				if( MDNS.begin(DeviceID.c_str()) ) { MDNS.addService("http", "tcp", 80);	addLog("MDNS "+DeviceID); }
			}
		}
		if( (WiFi.status() == WL_CONNECTED) && (ap_disconn_cnt>0) ){
			#ifdef ESP8266
				if(wifi_softap_get_station_num()==0) ap_disconn_cnt--;
			#endif
			#ifdef ESP32
				if(WiFi.softAPgetStationNum()==0) ap_disconn_cnt--;
			#endif
			if(ap_disconn_cnt==0){
				WiFi.softAPdisconnect(true);
				addLog(F("AP desconectado"));
			}
		}
		#ifdef PubSubClient_h
		if((minute(t)%10==0)&&(mqtt_con_cnt>0)){ mqtt_con_cnt--; }
		#endif
	}
	if(prev_hour!=hour(t))  	{ a_hour(t); }
	if(prev_day!=day(t)) 		{ a_day(t); }
	if(prev_month!=month(t))	{ a_month(t); }
	if(prev_wifiStatus!=WiFi.status()){
		on_wifi_status();
		addLog(wifiStatusText(WiFi.status()));
		if(WiFi.status() == WL_CONNECTED){ set_ip();
			if(MDNS.begin(DeviceID.c_str())) { MDNS.addService("http", "tcp", 80);	addLog("MDNS "+DeviceID); }
			softAP_conect();
		}
		if(prev_wifiStatus==WL_CONNECTED){ softAP_conect(); }
	}
	//
	if(prev_sec!=second(t))   {	prev_sec=second(t);		}
	if(prev_min!=minute(t))   { prev_min=minute(t);		}
	if(prev_hour!=hour(t))    { prev_hour=hour(t);		}
	if(prev_day!=day(t))      { prev_day=day(t);		}
	if(prev_month!=month(t))  { prev_month=month(t);	}
	if(prev_year!=year(t))    { prev_year=year(t);		}
	if(prev_wifiStatus!=WiFi.status()){	prev_wifiStatus=WiFi.status();}
	if(prev_localIP!=WiFi.localIP())  { prev_localIP=WiFi.localIP(); }
	////
	#ifdef rfrx_pin
		rfrx_loop();
	#endif
	loop_wdt_cnt=0;
	#ifdef PubSubClient_h
	mqtt_loop();
	#endif
	//a_loop();
}

void loop100(){
	#if defined (ESP32)
		uint8_t sn=WiFi.softAPgetStationNum();
		#endif
	#if defined (ESP8266)
		uint8_t sn=wifi_softap_get_station_num();
		#endif
	if(station_num!=sn){
		station_num=sn;
		addLog("station_num "+String(station_num) );
		}
	#ifdef rftx_pin
		rftxLoop100();
		#endif
	#ifdef pinIR_RX
		irrx_loop100();
		#endif
	#if defined(TOUCH_CS) && !defined(tft_no_touch)
	//#ifdef TOUCH_CS
	//#ifndef tft_no_touch
		uint16_t x = 0, y = 0;
		boolean pressed = tft.getTouch(&x, &y);
		if(pressed != was_pressed){
			#ifdef buzz_pin
					if(pressed){
						pinMode(buzz_pin,OUTPUT);
						digitalWrite(buzz_pin,HIGH); delayMicroseconds(500); digitalWrite(buzz_pin,LOW);
					}
			#endif
			//if(tft_rotation==0){ x=240-x; y=320-y; }
			on_tft_touch(pressed, x, y);
			was_pressed=pressed;
		}
	#endif
	//
	if(rst_cnt>0) {
		if( rst_cnt==20 ){
			addLog("Reset");
			#ifdef tft_eSPI
				tft_mode=0;
				tft.fillScreen(TFT_BLACK);
				tft.setTextColor(TFT_RED, TFT_BLACK);
				uint16_t left=120, top=140;
				if(tft_rotation==1){left=160; top=110;}
				tft.drawCentreString("Reset",left,top,4);
			#endif
			#if defined(i2c_SSD1306) || defined(i2c_SH1106)
				if(!oled_err){
					display.clear();
					display.displayOn();
					display.setColor(WHITE);
					display.normalDisplay();
					display.setTextAlignment(TEXT_ALIGN_CENTER);
					display.setFont(ArialMT_Plain_10);
					display.drawString(64, 27, "Reset");
					display.display();
				}
			#endif
			on_reset();
			rst_cnt--;
		}
		if(log_buffer.indexOf(";;")<0) rst_cnt--;
		if(rst_cnt==0){
			ESP.restart();
		}
	}
	//
	a_loop100(loop100_cnt);
}

////////// cnf

void DeviceID_gen(){
  String ChipId = String(ESP_getChipId(),HEX);
  ChipId.toUpperCase();
  DeviceID=Versao; DeviceID+="_";
  DeviceID=DeviceID.substring(0,DeviceID.indexOf("_"))+"_"+ChipId;
}

uint32_t ESP_getChipId(){
  uint32_t chipId = 0;
  #if defined (ESP32)
	for(int i=0; i<17; i=i+8) { chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i; }
  #endif
  #if defined (ESP8266)
	chipId=ESP.getChipId();
  #endif
  return chipId;
}

uint32_t get_mvrms(uint8_t pin){
  uint16_t readValue[128];
  uint16_t rv;
  uint32_t prev_micros = micros();
  uint32_t zero=0;
  for(uint16_t i=0;i<128;i++){
    rv=analogRead(pin);
    readValue[i]=rv;
    zero+=rv;
    while((micros()-prev_micros)<130){}
    prev_micros = prev_micros+130;
  }
  zero/=128;
  uint32_t mv,mv_rms=0;
  for(uint16_t i=0;i<128;i++){
    mv=0;
    if(readValue[i]>zero){ mv=readValue[i]-zero; }
    if(readValue[i]<zero){ mv=zero-readValue[i]; }
	#ifdef ESP32
		mv*=1815; //1650;
	#endif
	#ifdef ESP8266
		mv*=1650;
	#endif
	mv/=512; mv_rms+=pow(mv,2);
  }
  mv_rms=sqrt(mv_rms/128);
  if(mv_rms<10) return 0;
  return mv_rms;
}

void fs_config_write(){
  if(!fsok) return;
  addLog("FS_config_write");
  String sa,sb; sa=F("/config.txt");
  File wFile = SPIFFS.open(sa,"w");
  if(wFile){
    sb=F("Versao: ");		sb+=Versao; 		wFile.println(sb);
    sb=F("essid: ");		sb+=essid; 			wFile.println(sb);
    sb=F("epass: ");		sb+=epass; 			wFile.println(sb);
    sb=F("ipMode: ");		sb+=ipMode; 		wFile.println(sb);
    sb=F("DeviceID: ");		sb+=DeviceID; 		wFile.println(sb);
    sb=F("apPass: ");		sb+=apPass; 		wFile.println(sb);
    sb=F("APmode: ");		sb+=APmode; 		wFile.println(sb);
    for(byte i=0;i<3;i++){ if(i==0){ sb=F("ntpServer[]: "); }else{ sb+=","; } sb+=ntpServer[i]; } wFile.println(sb);
    sb=F("homePage: ");		sb+=homePage;		wFile.println(sb);
    sb=F("timeNTP_interval: ");	sb+=String(timeNTP_interval); wFile.println(sb);
	#ifdef PubSubClient_h
	sb="MQTT_BROKER: ";	sb+=MQTT_BROKER;	wFile.println(sb);
	sb="MQTT_PORT: ";	sb+=MQTT_PORT; 		wFile.println(sb);
	sb="MQTT_ID: ";		sb+=MQTT_ID;		wFile.println(sb);
	#endif
    //
    wFile.close(); delay(1);
  }
}

void fs_config_read(){
	if(!fsok) return;
	String path, sa, sb, sc, sd; path = F("/config.txt");
	if (!exists(path)) fs_config_write();
	File file = SPIFFS.open(path, "r");
	if (file) {
		while (file.available()) {
			sa = file.readStringUntil('\r'); sa.trim(); sb = sa.substring(0, sa.indexOf(":"));  sa = sa.substring(sa.indexOf(":") + 2);
		sc=F("essid");    if(sc==sb) essid = sa;
		sc=F("epass");    if(sc==sb){ epass = sa; }
		sc=F("ipMode");   if(sb==sc){ ipMode=sa.toInt(); if(ipMode<2) ipMode=0; }
		sc=F("DeviceID"); if(sc==sb) if(sa.length()>0) DeviceID=sa;
		sc=F("apPass");       if(sc==sb) apPass = sa;
		sc=F("APmode");       if(sc==sb) APmode=sa.toInt();
		sc=F("ntpServer[]");  if(sc==sb){ sa+=","; for(uint8_t i=0;i<3;i++){ sb=sa.substring(0,sa.indexOf(",")); sa=sa.substring(sa.indexOf(",")+1); ntpServer[i]=sb; } }
		sc=F("homePage");			if(sc==sb) homePage = sa;
		sc=F("timeNTP_interval");	if(sc==sb) timeNTP_interval = sa.toInt();
		#ifdef PubSubClient_h
			sc=F("MQTT_BROKER");	if(sc==sb) MQTT_BROKER=sa;
			sc=F("MQTT_PORT");		if(sc==sb) MQTT_PORT=sa.toInt();
			sc=F("MQTT_ID");		if(sc==sb) if(sa.length()>0) MQTT_ID=sa;
		#endif
		//
		}
		file.close();
	}
}

///// fs

bool exists(String path){
	bool yes = false;
	#ifdef ESP32
		File file = SPIFFS.open(path, "r");
		if(!file.isDirectory()){    yes = true;  }
		file.close();
	#endif
	#ifdef ESP8266
		if(SPIFFS.exists(path))		yes = true;
	#endif
  return yes;
}
	
String unsupportedFiles = String();

#if defined (ESP8266)
static const char FS_INIT_ERROR[] PROGMEM = "FS INIT ERROR";
static const char TEXT_PLAIN[] PROGMEM = "text/plain";
static const char FILE_NOT_FOUND[] PROGMEM = "FileNotFound";

void replyOKWithMsg(String msg);
void replyNotFound(String msg);
void replyBadRequest(String msg);
void 	replyServerError(String msg);
String 	checkForUnsupportedPath(String filename);

void replyOKWithMsg(String msg) { server.send(200, FPSTR(TEXT_PLAIN), msg);}
void replyNotFound(String msg) { server.send(404, FPSTR(TEXT_PLAIN), msg);}
void replyBadRequest(String msg) { server.send(400, FPSTR(TEXT_PLAIN), msg + "\r\n");}

void replyServerError(String msg) {
	server.send(500, FPSTR(TEXT_PLAIN), msg + "\r\n");
}

String checkForUnsupportedPath(String filename) {
  String error = String();
  if (!filename.startsWith("/")) {    error += F("!NO_LEADING_SLASH! ");  }
  if (filename.indexOf("//") != -1) {    error += F("!DOUBLE_SLASH! ");  }
  if (filename.endsWith("/")) {    error += F("!TRAILING_SLASH! ");  }
  return error;
}

#endif

void file_write(String path, String text){
  if(!fsok) return;
  File wFile = SPIFFS.open(path,"w");
  if(wFile){
    wFile.print(text);
    wFile.close(); delay(1);
  }
}

String file_read(String path){
  if(!fsok) return "";
  File file = SPIFFS.open(path, "r");
  String text="";
  if (file) {
    while (file.available()) {
      String sa=file.readStringUntil('\r');
      sa.trim();
      text+=sa+"\r";
    }
    file.close();
  }
  return text;
}

void handleFSformat(){
  String pg;
  pg+=F(
    "<html><meta http-equiv='refresh' content='5; url=/'>"
    "<body>Formatando...</body></html>");
  server.send(200, "text/html", pg);
  SPIFFS.format();
  //rst_cnt=20;
}

void handleUpload() {
  String pg;
  pg=F(
    "<html><head>\n"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>\n"
    "<style>\n"
    " input{border:solid 1px silver;border-radius:4px;background-color:white;cursor:hand;}\n"
    " .custom-file-input::-webkit-file-upload-button {visibility:hidden;width:0;}\n"
    " td{text-align:center;}"
    "</style>\n"
    "</head>\n"
    "<body><div align='center'>\n"
    "<table><tr><td><b>Upload (Arquivos)</b></td></tr>\n"
    "<tr><form target='if01' action='/edit' method='post' enctype='multipart/form-data'>\n"
    "<td><input id='update' type='file' name='name' class='custom-file-input'></td>\n"
    "</tr><tr><td><input id='submitBtn' type='submit' value='Upload' style='height:23px;'></td>\n"
    "</form></tr></table>\n"
    "<script>function voltar(){update.value='';}</script>\n"
    "<iframe id='if01' name='if01' onload='voltar();' style='visibility:hidden;'></iframe>\n"
    "</div></body></html>"
    );
  server.send(200, "text/html", pg);
}

void fs_minute(time_t t){
  if(minute(t)==42){
    if(hour(t)%12==0) fs_auto_del("/log/",7);
    if(hour(t)%12==3) fs_auto_del("/dia/",31);
    if(hour(t)%12==6) fs_auto_del("/mes/",12);
  }
}

void fs_auto_del(String path, uint8_t maxFiles){
	if(!fsok) return;
	#if defined (ESP32)
		uint32_t fs_usedBytes=SPIFFS.usedBytes()/1000;
		uint32_t fs_totalBytes=SPIFFS.totalBytes()/1000;
		String path2 = String();
		String sa;
		uint8_t n=0;
		File root = SPIFFS.open("/");
		if(root.isDirectory()){
		  File file = root.openNextFile();
		  while(file){
			sa = file.name();
			if(sa.indexOf(path)==0){
			  n++;
			  if(sa<path2 || path2=="") path2=sa;
			}
			file = root.openNextFile();
		  }
		}
		sa="fs_del "; sa+=n;
		if( (n>maxFiles) || (fs_usedBytes > (fs_totalBytes*0.9) ) ) {
			SPIFFS.remove(path2); delay(10);
			sa+=" "+path2;
		}
	#elif defined (ESP8266)
		FSInfo fs_info;
		SPIFFS.info(fs_info);
		uint32_t fs_usedBytes=fs_info.usedBytes/1000;
		uint32_t fs_totalBytes=fs_info.totalBytes/1000;
		Dir dir = SPIFFS.openDir(path);
		path=String();
		uint8_t n=0;
		String sa;
		while(dir.next()){
			n++;
			File entry = dir.openFile("r");
			sa=entry.name();
			if(path.length()==0)  path=sa;
			if(sa<path) path=sa;
			entry.close();
		}
		sa="fs_del ";
		sa+=" "; sa+=fs_usedBytes;  sa+="KB";
		sa+="/"; sa+=fs_totalBytes; sa+="KB";
		sa+=" "; sa+=n;
		if( (n>maxFiles) || ( (fs_usedBytes*2) > fs_totalBytes ) ){
			SPIFFS.remove(path); delay(10);
			sa+=" "+path;
		}
	#endif
	addLog(sa);
}

String formatBytes(size_t bytes) {
  if (bytes < 1024) { return String(bytes) + "B";  }
  else if (bytes < (1024 * 1024)) { return String(bytes / 1024.0) + "KB";  }
  else if (bytes < (1024 * 1024 * 1024)) { return String(bytes / 1024.0 / 1024.0) + "MB";  }
  else { return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";  }
}

String getContentType(String filename) {
  if (server.hasArg("download"))        { return "application/octet-stream"; }
  else if (filename.endsWith(".htm"))   { return "text/html"; }
  else if (filename.endsWith(".html"))  { return "text/html"; }
  else if (filename.endsWith(".css"))   { return "text/css"; }
  else if (filename.endsWith(".js"))    { return "application/javascript"; }
  else if (filename.endsWith(".png")) { return "image/png"; }
  else if (filename.endsWith(".gif")) { return "image/gif"; }
  else if (filename.endsWith(".jpg")) { return "image/jpeg"; }
  else if (filename.endsWith(".ico")) { return "image/x-icon"; }
  else if (filename.endsWith(".xml")) { return "text/xml"; }
  else if (filename.endsWith(".pdf")) { return "application/x-pdf"; }
  else if (filename.endsWith(".zip")) { return "application/x-zip"; }
  else if (filename.endsWith(".gz"))  { return "application/x-gzip"; }
  return "text/plain";
}

bool handleFileRead(String path) {
	#if defined (ESP32)
	  if(!fsok) return false;
	  if (path.endsWith("/")) { path += "index.htm"; }
	  String contentType = getContentType(path);
	  String pathWithGz = path + ".gz";
	  if (exists(pathWithGz) || exists(path)) {
		if (exists(pathWithGz)) {   path += ".gz";  }
		File file = SPIFFS.open(path, "r");
		server.sendHeader("Access-Control-Allow-Origin", "*");
		server.streamFile(file, contentType);
		file.close();
		delay(10);
		return true;
	  }
	  return false;
  #endif
  #if defined (ESP8266)
	  if (!fsok) {
		replyServerError(FPSTR(FS_INIT_ERROR));
		return true;
	  }
	  if (path.endsWith("/")) { path += "index.htm"; }
	  String contentType;
	  if (server.hasArg("download")) {    contentType = F("application/octet-stream");  } else {    contentType = mime::getContentType(path);  }
	  if (!SPIFFS.exists(path)) { path = path + ".gz"; }
	  if (SPIFFS.exists(path)) {
		File file = SPIFFS.open(path, "r");
		server.sendHeader("Access-Control-Allow-Origin", "*");
		if (server.streamFile(file, contentType) != file.size()){}
		file.close();
		return true;
	  }
	  return false;
  #endif
}

void handleFileUpload() {
  if (server.uri() != "/edit") {  return;  }
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.startsWith("/")) { filename = "/" + filename; }
    //DBG_OUTPUT_PORT.print("handleFileUpload Name: "); DBG_OUTPUT_PORT.println(filename);
    uploadFile = SPIFFS.open(filename, "w");
    filename = String();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    //DBG_OUTPUT_PORT.print("handleFileUpload Data: "); DBG_OUTPUT_PORT.println(upload.currentSize);
    if (uploadFile) { uploadFile.write(upload.buf, upload.currentSize); }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) { uploadFile.close(); }
    //DBG_OUTPUT_PORT.print("handleFileUpload Size: "); DBG_OUTPUT_PORT.println(upload.totalSize);
  }
}

void handleFileDelete() {
  if (server.args() == 0) { return server.send(500, "text/plain", "BAD ARGS");  }
  String path = server.arg(0);
  if (path == "/") { return server.send(500, "text/plain", "BAD PATH"); }
  #ifdef ESP32
	if (!exists(path)) { 			return server.send(404, "text/plain", "FileNotFound"); }
  #endif
  #ifdef ESP8266
	if (!SPIFFS.exists(path)) { return replyNotFound(FPSTR(FILE_NOT_FOUND)); }
  #endif
  SPIFFS.remove(path);
  server.send(200, "text/plain", "");
  path = String();
}

#ifdef ESP8266 	// handleFileCreate()

String lastExistingParent(String path);

String lastExistingParent(String path) {
  while (!path.isEmpty() && !SPIFFS.exists(path)){
    if (path.lastIndexOf('/') > 0) {
      path = path.substring(0, path.lastIndexOf('/'));
    } else {
      path = String();  // No slash => the top folder does not exist
    }
  }
  //DBG_OUTPUT_PORT.println(String("Last existing parent: ") + path);
  return path;
}

void handleFileCreate() {
	if (!fsok) { return replyServerError(FPSTR(FS_INIT_ERROR)); }
	String path = server.arg("path");
	if (path.isEmpty()) { return replyBadRequest(F("PATH ARG MISSING")); }
	#ifdef USE_SPIFFS
		if (checkForUnsupportedPath(path).length() > 0) {    return replyServerError(F("INVALID FILENAME"));  }
	#endif
	if (path == "/") {    return replyBadRequest("BAD PATH");  }
	if (SPIFFS.exists(path)) {    return replyBadRequest(F("PATH FILE EXISTS"));  }
	String src = server.arg("src");
	if (src.isEmpty()) {
		// No source specified: creation
		//DBG_OUTPUT_PORT.println(String("handleFileCreate: ") + path);
		if (path.endsWith("/")) {
			// Create a folder
			path.remove(path.length() - 1);
			if (!SPIFFS.mkdir(path)) {	return replyServerError(F("MKDIR FAILED"));	}
		} else {
			// Create a file
			File file = SPIFFS.open(path, "w");
			if (file) {	file.write((const char *)0);	file.close();	}
			else {	return replyServerError(F("CREATE FAILED"));	}
		}
		if (path.lastIndexOf('/') > -1) { path = path.substring(0, path.lastIndexOf('/')); }
		replyOKWithMsg(path);
	} else {
		// Source specified: rename
		if (src == "/") {	return replyBadRequest("BAD SRC");	}
		if (!SPIFFS.exists(src)) {	return replyBadRequest(F("SRC FILE NOT FOUND"));	}
		//DBG_OUTPUT_PORT.println(String("handleFileCreate: ") + path + " from " + src);
		if (path.endsWith("/")) {	path.remove(path.length() - 1);	}
		if (src.endsWith("/")) {	src.remove(src.length() - 1);	}
		if (!SPIFFS.rename(src, path)) {	return replyServerError(F("RENAME FAILED"));	}
		replyOKWithMsg(lastExistingParent(src));
	}
}

#endif
#ifdef ESP32	// handleFileCreate()
void handleFileCreate() {
	if (server.args() == 0) {	return server.send(500, "text/plain", "BAD ARGS");  }
	String path = server.arg(0);
	//DBG_OUTPUT_PORT.println("handleFileCreate: " + path);
	if (path == "/") {	return server.send(500, "text/plain", "BAD PATH");  }
	if (exists(path)) {	return server.send(500, "text/plain", "FILE EXISTS");  }
	File file = SPIFFS.open(path, "w");
	if (file) { file.close(); }
	else {	return server.send(500, "text/plain", "CREATE FAILED");  }
	server.send(200, "text/plain", "");
	path = String();
}
#endif

void handleFileList() {
	#if defined (ESP32)
	if (!server.hasArg("dir")) { server.send(500, "text/plain", "BAD ARGS"); return; }
	  String path = server.arg("dir");
	  File root = SPIFFS.open(path);
	  String output = "[";
	  if(root.isDirectory()){
		  File file = root.openNextFile();
		  while(file){
			  if (output != "[") { output += ','; }
			  output += "{\"type\":\"";  output += (file.isDirectory()) ? "dir" : "file";
			  output += "\",\"size\":\""; output += String(file.size());
			  output += "\",\"name\":\""; output += String(file.name()).substring(1);
			  output += "\"}";
			  file = root.openNextFile();
		  }
	}
	output += "]";
	server.send(200, "text/json", output);
	#endif
	#if defined (ESP8266)
	if (!fsok) { return replyServerError(FPSTR(FS_INIT_ERROR)); }
	if (!server.hasArg("dir")) { return replyBadRequest(F("DIR ARG MISSING")); }
	String path = server.arg("dir");
	//if (path != "/" && !fileSystem->exists(path)) { return replyBadRequest("BAD PATH"); }
	Dir dir = SPIFFS.openDir("/");
	//Dir dir = fileSystem->openDir(path);
	//path.clear();
	// use HTTP/1.1 Chunked response to avoid building a huge temporary string
	if (!server.chunkedResponseModeStart(200, "text/json")) {
		server.send(505, F("text/html"), F("HTTP1.1 required"));
		return;
	}
	  // use the same string for every line
	  String output;
	  output.reserve(64);
	while (dir.next()) {
		String error = checkForUnsupportedPath(dir.fileName());
		if (error.length() > 0) {      continue;    }
		String fn;
		if (dir.fileName()[0] == '/') {      fn= &(dir.fileName()[1]);    } else {     fn= dir.fileName();    }
		fn="/"+fn;
		if(fn.startsWith(path)){
		  if (output.length()) {
			// send string from previous iteration
			// as an HTTP chunk
			server.sendContent(output);
			output = ',';
		  } else {
			output = '[';
		  }
		  output += "{\"type\":\"";
		  if (dir.isDirectory()) {
			output += "dir";
		  } else {
			output += F("file\",\"size\":\"");
			output += dir.fileSize();
		  }
		  output += F("\",\"name\":\"");
		  // Always return names without leading "/"
		  if (dir.fileName()[0] == '/') {      output += &(dir.fileName()[1]);    } else {     output += dir.fileName();    }
		  output += "\"}";
		}
	}
	// send last string
	output += "]";
	server.sendContent(output);
	server.chunkedResponseFinalize();
	#endif
}

void handleStatus() {
  String json;
  json.reserve(128);
  json = "{\"type\":\"SPIFFS\", \"isOk\":";
  if (fsok) json += F("\"true\"");
  if (fsok) {
		#ifdef ESP8266
			FSInfo fs_info;
			SPIFFS.info(fs_info);
			json += F(", \"totalBytes\":\"");		json += fs_info.totalBytes;
			json += F("\", \"usedBytes\":\"");		json += fs_info.usedBytes;
			json += "\"";
		#endif
		#ifdef ESP32
			json += F(", \"totalBytes\":\"");		json += SPIFFS.totalBytes();
			json += F("\", \"usedBytes\":\"");		json += SPIFFS.usedBytes();
			json += "\"";
		#endif
  } else {
    json += "\"false\"";
  }
  json += F(",\"unsupportedFiles\":\"");	json += unsupportedFiles;
  json += "\"}";

  server.send(200, "application/json", json);
}

//////// json

void handleJson(){
  String query;
  String pg;
  for (int i = 0; i < server.args(); i++){ query+="&"+server.argName(i)+"="+server.arg(i); }
  pg+=query_resp(query);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  if(pg.startsWith("{")) server.send(200, "text/json", pg);
                    else server.send(200, "text/html", pg);
}

String query_resp(String str){
	String sa, sb, sc;
	String fields=",";
	String pg="{";
	uint8_t salvar=0;
	sa=F("&fields=");	if(str.indexOf(sa)>=0){ sb=req_value(str, sa); fields+=sb+","; fields.replace(",","\""); }
	sa=F("&time=");		if(str.indexOf(sa)>=0){ sb=req_value(str, sa); //sb=str.substring(str.indexOf(sa)+sa.length())+"&"; sb=sb.substring(0, sb.indexOf("&"));
		String stime=sb; stime = stime.substring(0,stime.length()-3); time_t itime = stime.toInt() + ( timeZone * SECS_PER_HOUR );
		setTime(itime); addLog("setTime(WEB)");
	}
	// Salvar e Reiniciar
	sa=F("&essid=");      if(str.indexOf(sa)>=0){ sb=req_value(str, sa); salvar=2; essid=sb; }
	sa=F("&epass=");      if(str.indexOf(sa)>=0){ sb=req_value(str, sa); salvar=2; epass=sb; }
	sa=F("&ipMode=");     if(str.indexOf(sa)>=0){ sb=req_value(str, sa); salvar=2; ipMode=sb.toInt(); }
	sa=F("&DeviceID=");   if(str.indexOf(sa)>=0){ sb=req_value(str, sa); bitSet(salvar, 1); DeviceID=sb; }  // Salvar e Reiniciar
	sa=F("&apPass=");     if(str.indexOf(sa)>=0){ sb=req_value(str, sa); salvar=2; apPass=sb; }
	// Salvar
	sa=F("&APmode=");   			if(str.indexOf(sa)>=0){ sb=req_value(str, sa); salvar=1; APmode=sb.toInt(); }
	sa=F("&ntpServer0="); 		if(str.indexOf(sa)>=0){ sb=req_value(str, sa); salvar=1; ntpServer[0]=sb; }
	sa=F("&ntpServer1="); 		if(str.indexOf(sa)>=0){ sb=req_value(str, sa); salvar=1; ntpServer[1]=sb; }
	sa=F("&ntpServer2="); 		if(str.indexOf(sa)>=0){ sb=req_value(str, sa); salvar=1; ntpServer[2]=sb; }
	sa=F("&homePage="); 			if(str.indexOf(sa)>=0){ sb=req_value(str, sa); salvar=1; homePage=sb; }
	// &w=[port],[v_trigger],[us],[num_amostras],[tr_port],[tr_mode]
	sa="&w="; 					if(str.indexOf(sa) >= 0){
    sb=str.substring(str.indexOf(sa)+sa.length())+"&";
    sb=sb.substring(0, sb.indexOf("&"));
    sb+=",,,,,,";
    uint8_t v_chn=0; uint16_t v_sinc=511;
    uint16_t v_us=11; uint16_t w_size=228;    // total=2.510ms    0.25ms/div
    //uint16_t v_us=22;  uint16_t w_size=228; // total=5.017ms    0.50ms/div
    //uint16_t v_us=44;  uint16_t w_size=228; // total=10.034ms   1.00ms/div
    //uint16_t v_us=88;  uint16_t w_size=228; // total=20.065ms   2.01ms/div
    //uint16_t v_us=219; uint16_t w_size=228; // total=49.934ms   4.99ms/div
    //uint16_t v_us=439; uint16_t w_size=228; // total=100.094ms  10.0ms/div
    //uint16_t v_us=877; uint16_t w_size=228; // total=199.956ms  20.0ms/div
    uint8_t tr_port=0, tr_mode=1;
    sc=sb.substring(0, sb.indexOf(",")); sb = sb.substring(sb.indexOf(",")+1); if(sc.length()>0) v_chn=sc.toInt();
    sc=sb.substring(0, sb.indexOf(",")); sb = sb.substring(sb.indexOf(",")+1); if(sc.length()>0) v_sinc=sc.toInt();
    sc=sb.substring(0, sb.indexOf(",")); sb = sb.substring(sb.indexOf(",")+1); if(sc.length()>0) v_us=sc.toInt();
    sc=sb.substring(0, sb.indexOf(",")); sb = sb.substring(sb.indexOf(",")+1); if(sc.length()>0) w_size=sc.toInt();
    sc=sb.substring(0, sb.indexOf(",")); sb = sb.substring(sb.indexOf(",")+1); if(sc.length()>0) tr_port=sc.toInt();
    sc=sb.substring(0, sb.indexOf(",")); sb = sb.substring(sb.indexOf(",")+1); if(sc.length()>0) tr_mode=sc.toInt();
	#ifdef ESP32
		uint8_t pin_por=v_chn;		if(v_chn<6){ 	pin_por=an_pins[v_chn]; }
		uint8_t pin_tri=tr_port;	if(tr_port<6){ 	pin_tri=an_pins[tr_port]; }
	#endif
	#ifdef ESP8266
		uint8_t pin_por=A0;
		uint8_t pin_tri=A0;
	#endif
    if(v_us<11) v_us=11;
    uint32_t prev_micros;
    // trigger
    if(v_sinc>0){
      prev_micros=micros();
      uint16_t rv; //, prev_rv=analogRead(an_pins[tr_port]);
      int st=-1, prev_st=-1;
      //uint8_t pin; if(tr_port<6){ pin=an_pins[tr_port]; }else{ pin=tr_port; }
      while(true){
        rv=analogRead(pin_tri);
        if(rv>(v_sinc+5)) st=1;
        if(rv<(v_sinc-5)) st=0;
        if(tr_mode==0){
          if(prev_st==1 && st==0) break;
        }else{
          if(prev_st==0 && st==1) break;
        }
        if( (micros()-prev_micros)>(v_us*w_size*2) ) break;
        prev_st=st;
      }
    }
      uint32_t dur;
      uint16_t rv[w_size];
      dur=micros();
      prev_micros=micros();
      for(uint16_t i=0; i<w_size;i++){
        rv[i]=analogRead(pin_por);
        while((micros()-prev_micros)<v_us) {} prev_micros+=v_us;
      }
      dur=micros()-dur;
      //
      pg+="\"v_chn\":";   pg+=v_chn; pg+=",";
      pg+="\"w_size\":";  pg+=w_size; pg+=",";
      pg+="\"v_us\":";    pg+=v_us; pg+=",";
      pg+="\"dur\":";     pg+=dur; pg+=",";
      for(uint8_t i=0;i<w_size;i++){
        if(i==0) pg+="\"w\":["; else pg+=",";
        pg+=rv[i];
      }
      pg+="],";
  }
	/// get_mvrms
	sa="&mvrms=";			if(str.indexOf(sa)>=0){ sb=req_value(str, sa);
	#ifdef ESP32
		uint8_t pin=sb.toInt(); if(pin<6){ pin=an_pins[pin]; }
	#endif
	#ifdef ESP8266
		uint8_t pin=A0;
	#endif
	pg+="\"mvrms\":"; pg+=get_mvrms(pin); pg+=",";
  }
	#ifdef rftx_pin
	sa=F("&rftx=");			if(str.indexOf(sa)>=0){ sb=req_value(str, sa); rftx(sb); }
	#endif
	#ifdef PubSubClient_h
	sa=F("&MQTT_BROKER=");  if(str.indexOf(sa)>=0){ sb=req_value(str, sa); salvar=3; MQTT_BROKER=sb; }
	sa=F("&MQTT_PORT=");    if(str.indexOf(sa)>=0){ sb=req_value(str, sa); salvar=3; MQTT_PORT=sb.toInt(); }
	sa=F("&MQTT_ID=");      if(str.indexOf(sa)>=0){ sb=req_value(str, sa); salvar=3; MQTT_ID=sb; }
	#endif
	
	sa=F("&timeNTP_interval=");	if(str.indexOf(sa)>=0){ sb=req_value(str, sa); salvar=1; timeNTP_interval=sb.toInt(); timeNTP_cnt = timeNTP_interval; }
	// fields
	sa=F("\"Versao\"");		if(fields.indexOf(sa)>=0){ pg+=sa; pg+=":\""; pg+=Versao;     pg+="\","; }
	sa=F("\"essid\"");		if(fields.indexOf(sa)>=0){ pg+=sa; pg+=":\""; pg+=essid;      pg+="\","; }
	sa=F("\"epass\"");		if(fields.indexOf(sa)>=0){ pg+=sa; pg+=":\""; pg+=epass;      pg+="\","; }
	sa=F("\"ipMode\"");		if(fields.indexOf(sa)>=0){ pg+=sa; pg+=":";   pg+=ipMode;     pg+=",";   }
	sa=F("\"localIP\"");	if(fields.indexOf(sa)>=0){ pg+=sa; pg+=":\""; pg+=WiFi.localIP().toString();  pg+="\","; }
	sa=F("\"DeviceID\"");	if(fields.indexOf(sa)>=0){ pg+=sa; pg+=":\""; pg+=DeviceID;   pg+="\","; }
	sa=F("\"apPass\"");		if(fields.indexOf(sa)>=0){ pg+=sa; pg+=":\""; pg+=apPass;     pg+="\","; }
	sa=F("\"APmode\"");		if(fields.indexOf(sa)>=0){ pg+=sa; pg+=":";   pg+=APmode; pg+=",";   }
	sa=F("\"time\"");		if(fields.indexOf(sa)>=0){ pg+=sa; pg+=":"; pg+=now();        pg+=","; }
	//
	sa=F("\"timeNTP_interval\"");	if(fields.indexOf(sa)>=0){ pg+=sa+":"+String(timeNTP_interval)+","; }
	//
	sa=F("\"log_recent\"");     if(fields.indexOf(sa)>=0){ pg+=sa; pg+=":\""; pg+=log_recent; pg+="\","; }
	#if defined (ESP32)
	  sa=F("\"fs_usedBytes\"");   if(fields.indexOf(sa)>=0){ pg+=sa; pg+=":"; pg+=SPIFFS.usedBytes(); pg+=","; }
	  sa=F("\"fs_totalBytes\"");	if(fields.indexOf(sa)>=0){ pg+=sa; pg+=":"; pg+=SPIFFS.totalBytes();  pg+=","; }
  #endif
	#if defined (ESP8266)
	  sa=F("\"fs_usedBytes\""); sb=F("\"fs_totalBytes\""); if((fields.indexOf(sa)>=0)||(fields.indexOf(sb)>=0)){ FSInfo fs_info; SPIFFS.info(fs_info);
		if(fields.indexOf(sa)>=0){ pg+=sa; pg+=":"; pg+=fs_info.usedBytes;  pg+=F(","); }
		if(fields.indexOf(sb)>=0){ pg+=sb; pg+=":"; pg+=fs_info.totalBytes; pg+=F(","); }
	  }
  #endif
	sa=F("\"millis\"");         	if(fields.indexOf(sa)>=0){ pg+=sa; pg+=":"; pg+=millis(); pg+=","; }
	//max_loop_wdt_cnt
	sa=F("\"max_loop_wdt_cnt\"");	if(fields.indexOf(sa)>=0){ pg+=sa+":"+String(max_loop_wdt_cnt)+","; }
	sa=F("\"ntpServer\"");      	if(fields.indexOf(sa)>=0){ pg+=sa; pg+=":[\""; pg+=ntpServer[0]; pg+="\",\""; pg+=ntpServer[1]; pg+="\",\""; pg+=ntpServer[2]; pg+="\"],"; }
	sa=F("\"list\""); if(fields.indexOf(sa)>=0){
	#if defined (ESP32)
		pg+=sa; pg+=":[";
		File root = SPIFFS.open("/");
		if(root.isDirectory()){
			File file = root.openNextFile();
			while(file){
			  pg+="\""+String(file.name())+"\",";
				//output += "\",\"size\":\""; output += String(file.size());
				//output += "\",\"name\":\""; output += String(file.name()).substring(1);
			  file = root.openNextFile();
		   }
		}
     pg=pg.substring(0,pg.length()-1);  pg+="],";
	 #endif
	#if defined (ESP8266)
		Dir dir = SPIFFS.openDir("/");
		pg+=sa; pg+=":["; while (dir.next()){ pg+="\""+dir.fileName()+"\","; }
		pg=pg.substring(0,pg.length()-1);  pg+="],";
	#endif
  }
	sa=F("&list="); if(str.indexOf(sa)>=0){ sb=req_value(str, sa);
	#if defined (ESP32)
    String path=sb;
    pg += F("\"list\":[");
    File root = SPIFFS.open(path);
    if(root.isDirectory()){
        File file = root.openNextFile();
        while(file){
          pg+="\""; pg+=String(file.name()).substring(1);
          pg+=" "; pg+=String(file.size()); pg+="\",";
            //output += "\",\"size\":\""; output += String(file.size());
            //output += "\",\"name\":\""; output += String(file.name()).substring(1);
          file = root.openNextFile();
       }
    }
    if(pg.substring(pg.length()-1)==",") pg = pg.substring(0,pg.length()-1);
    pg += "],";
	#endif
	#if defined (ESP8266)
		String path=sb;
		Dir dir = SPIFFS.openDir(path);
		pg += F("\"list\":[");
		while(dir.next()){
		  File entry = dir.openFile("r");
		  pg+="\""; pg+=String(entry.name()).substring(1);
		  size_t fileSize = dir.fileSize();
		  pg+=" "; pg+=String(fileSize); pg+="\",";
		  entry.close();
		}
		if(pg.substring(pg.length()-1)==",") pg = pg.substring(0,pg.length()-1);
		pg += "],";
	#endif
  }
	sa=F("\"wifiscan\""); 	if(fields.indexOf(sa)>=0){ pg+=wifiscan(); }
	sa=F("\"homePage\"");		if(fields.indexOf(sa)>=0){ pg+=sa; pg+=":\""; pg+=homePage; pg+="\","; }
	#ifdef ESP32
		sa=F("\"analogRead\"");	if(fields.indexOf(sa)>=0){ pg+=sa; for(uint8_t i=0;i<6;i++){ if(i==0){ pg+=":["; }else{ pg+=","; } pg+=analogRead(an_pins[i]); } pg+="],"; }
		sa=F("\"an_pins\"");	if(fields.indexOf(sa)>=0){ pg+=sa; for(uint8_t i=0;i<6;i++){ if(i==0){ pg+=":["; }else{ pg+=","; } pg+=String(an_pins[i]); } pg+="],"; }
		#endif
	#ifdef ESP8266
		sa=F("\"analogRead\"");	if(fields.indexOf(sa)>=0){ pg+=sa; pg+=":["; pg+=analogRead(0); pg+="],"; }
		#endif
	#ifdef rfrx_pin
		sa=F("\"lastRFcode\""); if(fields.indexOf(sa)>=0){ pg+=sa; pg+=":\""; pg+=lastRFcode; pg+="\","; }
	#endif
	#ifdef ds18_pin
		sa="\"tem\"";	if(fields.indexOf(sa)>=0){ pg+=sa; pg+=":"; pg+="["; pg+=String(ds18b20(),1); pg+="],"; }
	#endif
	#ifdef PubSubClient_h
		sa="\"MQTT_BROKER\""; if(fields.indexOf(sa)>=0){ pg+=sa; pg+=":\""; pg+=MQTT_BROKER; pg+="\","; }
		sa="\"MQTT_PORT\"";   if(fields.indexOf(sa)>=0){ pg+=sa; pg+=":"; pg+=MQTT_PORT; pg+=","; }
		sa="\"MQTT_ID\"";     if(fields.indexOf(sa)>=0){ pg+=sa; pg+=":\""; pg+=MQTT_ID; pg+="\","; }
		sa="\"mqtt_status\""; if(fields.indexOf(sa)>=0){ pg+=sa; pg+=":"; pg+=mqtt_status; pg+=","; }
	#endif
	sa="\"FreeHeap\"";    if(fields.indexOf(sa)>=0){ pg+=sa; pg+=":"; pg+=ESP.getFreeHeap(); pg+=","; }
	sa="\"minFreeHeap\""; if(fields.indexOf(sa)>=0){ pg+=sa; pg+=":"; pg+=minFreeHeap; pg+=","; }
	#ifdef ESP8266
		sa="\"ResetReason\""; if(fields.indexOf(sa)>=0){ pg+=sa; pg+=":\""+ESP.getResetReason()+"\","; }
	#endif
	#ifdef ESP32
		sa="\"ResetReason\""; if(fields.indexOf(sa)>=0){ pg+=sa; pg+=":\""+String(esp_reset_reason())+"\","; }
	#endif
	sa=F("\"wifi_rssi\"");	if(fields.indexOf(sa)>=0){ pg+=sa; pg+=":"+String(WiFi.RSSI())+","; }
	///
	pg+=a_json(str);
	if(salvar>0)	fs_config_write();
	if(salvar>1) 	rst_cnt=20;
	if(rst_cnt>0){
		pg=F("<script>cnt=0;function second(){sp01.innerHTML+='.';cnt++;if(cnt>=5){history.go(-2);}}</script><body onload='setInterval(second,1000);'>ok<span id='sp01'></span></body>");
		return pg;
	}
	//
	pg=pg.substring(0,pg.length()-1); if(pg.length()==0) pg="{";
	pg+="}";
	return pg;
}

String req_value(String str, String sa){
  String sb;
  sb=str.substring(str.indexOf(sa)+sa.length())+"&";
  sb=sb.substring(0, sb.indexOf("&"));
  return sb;
}

///// log

void addLog(String text){
	log_buffer+=text+";;";
	if(log_buffer.length()>500){ log_buffer=log_buffer.substring(log_buffer.indexOf(";;")+2); }
	//if(Serial) Serial.println(text);
}

void log_second(time_t t){
  if((year()>=2001)&&(log_buffer.indexOf(";;")>=0)){
    String sa;
    if(hour()<10){   sa+="0";} sa+=hour(); sa+=":";
    if(minute()<10){ sa+="0";} sa+=minute(); sa+=":";
    if(second()<10){ sa+="0";} sa+=second(); sa+=" ";
    sa+=log_buffer.substring(0,log_buffer.indexOf(";;"));
    log_buffer=log_buffer.substring(log_buffer.indexOf(";;")+2);
    fs_gravar_log(t, sa);
    log_recent+=sa+";;";
    if(log_recent.length()>300){ log_recent=log_recent.substring(log_recent.indexOf(";;")+2); }
	if(Serial) Serial.println(sa);
  }
}

void fs_gravar_log(uint32_t t, String text){
  if(!fsok) return;
  String path,sb; path=F("/log/");
  sb=year(t); sb=sb.substring(2); path+=sb;
  if(month(t)<10) sb="0"; else sb="";
  sb+=month(t); path+=sb;
  if(day(t)<10) sb="0"; else sb="";
  sb+=day(t); path+=sb; path+=F(".txt");
  File wFile = SPIFFS.open(path,"a");
  if(!wFile){
    addLog(F("Erro ao abrir arquivo!"));
  }else{
    sb=text;
    wFile.println(sb);
  }
  wFile.close(); delay(10);
}

void fs_gravar_log_dia(String text, uint16_t ano, uint16_t mes, uint16_t dia){
  String path; path=".txt";
  path=String(dia+100)+path;  path=path.substring(1);
  path=String(mes+100)+path;  path=path.substring(1);
  path=String(ano)+path;      path=path.substring(2);
  path="/dia/"+path; // file name
  File wFile = SPIFFS.open(path, "a");
  if (wFile) { wFile.println(text); wFile.close(); delay(1); }
}

void handle24H(){
	#ifdef ESP8266
		server.sendHeader("Access-Control-Allow-Origin", "*");
		if (!server.chunkedResponseModeStart(200, "text/plain")){ server.send(505, F("text/html"), F("HTTP1.1 required"));    return; }
	#else
		String pg="";
	#endif
	//
	uint32_t t=now();  uint32_t t0= t-86400;  String path="";
	path="/dia/"+str00(year(t0))+str00(month(t0))+str00(day(t0))+".txt";
	uint16_t min0=hour(t)*60+minute(t); min0/=10; min0*=10;
	File file;
	file = SPIFFS.open(path, "r");
	if(file){
			while(file.available()){
				String sa=file.readStringUntil('\r'); sa.trim();
				if(sa.indexOf(" ")>0){
					uint16_t ia=sa.substring(0,sa.indexOf(" ")).toInt();
					if( ia >= min0 ){
						#ifdef ESP8266
							server.sendContent(String(ia-min0)+" "+String(ia)+sa.substring(sa.indexOf(" "))+"\r\n");
						#else
							pg+=String(ia-min0)+" "+String(ia)+sa.substring(sa.indexOf(" "))+"\r\n";
						#endif
					}
				}
			}
		file.close();
	}
  //
  path="/dia/"+str00(year(t))+str00(month(t))+str00(day(t))+".txt";
  file = SPIFFS.open(path, "r");
  if(file){
    while (file.available()) {
      String sa=file.readStringUntil('\r'); sa.trim();
      if(sa.indexOf(" ")>0){
        uint16_t ia=sa.substring(0,sa.indexOf(" ")).toInt();
			#ifdef ESP8266
				server.sendContent(String(ia+1440-min0)+" "+String(ia)+sa.substring(sa.indexOf(" "))+"\r\n");
			#else
				pg+=String(ia+1440-min0)+" "+String(ia)+sa.substring(sa.indexOf(" "))+"\r\n";
			#endif
      }
    }
    file.close();
  }
  //
	#ifdef ESP8266
		server.chunkedResponseFinalize();
	#else
		server.sendHeader("Access-Control-Allow-Origin", "*");
		server.send(200, "text/plain", pg);
	#endif
}

void handle30D(){
	#ifdef ESP8266
		server.sendHeader("Access-Control-Allow-Origin", "*");
		if (!server.chunkedResponseModeStart(200, "text/plain")){ server.send(505, F("text/html"), F("HTTP1.1 required"));    return; }
	#else
		String pg="";
	#endif
	//		
	uint32_t  t=now();
	uint8_t   mes_fim=month(t);
	uint32_t  t0= t-(86400*30);
	int16_t   dia_ini=day(t0);
	String path="";
	File file;
	uint8_t   mes=month(t0);
	//
	int16_t dayoffset = -dia_ini;
	while(month(t0)!=mes_fim){
		path="/mes/"+str00(year(t0))+str00(month(t0))+".txt";
		file = SPIFFS.open(path, "r");
		if(file){
			while(file.available()){
				String sa=file.readStringUntil('\n'); sa.trim();
				if(sa.indexOf(" ")>0){
					int16_t ia=sa.substring(0,sa.indexOf(" ")).toInt();
					if(ia>=dia_ini){
					#ifdef ESP8266
						server.sendContent(String(ia+dayoffset)+" "+sa+"\r\n");
					#else
						pg+=String(ia+dayoffset)+" "+sa+"\r\n";
					#endif
					}
				}
			}
			file.close();
		}
		dayoffset+=days_of_month(t0);
		mes=month(t0); while(mes==month(t0)) t0+=86400;
		dia_ini=1;
	}
	//
	path="/mes/"+str00(year(t0))+str00(month(t0))+".txt";
	file = SPIFFS.open(path, "r");
	if(file){
		while(file.available()){
			String sa=file.readStringUntil('\n'); sa.trim();
			if(sa.indexOf(" ")>0){
				int16_t ia=sa.substring(0,sa.indexOf(" ")).toInt();
				#ifdef ESP8266
					server.sendContent(String(ia+dayoffset)+" "+sa+"\r\n");
				#else
					pg+=String(ia+dayoffset)+" "+sa+"\r\n";
				#endif
			}
		}
		file.close();
  }
	//
	#ifdef ESP8266
		server.chunkedResponseFinalize();
	#else
		server.sendHeader("Access-Control-Allow-Origin", "*");
		server.send(200, "text/plain", pg);
	#endif
}

///// time

const int 	NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte 		packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

void time_setup(){
  time_log();
  if(year()<2001) setTime(0,0,0,1,1,2001); // setTime(hr,min,sec,day,mnth,yr);
  time_t t=now();
  prev_hour=hour(t);
  prev_day=day(t); //-1;
  prev_month=month(t); prev_year=year(t);
}

void time_log(){ // Time from Log
  if(!fsok) return;
  #if defined (ESP32)
	String path="";
	File root = SPIFFS.open("/");
	File file = root.openNextFile();
	while(file){
		String fileName = file.name();
		if(fileName.startsWith("/log/")){
			if ( fileName > path ) path=fileName;
		}
		file = root.openNextFile();
	}
  #endif
  #if defined (ESP8266)
	Dir dir = SPIFFS.openDir("/log/");
	String path="";
	while (dir.next()) {
		if ( dir.fileName() > path ){ path=dir.fileName(); }
	}
  #endif
  
  if(path.length()>0){
    File rFile = SPIFFS.open(path,"r");
    if(rFile){
      String line,line2;
      char sa;
      while (rFile.available()) {
        sa=rFile.read();
        if(sa==10){
          line.trim();
          if(line.length()>2){
            line2=line;
            String sc;
            a_read_log(line);
            //sc=" loop"; if(line.indexOf(sc)>0){  } // reset 
          }
          line="";
        }else{
          line+=sa;
        }
      }
      uint16_t an,me,di; // /log/200818.txt
      an=path.substring(5,7).toInt()+2000;
      me=path.substring(7,9).toInt();
      di=path.substring(9,11).toInt();
      uint16_t ho,mi,se; // 13:31:11 update ok 368512
      ho=line2.substring(0,2).toInt();
      mi=line2.substring(3,5).toInt();
      se=line2.substring(6,8).toInt();
      setTime(ho,mi,se,di,me,an);
      addLog("TimeLog "+path.substring(5)+" "+line2.substring(0,8));
    }
    rFile.close();
  }
}

String hhmmss(uint32_t t) {
  String sa;
  sa= hour(t);          sa += ":";
  sa+=str00(minute(t)); sa += ":";
  sa+=str00(second(t));
  return sa;
}

String dd_mm_aaaa(time_t t) {
  String sa;
  sa += str00(day(t)) + "-";
  sa += str00(month(t)) + "-";
  sa += String(year(t));
  return sa;
}

String str00(uint16_t n){
  String result="0";
  result+=String(n);
  result=result.substring(result.length()-2);
  return result;
}

void get_ntp_time(){
  String sa; sa=F("TimeNTP");
  time_t t=0;
  if(WiFi.status() == WL_CONNECTED){
    Udp.begin(8888); // local port 8888
    t=getNtpTime(ntpServer[timeNTP_errCnt%3]);
	Udp.stop();
	#ifdef ESP8266
		WiFiUDP::stopAll(); //addLog("WiFiUDP::stopAll()");
	#endif
    sa+=" "; sa+=ntpServer[timeNTP_errCnt%3];
  }
  if(t>0){
    setTime(t);
    addLog(sa+" OK");
    timeNTP_errCnt=0;
    timeNTP_cnt = timeNTP_interval;
  }else{
    timeNTP_errCnt++; if(timeNTP_errCnt>20) rst_cnt=20; // reinicia apos 20 erros
    timeNTP_cnt=timeNTP_errCnt+1;
    addLog(sa+" Fail "+String(timeNTP_errCnt)+" :-(");
    timeNTP_cnt=1; // tentar novamente apos 1 minuto
  }
}

time_t getNtpTime(String ntpServerIPstr){
  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  String sa,sb;
  sa=ntpServerIPstr+".";
  IPAddress ip;
  for(uint8_t i=0;i<4;i++){
    sb=sa.substring(0,sa.indexOf("."));
    sa=sa.substring(sa.indexOf(".")+1);
    ip[i]=sb.toInt();
  }
  sendNTPpacket( ip );
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      //addLog("TimeNTP "+ipStr(ip)+" "+hhmmss());
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  return 0; // return 0 if unable to get the time
}

void sendNTPpacket(IPAddress &address){
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

uint8_t days_of_month(time_t t){
  int8_t days;
  int8_t prev_month=month(t);
  while(prev_month==month(t)){ days=day(t); t+=86400; }
  return days;
}

///// update

void update_setup(){
    server.on("/update", HTTP_GET, []() {
      server.sendHeader("Connection", "close");
      String pg;
      pg=F(
        "<meta name='viewport' content='width=device-width, initial-scale=1'>"
        "<style>\n"
        " .custom-file-input{border:solid 1px silver;border-radius:4px;}"
        " .custom-file-input::-webkit-file-upload-button {visibility:hidden;width:0;border-radius:4px;}\n"
        " td{text-align:center;}"
        "</style>\n"
        "<script>\n"
        " var ver='");pg+=Versao; pg+=F(
		#if defined (ESP32)
		".ino.esp32.bin';\n"
		#endif
		#if defined (ESP8266)
		".ino.d1_mini.bin';\n"
		#endif
        " function fnoc(xx){\n"
        "  btn1.disabled=true;\n"
        "  xx=xx.substring(xx.lastIndexOf('\\\\')+1);  sa=ver.substring(0,ver.indexOf('_')); sb=xx.substring(0,ver.indexOf('_'));\n"
		#if defined (ESP32)
        "  if(xx.indexOf('.ino.esp32.bin')<0){alert('Arquivo rejeitado!');}\n"
		#endif
		#if defined (ESP8266)
		"  if(xx.indexOf('.ino.d1_mini.bin')<0){alert('Arquivo rejeitado!');}\n"
		#endif
        "  else if(sa==sb){ btn1.disabled=false; }\n"
        "  else{ if(confirm(ver+'\\n'+xx)){btn1.disabled=false;} }\n"
        " }\n"
        "</script>\n"
        "<div align=center><table>\n"
        "<tr><td><b>Update (Firmware)</b></td></tr>\n"
		#if defined (ESP32)
			"<tr><td><b>ESP32</b></td></tr>\n"
		#endif
		#if defined (ESP8266)
			"<tr><td><b>ESP8266</b></td></tr>\n"
		#endif
        "<tr><td>Versao atual: ");pg+=Versao; pg+=F("</td></tr>\n"
        "<form method='POST' action='/update' enctype='multipart/form-data'>"
        "<tr><td><input class='custom-file-input' type='file' id='update' name='update' onchange='fnoc(this.value);'></td></tr>\n"
        "<tr><td><input id='btn1' type='submit' value='Update' disabled></form></td></tr>"
        "</table></div>"
      );
      server.send(200, "text/html", pg);
    });
    #if defined (ESP32)
	server.on("/update", HTTP_POST, []() {
      server.sendHeader("Connection", "close");
      String pg;
      pg=F("<meta http-equiv=\"refresh\" content=\"10;URL='/'\" />update OK");
      server.send(200, "text/html", (Update.hasError()) ? "FAIL" : pg);
      server.handleClient();
      //delay(100); ESP.restart();
      rst_cnt=20; // upload.totalSize
    },[]() {
      HTTPUpload& upload = server.upload();
		if (upload.status == UPLOAD_FILE_START) {
			addLog("update "+upload.filename);
			#ifdef tft_eSPI
				tft_mode=0;
				tft.fillScreen(TFT_BLACK);
				tft.setTextColor(TFT_BLUE, TFT_BLACK);
				uint16_t left=120, top=140; if(tft_rotation==1){left=160; top=110;}
				tft.drawCentreString("WEB Update",left,top,4);
			#endif
			#if defined(i2c_SSD1306) || defined(i2c_SH1106)
				if(!oled_err){
					display.displayOn();
					display.clear();
					display.setColor(WHITE);
					display.normalDisplay();
					display.setTextAlignment(TEXT_ALIGN_CENTER);
					display.setFont(ArialMT_Plain_10);
					display.drawString(64, 27, "WEB Update");
					display.display();
				}
			#endif
			on_update();
			if (!Update.begin()) {  } //start with max available size
		} else if (upload.status == UPLOAD_FILE_WRITE) {
			if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {}
		}else if (upload.status == UPLOAD_FILE_END) {
			if (Update.end(true)) { //true to set the size to the current progress
				addLog("update ok "+String(upload.totalSize));
			}else{
				addLog("update err ");
			}
		}
	} );
	#endif
	#if defined (ESP8266)
	server.on("/update", HTTP_POST, []() {
		String pg="";
		pg=F("<meta http-equiv=\"refresh\" content=\"5;URL='/'\" />update ");
		pg+=(Update.hasError()) ? "FAIL" : "OK";
		server.sendHeader("Connection", "close");
		server.send(200, "text/html", pg);
		server.handleClient();
		rst_cnt=20;
	  }, []() {
		HTTPUpload& upload = server.upload();
		if (upload.status == UPLOAD_FILE_START) {
			WiFiUDP::stopAll();
			addLog("update "+upload.filename);
			#ifdef tft_eSPI
				tft_mode=0;
				tft.fillScreen(TFT_BLACK);
				tft.setTextColor(TFT_BLUE, TFT_BLACK);
				uint16_t left=120, top=140;	if(tft_rotation==1){left=160; top=110;}
				tft.drawCentreString("WEB Update",left,top,4);
			#endif
			#if defined(i2c_SSD1306) || defined(i2c_SH1106)
				if(!oled_err){
					display.displayOn();
					display.clear();
					display.setColor(WHITE);
					display.normalDisplay();
					display.setTextAlignment(TEXT_ALIGN_CENTER);
					display.setFont(ArialMT_Plain_10);
					display.drawString(64, 27, "WEB Update");
					display.display();
				}
			#endif
			on_update();
			uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
			if (!Update.begin(maxSketchSpace)) { //start with max available size
				Update.printError(Serial);
			}
		} else if (upload.status == UPLOAD_FILE_WRITE) {
			if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
				Update.printError(Serial);
			}
		} else if (upload.status == UPLOAD_FILE_END) {
		  if (Update.end(true)) { //true to set the size to the current progress
			//Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
			addLog("update ok "+String(upload.totalSize));
		  } else {
			//Update.printError(Serial);
			addLog("update err ");
		  }
		  //Serial.setDebugOutput(false);
		}
		yield();
	  });
	#endif
}

///// WiFi
void wf_setup(){
	#ifdef ESP32
		WiFi.mode(WIFI_MODE_APSTA); //esp32
		if(essid.length()>2){
			if (String(WiFi.SSID()) != String(essid)){
				//WiFi.persistent(true);
				WiFi.begin(essid.c_str(), epass.c_str());
			}
		}
	#endif
	#ifdef ESP8266
		WiFi.mode(WIFI_AP_STA);
		if(essid.length()>2){
			if (String(WiFi.SSID()) != String(essid)){	WiFi.begin(essid.c_str(), epass.c_str()); }
		}else{
			WiFi.disconnect();
		}
	#endif
	softAP_conect();
	/////
	server.on("/",      		handleRoot);
	server.on("/home",  		handleRoot);
	server.on("/js" ,   		handleJson);
	server.on("/json" , 		handleJson);
	server.on("/reset", 		handleReset);
	server.on("/upload", 		handleUpload);
	server.on("/list",        	handleFileList);
	server.on("/status",  		handleStatus);
	server.on("/fsformat",    	handleFSformat);
	server.on("/edit", HTTP_GET, [](){ if (!handleFileRead("/edit.htm")){ server.send(404, "text/plain", "FileNotFound");}});
	server.on("/edit", HTTP_PUT, handleFileCreate);
	server.on("/edit", HTTP_DELETE, handleFileDelete);
	server.on("/edit", HTTP_POST, [](){server.send(200, "text/plain", "");}, handleFileUpload);
	server.onNotFound( [](){ if(!handleFileRead(server.uri())){ server.send(404, "text/plain", "FileNotFound"); } } );
	server.on("/wifi.htm",[]{
		if (handleFileRead("/wifi.htm")) return;
		String pg;
		pg=F("<html><head><meta name='viewport' content='width=device-width, initial-scale=1'><style>input{width:242px;padding:4px;margin:5px 0 2px 0;}.li{text-align:left;width:240px;cursor:pointer;margin:5px 0 2px 0; border:solid 1px #222244;border-radius:4px;}.pr_bar{background:#8888FF;white-space: nowrap;overflow: visible;padding:4px;}input[type=button]{width:119px;}</style></head>"
		"<body onload='iniciar();'><div align=center>"
		"essid<br><input type='text' id='essid'><br>epass<br><input type='text' id='epass'><br><input type='button' value='Desonectar' onclick='essid.value=\"\";epass.value=\"\";conectar();'> <input type='button' value='Conectar' onclick='conectar();'><br>Redes<br>"
		"<div id='div01'>wait...</div>"
		"<script>function iniciar(){client.open('GET','js?&fields=wifiscan,essid,epass',true); client.send();}function conectar(){client.open('GET','js?essid='+essid.value+'&epass='+epass.value,true); client.send();}var client=new XMLHttpRequest();client.onreadystatechange=function(){if(this.readyState==4 && this.status==200){sa=this.responseText;if(sa.startsWith('{')){myObj=JSON.parse(sa);if(typeof myObj.essid!='undefined'){essid.value=myObj.essid;}if(typeof myObj.epass!='undefined'){epass.value=myObj.epass;}if(typeof myObj.wifiscan!='undefined'){wfs=myObj.wifiscan;wfs.sort();pg='<div style=\"margin:0 0 0 0;\">';for(i in wfs){wfs[i]=wfs[i].split(',');wid=(100+Number(wfs[i][0]))/100*240;"
		"pg+='<div class=\"li\" onclick=\"essid.value=wfs['+i+'][2];\"><div class=\"pr_bar\" style=\"width:'+wid+'px;\">'+wfs[i][2]+'</div></div>';} pg+='</div>';div01.innerHTML=pg;}}else{alert('Reiniciando!');setTimeout(function(){location.reload();},4000);}}}</script>"
		"</div></body></html>");
		server.send(200, "text/html", pg);
	});
	server.on("/24h.txt",  handle24H);
	server.on("/30d.txt",  handle30D);
	//
	update_setup();
	//
	server.begin();
}

void softAP_conect(){
  String host=DeviceID;
  if(WiFi.status() == WL_CONNECTED) host+="-"+String(WiFi.localIP()[3]);
  if(WiFi.softAP(host.c_str(), apPass.c_str())){
    addLog("AP "+host+" "+ WiFi.softAPIP().toString());
    if(apPass.length()>0) ap_disconn_cnt=APmode; else ap_disconn_cnt=3;
  }else{
    addLog("AP err");
    rst_cnt=20;
  }
}

void handleReset(){
  String pg="<html><meta http-equiv='refresh' content='5; url=/'>";
  pg+=F("<body>Reset...</body></html>");
  server.send(200, "text/html", pg);
  rst_cnt=20;
}

String wifiStatusText(byte x) {
  String result;
  if (x == 0) result = F("WL_IDLE_STATUS");
  if (x == 1) result = F("WL_NO_SSID_AVAIL");
  if (x == 2) result = F("WL_SCAN_COMPLETED");
  if (x == 3){ result = "WL_CONNECTED "+WiFi.localIP().toString(); }
  if (x == 4) result = F("WL_CONNECT_FAILED");
  if (x == 5) result = F("WL_CONNECTION_LOST");
  if (x == 6) result = F("WL_DISCONNECTED");
  if (x == 255) result = F("WL_NO_SHIELD");
  if (result == "") result = String(x);
  return result;
}

void set_ip(){
  if(ipMode==0) return;
  IPAddress ip =  WiFi.localIP();
  if(ip[3] == ipMode) return;
  IPAddress gateway(255, 255, 255, 1);
  IPAddress subnet(255, 255, 0, 0);
  gateway[0] = ip[0]; gateway[1] = ip[1]; gateway[2] = ip[2];
  ip[3] = ipMode;
  if(Ping.ping(ip) == false){
    addLog("Ping "+ip.toString()+" Livre");
    //WiFi.disconnect(); delay(10);
    //WiFi.persistent(false);
    //WiFi.mode(WIFI_OFF);
    //WiFi.mode(WIFI_MODE_APSTA);
    WiFi.config(ip, gateway, subnet);
    WiFi.begin(essid.c_str(), epass.c_str());
    //wifi_wait_con();
  }else{
    addLog("Ping "+ip.toString()+" Ocupado");
  }
}

String wifiscan(){
  // scanNetworks(bool async = false, bool show_hidden = false);
  int n = WiFi.scanNetworks(false,true);
  String sa;
  if (n > 0){
    sa+="\"wifiscan\":[";
    for (int i = 0; i < n; ++i){
      sa+=F("\"");
      sa+=WiFi.RSSI(i);sa+=F(",");
      sa+=WiFi.channel(i);sa+=F(",");
      sa+=WiFi.SSID(i);sa+=F(",");
      //if(WiFi.encryptionType(i) == ENC_TYPE_NONE)sa+=F("*\""); else sa+=F(" \"");
    sa += String(WiFi.encryptionType(i)) + "\"";
      if(i<(n-1)) sa+=F(",");
    }
    sa+="],";
  }
  return sa;
}

void handleRoot(){
	if(homePage.indexOf(".htm")>0) if (handleFileRead("/"+homePage)) return;
	String pg;
  pg+=F(
    "<html>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<style>\n"
    "body,input{background-color:black;color:silver;}"
    "input[type=button]{padding:4px 8px 4px 8px;}"
    " ul{margin:0;}\n"
    " td{text-align:center;}\n"
    " input{border-radius:4px;border:solid 1px #333333;margin:2px; }\n"
    " a{text-decoration:none;}\n"
    //" hr{width:240px;height:1px;border:none;border-top:solid 1px silver;margin:2px;}\n"
    " hr{width:240px;height:1px;border:none;border-top: 1px dashed #333333;margin:2px;}\n"
	"</style>\n"
    "<title></title>"
    "<body onload='iniciar();'><div align=center>"
    "<table width=240>\n"
	#ifdef ESP32
		"<tr><td><b>ESP32<br></b></td></tr>"
		#endif
	#ifdef ESP8266
		"<tr><td><b>ESP8266<br></b></td></tr>"
		#endif"
    "<tr><td colspan='2'><hr></td></tr>"
    "<tr><td><b id='DeviceID'>&nbsp;</b></td></tr>"
    "<tr><td id='datahora' onclick='d=new Date();req=\"js?time=\"+d.getTime()+\"&fields=DeviceID,time\";' style='cursor:pointer;' title='Send Time'>&nbsp;</td></tr>"
    "</table>"
    "<hr>"
    "<table><tr><td id='td01' style='text-align:left;width:120px;'></td></tr></table>\n"
	
    "<hr>\n"
	
	"<a href='/wifi.htm'><input type=button value=WiFi></a>\n"
	
    "<hr>\n"
    "File System\n"
    "<br><a href='/upload'><input type=button value='File Upload'></a>\n"
    "<input type=button value=Formatar onclick='if(confirm(\"Formatar?\")){location.href=\"fsformat\";}'>\n"
    "<hr>\n"
	"<a href='/update'><input type=button value=update></a> <a href='/reset'><input type=button value=reset></a>"
    "<hr>"
    "<div id='Versao' style='font:11px Arial;'>&nbsp;</div>\n"
    "</div>\n"
    
    "<script>\n"
    
    "var aa=[];\n"
    "var loaded=true;\n"
    "var req='';\n"
    
    "function iniciar(){ req='js?fields=DeviceID,Versao&list=/&random='+Math.floor(Math.random()*1000);\n"
    " document.body.style.zoom='140%';\n"
    " if(localStorage.getItem('htms')){ sa=localStorage.getItem('htms'); aa=sa.split(','); writelist(); }\n"
    " if(localStorage.getItem('datahora')){ datahora.innerHTML=localStorage.getItem('datahora');}\n"
    " loadjs(); sto=setInterval(loadjs,1000);\n"
    "}\n"
    
    "function writelist(){"
    " sa=''; for(i in aa){ aa[i]=aa[i].replace('.gz','');"
    "  sb=aa[i].substring(0,aa[i].indexOf('.htm'));"
    "  sa+='<li><a href=\"'+aa[i]+'\">'+sb+'</a></li>';"
    "  }"
    "  td01.innerHTML='<ul>'+sa+'</ul>';"
    " }\n"
    
    "function loadjs(){ if(loaded){ if(req.length>0){client.open('GET',req,true);client.send();loaded=false; } } }\n"
    
    "var client=new XMLHttpRequest();\n"
    "client.onreadystatechange=function(){\n"
    " if(this.readyState==4){\n"
    "  loaded=true;\n"
    "  if(this.status==200){\n"
    "   myObj=JSON.parse(this.responseText);\n"
    
    "   if(typeof myObj.DeviceID!='undefined'){\n"
    "    DeviceID.innerHTML=myObj.DeviceID; document.title=myObj.DeviceID;\n"
    "    req='js?fields=time';\n"
    "   }\n"
    
    "   if(typeof myObj.Versao!='undefined'){ Versao.innerHTML=myObj.Versao; }\n"
    
    "   if(typeof myObj.list!='undefined'){\n"
    "    aa=[];for(i in myObj.list){ sa=myObj.list[i]; sa=sa.substring(0,sa.indexOf(' ')); if(sa.indexOf('.htm')>0) aa[aa.length]=sa; }\n"
    "    aa.sort(); localStorage.setItem('htms', aa); writelist();"
    "    sto=setTimeout(loadjs,100);\n"
    "   }\n"
    
    "   if(typeof myObj.time!='undefined'){\n"
    "    time=myObj.time; time*=1000; var d=new Date(time); se=d.toISOString(); sf=se.substring(0,se.indexOf('T'));\n"
    "    se=se.substring(se.indexOf('T')+1); se=se.substring(0,se.indexOf('.')); datahora.innerHTML=sf+' '+se;\n"
    "    localStorage.setItem('datahora', datahora.innerHTML);\n"
    "   }\n"
    "  }\n"
    " }\n"
    "}\n"
    "</script>\n"
    "</body></html>"
    );
  server.send(200, "text/html", pg);
}

void wifi_minute(){
	if( wifi_con_wait>0){ wifi_con_wait--; return; }
	if( WiFi.status() == WL_CONNECTED ) return;
	if( essid.length()<=2 ) return;
	#if defined (ESP32)
		if( WiFi.softAPgetStationNum()>0 ) return;
	#endif
	#if defined (ESP8266)
		if( wifi_softap_get_station_num()>0 ) return;
	#endif
	addLog("WiFi connecting to "+essid);
	//WiFi.reconnect();
	WiFi.begin(essid.c_str(), epass.c_str());
	if (WiFi.waitForConnectResult() == WL_CONNECTED) {
		addLog(WiFi.localIP().toString());
		wifi_err_cnt=0;
		wifi_con_wait=0;
	} else {
		//WiFi.disconnect();
		wifi_err_cnt++;
		wifi_con_wait=wifi_err_cnt;
		addLog("WiFi Failed " + String(wifi_err_cnt));
	}
}

String http_req(String url){
	String payload=String();
	if(WiFi.status()==WL_CONNECTED){
		uint32_t dur=millis();
		http.setReuse(true);
		#ifdef ESP32
			http.setConnectTimeout(15000);
			http.begin(url);
		#endif
		#ifdef ESP8266
			http.setTimeout(15000);
			http.begin(client2, url);
		#endif
		int httpCode = http.GET();
		if(httpCode > 0) {
			if(httpCode == HTTP_CODE_OK) {
				payload = String(millis()-dur)+" "+http.getString();
			}
		} else {
			Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
		}
		http.end();
  }
  return payload;
}

void minFreeHeap_chk(){
  if((minFreeHeap-1000)>ESP.getFreeHeap()){
    minFreeHeap=ESP.getFreeHeap();
    addLog("FreeHeap "+String(minFreeHeap)+" "+String(ESP.getFreeHeap()));
  }
}

////////// RF RX

#ifdef rfrx_pin

void rfrx_loop(){
  rf433rx3();
  if(RFcode.length()>0) {
    if(RFcode_millis<millis()) lastRFcode="";
    if(lastRFcode!=RFcode){
      rfrx_executar_codigo(RFcode);
      lastRFcode=RFcode;
      RFcode_millis=millis()+2000;
    }
    RFcode="";
  }
}

void rf433rx3(){
  unsigned int dur, dur0, dur1;
  String pg;
  unsigned long Buffer=0;
  byte rf_bits;
  int lambda;
  //
  if(digitalRead(rfrx_pin)) return;
  dur=micros(); while(!digitalRead(rfrx_pin)){ if((micros()-dur)> 24000) return; } // 12000
  dur=micros()-dur; if(dur<4000) return; // 8000
  //*/
  dur1=micros(); while(digitalRead(rfrx_pin)){ if((micros()-dur1)> 2000) return; } // 2000
  dur1=micros()-dur1; if(dur1<200) return; // 200

  dur0=micros(); while(!digitalRead(rfrx_pin)){ if((micros()-dur0)> 2000) return; }
  dur0=micros()-dur0; if(dur0<200) return;

  if(dur0<dur1) lambda=dur0; else lambda=dur1;
  
  pg+=dur; pg+=F("-");
  pg+=dur1; pg+=F("-");
  pg+=dur0; pg+=F("-"); rf_bits++;
  
  for(int i = 0;i<32;i++){
    dur1=micros(); while(digitalRead(rfrx_pin)){ if((micros()-dur1)> 2000) break; }
    dur1=micros()-dur1; if(dur1<200) break;    
    dur0=micros(); while(!digitalRead(rfrx_pin)){ if((micros()-dur0)> 2000) break; }
    dur0=micros()-dur0; if(dur0<200) break;
    if(dur0<2000){
      pg+=dur1; pg+=F("-");
      pg+=dur0; pg+=F("-");
      rf_bits++;
    }
    #ifdef pinRF_TX
      if(rf_bits>5) rftx_wait=5; // 500ms
    #endif
  }
  if(rf_bits==24){
    pg=pg.substring(pg.indexOf("-")+1);
    for(byte i=0; i<24; i++){
      dur0=pg.substring(0,pg.indexOf("-")).toInt(); pg=pg.substring(pg.indexOf("-")+1);
      dur1=pg.substring(0,pg.indexOf("-")).toInt(); pg=pg.substring(pg.indexOf("-")+1);
      Buffer = (Buffer << 1); if( dur0 > dur1 ) Buffer++;   // add "1" on data buffer
    }
    RFcode=F("a"); RFcode+=String(Buffer);
    return;
  }
  if(rf_bits==28){
    pg=pg.substring(pg.indexOf("-")+1); pg=pg.substring(pg.indexOf("-")+1);
    for(byte i=0; i<28; i++){
      dur0=pg.substring(0,pg.indexOf("-")).toInt(); pg=pg.substring(pg.indexOf("-")+1);
      dur1=pg.substring(0,pg.indexOf("-")).toInt(); pg=pg.substring(pg.indexOf("-")+1);
      Buffer = (Buffer << 1); if( dur0 > dur1 ) Buffer++;   // add "1" on data buffer
    }
    RFcode=String(Buffer>>4);
    return;
  }
}
#endif

////////// RF TX

#ifdef rftx_pin

void rftx(String sa){
  if(sa.length()<3) return;
  #ifdef rfrx_pin
	if(sa==lastRFcode) return;
	#endif
  String sb;
  sb=","+rftxBuffer2+",";
  if(sb.indexOf(","+sa+",")<0) rftxBuffer2+=sa+",";
  rftx_wait=2;
}

void rftxLoop100(){
  if(rftx_wait>0){ rftx_wait--; return; }
  if(rftxBuffer2.length()==0) return;
  String sa;
  unsigned long code;
  sa=rftxBuffer2.substring(0,rftxBuffer2.indexOf(","));
  #ifdef rfrx_pin
	lastRFcode=sa;
	#endif
  addLog("RFTX "+sa);
  if(sa.substring(0,1)=="a"){
    sa=sa.substring(1); code=sa.toInt(); EV1527TX(code);
  }else{
    code=sa.toInt(); HT6P20BTX3(code);
  }
  rftxBuffer2=rftxBuffer2.substring(rftxBuffer2.indexOf(",")+1);
  rftx_wait=1;
}

#define tx_repeat 15
#define ht6p20b_t0 10800
#define ht6p20b_t1 468
#define ht6p20b_t2 952
#define ev1527_t0 9940
#define ev1527_t1 366
#define ev1527_t2 936

void EV1527TX(uint32_t evcode){ // repeat minimo 2
  pinMode(rftx_pin,OUTPUT);
  uint32_t tcode;
  for(uint8_t rp=0; rp < tx_repeat ; rp++){
    tcode=evcode;
    for(uint8_t i=0;i<24;i++){
      if(tcode & 0x800000 ){
        digitalWrite(rftx_pin, HIGH); delayMicroseconds(ev1527_t2);
        digitalWrite(rftx_pin, LOW);  delayMicroseconds(ev1527_t1);
      }else{
        digitalWrite(rftx_pin, HIGH); delayMicroseconds(ev1527_t1);
        digitalWrite(rftx_pin, LOW);  delayMicroseconds(ev1527_t2);
      }
      tcode=tcode<<1;
    }
    digitalWrite(rftx_pin, HIGH); delayMicroseconds(ev1527_t1);
    digitalWrite(rftx_pin, LOW);  delayMicroseconds(ev1527_t0);
  }
}

void HT6P20BTX3(uint32_t htcode){ // repeat minimo 2
  pinMode(rftx_pin,OUTPUT);
  uint32_t tcode;
  htcode=(htcode<<4) + 5;
  for(uint8_t rp=0; rp < tx_repeat ; rp++){
    tcode=htcode;
    digitalWrite(rftx_pin, HIGH); delayMicroseconds(ht6p20b_t1);
    for(uint8_t i=0;i<28;i++){
      if(tcode & 0x8000000 ){
        digitalWrite(rftx_pin, LOW);  delayMicroseconds(ht6p20b_t2);
        digitalWrite(rftx_pin, HIGH); delayMicroseconds(ht6p20b_t1);
      }else{
        digitalWrite(rftx_pin, LOW);  delayMicroseconds(ht6p20b_t1);
        digitalWrite(rftx_pin, HIGH); delayMicroseconds(ht6p20b_t2);
      }
      tcode=tcode<<1;
    }
    digitalWrite(rftx_pin, LOW);  delayMicroseconds(ht6p20b_t0);
  }
}

#endif

////////// IR RX

#ifdef pinIR_RX
unsigned long prev_ir_micros;
unsigned int ir_dur0, ir_dur1; byte ir_bit_cnt; unsigned long ir_buffer;
  
void ICACHE_RAM_ATTR IRread(){  // attachInterrupt(digitalPinToInterrupt(pinIR_RX), IRread, CHANGE);
	noInterrupts();
	if(digitalRead(pinIR_RX)){ // TSOP4838
			ir_dur0=micros()-prev_ir_micros;
			prev_ir_micros=micros();
      if((ir_dur0<225)||(ir_dur0>850)) {ir_bit_cnt=0; return;}
    }else{
      ir_dur1=micros()-prev_ir_micros;
      prev_ir_micros=micros();
      //if((ir_dur1<350)||(ir_dur1>1800)) {ir_bit_cnt=0; return;}
      if((ir_dur1<175)||(ir_dur1>1800)) {ir_bit_cnt=0; return;}
      ir_bit_cnt++;
      if(ir_bit_cnt==1) ir_buffer=0;
      if(ir_bit_cnt>32) {ir_bit_cnt=0; return;}
      ir_buffer=(ir_buffer << 1);
      if( (ir_dur0*1.5) < ir_dur1 ) ir_buffer++; // add "1" on data buffer
      if(ir_bit_cnt==32){ ir_Code = ir_buffer; }
    }
	interrupts();
}
  
void irrx_loop100(){
	if(ir_Code > 0) {
	  on_irrx(ir_Code);
	  last_irCode=ir_Code; //ir_cnt++;
	  ir_Code=0;
	}
}
  
#endif

///// QR code

#ifdef __QRCODE_H_
	#ifdef tft_eSPI
		void tft_qrcode(String text, uint16_t top){
		  uint8_t ver=0; uint16_t left;
		  //uint32_t cor=TFT_BLACK; uint32_t bgc=TFT_WHITE;
		  uint32_t cor=TFT_WHITE; uint32_t bgc=TFT_BLACK; //cor=TFT_YELLOW;
		  cor=TFT_BLACK; bgc=TFT_WHITE;
		  tft.fillRect( 0, top, 240, 180, bgc );
		//  tft.println(rtc_get_reset_reason(0));
		//  tft.println(rtc_get_reset_reason(1));
		  ///// Create the QR code
		  QRCode qrcode;
		  uint8_t ecc=ECC_LOW; // ECC_LOW ECC_MEDIUM ECC_QUARTILE ECC_HIGH
		  /*/ECC_LOW 180
		  if(text.length()<750) ver=18; //  89x89  ( 700 ok, falhou com 750 )
		  if(text.length()<272) ver=10; //  57x57  ( 271 ok, falhou com 272 )
		  if(text.length()<155) ver=7;  //  45x45  ( 154 ok, falhou com 155 )
		  if(text.length()<79)  ver=4;  //  33x33  ( 78 ok, falhou com 79)
		  if(text.length()<54)  ver=3;  //  29x29  ( 53 ok, falhou com 54)
		  if(text.length()<33)  ver=2;  //  25x25  ( 32 ok, falhou com 33)
		  if(text.length()<18)  ver=1;  //  21x21  ( 17 ok, falhou com 18)
		  //*/
		  //text=""; for(uint16_t i=0;i<585;i++) text+="a";
		  uint8_t zoom=1;
		  //ECC_LOW max:160x160
		  if(text.length()<=585){ zoom=2; ver=16; } //  81x81  ( 585 ok, falhou com  )
		  //zoom=2; if(text.length()<500) ver=15; //  77x77  ( 360 ok, falhou com  )
		  if(text.length()<231){ zoom=3; ver=9; } //  53x53  ( 230 ok, falhou com 231 )
		  //if(text.length()<107){ zoom=4; ver=5; } //  37x37  ( 106 ok, falhou com 107 )
		  if(text.length()<135){ zoom=4; ver=6; } //  41x41  ( 134 ok, falhou com 135 )
		  if(text.length()<54) { zoom=5; ver=3; } //  29x29  ( 53 ok,  falhou com 54 ) 
		  if(text.length()<33) { zoom=6; ver=2; } //  25x25  ( 32 ok,  falhou com 33 ) 
		  if(text.length()<18) { zoom=7; ver=1; } //  21x21  ( 17 ok,  falhou com 18 ) 
		  //*/
		  //if(text.length()<=174) ver=8; // 49x49  ( 174 ok, falhou com 175 )
		  //if(text.length()<=134) ver=6; //        ( 134 ok, falhou com 135 )
		  if(ver==0) return;
		  uint8_t qrcodeData[qrcode_getBufferSize(ver)];
		  qrcode_initText(&qrcode, qrcodeData, ver, ecc, text.c_str());
		  //String sa; sa=String(qrcode.size)+"x"+String(qrcode.size)+" "+String(text.length())+"/224"; tft.println(sa);
		  // while(zoom>1){ if((qrcode.size*zoom)<160){ break; } zoom--; }
		  //tft.setTextColor(cor, bgc); tft.setCursor(0, 0, 2); tft.println(text.length());   tft.print("z");tft.println(zoom);    tft.print("v");tft.println(ver);
		  left= (240-(qrcode.size*zoom))/2;
		  top = top + (180-(qrcode.size*zoom))/2;
		  for (uint8_t y = 0; y < qrcode.size; y++) {
			for (uint8_t x = 0; x < qrcode.size; x++) {
			  if(qrcode_getModule(&qrcode, x, y)){
				tft.fillRect(left+(x*zoom),top+(y*zoom),zoom,zoom,cor);
			  }
			}
		  }
		}
	#endif
#endif

// oled ssd1306
#if defined(i2c_SSD1306) || defined(i2c_SH1106)
	void oled_setup0(){
		//Wire.begin(SDA_pin, SCL_pin);
		String sa="oled ";
		#ifdef i2c_SSD1306
			Wire.beginTransmission(i2c_SSD1306);
			sa+="SSD1306 ";
		#endif
		#ifdef i2c_SH1106
			Wire.beginTransmission(i2c_SH1106);
			sa+="SH1106 ";
		#endif
		oled_err = Wire.endTransmission();
		if(oled_err){
		  sa+="err";
		}else{
		  sa+="ok";
		  display.init();
		  display.displayOn();
		  display.clear();
		  display.setColor(WHITE);
		  display.normalDisplay();
		  //if(oled_invertDisplay==true) display.invertDisplay();
		  #ifdef oled_flip
			display.flipScreenVertically();
		  #endif
		  display.setTextAlignment(TEXT_ALIGN_CENTER);
		  display.setFont(ArialMT_Plain_10);
		  display.drawString(64, 27, Versao);
		  display.display();
		}
		addLog(sa);
	}
	
	void oled_qrcode( String text, uint8_t top,  uint8_t lef, uint8_t zoom ){
	  if(oled_err) return;
	  uint8_t ver=2; uint8_t ecc=ECC_LOW;   //  25x25  ( 32 ok, falhou com 33)
	  QRCode qrcode;
	  uint8_t qrcodeData[qrcode_getBufferSize(ver)];
	  qrcode_initText(&qrcode, qrcodeData, ver, ecc, text.c_str());
	  display.setColor(WHITE);
	  display.fillRect(lef-2, top-2, 54, 54);
	  display.setColor(BLACK);
	  for (uint8_t y = 0; y < qrcode.size; y++) {
		for (uint8_t x = 0; x < qrcode.size; x++) {
		  if(qrcode_getModule(&qrcode, x, y)){
			display.fillRect(lef+(x*zoom), top+(y*zoom), zoom, zoom);
		  }
		}
	  }
	  display.display();
	}
	
#endif

////////// Ticker

void tkr50(){
	#ifdef blink_led
		#ifdef ESP32
			if(tkr50_cnt%20==0) digitalWrite(blink_led, HIGH );		// led on
			if(tkr50_cnt%20==4 && WiFi.status()!=WL_CONNECTED) digitalWrite(blink_led, HIGH );   // led on
			if(tkr50_cnt%4==2) digitalWrite(blink_led, LOW );		// led off
			if(tkr50_cnt%4==0 && loop_wdt_cnt>20) digitalWrite(blink_led, HIGH );   // led on
		#endif
		#ifdef ESP8266
			if(tkr50_cnt%20==0) digitalWrite(blink_led, LOW );   // led on
			if(tkr50_cnt%4==0 && loop_wdt_cnt>10) digitalWrite(blink_led, LOW );   // led on 500ms
			if(tkr50_cnt%4==2) 	digitalWrite(blink_led, HIGH );  // lef off
			if(tkr50_cnt%20==4 && WiFi.status()!=WL_CONNECTED) digitalWrite(blink_led, LOW );   // led on
		#endif
	#endif
	tkr50_cnt++;
	loop_wdt_cnt++; if(loop_wdt_cnt>1000){ ESP.restart(); } // 50 segundos
	if(max_loop_wdt_cnt<loop_wdt_cnt) max_loop_wdt_cnt=loop_wdt_cnt;
	ticker50();
}

#ifdef ds18_pin

	float ds18b20() {
	  byte i;
	  byte present = 0;
	  byte type_s;
	  byte data[12];
	  byte addr[8];
	  float celsius; //, fahrenheit;
	  if ( !ds.search(addr) ){
		//Serial.println("No more addresses.");
		ds.reset_search();
		return -100.0;
	  }
	  //
	  ds.reset();
	  ds.select(addr);
	  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
	  present = ds.reset();
	  ds.select(addr);
	  ds.write(0xBE);         // Read Scratchpad
	  for ( i = 0; i < 9; i++) {           // we need 9 bytes
		data[i] = ds.read();
	  }
	  int16_t raw = (data[1] << 8) | data[0];
	  if (type_s) {
		raw = raw << 3; // 9 bit resolution default
		if (data[7] == 0x10) {
		  raw = (raw & 0xFFF0) + 12 - data[6];
		}
	  } else {
		byte cfg = (data[4] & 0x60);
		if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
		else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
		else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
	  }
	  celsius = (float)raw / 16.0;
	  ds.reset_search();
	  //
	  if(celsius < -55 || celsius > 125){ return -100.0; }
	  return celsius;
	}

#endif

// HC-SR04 Ultrasonic Distance Sensor
#if defined(sr04_trigPin) && defined(sr04_echoPin)
	float read_dist(float temperature) {
		uint32_t duration;
		digitalWrite(sr04_trigPin, HIGH);
		delayMicroseconds(10);
		digitalWrite(sr04_trigPin, LOW);
		uint32_t prev_micros = micros();
		while (!digitalRead(sr04_echoPin)) {
			duration = micros() - prev_micros;
			if (duration > 1000) break;
		}
		if (duration > 1000) return 0;
		prev_micros = micros();
		while (digitalRead(sr04_echoPin)) {
			duration = micros() - prev_micros;
			if (duration > 24000) break; // max 4 metros
		}
		if (duration > 24000) return 999;
		float speedofsound = 331.3 + (0.606 * temperature);
		return ( duration * (speedofsound / 10000.0) / 2.0 ) ;
	}
#endif

#ifdef PubSubClient_h ///// MQTT
//
void 	mqtt_callback0(char* topic, byte* payload, unsigned int length);
void 	mqtt_callback(String topic, String msg);

void 	mqtt_file(String path);
void 	mqtt_24h();
void 	mqtt_30d();
//
void initMQTT(){
	#ifdef default_broker
		if(MQTT_BROKER=="") MQTT_BROKER=default_broker;
	#endif
  if(MQTT_ID==""){
    MQTT_ID=Versao;
    if(MQTT_ID.indexOf("_")>=0) MQTT_ID=MQTT_ID.substring(0,MQTT_ID.indexOf("_"));
    MQTT_ID += "/"+String(ESP_getChipId(),HEX);
  }
  if( (MQTT_BROKER.length()<5) || (MQTT_PORT<1) || (MQTT_ID.length()<1) ){
    mqtt_ok=false;
  }else{
    mqtt_ok=true;
    addLog("MQTT: "+MQTT_BROKER+":"+String(MQTT_PORT)+" "+MQTT_ID);
    MQTT.setServer(MQTT_BROKER.c_str(), MQTT_PORT);   //informa qual broker e porta deve ser conectado
    MQTT.setCallback(mqtt_callback0);                  //atribui função de callback (função chamada quando qualquer informação de um dos tópicos subescritos chega)
  }
}

void mqtt_loop(){
  if(MQTT.connected()) MQTT.loop();
  if(mqtt_status!=MQTT.connected()){
    if(MQTT.connected()) addLog("MQTT ok "+String(mqtt_con_cnt)); else addLog("MQTT !!");
    mqtt_status=MQTT.connected();
  }
}

void mqtt_second(){
  if(MQTT_BROKER.length()<4)  mqtt_ok=false;
  if(MQTT_PORT<1)             mqtt_ok=false;
  if(MQTT_ID.length()<1)      mqtt_ok=false;
  if(!mqtt_ok) return;
  if(!MQTT.connected()){
    if(mqtt_connect_wait==0){ reconnectMQTT(); }else{ mqtt_connect_wait--; }
  }
}

void reconnectMQTT(){
  if(WiFi.status()!=WL_CONNECTED) return;
  if(!mqtt_ok) return;
  if(mqtt_con_cnt>2) return;
  String sa;
  if(MQTT.connect(MQTT_ID.c_str())){
    mqtt_con_cnt++;
    mqtt_err=0; mqtt_connect_wait=0;
	String sb; sb=MQTT_ID+"/sub"; MQTT.subscribe(sb.c_str());
	mqtt_on_connect();
  }else{
    mqtt_err++;
    mqtt_connect_wait=mqtt_err*2;
  }
}

void mqtt_callback0(char* topic, byte* payload, unsigned int length){
	String msg; for(int i = 0; i < length; i++){ char c = (char)payload[i]; msg += c; } //obtem a string do payload recebido
	String sa; sa=MQTT_ID+"/sub";
	String sb=String(topic);
	if(sa==sb){
		if(msg.startsWith("?")||msg.startsWith("&")){
			msg.replace("?","&");
			String resp=msg+" "+query_resp(msg);
			String pub; pub=MQTT_ID+"/pub";
			MQTT.beginPublish(pub.c_str(), resp.length(), false);
			MQTT.print(resp.c_str());
			MQTT.endPublish();
		}
		else if(msg=="/24h.txt"){ mqtt_24h(); }
		else if(msg=="/30d.txt"){ mqtt_30d(); }
		else if(msg.startsWith("/")) { mqtt_file(msg); }
	}
	mqtt_callback(String(topic), msg);
}

void mqtt_file(String path){
  File file; file = SPIFFS.open(path, "r");
  if(file){
    String pub; pub=MQTT_ID+"/pub";
	String pg;
	pg=path+" ";
    while(file.available()){ pg+=file.readStringUntil('\r')+"\r"; }
    file.close();
    MQTT.beginPublish(pub.c_str(), pg.length(), false);
	MQTT.print(pg.c_str());
    MQTT.endPublish();
  }
}

void mqtt_24h(){
  String pg="/24h.txt ";
  uint32_t t=now();  uint32_t t0= t-86400;  String path="";
  path="/dia/"+str00(year(t0))+str00(month(t0))+str00(day(t0))+".txt";
  uint16_t min0=hour(t)*60+minute(t); min0/=10; min0*=10;
  File file;
  file = SPIFFS.open(path, "r");
  if(file){
    while(file.available()){
      String sa=file.readStringUntil('\r'); sa.trim();
      if(sa.indexOf(" ")>0){
        uint16_t ia=sa.substring(0,sa.indexOf(" ")).toInt();
        if(ia>=min0){ pg+=String(ia-min0)+" "+String(ia)+sa.substring(sa.indexOf(" "))+"\r\n"; }
      }
    }
    file.close();
  }
  path="/dia/"+str00(year(t))+str00(month(t))+str00(day(t))+".txt";
  file = SPIFFS.open(path, "r");
  if(file){
    while (file.available()) {
      String sa=file.readStringUntil('\r'); sa.trim();
      if(sa.indexOf(" ")>0){
        uint16_t ia=sa.substring(0,sa.indexOf(" ")).toInt();
        pg+=String(ia+1440-min0)+" "+String(ia)+sa.substring(sa.indexOf(" "))+"\r\n";
      }
    }
    file.close();
  }
  String pub; pub=MQTT_ID+"/pub";
  MQTT.beginPublish(pub.c_str(), pg.length(), false);
  MQTT.print(pg.c_str());
  MQTT.endPublish();
}

void mqtt_30d(){
  String pg="/30d.txt ";
  uint32_t  t=now();
  uint8_t   mes_fim=month(t);
  uint32_t  t0= t-(86400*30);
  int16_t   dia_ini=day(t0);
  String path="";
  File file;
  uint8_t   mes=month(t0);
  //
  int16_t dayoffset = -dia_ini;
  while(month(t0)!=mes_fim){
    path="/mes/"+str00(year(t0))+str00(month(t0))+".txt";
    file = SPIFFS.open(path, "r");
    if(file){
      while(file.available()){ String sa=file.readStringUntil('\n'); sa.trim();
        if(sa.indexOf(" ")>0){
          int16_t ia=sa.substring(0,sa.indexOf(" ")).toInt();
          if(ia>=dia_ini){ pg+=String(ia+dayoffset)+" "+sa+"\r\n"; }
        }
      }
      file.close();
    }
    dayoffset+=days_of_month(t0);
    mes=month(t0); while(mes==month(t0)) t0+=86400;
    dia_ini=1;
  }
  //
  path="/mes/"+str00(year(t0))+str00(month(t0))+".txt";
  file = SPIFFS.open(path, "r");
  if(file){
    while(file.available()){ String sa=file.readStringUntil('\n'); sa.trim();
      if(sa.indexOf(" ")>0){
        int16_t ia=sa.substring(0,sa.indexOf(" ")).toInt();
        pg+=String(ia+dayoffset)+" "+sa+"\r\n";
      }
    }
    file.close();
  }
  
  String pub; pub=MQTT_ID+"/pub";
  MQTT.beginPublish(pub.c_str(), pg.length(), false);
  MQTT.print(pg.c_str());
  MQTT.endPublish();
}

#endif
//