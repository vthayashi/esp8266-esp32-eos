
#ifdef tft_eSPI

void eSPI_setup(){
	tft.init();
	tft.setRotation(tft_rotation); // pa
	#if defined(TOUCH_CS) && !defined(tft_no_touch)
	//#ifdef TOUCH_CS
	//#ifndef tft_no_touch
		touch_calibrate();
	#endif
	tft.fillScreen(TFT_BLACK);
	tft.setTextColor(TFT_YELLOW, TFT_BLACK);
	if( tft_rotation==1) tft.drawCentreString(Versao,160,110,4); // paisagem
	if( tft_rotation==2) tft.drawCentreString(Versao,120,150,4); // retrato
}

void tft_draw_btn(String text, uint16_t lef, uint16_t top, uint16_t wid, uint16_t hei, bool pressed ){
	if(pressed){
		tft.fillRoundRect(lef+2, top+2, wid-4, hei-4, 8, TFT_RED);
		tft.setTextColor(TFT_WHITE, TFT_RED);
	}else{
		tft.drawRoundRect(lef+2, top+2, wid-4, hei-4, 8, TFT_DARKGREY);
	}
	if(tft.textWidth(text, 4)<(wid-20)){
		tft.drawCentreString(text, lef+(wid/2), top+(hei/2)-13, 4);
	}else{
		tft.drawCentreString(text, lef+(wid/2), top+(hei/2)-8, 2);
	}
}

#if defined(TOUCH_CS) && !defined(tft_no_touch)
//#ifdef TOUCH_CS
//#ifndef tft_no_touch
		void touch_calibrate(){
		  String CALIBRATION_FILE="/touchCalData"; CALIBRATION_FILE+=tft_rotation;
		  uint16_t calData[5];
		  uint8_t calDataOK = 0;
		  // check if calibration file exists and size is correct
		  if (SPIFFS.exists(CALIBRATION_FILE)) {
			  File f = SPIFFS.open(CALIBRATION_FILE, "r");
			  if (f) {
				if (f.readBytes((char *)calData, 14) == 14)
				  calDataOK = 1;
				f.close();
			  }
		  }
		  if (calDataOK) {
			// calibration data valid
			tft.setTouch(calData);
		  } else {
			// data not valid so recalibrate
			tft.fillScreen(TFT_BLACK);
			tft.setCursor(20, 0);
			tft.setTextFont(2);
			tft.setTextSize(1);
			tft.setTextColor(TFT_WHITE, TFT_BLACK);
			tft.println("Touch corners as indicated");
			tft.setTextFont(1);
			tft.println();
			tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);

			tft.setTextColor(TFT_GREEN, TFT_BLACK);
			tft.println("Calibration complete!");

			// store data
			File f = SPIFFS.open(CALIBRATION_FILE, "w");
			if (f) {
			  f.write((const unsigned char *)calData, 14);
			  f.close();
			}
		  }
		}
	#endif

void drawBmp(const char *filename, int16_t x, int16_t y) {

  if ((x >= tft.width()) || (y >= tft.height())) return;

  fs::File bmpFS;

  // Open requested file on SD card
  bmpFS = SPIFFS.open(filename, "r");

  if (!bmpFS) return;

  uint32_t seekOffset;
  uint16_t w, h, row, col;
  uint8_t  r, g, b;

  uint32_t startTime = millis();

  if (read16(bmpFS) == 0x4D42)
  {
    read32(bmpFS);
    read32(bmpFS);
    seekOffset = read32(bmpFS);
    read32(bmpFS);
    w = read32(bmpFS);
    h = read32(bmpFS);

    if ((read16(bmpFS) == 1) && (read16(bmpFS) == 24) && (read32(bmpFS) == 0)){
      y += h - 1;

      bool oldSwapBytes = tft.getSwapBytes();
      tft.setSwapBytes(true);
      bmpFS.seek(seekOffset);

      uint16_t padding = (4 - ((w * 3) & 3)) & 3;
      uint8_t lineBuffer[w * 3 + padding];

      for (row = 0; row < h; row++) {
        
        bmpFS.read(lineBuffer, sizeof(lineBuffer));
        uint8_t*  bptr = lineBuffer;
        uint16_t* tptr = (uint16_t*)lineBuffer;
        // Convert 24 to 16 bit colours
        for (uint16_t col = 0; col < w; col++)
        {
          b = *bptr++;
          g = *bptr++;
          r = *bptr++;
          *tptr++ = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
        }

        // Push the pixel row to screen, pushImage will crop the line if needed
        // y is decremented as the BMP image is drawn bottom up
        tft.pushImage(x, y--, w, 1, (uint16_t*)lineBuffer);
      }
      tft.setSwapBytes(oldSwapBytes);
      Serial.print("Loaded in "); Serial.print(millis() - startTime);
      Serial.println(" ms");
    }
    else Serial.println("BMP format not recognized.");
  }
  bmpFS.close();
}

