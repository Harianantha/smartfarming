// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#include <stdio.h>
#include <stdint.h>

/* This sample uses the _LL APIs of iothub_client for example purposes.
That does not mean that MQTT only works with the _LL APIs.
Simply changing the using the convenience layer (functions not having _LL)
and removing calls to _DoWork will yield the same results. */

#ifdef ARDUINO
#include "AzureIoTHub.h"
#else
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/platform.h"
#include "serializer.h"
#include "iothub_client_ll.h"
#include "iothubtransportmqtt.h"
#endif

#ifdef MBED_BUILD_TIMESTAMP
#define SET_TRUSTED_CERT_IN_SAMPLES
#endif // MBED_BUILD_TIMESTAMP

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES


/*String containing Hostname, Device Id & Device Key in the format:             */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessKey=<device_key>"    */
static const char* connectionString = "HostName=techo-iothub.azure-devices.net;DeviceId=techo_smartfarming_soilmoisture_001;SharedAccessKey=Jh8KJbwrH8wNpElZlQbwXCyXY7qo5HxEug/R/igYgRs=";

static int pendingMessages = 0;
static bool anyMessagesPending = false;                                                                                                                                           

// Define the Model
BEGIN_NAMESPACE(TelemetryData);

DECLARE_MODEL(MetricsData,
WITH_DATA(ascii_char_ptr, DeviceId),
WITH_DATA(ascii_char_ptr, n),
WITH_DATA(int, v),
WITH_DATA(ascii_char_ptr, u)
//WITH_DATA(float, Temperature),
//WITH_DATA(float, Humidity),
//WITH_ACTION(TurnFanOn),
//WITH_ACTION(TurnFanOff),
//WITH_ACTION(SetAirResistance, int, Position)
);

END_NAMESPACE(TelemetryData);

static char propText[1024];

/*EXECUTE_COMMAND_RESULT TurnFanOn(ContosoAnemometer* device)
{
    (void)device;
    (void)printf("Turning fan on.\r\n");
    return EXECUTE_COMMAND_SUCCESS;
}
*/
/*EXECUTE_COMMAND_RESULT TurnFanOff(ContosoAnemometer* device)
{
    (void)device;
    (void)printf("Turning fan off.\r\n");
    return EXECUTE_COMMAND_SUCCESS;
}*/
/*
EXECUTE_COMMAND_RESULT SetAirResistance(ContosoAnemometer* device, int Position)
{
    (void)device;
    (void)printf("Setting Air Resistance Position to %d.\r\n", Position);
    return EXECUTE_COMMAND_SUCCESS;
}
*/
void sendCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    printf("pendingMessages in start of callbak is %u \r\n",pendingMessages);
    unsigned int messageTrackingId = (unsigned int)(uintptr_t)userContextCallback;

    (void)printf("Message Id: %u Received.\r\n", messageTrackingId);

    (void)printf("Result Call Back Called! Result is: %s \r\n", ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
    pendingMessages --;
    if (pendingMessages == 0){
      
    anyMessagesPending = false;  
     printf("Setting anyMessagesPending to false \r\n");
    }
    printf("pendingMessages in end of callbak is %u \r\n",pendingMessages);
    
    
}

static void sendMessage(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, const unsigned char* buffer, size_t size, MetricsData *myData)
{
  printf("pendingMessages in start of sendMessage is %u \r\n",pendingMessages);
    static unsigned int messageTrackingId;
    //String msg="{\"key\":\"Test Mesage\"}";
    char charBuf= "{\"key\":\"Test Mesage\"}";
    // msg.toCharArray(charBuf, msg.length()+1);
    IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromByteArray(buffer, size);
    //IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromByteArray(charBuf, strlen(charBuf));
    if (messageHandle == NULL)
    {
        printf("unable to create a new IoTHubMessage\r\n");
    }
    else
    {
   //     MAP_HANDLE propMap = IoTHubMessage_Properties(messageHandle);
    //    (void)sprintf_s(propText, sizeof(propText), myWeather->Temperature > 28 ? "true" : "false");
      /*  if (Map_AddOrUpdate(propMap, "temperatureAlert", propText) != MAP_OK)
        {
            (void)printf("ERROR: Map_AddOrUpdate Failed!\r\n");
        }*/

        if (IoTHubClient_LL_SendEventAsync(iotHubClientHandle, messageHandle, sendCallback, (void*)(uintptr_t)messageTrackingId) != IOTHUB_CLIENT_OK)
        {
            printf("failed to hand over the message to IoTHubClient");
            printf("failed to hand over the message to IoTHubClient");
             if (pendingMessages == 0){
      
            anyMessagesPending = false;  
            printf("Setting anyMessagesPending to false in falure of message sending \r\n");
          }
            
        }
        else
        {
            anyMessagesPending = true;
            printf("Setting anyMessagesPending to true \r\n");
            pendingMessages++;
            printf("IoTHubClient accepted the message for delivery\r\n");
            printf("IoTHubClient accepted the message for delivery\r\n");
        }
        IoTHubMessage_Destroy(messageHandle);
    }
    messageTrackingId++;
    printf("pendingMessages in end of sendMessage is %u \r\n",pendingMessages);
}

