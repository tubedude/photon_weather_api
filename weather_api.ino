/*
Display the weather condition using a Particle.io Photon and Weather Underground data.
Checks for hourly updates from Weather Underground and lights the led accordantly.
*/

#define HOOK_RESP	"hook-response/weatherU_hook"
#define HOOK_PUB	"weatherU_hook"
#define CURRENT_TZ -3


// Defines LEDStatus for each weather condition
LEDStatus weatherClear(RGB_COLOR_BLUE, LED_PATTERN_FADE); //0
LEDStatus weatherCloudy(RGB_COLOR_WHITE, LED_PATTERN_FADE); //1
LEDStatus weatherRain(RGB_COLOR_YELLOW, LED_PATTERN_FADE); //2
LEDStatus weatherThunderstorm(RGB_COLOR_YELLOW, LED_PATTERN_BLINK); //3


int updateweatherhour = -1; // Hour of the last update
bool weatherGood;
String condition = "";
int chanceRain;

//Updates Weather Forecast Data
void getWeather() {
  Serial.println("in getWeather");
  weatherGood = false;
  int badWeatherCall = 0 ;

	// publish the event that will trigger our weather webhook
  Particle.publish(HOOK_PUB);

  unsigned long wait = millis();
  //wait for subscribe to kick in or 5 secs
  while (!weatherGood && (millis() < wait + 5000UL))
    //Tells the Photon to check for incoming messages from particle cloud
    Particle.process();
  if (!weatherGood) {
    Serial.println("  Weather update failed");
    badWeatherCall++;
    if (badWeatherCall > 2) {
      //If 3 webhook call fail in a row, Print fail
      Serial.println("  Webhook Weathercall failed!");
    }
  }
  else
      Serial.println("  Webhook Weathercall succesfully updated!");
}//End of getWeather function


/*
Parse a string according to a separator and returns the partial string according to an index.
Source: http://stackoverflow.com/a/14824108/2317895
*/
String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

// Iterates the result to get the actual forecast
// as it might have an issue with the dates
String actualForecast(String str, int forecastIndex = 0) {
    Serial.println("    Finding appropriate forecast...");
    // Serial.print("Raw info: ");
    // Serial.println(str);

    String tempForecast = getValue(str, '@', forecastIndex);

    int day = atoi(getValue(tempForecast, '~', 0));

    if(day == Time.day()) {
        return tempForecast;
    } else {
        forecastIndex++;
        return actualForecast(str, forecastIndex );
    }
}


// Webhook callback
// splits the data and gets the appropriate value
void gotweatherData(const char *name, const char *data) {
    // {{date.day}}~{{date.hour}}~{{conditions}}~{{pop}}@
    Serial.println("  Running gotweatherData function");

    String str = String(data);
    String forecast = actualForecast(str);
    Serial.print("  Got a forecast: ");
    Serial.println(forecast);

    int day = atoi(getValue(forecast, '~', 0));
    int hour = atoi(getValue(forecast, '~', 1));

    condition = getValue(forecast, '~', 2);
    condition.toLowerCase();

    chanceRain = atoi(getValue(forecast, '~', 3));
    weatherGood = true;
    updateweatherhour = Time.hour();
}


void setup() {
    Time.zone (CURRENT_TZ);
    Particle.variable("hourUpdate", updateweatherhour);
    Particle.variable("condition", condition);
    Particle.variable("chanceRain", chanceRain);
    Particle.subscribe(HOOK_RESP, gotweatherData, MY_DEVICES);

    Spark.syncTime();
    Particle.process();
    delay (1000);
}

void defineWeatherStatus(){
    weatherClear.setActive(false);
    weatherCloudy.setActive(false);
    weatherRain.setActive(false);
    weatherThunderstorm.setActive(false);
    if(chanceRain <= 50) {
        if(condition.indexOf("clear") > 0 || condition.indexOf("sunny") > 0) {
            weatherClear.setActive(true); //0
        } else {
            weatherCloudy.setActive(true); //1
        }
    } else {
        if(condition.indexOf("thunder") > 0 ) {
            weatherThunderstorm.setActive(true); //3
        } else {
            weatherRain.setActive(true); //2
        }
    }
}

void loop() {
    if (Time.hour() != updateweatherhour){
        getWeather();
        defineWeatherStatus();
    }
}
