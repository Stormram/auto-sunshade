#include <ESP8266WiFi.h>

// Not giving these ^^
#include "keys.h"

#define FLOATCHARS "01234567890."
#define TEMPERATURE_THRESHOLD 18.0f
#define WIND_THRESHOLD 25.0f
#define FAIL_FLOAT 200.0f
#define RAIN_CHANCE_THRESHOLD 30.0f

#define UP_GPIO 14
#define DOWN_GPIO 12

/**
 * {locationKey} as last param of url
 * GET: "http://dataservice.accuweather.com/forecasts/v1/hourly/1hour/{location_id}"
 * required: apiKey
 * optional: language, details, metric
 */
#define FORECAST_API_URL String("http://dataservice.accuweather.com/forecasts/v1/hourly/1hour/247518?apikey=") + API_KEY + "&details=true&metric=true"

void setup() {
  // Setup pins
  pinMode(UP_GPIO,   OUTPUT);
  pinMode(DOWN_GPIO, OUTPUT);
  
  digitalWrite(UP_GPIO, LOW);
  digitalWrite(UP_GPIO, LOW);

  // Serial for debugging :D
  Serial.begin(115200);
  delay(10);

  // We start by connecting to a WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  connectWiFi(WIFI_SSID, WIFI_KEY);
}

void loop() {
  Serial.println("Starting program");
  /*
     Get request example: https://raw.githubusercontent.com/esp8266/Arduino/master/libraries/ESP8266WiFi/examples/WiFiClient/WiFiClient.ino
  */
  delay(10);

  const char * host = "dataservice.accuweather.com";
  Serial.print("connecting to ");
  Serial.println(host);

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }

  Serial.print("Requesting URL: ");
  Serial.println(FORECAST_API_URL);

  // This will send the request to the server
  client.print(String("GET ") + FORECAST_API_URL + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }

  String line;
  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    line = client.readStringUntil('\r');
    Serial.print(line);
  }
  Serial.println();
  
  float temperature = getValueFromJson(line, "Temperature");
  float mmRain = getValueFromJson(line, "TotalLiquid");
  float windSpeed = getValueFromJson(line, "Wind");
  bool sunLight = getPosition(line, "true", 0) || 0;
  float rainChance = getValueFromJsonSimple(line, "RainProbability");
  
  printVar("temp: ", temperature);
  printVar("wind: ", windSpeed);
  printVar("rain: ", mmRain);
  printVar("rain %", rainChance);
  printVar("isDay: ", sunLight);

  if (temperature == FAIL_FLOAT || mmRain == FAIL_FLOAT || rainChance == FAIL_FLOAT) {
    Serial.println("Parsing failed, closing shades as best effort");
    closeShades();
    ESP.deepSleep(15*60*1e6); // once per 15 minutes
    return;
  }

  // Close shades when there is not sunLight, there might be rain or
  // the temperature is to low
  if (!sunLight || mmRain >= 0.1f || temperature <= TEMPERATURE_THRESHOLD || windSpeed >= WIND_THRESHOLD || rainChance >= RAIN_CHANCE_THRESHOLD) {
    closeShades();
  }
  else {
    openShades();
  }

  Serial.println("Good night, sleep tight, don't let the bedbugs bite.");
  ESP.deepSleep(30*60*1e6); // once per half hour
}

