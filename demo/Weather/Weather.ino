#include <WiFi.h>
#include <ArduinoJson.h>   // https://github.com/bblanchon/ArduinoJson
#include "time.h"
#include <Wire.h>
#include <SSD1306.h>
#include <OLEDDisplayUi.h>
#include "config.h"

// WU Icon names, there are 37, but many are the same image, so use the *icon when required, giving 10 basic icons to be used
// *snow            == chanceflurries == chancesnow == flurries == nt_chanceflurries == nt_chancesnow == nt_flurries == nt_snow
// *rain            == chancerain  == nt_chancerain  == nt_rain
// *sleet           == chancesleet == nt_chancesleet == nt_sleet
// *sunny           == clear
// *cloudy          == nt_cloudy
// *mostlysunny     == partlycloudy == partlysunny == mostlycloudy
// *fog             == nt_fog == nt_hazy == hazy
// *tstorms         == chancetstorms == nt_tstorms == nt_chancetstorms
// *nt_mostlycloudy == nt_mostlysunny == nt_partlycloudy == nt_partlysunny
// *nt_clear

#define icon_width  50
#define icon_height 50

extern const char snow_bits[];
extern const char rain_bits[];
extern const char sleet_bits[];
extern const char sunny_bits[];
extern const char cloudy_bits[];
extern const char mostlysunny_bits[];
extern const char fog_bits[];
extern const char tstorms_bits[];
extern const char nt_mostlycloudy_bits[];
extern const char nt_clear_bits[];
extern const char snow_bits[];

unsigned long        lastConnectionTime = 0;          // Last time you connected to the server, in milliseconds
const unsigned long  postingInterval = 30L*60L*1000L; // Delay between updates, in milliseconds, WU allows 500 requests per-day maximum, this sets it to every 30-mins or 48/day
 
String currCondString, time_str; // strings to hold received API weather data and current time

SSD1306 display(0x3c, 4, 15); // OLED display object definition (address, SDA, SCL)
OLEDDisplayUi ui     ( &display );

WiFiClient client; // wifi client object

bool  api_error;
const char* CurrentObservation_location_full;
const char* CurrentObservation_location_city;
const char* CurrentObservation_location_state;
const char* CurrentObservation_location_state_name;
const char* CurrentObservation_location_zip;
const char* CurrentObservation_location_country;
const char* CurrentObservation_location_country_iso3166;
const char* CurrentObservation_latitude;
const char* CurrentObservation_longitude;
const char* CurrentObservation_elevation;
const char* CurrentObservation_local_time_rfc822;
const char* CurrentObservation_observation_time;
const char* CurrentObservation_weather;
const char* CurrentObservation_temperature_string;
const char* CurrentObservation_temp_f;
const char* CurrentObservation_temp_c;
const char* CurrentObservation_dewpoint_f;
const char* CurrentObservation_dewpoint_c;
const char* CurrentObservation_heat_index_f;
const char* CurrentObservation_heat_index_c;
const char* CurrentObservation_windchill_f;
const char* CurrentObservation_windchill_c;
const char* CurrentObservation_relative_humidity;
const char* CurrentObservation_wind_string;
const char* CurrentObservation_wind_mph;
const char* CurrentObservation_wind_kph;
const char* CurrentObservation_wind_dir;
const char* CurrentObservation_wind_degrees;
const char* CurrentObservation_visibility_mi;
const char* CurrentObservation_visibility_km;
const char* CurrentObservation_pressure_mb;
const char* CurrentObservation_pressure_in;
const char* CurrentObservation_pressure_trend;
const char* CurrentObservation_precip_today_string;
const char* CurrentObservation_precip_today_in;
const char* CurrentObservation_precip_today_metric;
const char* CurrentObservation_icon;

