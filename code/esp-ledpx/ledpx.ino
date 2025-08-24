#include <Arduino.h>
#include <WebServer.h>
#include <ArduinoJson.h>

#include <WiFiManager.h>


#include <Adafruit_GFX.h>
#include <Adafruit_SPITFT.h>
#include <Adafruit_SPITFT_Macros.h>
#include <gfxfont.h>
#include <Fonts/FreeMono24pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/Picopixel.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSerif18pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>


// This is how many color levels the display shows - the more the slower the update
//#define PxMATRIX_COLOR_DEPTH 8
#define PxMATRIX_COLOR_DEPTH 4

// Defines the buffer height / the maximum height of the matrix
// #define PxMATRIX_MAX_HEIGHT 64
#define PxMATRIX_MAX_HEIGHT 32

// Defines the buffer width / the maximum width of the matrix
#define PxMATRIX_MAX_WIDTH 64

// Defines how long we display things by default
//#define PxMATRIX_DEFAULT_SHOWTIME 30

// Defines the speed of the SPI bus (reducing this may help if you experience noisy images)
// #define PxMATRIX_SPI_FREQUENCY 20000000
#define PxMATRIX_SPI_FREQUENCY 10000000

// Creates a second buffer for backround drawing (doubles the required RAM)
#define PxMATRIX_double_buffer true

#include "PxMatrix.h"

// Pins for LED MATRIX
#ifdef ESP32

#define P_LAT 22
#define P_A 19
#define P_B 23
#define P_C 18
#define P_D 5 // N/C
// #define P_E 15
#define P_E 17 // N/C
// #define P_OE 2
#define P_OE 16
hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

#endif

#ifdef ESP8266

#include <Ticker.h>
Ticker display_ticker;
#define P_LAT 16
#define P_A 5
#define P_B 4
#define P_C 15
#define P_D 12
#define P_E 0
#define P_OE 2

#endif

// #define matrix_width 32
// #define matrix_height 16
#define matrix_width 64
#define matrix_height 32

// This defines the 'on' time of the display is us. The larger this number,
// the brighter the display. If too large the ESP will crash
uint8_t display_draw_time=60; //30-70 is usually fine

// PxMATRIX display(32,16,P_LAT, P_OE,P_A,P_B,P_C);
PxMATRIX display(64,32,P_LAT, P_OE,P_A,P_B,P_C,P_D);
//PxMATRIX display(64,64,P_LAT, P_OE,P_A,P_B,P_C,P_D,P_E);

// Some standard colors
uint16_t myRED = display.color565(255, 0, 0);
uint16_t myGREEN = display.color565(0, 100, 0);
uint16_t myBLUE = display.color565(0, 0, 255);
uint16_t myWHITE = display.color565(255, 255, 255);
uint16_t myYELLOW = display.color565(255, 255, 0);
uint16_t myCYAN = display.color565(0, 255, 255);
uint16_t myMAGENTA = display.color565(255, 0, 255);
uint16_t myBLACK = display.color565(0, 0, 0);
uint16_t myGRAY = display.color565(50, 50, 50);
uint16_t myDARKGRAY = display.color565(25, 25, 25);
uint16_t myLIGHTGRAY = display.color565(150, 150, 150);
uint16_t myLIGHTBLUE = display.color565(75, 75, 255);

#define num_colors 12
uint16_t myCOLORS[num_colors] = {myWHITE, myBLACK, myRED, myGREEN, myBLUE, myYELLOW, myCYAN, myMAGENTA, myGRAY, myDARKGRAY, myLIGHTGRAY, myLIGHTBLUE};
//                                     0.       1.     2.       3.      4.        5.      6.         7.      8.          9.          10.          11.

int x = 0;
int y = 0;
int width = 0;
int height = 0;
uint16_t myColor = display.color565(0, 0, 0);
boolean fill = false;
String content = "";
int size = 0;
int font = 0;
int brightness = 25;
int rotation = 0;

#ifdef ESP8266
// ISR for display refresh
void display_updater()
{
  display.display(display_draw_time);
}
#endif

#ifdef ESP32
void IRAM_ATTR display_updater(){
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);
  display.display(display_draw_time);
  portEXIT_CRITICAL_ISR(&timerMux);
}
#endif


void display_update_enable(bool is_enable)
{

#ifdef ESP8266
  if (is_enable)
    display_ticker.attach(0.004, display_updater);
  else
    display_ticker.detach();
#endif

#ifdef ESP32
  if (is_enable)
  {
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &display_updater, true);
    timerAlarmWrite(timer, 4000, true);
    timerAlarmEnable(timer);
  }
  else
  {
    timerDetachInterrupt(timer);
    timerAlarmDisable(timer);
  }
