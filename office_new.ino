#include <SPI.h>
#include <Wire.h>      // this is needed even tho we aren't using it
#include <ESP8266WiFi.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ILI9341.h> // Hardware-specific library
#include <SD.h>
#include <Adafruit_STMPE610.h>
#include "Adafruit_MCP9808.h"

#define STMPE_CS 16
#define TFT_CS   0
#define TFT_DC   15
#define SD_CS    2
#define TS_MINX 3800
#define TS_MAXX 100
#define TS_MINY 100
#define TS_MAXY 3750
#define PENRADIUS 3
#define BUFFPIXEL 20
String cast;
String cast2;
String timeStamp;
String dateStamp;
char lf=10;

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
Adafruit_STMPE610 ts = Adafruit_STMPE610(STMPE_CS);
Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();

class comm {
  public:
  comm()
  {

  }
  
  const char* ssid     = "errans";
  const char* password = "zamb0rah";
  const char* host = "192.168.1.143";
  const int httpPort = 80;
  int failCount = 0;
  bool connected = 0;
  bool started = 0;
  
  bool connect() {
    if (started == 0) {
      Serial.print("Connecting to ");
      Serial.println(ssid);
      WiFi.begin(ssid, password);
      started == 1;
      unsigned long connectMillis = millis();
       while (WiFi.status() != WL_CONNECTED) {
        if (connectMillis + 30000 <= millis()) {
          //we failed--reset and try again next time.
          //no loop here because we need to clock to update.
          WiFi.disconnect();
          connected = 0;
          return 1;
        };
        delay(500);
        Serial.print(".");
        };
        connected = 1;
    } else {
      if (connected == 0) {
        unsigned long connectMillis = millis();
        WiFi.reconnect();
        while (WiFi.status() != WL_CONNECTED) {
          if (connectMillis + 30000 <= millis()) {
            //we failed--reset and try again next time.
            //no loop here because we need to clock to update.
            WiFi.disconnect();
            connected = 0;
            return 1;
          };
          delay(500);
          Serial.print(".");
        };
        
        Serial.println("");
        Serial.println("WiFi connected");  
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
        connected = 1;
        return 0;
      };
    };
  };
  
  bool report(float sendTemp) {
    WiFiClient client;
    const int httpPort = 80;
    int failCount = 0;
    if (!client.connect(host, httpPort)) {
          failCount++;
      Serial.println(failCount);
      delay(1000);
      if (failCount == 5) {
        connect();
        failCount = 0;
        return 1;
      }
      return 0;
    }

    String url = "/thermostat_api.php?inSub=true&id=2&temp=";
    url = url + sendTemp;
    url = url + "&humidity=";
    Serial.println("Reporting...");
    Serial.print("Requesting URL: ");
    Serial.println(url);
    
    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" + 
                 "Connection: close\r\n\r\n");
                 
    delay(1000);

    while(client.available()){
      failCount = 0;
      String line = client.readStringUntil('\r');
      Serial.println(line);
      Serial.println();
    };
  }
  
  bool fetchRadar() {
    int loaded = 0;
    for (int i = 1; i <= 9; i++) {
      WiFiClient client;
      const int httpPort = 80;
      if (!client.connect(host, httpPort)) {
        Serial.println("no connect!");
        delay(1000);
        loaded = 0;
        return 1;
      };
      loaded = 1;
      
      String url = "/weather/";
      url = url + i;
      url = url + ".bmp";
      client.flush();
      client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                   "Host: " + host + "\r\n" + 
                   "Connection: close\r\n\r\n");
      delay(1000);
      
      char c;
      char filename[13];
      itoa(i,filename,10);
      Serial.println();
      Serial.println(filename);
      if (SD.exists(filename)) {
        SD.remove(filename);
      };
      File imgFile = SD.open(filename, FILE_WRITE);
      Serial.print("downloading file...");

      while (client.available()) {
        String line = client.readStringUntil(lf);
        if (line.length() == 1){
          while(client.available()){
            c = client.read();
            imgFile.print(c);
          };
        };
      };
      
      imgFile.close();
      Serial.println("stored!");
      client.stop();
    };
    Serial.println();
    return 0;
  }

  bool fetchData() {
    WiFiClient client;
    const int httpPort = 80;
    int loaded = 0;
    Serial.println("Fetching tiny_dash.php...");
    if (!client.connect(host, httpPort)) {
      Serial.println("no connect!");
      delay(1000);
      loaded = 0;
      return 1;
    };
    loaded = 1;
    String url = "/tiny_dash.php";
    
    
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" + 
                 "Connection: close\r\n\r\n");
    delay(1000);
    while(client.available()){
      String line = client.readStringUntil(lf);
      if (line.length() == 1) {
        line = client.readString();
        Serial.println(line);
        cast = line;
      };
    };
    Serial.println();
    return 0;
  }

  bool fetchCurrent() {
    WiFiClient client;
    const int httpPort = 80;
    int loaded = 0;
    Serial.println("Fetching tine_dash_2.php...");
    if (!client.connect(host, httpPort)) {
      Serial.println("no connect!");
      delay(1000);
      loaded = 0;
      return 1;
    };
    loaded = 1;
    String url = "/tiny_dash_2.php";
    
    
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" + 
                 "Connection: close\r\n\r\n");
    delay(1000);
    while(client.available()){
      String line = client.readStringUntil(lf);
      if (line.length() == 1) {
        line = client.readStringUntil(lf);
        Serial.println(line);
        cast2 = line;
        line = client.readStringUntil(lf);
        Serial.println(line);
        timeStamp = line;
        line = client.readString();
        Serial.println(line);
        dateStamp = line;
      };
    };
    Serial.println();
    return 0;
  }
};

