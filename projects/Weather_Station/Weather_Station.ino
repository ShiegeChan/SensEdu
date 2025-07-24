#include <Dps3xx.h>
/**
 * @details This example reads temperature and pressure values and estimates
 *          weather conditions based on equivalent sea level pressure
 */

/* -------------------------------------------------------------------------- */
/*                                User Settings                               */
/* -------------------------------------------------------------------------- */

#define NORMAL_PRESSURE           1009            // in hPa
#define HIGH_PRESSURE             1022            // in hPa
#define T_OFFSET                  0               // Temperature offset for accurate temperature display

/* -------------------------------------------------------------------------- */
/*                                 Parameters                                 */
/* -------------------------------------------------------------------------- */

float altitude = 0;             // Altitude in meters (user-defined)
float temperature;
float pressure;
float seaLevelPressure;
float humidity;
int16_t ret;

// Dps3xx Object
Dps3xx Dps3xxPressureSensor = Dps3xx();
uint8_t oversampling = 5;       /* Value from 0 to 7, the Dps 3xx will perform 2^oversampling internal
    temperature measurements and combine them to one result with higher precision
    measurements with higher precision take more time, consult datasheet for more information */

/* -------------------------------------------------------------------------- */
/*                                 Functions                                  */
/* -------------------------------------------------------------------------- */

// Function to calculate sea-level pressure
float calculateSeaLevelPressure(float p, float t) {
  return p * pow((1 - (0.0065 * altitude) / (t + 273.15)), -5.257);
}

//TODO : Function to predict weather based on sea-level pressure evolution

/* This function is not implemented correctly yet as it only predicts weather based on
instant measurement. It should measure the rate of change in pressure over time
(e.g. in hPa/hour) to predict weather. The rate should be measured by comparing an
initial pressure measurement with the current measurement.
This function should also include a humidity parameter from the humidity sensor to
help predict weather more accurately*/

void WeatherPredict(float seaLevelPressure) {
  if (seaLevelPressure < NORMAL_PRESSURE * 100) {
    Serial.println("Weather Status: possible rain or storm :(");
  }
  else if (seaLevelPressure < HIGH_PRESSURE * 100) {
    Serial.println("Weather Status: Fair weather");
  } 
  else {
    Serial.println("Weather Status: clear sky, calm weather");
  }
}

/* -------------------------------------------------------------------------- */
/*                                   Setup                                    */
/* -------------------------------------------------------------------------- */
void setup()
{
  Serial.begin(9600);
  while (!Serial); // Wait for Serial Monitor to connect

  Dps3xxPressureSensor.begin(Wire1);      /* Call begin to initialize Dps3xxPressureSensor
  using the default 0x77 bus adress of the sensor. The default adress
  does not need to be specified. */
   
  //Dps3xxPressureSensor.begin(Wire1, 0x76);   
  //Use the above line instead to use the secondary I2C address 0x76.
  //In this case a jumper has to be added on SensEdu between J19 and LOW to pull SDO pin to GND.

  //TODO : Initilize SHT35 humidity sensor with Wire1
  //I2C adress by default is 0x44 (logic low)
   
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

  //TODO : Function to read humidity sensor data
  
  WeatherPredict(seaLevelPressure);

  // Wait some time
  delay(300000);
}