#endif
}




// Web server running on port 80
WebServer server(80);


unsigned long lastTimeRan;

StaticJsonDocument<1024> jsonDocument;

char buffer[1024];



void handleLine() {
  Serial.println("line");
  
  uint16_t x1 = x + width;
  uint16_t y1 = y + height;
  display.drawLine(x, y, x1, y1, myColor);
}

void handleRect() {
  Serial.println("rect");

  if (fill) {
    display.fillRect(x, y, width, height, myColor);
  } else {
    display.drawRect(x, y, width, height, myColor);
  }
}

void handleCircle() {
  Serial.println("circle");

  if (fill) {
    display.fillCircle(x, y, width, myColor);
  } else {
    display.drawCircle(x, y, width, myColor);
  }
}

void handleText() {
  Serial.println("text");
  
  display.setTextColor(myColor, myBLACK);

  if (font == 0) {
    display.setFont();
  } else if (font == 1) {
    display.setFont(&FreeMonoBold18pt7b);
  } else if (font == 2) {
    display.setFont(&FreeMono24pt7b);
  } else if (font == 3) {
    display.setFont(&Picopixel);
  } else if (font == 4) {
    display.setFont(&FreeMonoBold12pt7b); // added
  } else if (font == 5) {
    display.setFont(&FreeSansBold18pt7b); // added
  } else if (font == 6) {
    display.setFont(&FreeSerif18pt7b); // added
  } else if (font == 7) {
    display.setFont(&FreeSansBold12pt7b); // added
  } else if (font == 8) {
    display.setFont(&FreeSans9pt7b); // added
  }


  // display.setTextWrap(false);  // we don't wrap text so it scrolls nicely
  display.setTextSize(size);

  display.setCursor(x,y);
  display.print(content);
}

void handleSetup() {
  Serial.println("setup");
  
  display.setBrightness(brightness);

  display.setRotation(rotation); // 0 1 2 3 ?
}

void handleMulti() {

  // can handle 6 objects

  if (server.hasArg("plain") == false) {
    //handle error here
  }
  String body = server.arg("plain");
  deserializeJson(jsonDocument, body);

  // Iterate through the array
  for (JsonVariant obj : jsonDocument.as<JsonArray>()) {
    const char* type = obj["type"];
    
    x = obj["x"];
    y = obj["y"];
    width = obj["width"];
    height = obj["height"];
    int color = obj["color"];
    boolean clear = obj["clear"];
      
    fill = obj["fill"]; // fillrect/circle
    String content1 = obj["content"]; // TODO
    content = content1;
    size = obj["size"]; // scales font
    font = obj["font"]; // different font size and types
    brightness = obj["brightness"];
    rotation = obj["rotation"];

    if (clear) {
      display.clearDisplay();
    }

    if (color >= 0 && color < num_colors) {
      myColor = myCOLORS[color];
    }
    
    // Check the type and execute the corresponding function
    if (strcmp(type, "text") == 0) {
      handleText();
    } else if (strcmp(type, "rect") == 0) {
      handleRect();
    } else if (strcmp(type, "circle") == 0) {
      handleCircle();
    } else if (strcmp(type, "line") == 0) {
      handleLine();
    } else if (strcmp(type, "setup") == 0) {
      handleSetup();
    } else {
      Serial.println("Unknown type");
    }
  }

  server.send(200, "application/json", "{\"cmd\":\"multi\"}");
}

 
void addJsonObject(char *name) {
  JsonObject obj = jsonDocument.createNestedObject();
  obj["name"] = name;
}

const char* htmlCode = R"(
<html>
<meta name=viewport content=width=device-width,initial-scale=1,user-scalable=no>
<script type=text/javascript>
	function send(type) {
		const data = [{
			content: document.getElementById(type + '_content').value,
			x: document.getElementById(type + '_x').value,
			y: document.getElementById(type + '_y').value,
			clear: document.getElementById(type + '_clear').checked,
			font: parseInt(document.getElementById(type + '_font').value),
			color: parseInt(document.getElementById(type + '_color').value),
			size: parseInt(document.getElementById(type + '_size').value),
			width: parseInt(document.getElementById(type + '_width').value),
			height: parseInt(document.getElementById(type + '_height').value),
			fill: document.getElementById(type + '_fill').checked ? 1 : 0,
			brightness: parseInt(document.getElementById(type + '_brightness').value),
			rotation: parseInt(document.getElementById(type + '_rotation').value),
			type: type
		}]
		api(data)
	}

	function api(postData) {
		const url = 'http://' + location.host + '/multi';
		const requestOptions = {method: 'POST',headers: {'Content-Type': 'application/json'}, body: JSON.stringify(postData)};
		fetch(url, requestOptions)
	}

	function writeFonts() {
		const colors = ['Default','FreeMonoBold18pt7b','FreeMono24pt7b','Picopixel','FreeMonoBold12pt7b','FreeSansBold18pt7b','FreeSerif18pt7b','FreeSansBold12pt7b','FreeSans9pt7b']
		document.write(colors.map((c, i) => '<option value='+i+'>' + c + '</option>'))
	}

	function writeColors() {
		const colors = ['White','Black','Red','Green','Blue','Yellow','Cyan','Magenta','Gray', 'Dark Gray', 'Light Gray', 'Light Blue']
		document.write(colors.map((c, i) => '<option value='+i+'>' + c + '</option>'))
	}
