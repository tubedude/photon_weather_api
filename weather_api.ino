
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
LEDStatus weatherThunderstorm(RGB_COLOR_RED, LED_PATTERN_FADE); //3


int updateweatherhour = -1; // Hour of the last update
bool weatherGood;
String condition = "";
int chanceRain;
int weatherStatus = 0;
const char *CLEAR_WEATHER[] = {"clear", "sunny", "partly cloudy"};
int sizeofClearWeather = 3;

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


void defineWeatherStatus(){
  // reset All Status.
  weatherClear.setActive(false);
  weatherCloudy.setActive(false);
  weatherRain.setActive(false);
  weatherThunderstorm.setActive(false);
  weatherStatus = 0;


  // Odd weatherStatus is odd if it is going to rain.
  if(chanceRain <= 50) {
    weatherStatus = 0;
  } else {
    weatherStatus = 1;
  }

  // Debug print
  Serial.print("     Switching for weather. Array size of: ");
  Serial.println(sizeofClearWeather);

  // Search through Clear Weather Array.
  for(int i = 0; i < sizeofClearWeather; i++) {
    Serial.print(".");
    if(condition.indexOf(CLEAR_WEATHER[i]) >= 0) {
      weatherStatus =+ 2;

      Serial.print(" Found a Clear Condition: ");
      Serial.println(CLEAR_WEATHER[i]);
    }
  }

  if(condition.indexOf("thunder") >= 0) {
      weatherStatus =+ 4;
      Serial.print(" Found a Thunderstorm Weather.");
  }


  Serial.print("      WeatherStatus: ");
  Serial.println(weatherStatus);

  switch (weatherStatus){
    case 0:
      weatherCloudy.setActive(true);
      break;
    case 1:
      weatherRain.setActive(true);
      break;
    case 2:
      weatherClear.setActive(true);
      break;
    case 3:
      weatherRain.setActive(true);
      break;
    case 4:
      weatherRain.setActive(true);
      break;
    case 5:
      weatherThunderstorm.setActive(true);
      break;
    default:
      Serial.print("No case found for: ");
      Serial.println(weatherStatus);
  }
}





void setup() {
    Time.zone (CURRENT_TZ);
    Particle.variable("hourUpdate", updateweatherhour);
    Particle.variable("condition", condition);
    Particle.variable("chanceRain", chanceRain);
    Particle.variable("wStatus", weatherStatus);
    Particle.subscribe(HOOK_RESP, gotweatherData, MY_DEVICES);

    Spark.syncTime();

}


void loop() {
    if (Time.hour() != updateweatherhour){
      Serial.println("Time to get new data...");
      getWeather();
      Serial.println("Running defineWeatherStatus...");
      defineWeatherStatus();
    }
}
