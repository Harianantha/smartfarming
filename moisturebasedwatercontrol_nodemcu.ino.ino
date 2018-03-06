//#include <avr/sleep.h>
#include <ESP8266WiFi.h>

#include <PubSubClient.h>

#include <DHT.h>       




#include <AzureIoTHub.h>
#include <AzureIoTProtocol_MQTT.h>
#include <AzureIoTUtility.h>
#include "simplesample_mqtt.h"

// DHT11 Sensor---------------
#define DHTTYPE DHT11   
#define dht_dpin 2
DHT dht(dht_dpin, DHTTYPE); 
// ----------------------------

#define MESSAGE_MAX_LEN 512
int sensor_pin = A0; 
int solenoidPin = 05;    //Testing with LED instead of solenoid
int flowPin = 04;
unsigned long flowcount = 0;
#define countof(a) (sizeof(a) / sizeof(a[0]))

const char* ssid = "Techolution";
const char* password = "wearethebest";

int moistureValue =0;
int valvePoistion = 0;

const char* mqtt_server = "iot.eclipse.org";
const char* TOPIC_NAME = "techo/smartfarm/321";

WiFiClient espClient;
PubSubClient client(espClient);

static char *connectionString = "HostName=techo-iothub.azure-devices.net;DeviceId=techo_smartfarming_soilmoisture_001;SharedAccessKey=Jh8KJbwrH8wNpElZlQbwXCyXY7qo5HxEug/R/igYgRs=";


char* clientId = "techo_smartfarming_soilmoisture_001";
char* userName= "techo-iothub.azure-devices.net/techo_smartfarming_soilmoisture_001/api-version=2016-11-14";
char* mqtt_password= "SharedAccessSignature sr=techo-iothub.azure-devices.net%2Fdevices%2Ftecho_smartfarming_soilmoisture_001&sig=5DC6WzpK7TVP21US7zstm%2B5BOH5KlX5Yf29DNBZ6qj4%3D&se=1546520267";





void setup_wifi() {
   delay(100);
  // We start by connecting to a WiFi network
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
    {
      delay(500);
      Serial.print(".");
    }
  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

//static IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle;
void setupMQTT(){
   client.setServer(mqtt_server, 1883);
   client.setCallback(callback);
}
void setup() {
  
  pinMode(solenoidPin, OUTPUT);
  Serial.begin(9600);
  Serial.println("Reading From the Sensor ...\n");
  Serial.println("\n");
  setup_wifi();
  delay(1000);
  setupMQTT();
  pinMode(flowPin, INPUT_PULLUP);
  attachInterrupt(flowPin, flow, CHANGE);
  initTime();
  dht.begin();
 
  }
  void flow()
{
flowcount +=1;
}

void loop() {
dht.begin();
  digitalWrite(solenoidPin, HIGH);     //Switch Solenoid OFF
  valvePoistion = 0;

  
float l = flowcount/450.0;
Serial.print("Flow in Liters: ");
Serial.print(l);
Serial.print("\n");
  //TESTing WITHOUT SENSOR END
 
  moistureValue= analogRead(sensor_pin);
   float h = dht.readHumidity();
   float t = dht.readTemperature();  
  

    Serial.print("tempereatue from DHT ");
    Serial.print(t);
    Serial.print("\nmoisture from DHT ");
    Serial.print(h);
    Serial.print("\n");
    
    
    
 // Serial.print("moistureValue  before conversion");
 // Serial.print(moistureValue);
 // Serial.print("%");
  moistureValue = map(moistureValue,1023,250,0,100);
 // Serial.print("moistureValue  after  conversion");
  Serial.print(moistureValue);

  

  signed int moistureValueToSend = moistureValue;
  Serial.println(valvePoistion);



  	Serial.print("\n");
  
  if(moistureValue < 20){
      if(valvePoistion == 0){  //Open only if it is closed
      Serial.print("Opening valve");
      digitalWrite(solenoidPin, LOW);    //Switch Solenoid ON  
      valvePoistion =1;
      }else{

        Serial.print("Not opening as it is already open");
      }
      
  }else{
    if(valvePoistion == 1){

        Serial.print("Closing valve ");
        digitalWrite(solenoidPin, HIGH);     //Switch Solenoid OFF
  
         valvePoistion=0;  

    }else{

      Serial.print("Not closing valve as it is already closed");
    }

  }
  publishToMQTT(l,h,t,moistureValue,valvePoistion);
  simplesample_mqtt_run(h,moistureValueToSend,t,valvePoistion,l);
     delay(60000);
    // delay(5000);
  }


void initTime() {
#if defined(ARDUINO_SAMD_ZERO) || defined(ARDUINO_SAMD_MKR1000) || defined(ARDUINO_SAMD_FEATHER_M0)
    WiFiUDP ntpUdp;

    NTPClient ntpClient(ntpUdp);

    ntpClient.begin();

    while (!ntpClient.update()) {
        Serial.println("Fetching NTP epoch time failed! Waiting 5 seconds to retry.");
        delay(5000);
    }

    ntpClient.end();

    unsigned long epochTime = ntpClient.getEpochTime();

    Serial.print("Fetched NTP epoch time is: ");
    Serial.println(epochTime);

#elif ARDUINO_ARCH_ESP8266
    time_t epochTime;

    configTime(0, 0, "pool.ntp.org", "time.nist.gov");

    while (true) {
        epochTime = time(NULL);

        if (epochTime == 0) {
            Serial.println("Fetching NTP epoch time failed! Waiting 2 seconds to retry.");
            delay(2000);
        } else {
            Serial.print("Fetched NTP epoch time is: ");
            Serial.println(epochTime);
            break;
        }
    }
#endif
}


void publishToMQTT(float litre,float relativehumid,float temperature,int sandmoisture,int valvePoistion) {
  // Loop until we're reconnected
      //while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
   if (client.connect("ESP826645525852Client")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      String msg="{\"deviceId\":\"techo_smartfarming_soilmoisture_001\",";
      delay(100);
      // msg = msg+"\"ambienttemperature\":"+temperature+",\"sandmoisture\":"+sandmoisture+",\"relativehumid\":"+relativehumid;
       msg = msg+"\"h\":"+relativehumid+",\"m\":"+sandmoisture+",\"t\":"+temperature+",\"l\":"+litre+",\"vp\":"+valvePoistion;
     delay(100);
      msg=msg+"}";
      delay(100);
      int n = msg.length(); 
      Serial.println(n);
      char char_array[n+1]; 
      strcpy(char_array, msg.c_str()); 
      delay(500);
      Serial.println("Message to Send to MQTT\n");
      Serial.println(char_array);
      client.publish("techo/smartfarm/321", char_array);
      //client.publish("techo/smartfarm/testing", "testmessage");
      Serial.println("Message Sent to MQTT\n");
      // ... and resubscribe
      //client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
 // }
}


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
/*
  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }
*/
}