/*this function "links" IoTHub to the serialization library*/
static IOTHUBMESSAGE_DISPOSITION_RESULT IoTHubMessage(IOTHUB_MESSAGE_HANDLE message, void* userContextCallback)
{
    IOTHUBMESSAGE_DISPOSITION_RESULT result;
    const unsigned char* buffer;
    size_t size;
    if (IoTHubMessage_GetByteArray(message, &buffer, &size) != IOTHUB_MESSAGE_OK)
    {
        printf("unable to IoTHubMessage_GetByteArray\r\n");
        result = IOTHUBMESSAGE_ABANDONED;
    }
    else
    {
        /*buffer is not zero terminated*/
        char* temp = malloc(size + 1);
        if (temp == NULL)
        {
            printf("failed to malloc\r\n");
            result = IOTHUBMESSAGE_ABANDONED;
        }
        else
        {
            (void)memcpy(temp, buffer, size);
            temp[size] = '\0';
            EXECUTE_COMMAND_RESULT executeCommandResult = EXECUTE_COMMAND(userContextCallback, temp);
            result =
                (executeCommandResult == EXECUTE_COMMAND_ERROR) ? IOTHUBMESSAGE_ABANDONED :
                (executeCommandResult == EXECUTE_COMMAND_SUCCESS) ? IOTHUBMESSAGE_ACCEPTED :
                IOTHUBMESSAGE_REJECTED;
            free(temp);
        }
    }
    return result;
}

void simplesample_mqtt_run( char * nameVal,int unitval,char * units)
{
    printf("In simplesample_mqtt_run\n");
    printf("Moisture value in simplesample_mqtt_run is");
  //  printf(unitval);
    //printf("\r\n");
    if (platform_init() != 0)
    {
        (void)printf("Failed to initialize platform.\r\n");
    }
    else
    {
        if (serializer_init(NULL) != SERIALIZER_OK)
        {
            (void)printf("Failed on serializer_init\r\n");
        }
        else
        {
          
            IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(connectionString, MQTT_Protocol);
            printf("Created connection string");
            srand((unsigned int)time(NULL));
            

            if (iotHubClientHandle == NULL)
            {
                (void)printf("Failed on IoTHubClient_LL_Create\r\n");
                printf("IotHub client handle is null");
            }
            else
            {
#ifdef SET_TRUSTED_CERT_IN_SAMPLES
             
                // For mbed add the certificate information
                if (IoTHubClient_LL_SetOption(iotHubClientHandle, "TrustedCerts", certificates) != IOTHUB_CLIENT_OK)
                {
                    (void)printf("failure to set option \"TrustedCerts\"\r\n");
                }
#endif // SET_TRUSTED_CERT_IN_SAMPLES
                

                MetricsData* moistureData = CREATE_MODEL_INSTANCE(TelemetryData, MetricsData);
                if (moistureData == NULL)
                {
                    (void)printf("Failed on CREATE_MODEL_INSTANCE\r\n");
                }
                else
                {
                           moistureData->DeviceId = "techo_smartfarming_soilmoisture_001";
                        moistureData->v =unitval;
                        moistureData->n =nameVal;
                        moistureData->u =units;
   
                            
                            unsigned char* destination;
                            size_t destinationSize;
                           if (SERIALIZE(&destination, &destinationSize, moistureData->DeviceId,moistureData->n,moistureData->v,moistureData->u) != CODEFIRST_OK)
                            {
                                (void)printf("Failed to serialize\r\n");
                                printf("Failed to serialize\r\n");
                            }else{
                              printf("Before sending message");
                                sendMessage(iotHubClientHandle, destination,destinationSize,moistureData);
                                printf("After sending message");
                                free(destination);
                            }
                            
                              
                            
                        
                        IoTHubClient_LL_DoWork(iotHubClientHandle);
                          serializer_deinit();
                        /* wait for commands */
                        while (anyMessagesPending)
                        {
                            IoTHubClient_LL_DoWork(iotHubClientHandle);
                            ThreadAPI_Sleep(100);
                        }
                        printf("No messages are pending. Exiting loop");
                    }

                    DESTROY_MODEL_INSTANCE(moistureData);
                
                IoTHubClient_LL_Destroy(iotHubClientHandle);
            }
            serializer_deinit();
        }
        platform_deinit();
    }
}