</script>
<style>
	input, select {
		/*
		margin-left: 20px;
		margin-bottom: 5px;*/
	}
	input[type=button] {
		margin-left: 100px;
	}
	th {
		text-align: left;
	}
</style>


<table>
<tr><th colspan=2>Text <input type=button value=update onclick=send('text')></th></tr>
<tr><td>Content:</td><td><input id=text_content type=text value=content></td></tr>
<tr><td>X,Y:</td><td><input id=text_x type=text value=0 size=3>,<input id=text_y type=text value=0 size=3></td></tr>
<tr><td>Font:</td><td><select id=text_font><script>writeFonts()</script></select></td></tr>
<tr><td>Color:</td><td><select id=text_color><script>writeColors()</script></select></td></tr>
<tr><td>Size:</td><td><input id=text_size type=text value=1 size=3></td></tr>
<tr><td>Clear:</td><td><input id=text_clear type=checkbox></td></tr>

<input id=text_width type=hidden value=1>
<input id=text_height type=hidden value=1>
<input id=text_fill type=hidden value=0>
<input id=text_brightness type=hidden value=25>
<input id=text_rotation type=hidden value=0>


<tr><th colspan=2><br/>Line <input type=button value=update onclick=send('line')></th></tr>
<tr><td>X,Y:</td><td><input id=line_x type=text value=0 size=3>,<input id=line_y type=text value=0 size=3></td></tr>
<tr><td>Width, Height:</td><td><input id=line_width type=text value=10 size=3>,<input id=line_height type=text value=50 size=3></td></tr>
<tr><td>Fill:</td><td><input id=line_fill type=checkbox></td></tr>
<tr><td>Color:</td><td><select id=line_color><script>writeColors()</script></select></td></tr>
<tr><td>Clear:</td><td><input id=line_clear type=checkbox></td></tr>


<input id=line_content type=hidden value=0>
<input id=line_size type=hidden value=0>
<input id=line_font type=hidden value=0>
<input id=line_brightness type=hidden value=25>
<input id=line_rotation type=hidden value=0>


<tr><th colspan=2><br/>Rect <input type=button value=update onclick=send('rect')></th></tr>
<tr><td>X,Y:</td><td><input id=rect_x type=text value=0 size=3>,<input id=rect_y type=text value=0 size=3></td></tr>
<tr><td>Width, Height:</td><td><input id=rect_width type=text value=10 size=3>,<input id=rect_height type=text value=50 size=3></td></tr>
<tr><td>Fill:</td><td><input id=rect_fill type=checkbox></td></tr>
<tr><td>Color:</td><td><select id=rect_color><script>writeColors()</script></select></td></tr>
<tr><td>Clear:</td><td><input id=rect_clear type=checkbox></td></tr>

<input id=rect_content type=hidden value=0>
<input id=rect_size type=hidden value=0>
<input id=rect_font type=hidden value=0>
<input id=rect_brightness type=hidden value=25>
<input id=rect_rotation type=hidden value=0>



<tr><th colspan=2><br/>Circle <input type=button value=update onclick=send('circle')></th></tr>
<tr><td>X,Y:</td><td><input id=circle_x type=text value=0 size=3>,<input id=circle_y type=text value=0 size=3></td></tr>
<tr><td>Width, Height:</td><td><input id=circle_width type=text value=10 size=3>,<input id=circle_height type=text value=50 size=3></td></tr>
<tr><td>Fill:</td><td><input id=circle_fill type=checkbox></td></tr>
<tr><td>Color:</td><td><select id=circle_color><script>writeColors()</script></select></td></tr>
<tr><td>Clear:</td><td><input id=circle_clear type=checkbox></td></tr>

