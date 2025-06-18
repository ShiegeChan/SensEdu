#include <Dps3xx.h>
/**
 * @details This example reads temperature and pressure values and estimates
 *          weather conditions based on equivalent sea level pressure
 */

/* -------------------------------------------------------------------------- */
/*                                User Settings                               */
/* -------------------------------------------------------------------------- */

#define GET_INSIDE                1000            // in hPa
#define VERY_CLOUDY               1005            // in hPa
#define CLOUDY                    1010            // in hPa
#define PARTLY_CLOUDY             1015            // in hPa 
#define ALMOST_CLEAR              1020            // in hPa
#define T_OFFSET                  0               // Temperature offset for accurate temperature display

/* -------------------------------------------------------------------------- */
/*                                 Parameters                                 */
/* -------------------------------------------------------------------------- */

float altitude = 0;             // Altitude in meters (user-defined)
float temperature;
float pressure;
float seaLevelPressure;
int16_t ret;

// Dps3xx Object
Dps3xx Dps3xxPressureSensor = Dps3xx();
uint8_t oversampling = 0;       // Value from 0 to 7

/* -------------------------------------------------------------------------- */
/*                                 Functions                                  */
/* -------------------------------------------------------------------------- */

// Function to calculate sea-level pressure
float calculateSeaLevelPressure(float p, float t) {
  return p * pow((1 - (0.0065 * altitude) / (t + 273.15)), -5.257);
}

// Function to predict weather based on sea-level pressure
void WeatherStat(float seaLevelPressure) {
  if (seaLevelPressure < GET_INSIDE * 100) {
    Serial.println("Weather Status: possible rain or storms :(");
  }
  else if (seaLevelPressure < VERY_CLOUDY * 100) {
    Serial.println("Weather Status: very cloudy :/");
  } 
  else if (seaLevelPressure < CLOUDY * 100) {
    Serial.println("Weather Status: cloudy :|");
  } 
  else if (seaLevelPressure < PARTLY_CLOUDY * 100) {
    Serial.println("Weather Status: partly cloudy");
  }
  else if (seaLevelPressure < ALMOST_CLEAR * 100) {
    Serial.println("Weather Status: almost clear sky");
  } else {
    Serial.println("Weather Status: clear sky :)");
  }
}

/* -------------------------------------------------------------------------- */
/*                                   Setup                                    */
/* -------------------------------------------------------------------------- */
void setup()
{
  Serial.begin(9600);
  while (!Serial); // Wait for Serial Monitor to connect

  /*
   * Call begin to initialize Dps3xxPressureSensor
   * The parameter 0x76 is the bus address. The default address is 0x77 and does not need to be given.
   * Dps3xxPressureSensor.begin(Wire, 0x76);
   * Use the line below instead of the one above to use the default I2C address.
   * if you are using the Pressure 3 click Board, you need 0x76
   */
  Dps3xxPressureSensor.begin(Wire1);

  Serial.println("Init complete!");

  // Ask the user to input the altitude
  Serial.println("Please input the current altitude (in meters):");

  // Wait for user input via Serial Monitor
  while (Serial.available() == 0) {
    // Do nothing, wait for the user to input data
  }

  // Read the user input and convert it to a float
  String input = Serial.readStringUntil('\n');
  altitude = input.toFloat();

  // Confirm the altitude
  Serial.print("Altitude set to: ");
  Serial.print(altitude);
  Serial.println(" meters");
}

/* -------------------------------------------------------------------------- */
/*                                    Loop                                    */
/* -------------------------------------------------------------------------- */
void loop()
{
  Serial.println();

  /*
   * lets the Dps3xx perform a Single temperature measurement with the last (or standard) configuration
   * The result will be written to the parameter temperature
   * ret = Dps3xxPressureSensor.measureTempOnce(temperature);
   * the commented line below does exactly the same as the one above, but you can also config the precision
   * oversampling can be a value from 0 to 7
   * the Dps 3xx will perform 2^oversampling internal temperature measurements and combine them to one result with higher precision
   * measurements with higher precision take more time, consult datasheet for more information
   */
  ret = Dps3xxPressureSensor.measureTempOnce(temperature, oversampling);

  if (ret != 0)
  {
    /*
     * Something went wrong.
     * Look at the library code for more information about return codes
     */
    Serial.print("FAIL! ret = ");
    Serial.println(ret);
  }
  else
  {
    Serial.print("Temperature: ");
    Serial.print(temperature + T_OFFSET);
    Serial.println("°C");
  }

  /*
   * Pressure measurement behaves like temperature measurement
   * ret = Dps3xxPressureSensor.measurePressureOnce(pressure);
   */
  ret = Dps3xxPressureSensor.measurePressureOnce(pressure, oversampling);
  if (ret != 0)
  {
    // Something went wrong.
    // Look at the library code for more information about return codes
    Serial.print("FAIL! ret = ");
    Serial.println(ret);
  }
  else
  {
    Serial.print("Pressure: ");
    Serial.print(pressure / 100);
    Serial.println(" hPa");
  }

  seaLevelPressure = calculateSeaLevelPressure(pressure, temperature);
  {
    Serial.print("Equivalent Sea Level Pressure: ");
    Serial.print(seaLevelPressure / 100);
    Serial.println(" hPa");
  }
  
  WeatherStat(seaLevelPressure);

  // Wait some time
  delay(5000);
}