bool
CurrCondObj(const String* currCondString)
{
  api_error = false;
  // When using a StaticJsonBuffer you must allocate sufficient memory for the json string returned by the WU api
  //Serial.println(*currCondString);
  Serial.println("Creating object...");
  DynamicJsonBuffer jsonBuffer(5*1024);
  // Create root object and parse the json file returned from the api. The API returns errors and these need to be checked to ensure successful decoding
  JsonObject& root = jsonBuffer.parseObject(*(currCondString));
  if(!root.success())
  {
    // if the root object could not be created, then return an error make an API call the next time around
    Serial.println("Unable to create a root object");
    api_error = true;
    return false;
  }

    Serial.println("root object created...");
    JsonObject& CurrentObservation = root["current_observation"];
    if(!CurrentObservation.success())
    {
      // if the object could not be created, then return an error make an API call the next time around
      Serial.println("Unable to obtain current observation object");
      api_error = true;
      return false;
    }

      Serial.println("CurrentObservation object created...");
      // Now process the  "display_location":
      JsonObject& CurrentObservation_display_location     = CurrentObservation["display_location"]; // Set root location
      
      CurrentObservation_location_full            = CurrentObservation_display_location["full"]; 
      CurrentObservation_location_city            = CurrentObservation_display_location["city"];
      CurrentObservation_location_state           = CurrentObservation_display_location["state"]; 
      CurrentObservation_latitude                 = CurrentObservation_display_location["latitude"]; 
      CurrentObservation_longitude                = CurrentObservation_display_location["longitude"]; 
      CurrentObservation_elevation                = CurrentObservation_display_location["elevation"]; 
      CurrentObservation_location_state_name      = CurrentObservation_display_location["state_name"]; 
      CurrentObservation_location_country         = CurrentObservation_display_location["country"]; 
      CurrentObservation_location_country_iso3166 = CurrentObservation_display_location["country_iso3166"]; 
      CurrentObservation_location_zip             = CurrentObservation_display_location["zip"]; 
      
      CurrentObservation_local_time_rfc822        = CurrentObservation["local_time_rfc822"]; 
      CurrentObservation_observation_time         = CurrentObservation["observation_time"]; 
      CurrentObservation_weather                  = CurrentObservation["weather"]; 
      CurrentObservation_temperature_string       = CurrentObservation["temperature_string"]; 
      CurrentObservation_temp_f                   = CurrentObservation["temp_f"]; 
      CurrentObservation_temp_c                   = CurrentObservation["temp_c"]; 
      CurrentObservation_dewpoint_f               = CurrentObservation["dewpoint_f"]; 
      CurrentObservation_dewpoint_c               = CurrentObservation["dewpoint_c"]; 
      CurrentObservation_heat_index_f             = CurrentObservation["heat_index_f"]; 
      CurrentObservation_heat_index_c             = CurrentObservation["heat_index_c"]; 
      CurrentObservation_windchill_f              = CurrentObservation["windchill_f"]; 
      CurrentObservation_windchill_c              = CurrentObservation["windchill_c"];       
      CurrentObservation_relative_humidity        = CurrentObservation["relative_humidity"]; 
      CurrentObservation_wind_string              = CurrentObservation["wind_string"]; 
      CurrentObservation_wind_mph                 = CurrentObservation["wind_mph"]; 
      CurrentObservation_wind_kph                 = CurrentObservation["wind_kph"]; 
      CurrentObservation_wind_dir                 = CurrentObservation["wind_dir"]; 
      CurrentObservation_wind_degrees             = CurrentObservation["wind_degrees"]; 
      CurrentObservation_pressure_mb              = CurrentObservation["pressure_mb"]; 
      CurrentObservation_pressure_in              = CurrentObservation["pressure_in"]; 
      CurrentObservation_pressure_trend           = CurrentObservation["pressure_trend"]; 
      CurrentObservation_precip_today_string      = CurrentObservation["precip_today_string"]; 
      CurrentObservation_precip_today_in          = CurrentObservation["precip_today_in"];
      CurrentObservation_precip_today_metric      = CurrentObservation["precip_today_metric"]; 
      CurrentObservation_visibility_mi            = CurrentObservation["visibility_mi"];
      CurrentObservation_visibility_km            = CurrentObservation["visibility_km"];
      CurrentObservation_icon                     = CurrentObservation["icon"]; 
      return true;
}