uint16_t read16(File f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(File f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

	////////// jpeg

	#ifdef JPEGDECODER_H
	void drawJpg(const char *filename, int xpos, int ypos ){
		File jpegFile = SPIFFS.open( filename, "r");
		if ( jpegFile ) {
		  boolean decoded = JpegDec.decodeSdFile(jpegFile);
		  if (decoded) {
			jpegRender(xpos, ypos);
		  }
		}
	}

	//####################################################################################################
	// Draw a JPEG on the TFT, images will be cropped on the right/bottom sides if they do not fit
	//####################################################################################################
	// This function assumes xpos,ypos is a valid screen coordinate. For convenience images that do not
	// fit totally on the screen are cropped to the nearest MCU size and may leave right/bottom borders.
	void jpegRender(int xpos, int ypos) {

	  //jpegInfo(); // Print information from the JPEG file (could comment this line out)

	  uint16_t *pImg;
	  uint16_t mcu_w = JpegDec.MCUWidth;
	  uint16_t mcu_h = JpegDec.MCUHeight;
	  uint32_t max_x = JpegDec.width;
	  uint32_t max_y = JpegDec.height;

	  bool swapBytes = tft.getSwapBytes();
	  tft.setSwapBytes(true);
	  
	  // Jpeg images are draw as a set of image block (tiles) called Minimum Coding Units (MCUs)
	  // Typically these MCUs are 16x16 pixel blocks
	  // Determine the width and height of the right and bottom edge image blocks
	  uint32_t min_w = jpg_min(mcu_w, max_x % mcu_w);
	  uint32_t min_h = jpg_min(mcu_h, max_y % mcu_h);

	  // save the current image block size
	  uint32_t win_w = mcu_w;
	  uint32_t win_h = mcu_h;

	  // record the current time so we can measure how long it takes to draw an image
	  uint32_t drawTime = millis();

	  // save the coordinate of the right and bottom edges to assist image cropping
	  // to the screen size
	  max_x += xpos;
	  max_y += ypos;

	  // Fetch data from the file, decode and display
	  while (JpegDec.read()) {    // While there is more data in the file
		pImg = JpegDec.pImage ;   // Decode a MCU (Minimum Coding Unit, typically a 8x8 or 16x16 pixel block)

		// Calculate coordinates of top left corner of current MCU
		int mcu_x = JpegDec.MCUx * mcu_w + xpos;
		int mcu_y = JpegDec.MCUy * mcu_h + ypos;

		// check if the image block size needs to be changed for the right edge
		if (mcu_x + mcu_w <= max_x) win_w = mcu_w;
		else win_w = min_w;

		// check if the image block size needs to be changed for the bottom edge
		if (mcu_y + mcu_h <= max_y) win_h = mcu_h;
		else win_h = min_h;

		// copy pixels into a contiguous block
		if (win_w != mcu_w)
		{
		  uint16_t *cImg;
		  int p = 0;
		  cImg = pImg + win_w;
		  for (int h = 1; h < win_h; h++)
		  {
			p += mcu_w;
			for (int w = 0; w < win_w; w++)
			{
			  *cImg = *(pImg + w + p);
			  cImg++;
			}
		  }
		}

		// calculate how many pixels must be drawn
		uint32_t mcu_pixels = win_w * win_h;

		// draw image MCU block only if it will fit on the screen
		if (( mcu_x + win_w ) <= tft.width() && ( mcu_y + win_h ) <= tft.height())
		  tft.pushImage(mcu_x, mcu_y, win_w, win_h, pImg);
		else if ( (mcu_y + win_h) >= tft.height())
		  JpegDec.abort(); // Image has run off bottom of screen so abort decoding
	  }

	  tft.setSwapBytes(swapBytes);

	  //showTime(millis() - drawTime); // These lines are for sketch testing only
	}

	#endif

#endif