<input id=circle_content type=hidden value=0>
<input id=circle_size type=hidden value=0>
<input id=circle_font type=hidden value=0>
<input id=circle_brightness type=hidden value=25>
<input id=circle_rotation type=hidden value=0>


<tr><th colspan=2><br/>Setup <input type=button value=update onclick=send('setup')></th></tr>
<tr><td>Brightness:</td><td><input id=setup_brightness type=text value=25 size=3></td></tr>
<tr><td>Rotation:</td><td><select id=setup_rotation><option>0</option><option>1</option><option>2</option><option>3</option></select></td></tr>
<tr><td>Clear:</td><td><input id=setup_clear type=checkbox></td></tr>

<input id=setup_content type=hidden value=0 size=3>
<input id=setup_x type=hidden value=0 size=3>
<input id=setup_y type=hidden value=0 size=3>
<input id=setup_font type=hidden value=0>
<input id=setup_color type=hidden value=0>
<input id=setup_size type=hidden value=1 size=3>

<input id=setup_width type=hidden value=1>
<input id=setup_height type=hidden value=1>
<input id=setup_fill type=hidden value=0>

</table>

</html>


)";




void getRoot() {
  Serial.println("Get ");
  // jsonDocument.clear(); // Clear json buffer
  // addJsonObject("PxMatrix");
  
  // serializeJson(jsonDocument, buffer);
  // server.send(200, "application/json", buffer);

  server.send(200, "text/html", htmlCode);
}

void sendCrossOriginHeader() {

  // server.sendHeader("Access-Control-Allow-Origin", "*"));
  // server.sendHeader("Access-Control-Max-Age", "600"));
  // server.sendHeader("Access-Control-Allow-Methods", "PUT,POST,GET,OPTIONS"));
  // server.sendHeader("Access-Control-Allow-Headers", "*"));

  server.send(204);
}

void setupApi() {
  server.enableCORS();

  server.on("/", HTTP_OPTIONS, sendCrossOriginHeader);
  server.on("/setup", HTTP_OPTIONS, sendCrossOriginHeader);
  server.on("/multi", HTTP_OPTIONS, sendCrossOriginHeader);

  server.on("/", getRoot);
  server.on("/setup", HTTP_POST, handleSetup);
  server.on("/multi", HTTP_POST, handleMulti);
 
  // start server
  server.begin();
}


void setupDisplay() {

  // Define your display layout here, e.g. 1/8 step, and optional SPI pins begin(row_pattern, CLK, MOSI, MISO, SS)
  display.begin(8);

  // Define multiplex implemention here {BINARY, STRAIGHT} (default is BINARY)
  display.setMuxPattern(BINARY);

  // Set the multiplex pattern {LINE, ZIGZAG,ZZAGG, ZAGGIZ, WZAGZIG, VZAG, ZAGZIG} (default is LINE)
  display.setScanPattern(LINE);

  // Set the number of panels that make up the display area width (default is 1)
  display.setPanelsWidth(2);

  display.setScanPattern(WZAGZIG2);

  display_update_enable(true);

  brightness = 50;
  rotation = 1;
  handleSetup();

  // Set the brightness of the panels (default is 255)
  // display.setBrightness(50);

  display.clearDisplay();
  display.setTextColor(myWHITE, myBLACK);
  display.setCursor(1,1);
  display.print("ROGUE");
}

void setupWifi() {

  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  // it is a good practice to make sure your code sets wifi mode how you want it.

  //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;

  // reset settings - wipe stored credentials for testing
  // these are stored by the esp library
  // wm.resetSettings();  

  // Automatically connect using saved credentials,
  // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
  // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
  // then goes into a blocking loop awaiting configuration and will return success result

  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  res = wm.autoConnect("ledpx","password"); // password protected ap

  if(!res) {
      Serial.println("Failed to connect");
      ESP.restart();
  } else {
      //if you get here you have connected to the WiFi    
      Serial.println("Connected...yeey :)");
  }

  setupApi();
}

void showIp() {
  Serial.print("local ip: ");
  Serial.println(WiFi.localIP());
  
  // display.setFont(&Picopixel);
  display.setTextColor(myYELLOW, myBLACK);
  display.setCursor(1,20);
  display.print(WiFi.localIP());
}

void setup() {

  Serial.begin(115200);
  // This delay gives the chance to wait for a Serial Monitor without blocking if none is found
  delay(2500); 

// has to be before setupDisplay or xQueueSemaphoreTake queue.c:1554 error
  Serial.println("setupWifi");
  setupWifi();

  Serial.println("setupDisplay");
  setupDisplay();

  Serial.println("showIp");
  showIp();
}

void loop() {
  // put your main code here, to run repeatedly:
  server.handleClient();
}