// Functions for weather data.
String getCountry()          {return CurrentObservation_location_country;}
String getCity()             {return CurrentObservation_location_city;}
String getCityState()        {return CurrentObservation_location_full;}
String getState()            {return CurrentObservation_location_state_name;}
String getZip()              {return CurrentObservation_location_zip;}
String getLatitude()         {return CurrentObservation_latitude;}
String getLongitude()        {return CurrentObservation_longitude;}
String getElevation()        {return CurrentObservation_elevation;}; 
String getLocalTime()        {return CurrentObservation_local_time_rfc822;}
String getObsTime()          {return CurrentObservation_observation_time;}
String getCurrWeather()      {return CurrentObservation_weather;}
String getCurrTempString()   {return CurrentObservation_temperature_string;}
String getCurrF()            {return CurrentObservation_temp_f;}
String getCurrC()            {return CurrentObservation_temp_c;}
String getDewPointF()        {return CurrentObservation_dewpoint_f;}
String getDewPointC()        {return CurrentObservation_dewpoint_c;}
String getHeatindexF()       {return CurrentObservation_heat_index_f;}
String getHeatindexC()       {return CurrentObservation_heat_index_c;}
String getWindchillF()       {return CurrentObservation_windchill_f;}
String getWindchillC()       {return CurrentObservation_windchill_c;}
String getRelHum()           {return CurrentObservation_relative_humidity;}
String getWindString()       {return CurrentObservation_wind_string;}
String getWindMPH()          {return CurrentObservation_wind_mph;}
String getWindKPH()          {return CurrentObservation_wind_kph;}
String getWindDir()          {return CurrentObservation_wind_dir;}
String getWindDegrees()      {return CurrentObservation_wind_degrees;}
String getPressure_mb()      {return CurrentObservation_pressure_mb;}
String getPressure_in()      {return CurrentObservation_pressure_in;}
String getPressure_trend()   {return CurrentObservation_pressure_trend;}
String getPrecipTodayString(){return CurrentObservation_precip_today_string;}
String getPrecipTodayInches(){return CurrentObservation_precip_today_in;}
String getPrecipTodayMet()   {return CurrentObservation_precip_today_metric;}
String getVisibility_mi()    {return CurrentObservation_visibility_mi;}
String getVisibility_km()    {return CurrentObservation_visibility_km;}
String getWeatherIcon()      {return CurrentObservation_icon;}

