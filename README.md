<p align="center">
  <img src="https://ik.imagekit.io/vladysor/SensEdu/SensEdu.png?updatedAt=1737991670936">
</p>

<b>
<p align="center">RADAR and Communication Development Shield for Arduino GIGA R1</p>
</b>

<p align="center">
  <a href="https://github.com/ShiegeChan/SensEdu/releases/">
    <img src="https://img.shields.io/github/v/release/ShiegeChan/Edusense?include_prereleases" alt="latest release" />
  </a>
  <a href="https://github.com/ShiegeChan/SensEdu/commits/main">
    <img src="https://img.shields.io/github/last-commit/ShiegeChan/Edusense" alt="latest commit" />
  </a>
</p>

<b>
<p align="center">
	<a href="https://github.com/ShiegeChan/SensEdu/releases">Download</a> · 
	<a href="https://github.com/ShiegeChan/SensEdu/releases">Getting Started</a> · 
	<a href="https://github.com/ShiegeChan/SensEdu/releases">Example Projects</a> · 
	<a href="https://github.com/ShiegeChan/SensEdu/releases">Documentation</a> · 
	<a href="https://github.com/ShiegeChan/SensEdu/releases">Contribution</a>
</p>
</b>


## Features

* **Custom Shield Design** developed for the <a href="https://docs.arduino.cc/hardware/giga-r1-wifi/?queryID=undefined">Arduino GIGA R1 WiFi</a> or compatible boards with a similar form factor, such as the <a href="https://www.infineon.com/cms/en/product/promopages/AURIX-microcontroller-boards/low-cost-arduino-kits/AURIX-TC275-Schieldbuddy-/">AURIX TC275 Schieldbuddy</a>. This shield extends the board's capabilities with various sensors, specifically for communication development purposes:
  * **4x Infineon MEMS microphones** with extended capabilities into the ultrasonic range, for both receiving and sending ultrasonic pulses. Its ultrasonic receiving characteristic allows for unique detection of ultrasonic frequencies between 20-100 kHz (<a href="https://www.infineon.com/dgdl/Infineon-MEMS_IM70A135UT-ProductBrief-v01_00-EN.pdf?fileId=8ac78c8c7ddc01d7017e4d7af9084967">datasheet</a>).
  * **1x Infineon Barometric Air Pressure Sensor** measures both pressure and temperature, making it ideal for indoor and outdoor navigation, weather stations, drones, and more (<a href="https://www.infineon.com/dgdl/Infineon-DPS310-DataSheet-v01_02-EN.pdf?fileId=5546d462576f34750157750826c42242">datasheet</a>).
  * **2x Ultrasonic Transducers** with center frequency at ~32kHz (<a href="https://www.farnell.com/datasheets/4413630.pdf?_gl=1*1fltz5c*_gcl_au*MTQwMTY3ODgxOC4xNzI2NDc2MDYw">datasheet</a>).
* **Low-Level Library "SensEdu"** designed for the GIGA R1, this library serves as a basic abstraction layer for the MCU's peripherals and provides default configurations for all sensors. It is implemented at the register level with the help of <a href="https://www.st.com/resource/en/reference_manual/rm0399-stm32h745755-and-stm32h747757-advanced-armbased-32bit-mcus-stmicroelectronics.pdf">STM32H747 Reference Manual</a> to ensure simplicity and expandability. Extensive examples are included to demonstrate library functionality.
* **Firmware for example projects** showcase how to use multiple sensors to achieve goals such as ultrasonic distance measurements, FSK-modulated ultrasonic communication, FMCR radar measurements, and more.
* **MATLAB scripts** enable visualization, analysis and processing of data transferred from the MCU to a PC via USB or WiFi.


## Preview

<p align="center" style="margin:0">
  <img src="https://ik.imagekit.io/vladysor/SensEdu/UltraSoundDistanceMeasurements.png?updatedAt=1737991669929" width="49%">
  <img src="https://ik.imagekit.io/vladysor/SensEdu/xcorr.png?updatedAt=1737991668627" width="49%">
  <img src="https://ik.imagekit.io/vladysor/SensEdu/WiFi_comms.png?updatedAt=1737991665342" width="49%">
  <img src="https://ik.imagekit.io/vladysor/SensEdu/ChirpSignalGen.png?updatedAt=1737991666448" width="49%">
  <img src="https://ik.imagekit.io/vladysor/SensEdu/placeholder.png?updatedAt=1737991667556" width="49%">
  <img src="https://ik.imagekit.io/vladysor/SensEdu/placeholder.png?updatedAt=1737991667556" width="49%">
</p>


## Installation

1. Download the latest release version from the <a href="https://github.com/ShiegeChan/SensEdu/releases">Download page</a>.
2. Extract the downloaded files and place the **libraries** folder into: `C:\Users\your_username\Documents\Arduino\`.
3. The **projects** folder can be placed anywhere in your system.
4. Open the <a href="https://www.arduino.cc/en/software">Arduino IDE</a> and install **Arduino Giga R1 board package** via the Boards Manager.

<p align="left" style="margin:0">
  <img src="https://ik.imagekit.io/vladysor/SensEdu/installation_guide.gif?updatedAt=1737991664227">
</p>
<p align="left" style="margin:0">
  <img src="https://ik.imagekit.io/vladysor/SensEdu/boards_manager.png?updatedAt=1737991662410" width="50%">
</p>


## Starting a Project

To get started, you can explore the hardware interface by following the examples provided in the SensEdu library (`Arduino\libraries\SensEdu\examples\`). A detailed description of all functions can be found in the <a href="https://github.com/ShiegeChan/SensEdu/releases">Documentation</a> under the "SensEdu Library" section.

In the `projects/` directory, you will find several complete projects designed for this board, including:
* **Ultrasonic Distance Measurements** *(in progress)*
* **Ultrasonic FSK Communication** *(coming soon)*
* **Chirp Radio Wave Generation** *(coming soon)*
* **Weather Station** *(coming soon)*
* **WiFi MATLAB Communication** *(coming soon)*

Detailed explanations for each project are available in the "Projects" section of the <a href="https://github.com/ShiegeChan/SensEdu/releases">Documentation</a>.


## How to host wiki locally on Windows

For documentation editing it is much easier to host webpage locally on your machine.

0. Administrator rights may be required to install Ruby and its gems.
1. Visit the <a href="https://rubyinstaller.org/downloads/">Ruby installation page</a>. Download the **x64 version with devkit**.
During installtion you will be asked which components to install, press `Enter` for default.
2. Open the terminal with admin rights in `/docs` folder.
3. Install gems with `bundle install` command.
4. Boot the website with `bundle exec jekyll serve`.
You can put optional `--livereload` parameter to automatically reload the website if you make some changes to styles/text etc.
5. Go to the page `localhost:4000` in your browser to see the website.

#### Notes:
* Stop the running website with `Ctrl+C` in the terminal.
* If you modify `_config.yml`, restart the page (even if `--livereload` enabled).

<p align="center" style="margin:0">
  <img src="https://ik.imagekit.io/vladysor/SensEdu/readme_docs.png?updatedAt=1738590979641" width="100%">
</p>


## Support

If you would like to contribute, please open a pull request!
You can also suggest improvements or check already opened <a href="https://github.com/ShiegeChan/SensEdu/issues">issues</a> to help fix bugs or add new features.


## License

* [**GPL-3.0 license**](https://github.com/ShiegeChan/SensEdu/blob/main/LICENSE)
