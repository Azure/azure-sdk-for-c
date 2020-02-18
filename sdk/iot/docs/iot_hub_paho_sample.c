#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTClient.h"
#include "az_iot_client.h"

#define TIMEOUT     4 * 60 * 1000L // Recommended 4 minute timeouts for all Azure IoT Services.

az_iot_client iot_client;

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

    az_iot_c2d_request c2d_request;
    az_iot_method_request method_request;
    az_iot_twin_response twin_response;
    
    // TODO: There are 2 failure modes: az_result_iot_not_handled which is expected and other errors.
    // Proposed solutions:
    // 1. use logging from the SDK
    // 2. create an app function az_iot_handled (similar to az_succeded but that prints out errors)
    if (az_succeeded(ret = az_iot_c2d_handle(pub_received, &c2d_request)))
    {
        // c2d_request contains .payload and .properties / count
    }
    else if (az_succeeded(ret = az_iot_methods_handle(pub_received, &method_request)))
    {
        printf("Method called: %s with payload_size: %d\n", 
            az_span_ptr(method_request.name), 
            az_span_length(method_response.payload));

        az_iot_method_response method_response;
        method_response.status = 200;
        method_response.rid = method_request.rid;
        method_response.payload = NULL;

        az_iot_mqtt_pub mqtt_pub;

        if (!az_succeeded(rc = az_iot_methods_send_response(iot_client, method_response, &mqtt_pub)))
        {
            printf("Failed to create method response: %d\n", rc);
            exit(EXIT_FAILURE);
        }

        pubmsg.payload = az_span_ptr(mqtt_pub.payload);
        pubmsg.payloadlen = az_span_length(mqtt_pub.payload);
        pubmsg.qos = mqtt_pub.qos;
        pubmsg.retained = 0;
        MQTTClient_publishMessage(client, az_span_ptr(mqtt_pub.topic), &pubmsg, &token);
    }
    else if (az_succeeded(ret = az_iot_twin_handle(pub_received, &twin_response)))
    {
        // process twin update.
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

    // Note: When Device Provisioning is used, AZ_IOT_HUB_HOST_NAME and AZ_DEVICE_ID will be 
    //       replaced with the provisioning result values:
    //       - dps_result.registration_state.assigned_hub
    //       - dps_result.registration_state.device_id

    MQTTClient_create(&client, AZ_IOT_HUB_HOST_NAME, AZ_DEVICE_ID,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);

    // 1. Get Az IoT MQTT credentials

    // Create client
    az_iot_client_init(iot_client, AZ_DEVICE_ID))
    
    if (!az_succeeded(rc = az_iot_get_connect_information(
                        iot_client,
                        AZ_IOT_HUB_HOST_NAME,
                        &mqtt_connect)))
    {
        printf("Error getting IoT Credentials %d\n", rc);
        exit(EXIT_FAILURE);
    }

    // Note: supported _only_ by the async client:
    // conn_opts.automaticReconnect = 1;
    conn_opts.connectTimeout = TIMEOUT; // Recommended 4minute timeout.
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

    // 2. Get Topics + QoSs
    az_iot_topic iot_topics[3];

    if (!az_succeeded(rc = az_iot_get_c2d_subscribe_topic(iot_client, &(iot_topics[0]))))
    {
        printf("Failed to get C2D topic filter %d\n", rc);
        exit(EXIT_FAILURE);
    }
    
    if (!az_succeeded(rc = az_iot_get_methods_subscribe_topic(iot_client, &(iot_topics[1]))))
    {
        printf("Failed to get Method topic filter %d\n", rc);
        exit(EXIT_FAILURE);
    }

    if (!az_succeeded(rc = az_iot_get_twin_subscribe_topic(iot_client, &(iot_topics[2]))))
    {
        printf("Failed to get Twin topic filter %d\n", rc);
        exit(EXIT_FAILURE);
    }

    // NOTE: name is NULL terminated even if it's just a span.
    printf("Subscribing to topics\n");

    char* topics[3];
    int qoss[3];
    topics[0] = az_span_ptr(iot_topics[0].name);
    topics[1] = az_span_ptr(iot_topics[1].name);
    topics[2] = az_span_ptr(iot_topics[2].name);
    qoss[0] = iot_topics[0].qos;
    qoss[1] = iot_topics[1].qos;
    qoss[2] = iot_topics[2].qos;

    MQTTClient_subscribeMany(client, 3, topics, qoss);

    az_iot_mqtt_pub mqtt_pub;

    // Send 5 messages.
    for (int i = 0; i < 5; i++)
    {

        az_iot_telemetry_property properties[1];
        properties[0].name = AZ_SPAN_LITERAL_FROM_STR("my_property_name");
        properties[0].value = AZ_SPAN_LITERAL_FROM_STR("my_property_value");

        az_span payload = AZ_SPAN_LITERAL_FROM_STR("Hello IoT world");

        if (!az_succeeded(rc = az_iot_sendtelemetry(iot_client, payload, properties, ARRAY_SIZE(properties), &mqtt_pub)))
        {
            printf("Failed to create telemetry message: %d\n", rc);
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
    }

    do 
    {
        ch = getchar();
    } while(ch!='Q' && ch != 'q');

    return rc;
}