class gui {
  public:
  gui() {
    
  }
  
  void begin() {
    CCC.connect();
  };
  
  //these set the timers for the home screen areas and other screens
  unsigned int fetchCurrentMillis = millis();
  unsigned int fetchWeatherMillis = millis();
  unsigned int fetchCastMillis = millis();
  float myTemp;
  bool init = 1;
  
  comm CCC = comm();
  
  void bmpDraw(char *filename, uint8_t x, uint16_t y) {
    File     bmpFile;
    int      bmpWidth, bmpHeight;   // W+H in pixels
    uint8_t  bmpDepth;              // Bit depth (currently must be 24)
    uint32_t bmpImageoffset;        // Start of image data in file
    uint32_t rowSize;               // Not always = bmpWidth; may have padding
    uint8_t  sdbuffer[3*BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
    uint8_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
    boolean  goodBmp = false;       // Set to true on valid header parse
    boolean  flip    = true;        // BMP is stored bottom-to-top
    int      w, h, row, col;
    uint8_t  r, g, b;
    uint32_t pos = 0, startTime = millis();

    if((x >= tft.width()) || (y >= tft.height())) return;

    Serial.println();
    Serial.print(F("Loading image '"));
    Serial.print(filename);
    Serial.println('\'');

    // Open requested file on SD card
    if ((bmpFile = SD.open(filename)) == NULL) {
      Serial.print(F("File not found"));
      return;
    }

    // Parse BMP header
    if(read16(bmpFile) == 0x4D42) { // BMP signature
      Serial.print(F("File size: ")); Serial.println(read32(bmpFile));
      (void)read32(bmpFile); // Read & ignore creator bytes
      bmpImageoffset = read32(bmpFile); // Start of image data
      Serial.print(F("Image Offset: ")); Serial.println(bmpImageoffset, DEC);
      // Read DIB header
      Serial.print(F("Header size: ")); Serial.println(read32(bmpFile));
      bmpWidth  = read32(bmpFile);
      bmpHeight = read32(bmpFile);
      if(read16(bmpFile) == 1) { // # planes -- must be '1'
        bmpDepth = read16(bmpFile); // bits per pixel
        Serial.print(F("Bit Depth: ")); Serial.println(bmpDepth);
        if((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

          goodBmp = true; // Supported BMP format -- proceed!
          Serial.print(F("Image size: "));
          Serial.print(bmpWidth);
          Serial.print('x');
          Serial.println(bmpHeight);

          // BMP rows are padded (if needed) to 4-byte boundary
          rowSize = (bmpWidth * 3 + 3) & ~3;

          // If bmpHeight is negative, image is in top-down order.
          // This is not canon but has been observed in the wild.
          if(bmpHeight < 0) {
            bmpHeight = -bmpHeight;
            flip      = false;
          }

          // Crop area to be loaded
          w = bmpWidth;
          h = bmpHeight;
          if((x+w-1) >= tft.width())  w = tft.width()  - x;
          if((y+h-1) >= tft.height()) h = tft.height() - y;

          // Set TFT address window to clipped image bounds
          tft.setAddrWindow(x, y, x+w-1, y+h-1);

          for (row=0; row<h; row++) { // For each scanline...

            // Seek to start of scan line.  It might seem labor-
            // intensive to be doing this on every line, but this
            // method covers a lot of gritty details like cropping
            // and scanline padding.  Also, the seek only takes
            // place if the file position actually needs to change
            // (avoids a lot of cluster math in SD library).
            if(flip) // Bitmap is stored bottom-to-top order (normal BMP)
              pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
            else     // Bitmap is stored top-to-bottom
              pos = bmpImageoffset + row * rowSize;
            if(bmpFile.position() != pos) { // Need seek?
              bmpFile.seek(pos);
              buffidx = sizeof(sdbuffer); // Force buffer reload
            }

            for (col=0; col<w; col++) { // For each pixel...
              // Time to read more pixel data?
              if (buffidx >= sizeof(sdbuffer)) { // Indeed
                bmpFile.read(sdbuffer, sizeof(sdbuffer));
                buffidx = 0; // Set index to beginning
              }

              // Convert pixel from BMP to TFT format, push to display
              b = sdbuffer[buffidx++];
              g = sdbuffer[buffidx++];
              r = sdbuffer[buffidx++];
              tft.pushColor(tft.color565(r,g,b));
            } // end pixel
          } // end scanline
          Serial.print(F("Loaded in "));
          Serial.print(millis() - startTime);
          Serial.println(" ms");
        } // end goodBmp
      }
    }

    bmpFile.close();
    if(!goodBmp) Serial.println(F("BMP format not recognized."));
  }

  // These read 16- and 32-bit types from the SD card file.
  // BMP data is stored little-endian, Arduino is little-endian too.
  // May need to reverse subscript order if porting elsewhere.

  uint16_t read16(File &f) {
    uint16_t result;
    ((uint8_t *)&result)[0] = f.read(); // LSB
    ((uint8_t *)&result)[1] = f.read(); // MSB
    return result;
  }

  uint32_t read32(File &f) {
    uint32_t result;
    ((uint8_t *)&result)[0] = f.read(); // LSB
    ((uint8_t *)&result)[1] = f.read();
    ((uint8_t *)&result)[2] = f.read();
    ((uint8_t *)&result)[3] = f.read(); // MSB
    return result;
  }
  
  void drawMap() {
    if (fetchWeatherMillis + 300000 < millis() || init == 1) {
      CCC.fetchRadar();
      tft.fillRect(10,76,160,226,ILI9341_BLACK);
      bmpDraw("1", 10, 76);
      bmpDraw("2", 60, 76);
      bmpDraw("3", 110, 76);
      bmpDraw("4", 10, 126);
      bmpDraw("5", 60, 126);
      bmpDraw("6", 110, 126);
      bmpDraw("7", 10, 176);
      bmpDraw("8", 60, 176);
      bmpDraw("9", 110, 176);
      fetchWeatherMillis = millis();
    };
  }
  
  void drawCurrent() {
    if (fetchCurrentMillis + 60000 < millis() || init == 1) {
      CCC.fetchCurrent();
      myTemp = tempsensor.readTempC();
      myTemp = myTemp * 1.8 + 32;
      CCC.report(myTemp);
      tft.fillRect(170,76,320,240,ILI9341_BLACK);
      tft.setTextSize(2);
      tft.setTextColor(ILI9341_YELLOW);
      tft.setCursor(170,76);
      tft.print(dateStamp);
      tft.setCursor(170,96);
      tft.setTextColor(ILI9341_CYAN);
      tft.print(timeStamp);
      tft.setCursor(170,136);
      tft.setTextColor(ILI9341_YELLOW);
      tft.print("Office:");
      tft.setCursor(170,156);
      tft.setTextColor(ILI9341_CYAN);
      tft.print(myTemp);
      tft.print("F");
      tft.setCursor(170,196);
      tft.setTextColor(ILI9341_YELLOW);
      tft.print("Outside:");
      tft.setCursor(170,216);
      tft.setTextColor(ILI9341_CYAN);
      tft.print(cast2);
      fetchCurrentMillis = millis();
    };
  }
  
  void drawCast() {
    if (fetchCastMillis + 300000 < millis() || init == 1) {
      CCC.fetchData();
      tft.setTextColor(ILI9341_WHITE);
      tft.setTextSize(1);
      tft.setTextWrap(true);
      tft.fillRect(0,0,320,66,ILI9341_BLACK);
      tft.setCursor(1,1);
      tft.print(cast);
      fetchCastMillis = millis();
    };
  }

};

gui IO = gui();

void setup(void) {
  Serial.begin(9600);
  delay(10);
  Serial.println("FeatherWing TFT");
  if (!ts.begin()) {
    Serial.println("Couldn't start touchscreen controller");
    while (1);
  }
  Serial.println("Touchscreen started");
  
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);
  
  yield();

  Serial.print("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("failed!");
  }
  Serial.println("OK!");
  
  if (!tempsensor.begin()) {
    Serial.println("Couldn't find MCP9808!");
    while(1) {
      
    };
  };
  
  IO.begin();
  IO.drawMap();
  IO.drawCurrent();
  IO.drawCast();
  IO.init = 0;
};

void loop() {
  IO.drawMap();
  IO.drawCurrent();
  IO.drawCast();
  unsigned int cp = millis();
  /*while ((cp + 60000) > millis()) {
    TS_Point p = ts.getPoint();
    Serial.print("X = "); Serial.print(p.x);
    Serial.print("\tY = "); Serial.print(p.y);
    Serial.print("\tPressure = "); Serial.println(p.z);
    p.x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
    p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());
    delay(0);
  };*/
};