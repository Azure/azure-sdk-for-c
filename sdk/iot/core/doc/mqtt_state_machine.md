# Azure IoT SDK MQTT State Machine

## High-level architecture

Device Provisioning and IoT Hub service protocols require additional state management on top of the MQTT protocol. The Azure IoT SDK for C provides a common programming model layered on an MQTT client selected by the application developer.

The following aspects are being handled by the SDK:
1. Generate MQTT CONNECT credentials.
1. Obtain SUBSCRIBE topic filters and PUBLISH topic strings required by various service features.
1. Parse service errors and output an uniform error object model. 
1. Provide the correct sequence of events required to perform an operation. 
1. Provide suggested timing information when retrying operations.

The following aspects need to be handled by the application or convenience layers:
1. Ensure secure TLS communication using either server or mutual X509 authentication.
1. Perform MQTT transport-level operations.
1. Delay execution for retry purposes.
1. (Optional) Provide real-time clock information and perform HMAC-SHA256 operations for SAS token generation.

## Components

### IoT Hub
![image](resources/iot_hub_flow.png "IoT MQTT State Machine")

### Device Provisioning Service
![image](resources/iot_provisioning_flow.png "Device Provisioning MQTT State Machine")

## Design Decisions
Porting requirements:
- The target platform C99 compiler is generating reentrant code. The target platform supports a stack of several kB.
- Our SDK relies on types such as `uint8_t` existing. If a platform doesn't already have them defined, they should be added to the application code using `typedef` statements.

The SDK is provided only for the MQTT protocol. Support for WebSocket/WebProxy tunneling as well as TLS security is not handled by the SDK.

Assumptions and recommendations for the application:
- Our API does not support unsubscribing from any of the previously subscribed topics. We assume that the device will only set-up topics that must be used.

## API

### Connecting

The application code is required to initialize the TLS and MQTT stacks.
Two authentication schemes are currently supported: _X509 Client Certificate Authentication_ and _Shared Access Signature_ authentication. 

When X509 client authenticaiton is used, the MQTT password field should be an empty string.

If SAS tokens are used the following APIs provide a way to create as well as refresh the lifetime of the used token upon reconnect.

_Example:_
```C
if(az_failed(az_iot_hub_client_sas_signature_get(client, unix_time + 3600, signature, &signature)));
{
    // error.
}

// Base64Encode the HMAC256 of the az_span_ptr(signature) with the Shared Access Key.

if(az_failed(az_iot_hub_client_sas_password_get(client, base64_hmac_sha256_signature, NULL, password, &password)))
{
    // error.
}

// Use az_span_ptr(password) and az_span_len(password).
```

### Subscribe to topics
Each service requiring a subscription implements a function similar to the following:

_Example:_
```C

if(az_failed(az_iot_hub_client_c2d_subscribe_topic_filter_get(client, mqtt_topic_filter, &mqtt_topic_filter))
{
    // error.
}

// Use az_span_ptr(mqtt_topic_filter) and az_span_len(mqtt_topic_filter).
```

### Sending APIs

Each action (e.g. send telemetry, request twin) is represented by a separate public API.
The application is responsible for filling in the MQTT payload with the format expected by the service.

_Example:_
```C
if(az_failed(az_iot_hub_client_telemetry_publish_topic_get(client, NULL, mqtt_topic, &mqtt_topic)))
{
    // error.
}

// Use az_span_ptr(mqtt_topic) and az_span_len(mqtt_topic) to create an MQTT Publish message.
```

### Receiving APIs

We recommend that the handling of incoming MQTT PUB messages is implemented by a delegating handler architecture. Each handler is passed the topic and will either accept and return a response or pass it to the next handler.

_Example:_

```C
    az_iot_hub_client_c2d_request c2d_request;
    az_iot_hub_client_method_request method_request;
    az_iot_hub_client_twin_response twin_response;
    
    //az_span received_topic is filled by the application.

    if (az_succeeded(az_iot_hub_client_c2d_received_topic_parse(client, received_topic, &c2d_request)))
    {
        // This is a C2D message: 
        //  c2d_request.properties contain the properties of the message.
        //  the MQTT message payload contains the data.
    }
    else if (az_succeeded(ret = az_iot_hub_client_methods_received_topic_parse(client, received_topic, &method_request)))
    {
        // This is a Method request:
        //  method_request.name contains the method
        //  method_request.request_id contains the request ID that must be used to submit the response using az_iot_hub_client_methods_response_publish_topic_get()
    }
    else if (az_succeeded(ret = az_iot_hub_client_twin_received_topic_parse(client, received_topic, &twin_response)))
    {
        // This is a Twin operation.
        switch (twin_response.response_type)
        {
            case AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_GET:
                // This is a response to a az_iot_hub_client_twin_get_publish_topic_get.
                break;
            case AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_DESIRED_PROPERTIES:
                // This is received as the Twin desired properties were changed using the service client.
                break;
            case AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_REPORTED_PROPERTIES:
                // This is a response received after patching the reported properties using az_iot_hub_client_twin_patch_publish_topic_get().
                break;
            default:
                // error.
        }
    }
```
