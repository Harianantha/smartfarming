//#include <avr/sleep.h>
#include <ESP8266WiFi.h>




#include <AzureIoTHub.h>
#include <AzureIoTProtocol_MQTT.h>
#include <AzureIoTUtility.h>
#define MESSAGE_MAX_LEN 256
int sensor_pin = A0; 
//int sensor_pin = 12; 
//int solenoidPin = 4;    //This is the output pin on the Arduino we are using
int solenoidPin = 05;    //Testing with LED instead of solenoid

const char* ssid = "Wifi One";
const char* password = "wearethebest";

int moistureValue ;
int valvePoistion = 0;



static char *connectionString = "HostName=techo-iothub.azure-devices.net;DeviceId=techo_smartfarming_soilmoisture_001;SharedAccessKey=Jh8KJbwrH8wNpElZlQbwXCyXY7qo5HxEug/R/igYgRs=";
//static char *ssid = "Wifi One";
static char *pass = "wearethebest";

//mqtt m = mqtt.Client("techo_smartfarming_soilmoisture_001", 120, "techo-iothub.azure-devices.net/techo_smartfarming_soilmoisture_001/api-version=2016-11-14", "SharedAccessSignature sr=techo-iothub.azure-devices.net%2Fdevices%2Ftecho_smartfarming_soilmoisture_001&sig=YtOk64m1HpF8f2lIbVejDfX8R6ebQIy4aIedGfpwZQE%3D&se=1546416624")

WiFiClient espClient;
//PubSubClient client(espClient);


char* clientId = "techo_smartfarming_soilmoisture_001";
char* userName= "techo-iothub.azure-devices.net/techo_smartfarming_soilmoisture_001/api-version=2016-11-14";
char* mqtt_password= "SharedAccessSignature sr=techo-iothub.azure-devices.net%2Fdevices%2Ftecho_smartfarming_soilmoisture_001&sig=5DC6WzpK7TVP21US7zstm%2B5BOH5KlX5Yf29DNBZ6qj4%3D&se=1546520267";
char* mqtt_server="techo-iothub.azure-devices.net";
int mqtt_port= 8883;
char* topicName = "devices/techo_smartfarming_soilmoisture_001/messages/events";
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

static IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle;
void setup() {
  Serial.begin(9600);
  Serial.println("Reading From the Sensor ...");
  delay(2000);
  //set_sleep_mode (SLEEP_MODE_PWR_DOWN);  
  //sleep_enable();
  pinMode(solenoidPin, OUTPUT);

  setup_wifi();
  //client.setServer(mqtt_server, 1883);

//setup Last Will and Testament (optional)
// Broker will publish a message with qos = 0, retain = 0, data = "offline" 
// to topic "/lwt" if client don't send keepalive packet
  /*m:lwt("/lwt", "offline", 1, 0)

  m:on("connect", function(client) print ("connected") end)
  m:on("offline", function(client) print ("offline") end)
  */

  iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(connectionString, MQTT_Protocol);
    if (iotHubClientHandle == NULL)
    {
        Serial.println("Failed on IoTHubClient_CreateFromConnectionString.");
        while (1);
    }

    IoTHubClient_LL_SetOption(iotHubClientHandle, "product_info", "Techolution_SmartFarmingDemo-C");
    IoTHubClient_LL_SetMessageCallback(iotHubClientHandle, receiveMessageCallback, NULL);
    IoTHubClient_LL_SetDeviceMethodCallback(iotHubClientHandle, deviceMethodCallback, NULL);
    IoTHubClient_LL_SetDeviceTwinCallback(iotHubClientHandle, twinCallback, NULL);

  
  }

void loop() {

  //Serial.print("OPening valve ");
//  digitalWrite(solenoidPin, HIGH);    //Switch Solenoid ON
//  delay(20000);                      //Wait 1 Second
 // Serial.print("Closing valve ");
  digitalWrite(solenoidPin, HIGH);     //Switch Solenoid OFF
  delay(10000);                      //Wait 1 Second
  moistureValue= analogRead(sensor_pin);
  //moistureValue= digitalRead(sensor_pin);
  Serial.print("moistureValue  before conversion");
  Serial.print(moistureValue);
  Serial.print("%");
 // moistureValue = map(moistureValue,550,0,0,100);
  moistureValue = map(moistureValue,1023,250,0,100);
  Serial.print(moistureValue);
  time_t now = time(nullptr);
 // Serial.println(ctime(&now));
  String msg="{\"deviceId\":\"techo_smartfarming_soilmoisture_001\",\"moisture\":\"";
  msg= msg+ moistureValue+"\"}";
  Serial.print("Message is ");
  Serial.print(msg);
  char message[500];
  msg.toCharArray(message,500);
  //SENDING TO MQTT ENDPOINT though successful from client, its not getting picked up at server side
 // client.publish(topicName, message,1);

  char messagePayload[MESSAGE_MAX_LEN];
   bool temperatureAlert = false;
   char charBuf[msg.length()+1];
//  bool alert = readMessage(moistureValue, messagePayload);
   msg.toCharArray(charBuf, msg.length()+1);
  sendMessage(iotHubClientHandle,charBuf,temperatureAlert);
  
  if(moistureValue < 20){
      if(valvePoistion == 0){  //Open only if it is closed
        Serial.print("OPening valve ");
      digitalWrite(solenoidPin, LOW);    //Switch Solenoid ON
        
      valvePoistion =1;
      delay(60000);                      //Wait 1 Minute  
      }else{
        Serial.print("Not opening as it is already open");
        delay(10000);                      //Wait 1 Minute  
      }
      
  }else{
    if(valvePoistion == 1){

        Serial.print("Closing valve ");
        digitalWrite(solenoidPin, HIGH);     //Switch Solenoid OFF
  
         valvePoistion=0;  
    }else{

      Serial.print("Not closing valve as it is already closed");
    }
    
    //delay(3600000);                      //Wait 1 Hour  
    delay(10000);                      //Wait 10 sec Hour  

  }
  //Serial.print("Mositure : ");
  //Serial.print(moistureValue);
  //Serial.println("%");
  //delay(10000);
  }

/*
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) 
  {
    Serial.print("Attempting MQTT connection...");
    
        // Attempt to connect
    //if you MQTT broker has clientID,username and password
    //please change following line to    if (client.connect(clientId,userName,passWord))
    //if (client.connect(clientId.c_str()))
    if (client.connect(clientId,userName,mqtt_password))
    {
      Serial.println("connected");
     //once connected to MQTT broker, subscribe command if any
    //  client.subscribe("OsoyooCommand");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 6 seconds before retrying
      delay(6000);
    }
  }



  
} 
*/
