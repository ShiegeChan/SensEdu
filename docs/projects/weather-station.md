---
title: Weather Station
layout: default
math: mathjax
parent: Projects
nav_order: 6
---

# Weather Station
{: .no_toc .fs-8 .fw-500}
---

- TOC
{:toc}

## Introduction
<img src="{{site.baseurl}}/assets/images/Weather_forecasting.jpg"/>

[Weather prediction or weather forecasting](https://en.wikipedia.org/wiki/Weather_forecasting) is the application of science and technology to predict the condition of the atmosphere for a particular location and time. People have been trying to predict the weather for years, since it highely effects our lives. Albeit predicting the weather became much more precise and correct, it remains a difficult task, especially when predicting far in the future. This is due to the chaotic nature of the atmosphere and many different parameters that need to be taken into account. This nonlinearity requires a massive computational power to solve such equations, and the more difference between the current time and the time for which the forecast is being made increases, the less accurate forecasts are. In simple terms, the initial conditions become less accurate, and the fact that we don't fully understand the atmosphere, thus we can never incorporate all the different parameters that affect the weather.

There are many different models that are trained repeatedly in order to improve the prediction. In SensEdu, we demonstrate the most simple way of weather prediction based on pressure, temperature and humidity. We use sensor readings of the barometric pressure sensor, as well as humidity sensor that can be added to the board, and predict the weather according to a simple rule-based model. Not all parameters taken into account for predicting the weather are equally important, some of them carry more useful information than others. Therefore, knowing just a few parameters can lead to good weather predictions, especially real time predictions. 

## Science behind

Atmospheric pressure, temperature and humidity are the main parameters for the numerical weather prediction. Their values, rate of changes of these values, and interaction between the three parameters play a crucial role and influence predictions massively. 

### Temperature
**Temperature** plays a crucial role in weather prediction as it directly influences the atmospheric dynamics, moisture, pressure systems and other meteorological phenomena. Any kind of weather model relies analysis of temperature patterns to define the state of the atmosphere. It drives the motion of air and the formation of weather systems due to its effects on pressure gradients. Higer temperature result in warmer air, while lower temperatures result in colder air. Heat energy from the sun warms the surface of the Earth differently across regions, which create temperature gradients. This then helps to forecast sea breezes, storms and movement of air. Temperature measured in a vertical profile is also important since it determines the stability of the atmosphere and if the conditions pull more towards thunderstorms or cloud formation. Temperature trends are highly essential for seasonal and-long term forecasting. 

### Humidity
**Humidity** is a measure of amount of water vapor in the air, gaseous form of water. Humidity is another crucial factor for atmospheric processes, such as cloud formation, precipitation, and temperature regulation. It affects the amount of water vapor in atmosphere. Three main terms to be aware of when it comes to humidity is: 

* **Absolute humidity** - measures the total amount of water vapor in a given air volume
* **Relative humidity** - indicates how much more water vapor the air currently holds compared to the maximum amount it could hold at a specific temperature
* **Dew point** - the temperature at which air becomes saturated and condensation begins

Humidity affects cloud formation the most. If it is high, the possibility of cloud formation is higher, increasing the likelihood of precipitation. 

### Atmospheric pressure
[**Atmospheric or air pressure**](https://iere.org/how-does-the-air-pressure-affect-the-weather/) is the force exerted by the weight of air above a given point. As everything else regarding weather, the pressure is also not uniform accross the Earth, and these differences kickstart every atmospheric movement. It is important to understand the difference between **high-pressure** (anticyclones) and **low-pressure** (cyclones) system in order to understand the weather conditions. High air pressure indicates that air is descending. As it sinks, it gets warmer and dryer, preventing forming of clouds and leading to clear skies and pleasent weather with calm winds. Low air pressure indicates ascending air which is getting colder with height leading to condensation, cloud formation and precipitation. 

The difference in air pressure between different locations creates a pressure gradient force, which drives air from areas of high pressure towards the area of low pressure. This movement is actually wind. The greater the difference between two points, the stronger the winds. 

Standard atmospheric pressure at sea level is approximately 1013.25 hPa, 29.92 inches of mercury (inHg) and it serves as a reference point in measuring deviations in air pressure. Another important term is **sea level air pressure** which represents the pressure adjusted to mean sea level. This allows for a more accurate comparison of pressure readings from different locations, regardless of their alitiude. 

Based on these three values over time, a simple way (yet not very accurate) to predict the weather is using rule-based model - depending on the range of values of different parameters, the model decides if the weather is clear, cloudy or rainy. If no condition is satisfied, the weather is classified as uncertain. The function describing the model is shown next.

```c
void WeatherPredict(float temp, float pressure, float humidity) {
    // temp -> in degreees 
    // pressure -> in hPa (hectopascal is 100 pascals)
    // humidity -> in % 

    if (humidity > 70 && pressure < 1000 && (15 < temp < 25)) {
        Serial.println("Weather Status: Rain"); 
    }
    else if ((50 <= humidity <= 70) && pressure < 1000) {
        Serial.println("Weather Status: Cloudy");
    } 
    else if (pressure > 1020 && humidity < 50 && temp > 20) {
        Serial.println("Weather Status: Clear");
    } 
    else {
        Serial.println("Weather Status: Uncertain");
    }
}
```
## Hardware setup

SensEdu shield has integrated barometric pressure sensor that can measure both pressure and temperature. For humidity measurement, we use a third party sensor SHT35 from Sensirion on a custom pcb adapter board for SensEdu. The full list of components includes:

* Arduino GIGA R1 
* SensEdu shield
* SensEdu platform for humidity sensor
* USB-c cable 

{: .NOTE}
Other humidity sensors that are compatible with Arduino GIGA platform can be used as well. In that case, the code would have to be modified accordingly. 

<img src="{{site.baseurl}}/assets/images/sht35.jpeg" width="600"/>
{: .text-center .mb-1}

Humidity sensor PCB adapter for Sensedu board 
{: .text-center .mt-0 .fw-500}

<img src="{{site.baseurl}}/assets/images/sensedu_sht35.jpeg" width="600"/>
{: .text-center .mb-1}

Sensedu board with integrated DPS310 pressure sensor and attached PCB board for humidity sensor is connected to the Arduino GIGA board
{: .text-center .mt-0 .fw-500}

## Software setup 

### Communication and Data acquisition

Both sensors use I2C communication protocol. I2C protocol allows for one master and multiple slave devices. In this case, the master device is Arduino powered with STM32 microcontroller and the slave devices are two mentioned sensors. Each sensor has a unique address with which they are accessed. The communication is done in the background using **Arduino "Wire.h" library**. 

Barometric pressure sensor has a default address of 0x77 (pull-up configuration), and it can be changed to 0x76 with the pull-down configuration. Pull-up configuration refers to SDO pin being pulled-up to VDD or keeping it floating, while pull-down configuration refers to pulling-down the SDO pin to GND. This is done by changing the position of the jumper J19 on Sensedu board. 

Similarly, the default address for SHT35 humidity sensor on PCB costumized for Sensedu is 0x44 since ADDR pin is connected to logic low (GND) and cannot be changed. If it would be connected to logic high, the I2C address would be 0x45. 

{: .NOTE}
Since both sensors are connected to SDA1 and SCL1 pins, when using the Wire.h library functions, Wire1.begin() has to be used for I2C1 and Wire2.begin() for I2C2. More information about this can be found in the library   [ documentation webpage](https://docs.arduino.cc/language-reference/en/functions/communication/wire/). 

Moreover, to interact with SHT35 sensor, we use third party (developed by the company) Arduino library that can be found on [GitHub](https://github.com/Sensirion/arduino-sht/tree/master) and it is free for download. For the barometric pressure sensor, DPS310, we also use Arduino library (developed by Infineon) available on [GitHub](https://github.com/Infineon/arduino-xensiv-dps3xx/tree/master). 

In the setup phase, both sensors are initialized and communication is established with both sensors. 

```c
Dps3xx dps_sensor = Dps3xx(); // initilize dps sensor
...
SHTSensor sht_sensor(SHTSensor::SHT3X); // initialize sht sensor

void setup() {
    Wire1.begin(); // establish communication
    Serial.begin(9600);
    while (!Serial); // wait for Serial Monitor to connect

    // start communication with both sensors
    dps_sensor.begin(Wire1);
    sht_sensor.init(Wire1); 
    ...
}
```

Both sensors are capable of measuring the temperature. Since the pressure sensor on the board is near the power supply, we will use temperature measurements from the humidity sensor. 

```c
    // ----------------------------- pressure measurement -------------------------

    ret = dps_sensor.measurePressureOnce(pressure, oversampling);
    if (ret != 0) {
        // Something went wrong.
        // Look at the library code for more information about return codes
        Serial.print("FAIL! ret = ");
        Serial.println(ret);
    }
    else {
        Serial.print("Pressure: ");
        Serial.print(pressure/100);
        Serial.println(" hPa");
    }
```


```c
    // -------- humidity & temperature measurement ------------------------------

    if (sht_sensor.readSample()) {
        humidity = sht_sensor.getHumidity();
        Serial.print("Humidity level: ");
        Serial.println(humidity, 2);
        Serial.print("Temperature (sht)");
        temperature = sht_sensor.getTemperature();
        Serial.println(temperature, 2);
    } 
    else {
        Serial.println("SHT sensor Error\n");
    }
```


## Showcase

