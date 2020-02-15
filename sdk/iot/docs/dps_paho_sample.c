#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTClient.h"
#include "az_dps_client.h"

#define TIMEOUT     4 * 60 * 1000L // Recommended 4 minute timeouts for all Azure IoT Services.

az_dps_client dps_client;

volatile MQTTClient_deliveryToken deliveredtoken;

void delivered(void *context, MQTTClient_deliveryToken dt)
{
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    int i;
    char* payloadptr;

    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: ");

    payloadptr = message->payload;
    for(i=0; i<message->payloadlen; i++)
    {
        putchar(*payloadptr++);
    }
    putchar('\n');
    
    // Processing 
    az_result ret;
    az_iot_mqtt_pub pub_received;
    pub_received.topic = az_span_init(topicName, topicLen, topicLen);
    pub_received.qos = message.qos;
    pub_received.payload = az_span_init(message.payload, message.payloadlen, message.payloadlen);

    az_dps_register_result dps_result;
    
    if (az_succeeded(ret = az_dps_register_handle(dps_client, pub_received, &dps_result)))
    {
        if (dps_result->status == az_dps_status_assigned)
        {
            // Registration complete:
            printf(
                "DPS registration complete: hub = %s, device_id = %s\n", 
                az_span_ptr(dps_result.registration_state.assigned_hub), 
                az_span_ptr(dps_result.registration_state.device_id));
            
            MQTTClient_disconnect(client, 10000);
            MQTTClient_destroy(&client);
            exit(EXIT_SUCCESS);
        }
        else    //az_dps_status_assigning or az_dps_status_unassigned
        {
            sleep(dps_result->retry_after);
            az_iot_mqtt_pub mqtt_pub;
            if (!az_succeeded(rc = az_dps_query(dps_client, dps_result.operation_id, &mqtt_pub)))
            {
                printf("Failed to create DPS register device request: %d\n", rc);
                exit(EXIT_FAILURE);
            }

            pubmsg.payload = az_span_ptr(mqtt_pub.payload);
            pubmsg.payloadlen = az_span_length(mqtt_pub.payload);
            pubmsg.qos = mqtt_pub.qos;
            pubmsg.retained = 0;
            MQTTClient_publishMessage(client, az_span_ptr(mqtt_pub.topic), &pubmsg, &token);
        }
    }
    else if (ret != az_result_iot_not_handled)
    {
        // TODO: Question: how many separate error cases do we need?
        if (ret == az_iot_service_error)
        {
            printf("DPS Server error: [%d] %s\n", dps_result.error_code, dps_result.error_message);
        }
        else
        {
            printf("Critical error trying to handle DPS message %d\n", ret);
        }

        exit(EXIT_FAILURE);
    }
  
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void connlost(void *context, char *cause)
{
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);

    // TODO: retry
}

int main(int argc, char* argv[])
{
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int rc;
    int ch;

    az_iot_mqtt_connect mqtt_connect;

    MQTTClient_create(&client, AZ_PROVISIONING_GLOBAL_HOST_NAME, AZ_PROVISIONING_REGISTRATION_ID,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);

    // 1. Get Az DPS MQTT credentials
    
    az_dps_client_init(dps_client, AZ_PROVISIONING_REGISTRATION_ID);

    if (!az_succeeded(rc = az_dps_get_connect_information(
                        dps_client,
                        AZ_PROVISIONING_IDSCOPE, 
                        &mqtt_connect)))
    {
        printf("Error getting DPS Credentials %d\n", rc);
        exit(EXIT_FAILURE);
    }

    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.username = mqtt_connect.user;
    conn_opts.password = mqtt_connect.password;

    MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }

    // 2. Get Topic and subscribe
    az_iot_topic dps_topic;

    if (!az_succeeded(rc = az_dps_register_get_subscribe_topic(dps_client, &dps_topic)))
    {
        printf("Failed to get topics %d\n", rc);
        exit(EXIT_FAILURE);
    }

    // NOTE: name is NULL terminated even if it's just a span.
    printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", az_span_ptr(dps_topic.name), AZ_PROVISIONING_IDSCOPE, dps_topic.qos);
    MQTTClient_subscribe(client, az_span_ptr(ps_topic.name), dps_topic.qos);

    // 3. Call RegisterDevice
    az_iot_mqtt_pub mqtt_pub;
    if (!az_succeeded(rc = az_dps_register(dps_client, OPT_CERT_SIGNING_REQUEST, OPT_PNP_DCMID, &mqtt_pub)))
    {
        printf("Failed to create DPS register device request: %d\n", rc);
        exit(EXIT_FAILURE);
    }

    pubmsg.payload = az_span_ptr(mqtt_pub.payload);
    pubmsg.payloadlen = az_span_length(mqtt_pub.payload);
    pubmsg.qos = mqtt_pub.qos;
    pubmsg.retained = 0;
    MQTTClient_publishMessage(client, az_span_ptr(mqtt_pub.topic), &pubmsg, &token);
    printf("Waiting for up to %d seconds for publication of %s\n"
            "on topic %s for client with ClientID: %s\n",
            (int)(TIMEOUT/1000), PAYLOAD, TOPIC, CLIENTID);
    rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
    printf("Message with delivery token %d delivered\n", token);

    // DPS flow moves on the callback threads.

    do 
    {
        ch = getchar();
    } while(ch!='Q' && ch != 'q');

    return rc;
}