/* Typical API response:
{
  "response": {
  "version":"0.1",
  "termsofService":"http://www.wunderground.com/weather/api/d/terms.html",
  "features": {
  "conditions": 1
  }
  }
  , "CurrentObservation": {
    "image": {
    "url":"http://icons.wxug.com/graphics/wu2/logo_130x80.png",
    "title":"Weather Underground",
    "link":"http://www.wunderground.com"
    },
    "display_location": {
    "full":"Bath, United Kingdom",
    "city":"Bath",
    "state":"WIL",
    "state_name":"United Kingdom",
    "country":"UK",
    "country_iso3166":"GB",
    "zip":"00000",
    "magic":"58",
    "wmo":"03740",
    "latitude":"51.36999893",
    "longitude":"-2.14000010",
    "elevation":"64.0"
    },
    "observation_location": {
    "full":"Bath, Bath, ",
    "city":"Bath, Bath",
    "state":"",
    "country":"GB",
    "country_iso3166":"GB",
    "latitude":"51.364689",
    "longitude":"-2.129354",
    "elevation":"131 ft"
    },
    "estimated": {
    },
    "station_id":"IBATH2",
    "observation_time":"Last Updated on June 21, 9:00 AM BST",
    "observation_time_rfc822":"Wed, 21 Jun 2017 09:00:16 +0100",
    "observation_epoch":"1498032016",
    "local_time_rfc822":"Wed, 21 Jun 2017 09:09:36 +0100",
    "local_epoch":"1498032576",
    "local_tz_short":"BST",
    "local_tz_long":"Europe/London",
    "local_tz_offset":"+0100",
    "weather":"Clear",
    "temperature_string":"70.6 F (21.4 C)",
    "temp_f":70.6,
    "temp_c":21.4,
    "relative_humidity":"82%",
    "wind_string":"From the ENE at 2.3 MPH Gusting to 6.0 MPH",
    "wind_dir":"ENE",
    "wind_degrees":71,
    "wind_mph":2.3,
    "wind_gust_mph":"6.0",
    "wind_kph":3.7,
    "wind_gust_kph":"9.7",
    "pressure_mb":"1016",
    "pressure_in":"30.01",
    "pressure_trend":"-",
    "dewpoint_string":"65 F (18 C)",
    "dewpoint_f":65,
    "dewpoint_c":18,
    "heat_index_string":"NA",
    "heat_index_f":"NA",
    "heat_index_c":"NA",
    "windchill_string":"NA",
    "windchill_f":"NA",
    "windchill_c":"NA",
    "feelslike_string":"70.6 F (21.4 C)",
    "feelslike_f":"70.6",
    "feelslike_c":"21.4",
    "visibility_mi":"7",
    "visibility_km":"11",
    "solarradiation":"--",
    "UV":"-1","precip_1hr_string":"0.00 in ( 0 mm)",
    "precip_1hr_in":"0.00",
    "precip_1hr_metric":" 0",
    "precip_today_string":"0.00 in (0 mm)",
    "precip_today_in":"0.00",
    "precip_today_metric":"0",
    "soil_temp_f": "-88.0",
    "soil_moisture": "1.0",
    "leaf_wetness": "4.0",
    "icon":"clear",
    "icon_url":"http://icons.wxug.com/i/c/k/clear.gif",
    "forecast_url":"http://www.wunderground.com/global/stations/03740.html",
    "history_url":"http://www.wunderground.com/weatherstation/WXDailyHistory.asp?ID=IMELKSHA2",
    "ob_url":"http://www.wunderground.com/cgi-bin/findweather/getForecast?query=51.364689,-2.129354",
    "nowcast":""
  }
}
*/

void msOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  //See http://www.cplusplus.com/reference/ctime/strftime/
  time_str = asctime(&timeinfo); // Displays: Sat Jun 24 14:05:49 2017
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_CENTER); // The coordinates define the center of the screen!
/*
  if (time_str.substring(0,3) =="Sun") display->drawString(64,0,"Sunday");
  if (time_str.substring(0,3) =="Mon") display->drawString(64,0,"Monday");
  if (time_str.substring(0,3) =="Tue") display->drawString(64,0,"Tuesday");
  if (time_str.substring(0,3) =="Wed") display->drawString(64,0,"Wednesday");
  if (time_str.substring(0,3) =="Thu") display->drawString(64,0,"Thursday");
  if (time_str.substring(0,3) =="Fri") display->drawString(64,0,"Friday");
  if (time_str.substring(0,3) =="Sat") display->drawString(64,0,"Saturday");
  display->drawLine(0, 12, DISPLAY_WIDTH-1, 12);
*/
  display->drawString(18,53,time_str.substring(4,10));
  display->drawString(107,53,time_str.substring(11,19));
}

void drawFrame1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  //See http://www.cplusplus.com/reference/ctime/strftime/
  time_str = asctime(&timeinfo); // Displays: Sat Jun 24 14:05:49 2017
  display->setTextAlignment(TEXT_ALIGN_CENTER); // The coordinates define the center of the screen!
  display->drawString(x+64,y+25,time_str.substring(0,16));
  display->drawString(x+64,y+35,time_str.substring(20));
  display->setFont(ArialMT_Plain_10);
}

