#include "SPI.h"
#include "Wire.h"
#include "FT_VM800P43_50.h"
#include "src/Adafruit_Sensor.h"
#include "src/Adafruit_BME280.h"
#include "Timers.h"
#include <TEA5767.h>
//Termometr
#define SEALEVELPRESSURE_HPA (1013.25)
#define USEIIC 0
#if(USEIIC)
Adafruit_BME280 bme;
#else
#define SPI_SCK 13
#define SPI_MISO 12
#define SPI_MOSI 11
#define SPI_CS 10
#endif
//end
unsigned int oswietlenie_czas1 = 0, oswietlenie_czas2 = 0, red = 0, blue = 0, green = 255, red_czas = 0,green_czas= 65535,blue_czas=0;
//Wartosci
int led = 5, swiatlo_green =6, swiatlo_red = 2, swiatlo_blue = 7;
int RelayPin1 = 44, RelayPin2 = 42, RelayPin3 = 40;
int smokeA0 = A0;
int sensorThres = 400;
bool start_sesja = false, radio_on = true, wzmacniacz = true,warning_led_sesja=false;
String czas_wys = "15";
//END
//Inicjacja
sTagXY sTagxy;
Timer sesja_timer;
TEA5767 Radio;
FT800IMPL_SPI FTImpl(FT_CS_PIN, FT_PDN_PIN, FT_INT_PIN);
Adafruit_BME280 bme(SPI_CS, SPI_MOSI, SPI_MISO, SPI_SCK);
//end
//Radio zmienne
//Variables:
double old_frequency;
double frequency;
int search_mode = 0,side=0;
int search_direction;
unsigned long last_pressed;
unsigned char buf[5];
int stereo;
int signal_level;
double current_freq;
unsigned long current_millis = millis();
int inByte;
int flag = 0;
//end
int16_t BootupConfigure()
{
	uint32_t chipid = 0;
	FTImpl.Init(FT_DISPLAY_RESOLUTION);//configure the display to the WQVGA

	delay(20);//for safer side
	chipid = FTImpl.Read32(FT_ROM_CHIPID);

	/* Identify the chip */
	if (FT800_CHIPID != chipid)
	{
		Serial.print("Error in chip id read ");
		Serial.println(chipid, HEX);
		return 1;
	}

	/* Set the Display & audio pins */
	FTImpl.SetDisplayEnablePin(FT_DISPENABLE_PIN);
	FTImpl.SetAudioEnablePin(FT_AUDIOENABLE_PIN);
	FTImpl.DisplayOn();
	FTImpl.AudioOn();
	return 0;
}
void Calibrate()
{
	
	FTImpl.DLStart();
	FTImpl.ClearColorRGB(64, 64, 64);
	FTImpl.Clear(1, 1, 1);
	FTImpl.ColorRGB(0xff, 0xff, 0xff);
	FTImpl.Cmd_Text((FT_DISPLAYWIDTH / 2), (FT_DISPLAYHEIGHT / 2), 27, FT_OPT_CENTER, "Prosze klikac w kropki.");
	FTImpl.Cmd_Calibrate(0);
	FTImpl.Finish();
}
void AlphaBlend()
{

	/* Change the below string for experimentation */
	const char Display_string[12] = "Hello World";
	uint8_t i;
	int16_t hoffset, voffset;

	/* Display list to display "Hello World" at the centre of display area */
	FTImpl.DLStart();//start the display list. Note DLStart and DLEnd are helper apis, Cmd_DLStart() and Display() can also be utilized.
	FTImpl.ColorRGB(0xFF, 0xFF, 0xFF);//set the color of the string to while color

	/* display list that demonstrates transparency */
	FTImpl.Begin(FT_BITMAPS);
	FTImpl.Vertex2ii(64, 114, 31, 'S');//character G with alpha transparency being 255
	FTImpl.ColorA(128);
	FTImpl.Vertex2ii(94, 114, 31, 'A');//character G with alpha transparency being 128
	FTImpl.ColorA(64);
	FTImpl.Vertex2ii(124, 114, 31, 'U');//character G with alpha transparency being 64
	FTImpl.ColorA(35);
	FTImpl.Vertex2ii(154, 114, 31, 'N');
	FTImpl.ColorA(25);
	FTImpl.Vertex2ii(184, 114, 31, 'A');
	FTImpl.End();//end the bitmap primitive

	/* display list that demonstrates alpha blend functionality - additive blending */
	/* draw 20 points at random locations within second half of the screen */
	FTImpl.ColorRGB(20, 91, 20); // green color for additive blending
	FTImpl.BlendFunc(FT_SRC_ALPHA, FT_ONE);//input is source alpha and destination is whole color
	FTImpl.PointSize(30 * 16);  //set the point radius to be 30 pixels
	FTImpl.Begin(FT_POINTS);
	/* compute the coordinates of the points randomly */
	for (i = 0; i < 20; i++)
	{
		hoffset = FT_DISPLAYWIDTH / 2 + random(FT_DISPLAYWIDTH / 2);
		voffset = random(FT_DISPLAYHEIGHT);
		FTImpl.Vertex2f(hoffset * 16, voffset * 16);
	}
	FTImpl.End();//end the points primitive

	FTImpl.DLEnd();//end the display list
	FTImpl.Finish();//render the display list and wait for the completion of the DL

	delay(4000);

}
void czas_function(int option,int val) {
	switch (option)
	{
	case 15:
		czas_wys += val - 48;
		delay(200);
		break;
	case 17:
		czas_wys.remove(czas_wys.length());
		break;
	default:
		break;
	}
}
void sesja()
{
	bool run = true,ustawienie=false;
	uint32_t ReadWord,touch;
	String blad="";
	while (run)
	{
		FTImpl.GetTagXY(sTagxy);
		FTImpl.DLStart();//start the display list. Note DLStart and DLEnd are helper apis, Cmd_DLStart() and Display() can also be utilized.
		FTImpl.ColorRGB(255, 255, 255);
		ReadWord = FTImpl.Read32(REG_TRACKER);
		switch (ReadWord)
		{
		case 10:
			HelloWorld(1);
			run = false;
		case 13:
			blad = "";
			ustawienie = true;
			break;
		case 12:
			if (czas_wys.length()==0 || czas_wys[0] == '0') {
				blad = "Podaj poprawny czas";
				break;
			}
			blad = "";
			sesja_timer.begin(MINS(czas_wys.toInt()));
			start_sesja = true;
			warning_led_sesja = true;
			HelloWorld(1);
			break;
		case 48:
			czas_function(15, ReadWord);
			break;
		case 49:
			czas_function(15, ReadWord);
			break;
		case 50:
			czas_function(15, ReadWord);
			break;
		case 51:
			czas_function(15, ReadWord);
			break;
		case 52:
			czas_function(15, ReadWord);
			break;
		case 53:
			czas_function(15, ReadWord);
			break;
		case 54:
			czas_function(15, ReadWord);
			break;
		case 55:
			czas_function(15, ReadWord);
			break;
		case 56:
			czas_function(15, ReadWord);
			break;
		case 57:
			czas_function(15, ReadWord);
			break;
		case 17:
			czas_wys.remove(czas_wys.length()-1);
			delay(100);
			break;
		case 16:
			ustawienie = false;
			break;
		case 18:
			ustawienie = false;
			break;
		case 15:
			start_sesja = false;
			sesja_timer.time(0);
			break;
		default:
			break;
		}
		char temp[20];
		FTImpl.ClearColorRGB(204, 204, 204);
		FTImpl.Clear(1, 1, 1);
		String temp_text = "Czas sesji: ";
		temp_text += czas_wys + " min";
		temp_text.toCharArray(temp, 20);
		if (!ustawienie) {
			FTImpl.Begin(FT_RECTS);
			FTImpl.ColorRGB(0, 0, 0);
			FTImpl.Vertex2ii(18, 79, 0, 0);
			FTImpl.Vertex2ii(294, 159, 0, 0);
			FTImpl.End();
			FTImpl.ColorRGB(255, 255, 255);
			FTImpl.Cmd_Text(50, 105, 29, 0, temp);
			FTImpl.Tag(13);
			FTImpl.Cmd_Button(310, 50, 150, 71, 27, 0, "Zmien czas");
			if (start_sesja) {
				FTImpl.Tag(15);
				FTImpl.Cmd_Button(310, 130, 150, 71, 27, 0, "Zatrzymaj Sesje");
			}
			else {
				FTImpl.Tag(12);
				FTImpl.Cmd_Button(310, 130, 150, 71, 27, 0, "Zacznij sesje");
			}
			FTImpl.Tag(10);
			FTImpl.Cmd_Button(351, 230, 120, 36, 27, 0, "Cofnij");
		}
		else {
			FTImpl.Begin(FT_RECTS);
			FTImpl.ColorRGB(0, 0, 0);
			FTImpl.Vertex2ii(21, 24, 0, 0);
			FTImpl.Vertex2ii(275, 95, 0, 0);
			FTImpl.End();
			FTImpl.ColorRGB(255, 255, 255);
			FTImpl.Cmd_Text(50, 46, 29, 0, temp);
			FTImpl.Cmd_Keys(70, 110, 160, 36, 29, 0, "123");
			FTImpl.Cmd_Keys(70, 155, 160, 36, 29, 0, "456");
			FTImpl.Cmd_Keys(70, 200, 160, 36, 29, 0, "789");
			FTImpl.Tag(16);
			FTImpl.Cmd_Button(320, 60, 121, 60, 27, 0, "Zatwierdz");
			FTImpl.Tag(17);
			FTImpl.Cmd_Button(320, 130, 121, 60, 27, 0, "Usun");
			FTImpl.Tag(18);
			FTImpl.Cmd_Button(354, 228, 120, 36, 27, 0, "Cofnij");
		}
		char war[30]="";
		if (blad != NULL) {
			
			blad.toCharArray(war, blad.length());
		}
		FTImpl.ColorRGB(255, 0, 0);
		FTImpl.Cmd_Text(39, 216, 24, 0, war);

		FTImpl.DLEnd();//end the display list
		FTImpl.Finish();//render the display list and wait for the completion of the DL
	}
}
void HelloWorld(int q)
{
	bool run = true;
	int32_t LoopFlag = 0, wbutton, hbutton, tracker;
	int16_t state_1=0, val1 = 0, val2 = 100, read_x, read_y, xOffset, yOffset, cRadius, xDistBtwClocks;
	sTagXY sTagxy;
	wbutton = FT_DISPLAYWIDTH / 8;
	hbutton = FT_DISPLAYHEIGHT / 8;
	FTImpl.Cmd_Track((FT_DISPLAYWIDTH - 38), 40, 8, (FT_DISPLAYHEIGHT - 65), 2);
	FTImpl.Finish();
	while (run)
	{
		int analogSensor = analogRead(smokeA0);
		FTImpl.GetTagXY(sTagxy);
		FTImpl.DLStart();
		FTImpl.TagMask(0);
		tracker = FTImpl.Read32(REG_TRACKER);
		if ((tracker & 0xff) == 2) {
			state_1 = (tracker >> 16);
		}
		else if ((tracker & 0xff) == 14) {
			sesja();
			run = false;
			break;
		}
		else if ((tracker & 0xff) == 19) {
			start_sesja = false;
		}
		else if ((tracker & 0xff) == 20) {
			sesja_timer.restart();
		}
		
		else if ((tracker & 0xff) == 25) {
			radio_menu();
		}
		else if ((tracker & 0xff) == 26) {
			oswietlenie_menu();
		}
		
		FTImpl.TagMask(1);
		FTImpl.ClearColorRGB(204, 204, 204);
		FTImpl.Clear(1, 1, 1);
		FTImpl.Tag(14);
		FTImpl.Cmd_Button(165, 234, 100, 35, 27, 0, "Sesja");
		FTImpl.Tag(25);
		FTImpl.Cmd_Button(270, 234, 100, 35, 27, 0, "Radio");
		FTImpl.Tag(26);
		FTImpl.Cmd_Button(375, 234, 100, 35, 27, 0, "Oswietlenie");
		FTImpl.ColorRGB(255, 255, 0);
		FTImpl.Begin(FT_RECTS);
		FTImpl.ColorRGB(0, 0, 0);
		FTImpl.Vertex2ii(10, 10, 0, 0);
		FTImpl.Vertex2ii(220, 80, 0, 0);
		FTImpl.Vertex2ii(10, 90, 0, 0);
		FTImpl.Vertex2ii(220, 160, 0, 0);
		FTImpl.Vertex2ii(10, 170, 0, 0);
		FTImpl.Vertex2ii(220, 230, 0, 0);
		FTImpl.Vertex2ii(260, 10, 0, 0);
		FTImpl.Vertex2ii(470, 80, 0, 0);
		FTImpl.End();
		char temp_1[3], wilg[4], press[5], sesja_temp[20], warning[4];
		uint16_t wil = bme.readHumidity(), temp = bme.readTemperature(), pres = bme.readPressure() / 100.0F;
		sprintf(warning, "%d", analogSensor / 10);
		sprintf(wilg, "%d", wil);
		sprintf(press, "%d", pres);
		sprintf(temp_1, "%d", temp);
		sprintf(warning, "%d", analogSensor);
		FTImpl.ColorRGB(255, 255, 255);
		FTImpl.Cmd_BGColor(0xD40000);
		FTImpl.Cmd_Progress(200, 20, 11, 50, 0, 100-temp, 100);
		FTImpl.Cmd_Progress(200, 177, 11, 45, 0, 100 - wil, 100);
		FTImpl.Cmd_Progress(200, 100, 11, 50, 0, 1000 - pres, 1000);
		FTImpl.Cmd_Progress(455, 18, 11, 50, 0, 1000 - analogSensor, 1000);
		FTImpl.ColorRGB(255, 255, 255);
		
		FTImpl.Cmd_Text(15, 15, 22, 0, "Temperatura");
		FTImpl.Cmd_Text(95, 28, 31, 0, temp_1);
		FTImpl.Cmd_Text(150, 28, 31, 0, "C");

		FTImpl.Cmd_Text(14, 90, 22, 0, "Wilgotnosc");
		FTImpl.Cmd_Text(95, 110, 31, 0, wilg);
		FTImpl.Cmd_Text(150, 110, 31, 0, "%");

		FTImpl.Cmd_Text(14, 170, 22, 0, "Cisnienie");
		FTImpl.Cmd_Text(48, 185, 31, 0, press);
		FTImpl.Cmd_Text(150, 185, 31, 0, "P");

		FTImpl.Cmd_Text(265, 13, 22, 0, "Jakosc powietrza");
		FTImpl.Cmd_Text(307, 29, 31, 0, warning);
		FTImpl.Cmd_Text(380, 43, 29, 0, "/1000");
		FTImpl.ColorRGB(212, 0, 0);

		if (start_sesja) {
			if (sesja_timer.available())
			{
				FTImpl.ClearColorRGB(204, 204, 204);
				FTImpl.Clear(1, 1, 1);
				FTImpl.ColorRGB(0, 0, 0);
				FTImpl.Cmd_Text(190, 110, 28, 0, "Koniec Sesji");
				FTImpl.ColorRGB(255, 255, 255);
				FTImpl.Tag(19);
				FTImpl.Cmd_Button(100, 140, 120, 36, 27, 0, "Koniec");
				FTImpl.Tag(20);
				FTImpl.Cmd_Button(250, 140, 120, 36, 27, 0, "Nastepna");
				if (warning_led_sesja) {
					for (int w = 0; w < 3; w++) {
						int brightness, fadeAmount = 5;
						for (int q = 0; q <= 255 / 5; q++) {
							analogWrite(led, brightness);
							brightness = brightness + fadeAmount;
							if (brightness == 0 || brightness == 255) {
								fadeAmount = -fadeAmount;
							}
							delay(50);
						}
					}
					analogWrite(led, oswietlenie_czas1);
					warning_led_sesja = false;
				}
			}
			else {
				FTImpl.Begin(FT_RECTS);
				FTImpl.ColorRGB(0, 0, 0);
				FTImpl.Vertex2ii(260, 90, 0, 0);
				FTImpl.Vertex2ii(470, 160, 0, 0);
				FTImpl.End();
				unsigned long timeToEnd = sesja_timer.time() / 1000;
				FTImpl.ColorRGB(212, 0, 0);
				FTImpl.Cmd_BGColor(0xFFFFFF);
				FTImpl.Cmd_Progress(276, 137, 180, 12, 0, timeToEnd, czas_wys.toInt() * 60);
				if (timeToEnd <=60) {
					sprintf(sesja_temp, "Pozostalo: %d sek", timeToEnd);
					FTImpl.ColorRGB(255, 255, 255);
					FTImpl.Cmd_Text(265, 95, 23, 0, sesja_temp);
				}
				else {
					sprintf(sesja_temp, "Pozostalo: %d min", timeToEnd / 60);
					FTImpl.ColorRGB(255, 255, 255);
					FTImpl.Cmd_Text(265, 95, 23, 0, sesja_temp);
				}
				
			}
		}
		
		FTImpl.DLEnd();
		FTImpl.Finish();
	}
}
void radio_menu() {
	bool run = true;
	uint32_t ReadWord;
	sTagXY sTagxy;
	while (run)
	{
		FTImpl.GetTagXY(sTagxy);
		FTImpl.DLStart();
		FTImpl.ColorRGB(0xFF, 0xFF, 0xFF);
		ReadWord = FTImpl.Read32(REG_TRACKER);
		switch (ReadWord)
		{
		case 10:
			HelloWorld(1);
			run = false;
		case 30:
			HelloWorld(1);
			run = false;
			break;
		case 31:
			next_channel();
			side = 1;
			delay(500);
			break;
		case 32:
			previous_channel();
			side = 0;
			delay(500);
			break;
		case 34:
			if (wzmacniacz) {
				digitalWrite(RelayPin1, LOW);
				wzmacniacz = false;
			}
			else {
				digitalWrite(RelayPin1, HIGH);
				wzmacniacz = true;
			}
			delay(300);
			break;
		default:
			break;
		}
		char radio1[25]="Stacja: ",radio2[25]="Si³a: ";
		FTImpl.TagMask(1);
		FTImpl.ClearColorRGB(204, 204, 204);
		FTImpl.Clear(1, 1, 1);
		FTImpl.Begin(FT_RECTS);
		FTImpl.ColorRGB(0, 0, 0);
		FTImpl.Vertex2ii(10, 22, 0, 0);
		FTImpl.Vertex2ii(220, 115, 0, 0);
		FTImpl.Vertex2ii(260, 22, 0, 0);
		FTImpl.Vertex2ii(470, 115, 0, 0);
		FTImpl.End();
		FTImpl.ColorRGB(255, 255, 255);
		if (Radio.read_status(buf) == 1) {

			signal_level = Radio.signal_level(buf);
			current_freq = floor(Radio.frequency_available(buf) / 100000 + .5) / 10;
			
			dtostrf(current_freq, 2, 2, radio1);
			FTImpl.Cmd_Text(15, 52, 30, 0, radio1);
			dtostrf(signal_level, 2, 2, radio2);
			strcat(radio2, "/15");
			FTImpl.Cmd_Text(281, 52, 30, 0, radio2);
			
			
			if (signal_level < 10) {
				if (side==1) {
					next_channel();
				}
				else {
					previous_channel();
				}
				
			}
		}
		FTImpl.Tag(30);
		FTImpl.Cmd_Button(354, 230, 120, 36, 27, 0, "Cofnij");
		FTImpl.Tag(31);
		FTImpl.Cmd_Button(260, 156, 166, 44, 27, 0, "Nastepna stacja");
		FTImpl.Tag(32);
		FTImpl.Cmd_Button(54, 156, 166, 44, 27, 0, "Poprzednia stacja");
		FTImpl.Tag(34);
		if (wzmacniacz) {
			FTImpl.Cmd_Button(5, 231, 144, 36, 27, 0, "Wlacz radio");
		}
		else {
			FTImpl.Cmd_Button(5, 231, 144, 36, 27, 0, "Wylacz radio");
		}
		
		FTImpl.DLEnd();//end the display list
		FTImpl.Finish();//render the display list and wait for the completion of the DL
	}
}
void next_channel() {
	if (floor(Radio.frequency_available(buf) / 100000 + .5) / 10 > 108.0) {
		Radio.set_frequency(88.0);
		
	}
	else {
		last_pressed = current_millis;
		search_mode = 1;
		search_direction = TEA5767_SEARCH_DIR_UP;
		Radio.search_up(buf);
		delay(200);
	}
}
void previous_channel() {
	if (floor(Radio.frequency_available(buf) / 100000 + .5) / 10 < 88.0) {
		Radio.set_frequency(108.0);
	}
	else {
		last_pressed = current_millis;
		search_mode = 1;
		search_direction = TEA5767_SEARCH_DIR_DOWN;
		Radio.search_down(buf);
		delay(200);
	}
	
}
void oswietlenie_menu()
{
	bool run = true, kolorval=false;
	int32_t tracker;
	int16_t val1 = 0,val2=0;
	sTagXY sTagxy;
	FTImpl.Cmd_Track(111, 62, 226, 13, 1);
	FTImpl.Cmd_Track(111, 162, 111, 13, 2);
	FTImpl.Cmd_Track(100, 130, 242, 14, 3);
	FTImpl.Cmd_Track(100, 170, 242, 14, 5);
	FTImpl.Cmd_Track(100, 210, 242, 14, 4);
	FTImpl.DLEnd();
	FTImpl.Finish();

	while (run)
	{
		int analogSensor = analogRead(smokeA0);
		FTImpl.GetTagXY(sTagxy);
		FTImpl.DLStart();
		tracker = FTImpl.Read32(REG_TRACKER);
		if ((tracker & 0xff) == 1)
		{

			digitalWrite(RelayPin2, LOW);
			val1 = (tracker >> 16);
			int temp = (tracker >> 16) / 655;
			analogWrite(led, temp * 2.50);
			oswietlenie_czas1 = val1;
			if (val1 == 0) {
				digitalWrite(RelayPin2, HIGH);
			}
		}
		else if ((tracker & 0xff) == 2)
		{

			digitalWrite(RelayPin3, LOW);
			val2 = (tracker >> 16);
			int temp = (tracker >> 16) / 655 *2.3;
			
			if (temp < 0) {
				temp = 124 + (124 - (temp * (-1)));
			}
			double temp2 = green * temp/250;
			rgb_led_br(red * temp / 250, green * temp / 250, blue * temp / 250);
			oswietlenie_czas2 = val2;
			if (val2 == 0) {
				digitalWrite(RelayPin3, HIGH);
			}
		}
		else if ((tracker & 0xff) == 3)
		{
			red_czas = (tracker >> 16);
			int temp = (red_czas / 655 * 2.30);
			if (temp<0) {
				red = 124 + (124 - (temp * (-1)));
			}
			else {
				red = temp;
			}
			double temp2 = red * ((oswietlenie_czas2 / 655 * 2.3) / 250);
			analogWrite(swiatlo_red, temp2);
		}
		else if ((tracker & 0xff) == 4)
		{
			blue_czas = (tracker >> 16);
			int temp = (blue_czas / 655 * 2.30);
			if (temp < 0) {
				blue=124 + (124-(temp * (-1)));
			}
			else {
				blue = temp;
			}
			double temp2 = blue * ((oswietlenie_czas2 / 655 * 2.3) / 250);
			analogWrite(swiatlo_blue, temp2);
		}
		else if ((tracker & 0xff) == 5)
		{
			green_czas = (tracker >> 16) ;
			int temp = (green_czas / 655 * 2.30);
			if (temp < 0) {
				green = 124 + (124 - (temp * (-1)));
			}
			else {
				green = temp;
			}
			double temp2 = green * ((oswietlenie_czas2 / 655 * 2.3) / 250);
			analogWrite(swiatlo_green, temp2);
		}
		else if ((tracker & 0xff) == 50) {
			HelloWorld(1);
			run = false;
		}
		else if ((tracker & 0xff) == 51) {
			kolorval = true;

		}
		else if ((tracker & 0xff) == 52) {
			kolorval = false;
			int temp = val2 / 655 * 2.3;

			if (temp < 0) {
				temp = 124 + (124 - (temp * (-1)));
			}
			rgb_led_br(red * temp / 250, green * temp / 250, blue * temp / 250);

		}
		FTImpl.ClearColorRGB(204, 204, 204);
		FTImpl.Clear(1, 1, 1);
		FTImpl.TagMask(1);
		if (kolorval) {
			FTImpl.TagMask(1);
			FTImpl.Tag(3);
			FTImpl.Cmd_Slider(100, 130, 242, 14, 0, red_czas, 65535);
			FTImpl.ColorRGB(85, 255, 0);
			FTImpl.Tag(5);
			FTImpl.Cmd_Slider(100, 170, 242, 14, 0, green_czas, 65535);
			FTImpl.ColorRGB(0, 0, 255);
			FTImpl.Tag(4);
			FTImpl.Cmd_Slider(100, 210, 242, 14, 0, blue_czas, 65535);
			FTImpl.ColorRGB(0, 0, 0);
			FTImpl.Begin(FT_RECTS);
			FTImpl.ColorRGB(0, 0, 0);
			FTImpl.Vertex2ii(24, 28, 0, 0);
			FTImpl.Vertex2ii(288, 100, 0, 0);
			FTImpl.Vertex2ii(368, 123, 0, 0);
			FTImpl.Vertex2ii(465, 147, 0, 0);
			FTImpl.Vertex2ii(362, 123, 0, 0);
			FTImpl.Vertex2ii(471, 147, 0, 0);
			FTImpl.Vertex2ii(362, 163, 0, 0);
			FTImpl.Vertex2ii(471, 190, 0, 0);
			FTImpl.Vertex2ii(362, 203, 0, 0);
			FTImpl.Vertex2ii(471, 230, 0, 0);
			FTImpl.End();
			FTImpl.ColorRGB(255, 0, 0);
			
			FTImpl.TagMask(0);
			FTImpl.Tag(400);
			FTImpl.ColorRGB(255, 255, 255);
			FTImpl.Cmd_Text(8, 126, 27, 0, "Czewony:");
			FTImpl.Cmd_Text(8, 205, 27, 0, "Niebieski:");
			FTImpl.Cmd_Text(8, 167, 27, 0, "Zielony:");
			char red_string[25],blue_string[20],green_string[20];
			sprintf(red_string, "Wartosc: %d /255", red);
			sprintf(blue_string, "Wartosc: %d /255", blue);
			sprintf(green_string, "Wartosc: %d /255", green);
			FTImpl.Cmd_Text(370, 125, 20, 0, red_string);
			FTImpl.Cmd_Text(370, 206, 20, 0, blue_string);
			FTImpl.Cmd_Text(370, 166, 20, 0, green_string);
			FTImpl.Cmd_Text(33, 51, 28, 0, "Kolor:");
			FTImpl.Begin(FT_RECTS);
			FTImpl.ColorRGB(red, green, blue);
			FTImpl.Vertex2ii(140, 45, 0, 0);
			FTImpl.Vertex2ii(257, 82, 0, 0);
			FTImpl.End();
			FTImpl.Cmd_FGColor(0x00009A);
			FTImpl.ColorRGB(255, 255, 255);
			FTImpl.Tag(52);
			FTImpl.TagMask(1);
			FTImpl.Cmd_Button(353, 237, 120, 30, 27, 0, "Zatwierdz");
		}
		else {
			
			
			FTImpl.Begin(FT_RECTS);
			FTImpl.ColorRGB(0, 0, 0);
			FTImpl.Vertex2ii(20, 30, 0, 0);
			FTImpl.Vertex2ii(450, 110, 0, 0);
			FTImpl.Vertex2ii(20, 130, 0, 0);
			FTImpl.Vertex2ii(450, 210, 0, 0);
			FTImpl.End();
			FTImpl.ColorRGB(255, 255, 255);
			FTImpl.Cmd_BGColor(0x00005F);
			FTImpl.TagMask(1);
			FTImpl.Tag(1);
			FTImpl.Cmd_Slider(111, 62, 226, 13, 0, oswietlenie_czas1, 65535);
			FTImpl.Tag(2);
			FTImpl.Cmd_Slider(111, 162, 111, 13, 0, oswietlenie_czas2, 65535);
			FTImpl.TagMask(0);
			FTImpl.Tag(100);
			FTImpl.ColorRGB(255, 255, 255);
			FTImpl.TagMask(1);
			FTImpl.Cmd_Text(372, 36, 28, 0, "Status:");
			FTImpl.Cmd_Text(372, 136, 28, 0, "Status:");
			FTImpl.Cmd_Text(33, 54, 29, 0, "Ledy: ");
			FTImpl.Cmd_Text(25, 156, 28, 0, "Swiatlo: ");
			if (oswietlenie_czas1 == 0) {
				FTImpl.ColorRGB(255, 0, 0);
				FTImpl.Cmd_Text(365, 76, 22, 0, "Wylaczone");
			}
			else {
				FTImpl.ColorRGB(0, 255, 0);
				FTImpl.Cmd_Text(365, 76, 22, 0, "Wlaczone");
			}
			if (oswietlenie_czas2 == 0) {
				FTImpl.ColorRGB(255, 0, 0);
				FTImpl.Cmd_Text(365, 176, 22, 0, "Wylaczone");
			}
			else {
				FTImpl.ColorRGB(0, 255, 0);
				FTImpl.Cmd_Text(365, 176, 22, 0, "Wlaczone");
			}
			FTImpl.ColorRGB(255, 255, 255);
			FTImpl.Cmd_FGColor(0x00009A);
			FTImpl.Tag(50);
			FTImpl.Cmd_Button(355, 230, 120, 36, 27, 0, "Cofnij");
			FTImpl.Tag(51);
			FTImpl.Cmd_Button(258, 154, 95, 31, 27, 0, "Kolor");
		}
		

		FTImpl.DLEnd();
		FTImpl.Finish();
	};
}
void rgb_led_br(double r, double g, double b) {
	analogWrite(swiatlo_red, r);
	analogWrite(swiatlo_blue, b);
	analogWrite(swiatlo_green, g);
}
unsigned long rgb2hex(int r, int b,int g) {
	return ((long)r << 16L) | ((long)g << 8L) | (long)b;
}
void setup()
{
	bool rslt;
	Serial.begin(9600);
	if (BootupConfigure())
	{
		
		//error case - do not do any thing
	}
	else
	{
		rslt = bme.begin();
		if (!rslt) {
			Serial.println("Init Fail,Please Check your address or the wire you connected!!!");
			
		}
		pinMode(RelayPin1, OUTPUT);
		pinMode(RelayPin2, OUTPUT);
		pinMode(RelayPin3, OUTPUT);
		digitalWrite(RelayPin1, HIGH);
		digitalWrite(RelayPin2, HIGH);
		digitalWrite(RelayPin3, HIGH);
		pinMode(led, OUTPUT);
		pinMode(swiatlo_red, OUTPUT);
		pinMode(swiatlo_blue, OUTPUT);
		pinMode(swiatlo_green, OUTPUT);
		pinMode(smokeA0, INPUT);
		Calibrate();
		AlphaBlend();
		HelloWorld(1);
	}
	
}
int brightness = 0; // how bright the LED is 
int fadeAmount = 5;
void loop()
{

}
