#include <SoftwareSerial.h>

#include "ABlocksIOTMQTTESP8266.h"

#include <HardwareSerial.h>

#include "ABlocks_DHT.h"

double humedad;
double no2_ppb;
double temperatura;
double co_ppm;
double densidad_polvo;
double particulas_m3;

const char mqtt_broker[]="mqtt.thingspeak.com";
const int mqtt_port=1883;
const char mqtt_user[]="";
const char mqtt_pass[]="";
const char mqtt_clientid[]="KutsaZalla01";
const char mqtt_wifi_ssid[]="CasaBlanca2.4";
const char mqtt_wifi_pass[]="0413E+1615N";
//ABlocksIOT: esp8266
HardwareSerial &mqtt_esp8266_serial=Serial;
ESP8266 mqtt_esp8266_wifi(&mqtt_esp8266_serial);
char mqtt_payload[64];
DHT dht9(9,DHT22);

double mqtt_payload2double(unsigned char *_payload, int _length){
	int i;
	for (i = 0; i<_length && i<64; i++){
		mqtt_payload[i] = _payload[i];
	}
	mqtt_payload[i] = 0;
	return atof(mqtt_payload);
}

String mqtt_payload2string(unsigned char *_payload, int _length){
	int i;
	for (i = 0; i<_length && i<64; i++){
		mqtt_payload[i] = _payload[i];
	}
	mqtt_payload[i] = 0;
	return String(mqtt_payload);
}
void mqtt_callback(char* _topic, unsigned char* _payload, unsigned int _payloadlength){
	double v=mqtt_payload2double(_payload,_payloadlength);
	String vt=mqtt_payload2string(_payload,_payloadlength);
}

void mqtt_subscribe(){
}

void subir_datos_a_la_nube() {
  ABlocksIOT.Publish(String("channels/981311/publish/fields/field1/CLRDUYQ6GK0EF2WO"), String(no2_ppb));
  ABlocksIOT.Publish(String("channels/981311/publish/fields/field2/CLRDUYQ6GK0EF2WO"), String(co_ppm));
  ABlocksIOT.Publish(String("channels/981311/publish/fields/field3/CLRDUYQ6GK0EF2WO"), String(temperatura));
  ABlocksIOT.Publish(String("channels/981311/publish/fields/field4/CLRDUYQ6GK0EF2WO"), String(humedad));
  ABlocksIOT.Publish(String("channels/981311/publish/fields/field5/CLRDUYQ6GK0EF2WO"), String(particulas_m3));
}

void fnc_mics4514_preheat(int _prepin)
{
	pinMode(_prepin, OUTPUT);
	digitalWrite(_prepin, 1);
	delay(10 * 1000);
	digitalWrite(_prepin, 0);
}

double fnc_mics4514(int _prepin, int _noxpin, int _redpin, int _result)
{
	double vnox_value = analogRead(_noxpin);
	double ppbNO2  = -85.26*log(vnox_value*5.0/1023.0)+121.02;
	double ugm3NO2  = (ppbNO2*1.88);

	double vred_value = analogRead(_redpin)/409.2;
	double RsCO = 100000/((5/vred_value) - 1);
	double ppmCO  = 911.19*pow(2.71828,(-8.577*RsCO/100000));

	if(_result==0)return ppbNO2;
	if(_result==1)return ppmCO;
	if(_result==2)return ugm3NO2;
	return 0.0;
}

double fnc_pm25(int _ledpin, int _outpin, int _result)
{
	digitalWrite(_ledpin,LOW);
	delayMicroseconds(280);
	double voMeasured = analogRead(_outpin);
	delayMicroseconds(40);
	digitalWrite(_ledpin,HIGH);
	delayMicroseconds(9680);
	double calcVoltage = voMeasured * (5.0 / 1024.0);
	double dustDensity = 0.172 * calcVoltage - 0.0999;
	double particlesM3 = float((voMeasured/1024)-0.0356)*120000*0.035;
	if(_result==0)return dustDensity;
	return particlesM3;
}

void leer_sensores() {
  humedad = dht9.readHumidity();
  temperatura = dht9.readTemperature();
  no2_ppb = fnc_mics4514(2,A0,A1,0);
  co_ppm = fnc_mics4514(2,A0,A1,1);
  densidad_polvo = fnc_pm25(8,A2,0);
  particulas_m3 = fnc_pm25(8,A2,1);
  if ((densidad_polvo < 0)) {
    densidad_polvo = 0;

  }
}

void setup()
{
  	mqtt_esp8266_serial.begin(115200);
	ABlocksIOT.begin(mqtt_broker,mqtt_port, mqtt_user,mqtt_pass, mqtt_clientid, mqtt_esp8266_wifi, mqtt_wifi_ssid, mqtt_wifi_pass, mqtt_callback, mqtt_subscribe);
pinMode(9, INPUT);
dht9.begin();
pinMode(2, OUTPUT);
pinMode(A0, INPUT );
pinMode(A1, INPUT);
fnc_mics4514_preheat(2);
pinMode(A2, INPUT);
pinMode(8, OUTPUT);
}


void loop()
{
	ABlocksIOT.loop();
    delay(1000);
    leer_sensores();
    subir_datos_a_la_nube();

}