void drawFrame2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
    display->setFont(ArialMT_Plain_16);
  if(!CurrCondObj(&currCondString)) {
    display->drawString(0, 0, "No observation");
    return;
  }

    String icon = getWeatherIcon();
    // The icons are drawn within a 40x20 box, each position can be slighly diiferent depending on icon shape
	const char * icon_img = NULL;

    if (icon == "snow" ||
        icon == "chanceflurries" ||
        icon == "chancesnow" ||
        icon == "flurries" ||
        icon == "nt_chanceflurries" ||
        icon == "nt_chancesnow" ||
        icon == "nt_flurries" || 
        icon == "nt_snow")
		icon_img = snow_bits;

    if (icon == "rain" ||
        icon == "chancerain" ||
        icon == "nt_chancerain" ||
        icon == "nt_rain")
		icon_img = rain_bits;

    if (icon == "sleet" ||
        icon == "chancesleet" ||
        icon == "nt_chancesleet" ||
        icon == "nt_sleet")
		icon_img = sleet_bits;

    if (icon == "sunny" || 
        icon == "clear")
		icon_img = sunny_bits;

    if (icon == "cloudy" ||
        icon == "nt_cloudy")
		icon_img = cloudy_bits;

    if (icon == "mostlysunny" ||
        icon == "partlycloudy" ||
        icon == "partlysunny" ||
        icon == "mostlycloudy")
		icon_img = mostlysunny_bits;

    if (icon == "fog" || 
        icon == "nt_fog" ||
        icon == "nt_hazy" ||
        icon == "hazy")
		icon_img = fog_bits;

    if (icon == "tstorms" ||
        icon == "chancetstorms" ||
        icon == "nt_tstorms" || 
        icon == "nt_chancetstorms")
		icon_img = tstorms_bits;

    if (icon == "nt_mostlycloudy" ||
        icon == "nt_mostlysunny" || 
        icon == "nt_partlycloudy" ||
        icon == "nt_partlysunny")
		icon_img = nt_mostlycloudy_bits;

    if (icon == "nt_clear")
		icon_img = nt_clear_bits;

	if (icon_img)
		display->drawXbm(x+0,y+2, icon_width, icon_height, icon_img);

    //display->drawRect(0,23,40,33); // Icon alignment rectangle
    display->setTextAlignment(TEXT_ALIGN_RIGHT);
    //display->drawString(x+128,y+4,getLocalTime().substring(0,12)+getLocalTime().substring(14,22)); 
    display->drawString(x+128,y+8,getCurrWeather());
    display->drawString(x+128,y+30,getCurrC()+"°C / "+getRelHum()); // use getCurrF for fahrenheit
}

String display_Ptrend(String indicator){
  if (indicator == "+")return "+";
   else if (indicator == "-") return "-";
     else return " ";
}

void
drawFrame3(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
	display->setTextAlignment(TEXT_ALIGN_CENTER);
	if(!CurrCondObj(&currCondString))
	{
		display->setTextAlignment(TEXT_ALIGN_CENTER);
		display->drawString(x+0,y+0, "No connection");
		return;
	}

	display->setFont(ArialMT_Plain_10);
	display->setTextAlignment(TEXT_ALIGN_LEFT);
	display->drawString(x, y+0, "Pressure");
	display->drawString(x, y+18, "Dewpoint");
	display->drawString(x, y+36, "Wind");

	display->setFont(ArialMT_Plain_16);
	display->setTextAlignment(TEXT_ALIGN_RIGHT);
	display->drawString(x+128-32,y+0,getPressure_mb());
	display->drawString(x+128-32,y+18,getDewPointC());
	display->drawString(x+128,y+36,getWindDir());

	display->setTextAlignment(TEXT_ALIGN_LEFT);
	display->drawString(x+128-30,y+0,"mB"+display_Ptrend(getPressure_trend()));
	display->drawString(x+128-30,y+18,"°C");
}

void
drawFrame4(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y)
{
	display->setFont(ArialMT_Plain_10);
	if(!CurrCondObj(&currCondString))
	{
		display->setTextAlignment(TEXT_ALIGN_CENTER);
		display->drawString(x+0,y+0, "No connection");
		return;
	}

	display->setFont(ArialMT_Plain_10);
	display->setTextAlignment(TEXT_ALIGN_LEFT);
	display->drawString(x+0,y+0,"Visibility");
	display->drawString(x+0,y+18,"Rain");
	display->drawString(x+0,y+36,"Wind");

	display->setFont(ArialMT_Plain_16);
	display->setTextAlignment(TEXT_ALIGN_RIGHT);
	display->drawString(x+128-34,y+0, getVisibility_km());
	display->drawString(x+128-34,y+18, getPrecipTodayMet());
	display->drawString(x+128-34,y+36, getWindKPH());

	display->setTextAlignment(TEXT_ALIGN_LEFT);
	display->drawString(x+128-32,y+0,"km");
	display->drawString(x+128-32,y+18,"mm");
	display->drawString(x+128-32,y+36,"km/h");
}

