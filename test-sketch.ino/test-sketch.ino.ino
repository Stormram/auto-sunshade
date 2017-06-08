// Not giving these ^^
#include "keys.h" 
#include <ESP8266WiFi.h>

void setup() {
  // Setup pins 
  // TODO: set pins
  pinMode(2, OUTPUT);

  // Serial for debugging :D
  Serial.begin(115200);
  delay(10);

  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  
  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_KEY);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(2, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);              // wait for a second
  digitalWrite(2, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);              // wait for a second

  /*
   * Get request example: https://raw.githubusercontent.com/esp8266/Arduino/master/libraries/ESP8266WiFi/examples/WiFiClient/WiFiClient.ino
   */
  delay(5000);

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
  
  // Read all the lines of the reply from server and print them to Serial
  while(client.available()){
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }
  
  Serial.println();
  Serial.println("closing connection");
  delay(10000000);
}


/*
 * [
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
