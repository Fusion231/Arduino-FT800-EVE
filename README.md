# saunaarduino
Sauna with ft800 EVE LCD, thermometer, humidity, pressure and CO2 sensor. 
<h1>Software</h1>
I used Visual Studio 2019 to create this project.
<h1>2 type led strip: </h1>
* First normal white led with brightness control throught system. <br>
* Secound RGB Led Strip with brightness control throught system and option change color.
<h1>Audio System</h1>
In project i connect Radio TEA5767 to arduino board and to Audio amplifier module through the JACK cable.
<h1>In project i used:</h1>
* FM radio STEREO TEA5767<br>
* Relay 4x<br>
* Speaker 8 Ohm<br>
* Audio amplifier module 2x50W XH-M189<br>
* Power Supply 12V 36W 3A ULTRA SLIM<br>
* Power Supply 24V 8A<br>
* Arduino Mega<br>
* Resistor 1K<br>
* Transistor 3x TIP122 to RGB Led strip<br>
* Transistor 1x IRF840 to white LED strip
* BME280 Evironmental Sensor
<h1>Directory in Project</h1>
* SPI.h<br>
* Wire.h<br>
* FT_VM800P43_50.h<br>
* Adafruit_Sensor.h<br>
* Adafruit_BME280.h<br>
* Timers.h<br>
* TEA5767.h