// This array keeps function pointers to all frames frames are the single views that slide in
FrameCallback frames[] = { drawFrame2, drawFrame3, drawFrame4 };

// how many frames are there?
const int frameCount = sizeof(frames) / sizeof(*frames);

// Overlays are statically drawn on top of a frame eg. a clock
OverlayCallback overlays[] = { msOverlay };
const int overlaysCount = sizeof(overlays) / sizeof(*overlays);

int Start_WiFi(const char* ssid, const char* password){
 int connAttempts = 0;
 Serial.println("\r\nConnecting to: "+String(ssid));
 WiFi.begin(ssid, password);
 while (WiFi.status() != WL_CONNECTED ) {
   delay(500);
   Serial.print(".");
   if(connAttempts > 20) return -5;
   connAttempts++;
 }
 Serial.println("WiFi connected\r\n");
 Serial.print("IP address: ");
 Serial.println(WiFi.localIP());
 return 1;
}
 
int Call_API(String* resultString) {
  client.stop();  // Clear any current connections
  Serial.println("Connecting to "+String(host)); // start a new connection
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
   Serial.println("Connection failed");
   return false;
  }
  // Weather Underground API address http://api.wunderground.com/api/YOUR_API_KEY/conditions/q/YOUR_STATE/YOUR_CITY.json
  String url = "http://api.wunderground.com/api/"+apikey+"/conditions/q/"+country+"/"+city+".json";
  Serial.println("Requesting URL: "+String(url));
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
  "Host: " + host + "\r\n" +
  "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Connection Timeout...Stopping");
      client.stop();
      return false;
    }
  }
  Serial.print("Receiving API weather data");
  while(client.available()) {
    *(resultString) = client.readStringUntil('\r');
    Serial.print(".");
  }
  Serial.println("\r\nClosing connection");
  return true;
}

void setup() { 
  pinMode(16,OUTPUT);
  digitalWrite(16, LOW);    // set GPIO16 low to reset OLED
  delay(50); 
  digitalWrite(16, HIGH); // while OLED is running, must set GPIO16 in high

  Serial.begin(115200);
    // The ESP is capable of rendering 60fps in 80Mhz mode but that won't give you much time for anything else run it in 160Mhz mode or just set it to 30 fps
  ui.setTargetFPS(30);
  // Customize the active and inactive symbol
//  ui.setActiveSymbol(activeSymbol);
//  ui.setInactiveSymbol(inactiveSymbol);
  // You can change this to TOP, LEFT, BOTTOM, RIGHT
  ui.setIndicatorPosition(BOTTOM);
  // Defines where the first frame is located in the bar.
  ui.setIndicatorDirection(LEFT_RIGHT);
  // You can change the transition that is used SLIDE_LEFT, SLIDE_RIGHT, SLIDE_UP, SLIDE_DOWN
  ui.setFrameAnimation(SLIDE_LEFT);
  // Add frames
  ui.setFrames(frames, frameCount);
  // Add overlays
  ui.setOverlays(overlays, overlaysCount);
  // Initialising the UI will init the display too.
  ui.init();
  display.flipScreenVertically();
  Start_WiFi(ssid,password);
  configTime(1, 3600, "pool.ntp.org");
  lastConnectionTime = millis();
  Call_API(&currCondString);      // Get data with an API call and place response in a String
}
 
void loop() {
  int remainingTimeBudget = ui.update();
  if (remainingTimeBudget > 0) {
    // You can do some work here
    // Don't do stuff if you are below your
    // time budget.
    if (millis() - lastConnectionTime > postingInterval) {
      Call_API(&currCondString);      // Get data with an API call and place response in a String
      Serial.println(currCondString); // Display the response
      lastConnectionTime = millis();
    }
    delay(remainingTimeBudget);
  }
}