void connectWiFi(const char* ssid, const char* key) {
  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  delay(10);
  WiFi.begin(ssid, key);

  unsigned long timeout = millis();
  while (WiFi.status() != WL_CONNECTED) {
    // Timeout after 10 seconds
    if (millis() - timeout > 10000) {
      // Don't know what went wrong
      // Sometimes this part just hangs forever.
      Serial.println("Hanging, resetting");
      ESP.restart();
    }

    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void FakeDeepSleep(unsigned long duration) {
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  delay(duration);
  ESP.restart();
}

void closeShades() {
  Serial.println("Closing shades");
  digitalWrite(UP_GPIO, HIGH);
  delay(1000);
  digitalWrite(UP_GPIO, LOW);
}

void openShades() {
  Serial.println("Opening shades");
  digitalWrite(DOWN_GPIO, HIGH);
  delay(1000);
  digitalWrite(DOWN_GPIO, LOW);
}

float getValueFromJson(String json, String needle) {
  // First look for the expected param from position 0
  unsigned int pos = getPosition(json, needle, 0);

  // Failure!
  if (pos == 0) {
    return FAIL_FLOAT;
  }

  // Look for the Value
  pos = getPosition(json, "Value", pos);
  // Failure!
  if (pos == 0) {
    return FAIL_FLOAT;
  }

  // Yay now try parsing a number
  String floatMatch = "";
  for (; pos <= json.length(); pos++) {
    if (charInString(json[pos], FLOATCHARS)) {
      floatMatch += json[pos];
    }
    else if (floatMatch != "") {
      return floatMatch.toFloat();
    }
  }
  return FAIL_FLOAT;
}

float getValueFromJsonSimple(String json, String needle) {
  // First look for the expected param from position 0
  unsigned int pos = getPosition(json, needle, 0);

  // Failure!
  if (pos == 0) {
    return FAIL_FLOAT;
  }

  // Yay now try parsing a number
  String floatMatch = "";
  for (; pos <= json.length(); pos++) {
    if (charInString(json[pos], FLOATCHARS)) {
      floatMatch += json[pos];
    }
    else if (floatMatch != "") {
      return floatMatch.toFloat();
    }
  }
  return FAIL_FLOAT;
}

bool charInString(char needle, String haystack) {
  // Looks if the given char is in the String
  // Returns true if found and false otherwise
  for (unsigned int i = 0; i <= haystack.length(); i++) {
    if (needle == haystack[i]) {
      return true;
    }
  }
  return false;
}

unsigned int getPosition(String in, String needle, unsigned int i) {
  /**
   * Search for the given `needle` in the string
   * returns the last position of the char found in `in`
   * returns 0 if not found
   */
  unsigned int matching = 0;

  for (; i <= in.length(); i++) {
    // Check for matching char
    if (in[i] == needle[matching]) {
      if (++matching >= needle.length()) {
        // w00t total match found
        //Serial.print(needle);
        //Serial.println(" found!");
        return ++i;
      }
    }
    else {
      // Otherwise always reset matching
      matching = 0;
    }
  }

  // Return 0 to indicate failure
  // (position 0 in json won't make sense anyway...)
  return 0;
}

void printVar(String msg, float var) {
  Serial.print(msg);
  Serial.println(var);
}

void printVar(String msg, bool var) {
  Serial.print(msg);
  Serial.println(var);
}

/**
 * Example output:
   [
  {
    "DateTime": "2017-06-08T22:00:00+02:00",
    "EpochDateTime": 1496952000,
    "WeatherIcon": 7,
    "IconPhrase": "Cloudy",
    "IsDaylight": false,
    "Temperature": {
      "Value": 17.5,
      "Unit": "C",
      "UnitType": 17
    },
    "RealFeelTemperature": {
      "Value": 16.5,
      "Unit": "C",
      "UnitType": 17
    },
    "WetBulbTemperature": {
      "Value": 13.7,
      "Unit": "C",
      "UnitType": 17
    },
    "DewPoint": {
      "Value": 10.5,
      "Unit": "C",
      "UnitType": 17
    },
    "Wind": {
      "Speed": {
        "Value": 11.1,
        "Unit": "km/h",
        "UnitType": 7
      },
      "Direction": {
        "Degrees": 224,
        "Localized": "SW",
        "English": "SW"
      }
    },
    "WindGust": {
      "Speed": {
        "Value": 18.5,
        "Unit": "km/h",
        "UnitType": 7
      }
    },
    "RelativeHumidity": 63,
    "Visibility": {
      "Value": 16.1,
      "Unit": "km",
      "UnitType": 6
    },
    "Ceiling": {
      "Value": 579,
      "Unit": "m",
      "UnitType": 5
    },
    "UVIndex": 0,
    "UVIndexText": "Low",
    "PrecipitationProbability": 44,
    "RainProbability": 44,
    "SnowProbability": 0,
    "IceProbability": 0,
    "TotalLiquid": {
      "Value": 0,
      "Unit": "mm",
      "UnitType": 3
    },
    "Rain": {
      "Value": 0,
      "Unit": "mm",
      "UnitType": 3
    },
    "Snow": {
      "Value": 0,
      "Unit": "cm",
      "UnitType": 4
    },
    "Ice": {
      "Value": 0,
      "Unit": "mm",
      "UnitType": 3
    },
    "CloudCover": 93,
    "MobileLink": "http://m.accuweather.com/en/nl/heerenveen/247518/hourly-weather-forecast/247518?day=1&unit=c&lang=en-us",
    "Link": "http://www.accuweather.com/en/nl/heerenveen/247518/hourly-weather-forecast/247518?day=1&hbhhour=22&unit=c&lang=en-us"
  }
  ]
*/

