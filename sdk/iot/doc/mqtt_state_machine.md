# Azure IoT Client MQTT State Machine

## High-level architecture

Device Provisioning and IoT Hub service protocols require additional state management on top of the MQTT protocol. The Azure IoT Hub and Provisioning clients for C provide a common programming model. The clients must be layered on top of an MQTT client selected by the application developer.

The following aspects are being handled by the IoT Clients:

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

For more information about Azure IoT services using MQTT see [this article](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-mqtt-support).

## Components

### IoT Hub

![iot_hub_flow](https://www.plantuml.com/plantuml/png/0/dLTTRzis57tNhxXlJOTvAuAbG8qYgE84SzQayM8L650K12tTM4GaKYMff5wn_pvI7vRZYgBbF1W1UUwvXuVKvUK7fQdKPPu5WNfalA2Ivc36DCAF0CpV1OqVrrAKF9c9JCZtxyF5ulmSpE_DzIoChky-zSEMqVtdMMIGD_G9UO8o-C-ag8XZYo2nI1XVDrdgYUNbHn8khlT6BiCe97SNVrtD8kXuTq0xMiZixl8A6f4suokuvam7Ntxv2fF9ET1_2HEQfEMAn1b3hWapqrDOHk5z5FJFGczSgL0IJdCaQzI1PxwVWYMEs7gY2YgAZCLKCy59_arhunhd61l5_vozUGUp2WzXgTCf55IfPo4JO0emB_IciTl5iRN9c7HNs6cQmJ9SWXQFo66D76KLDHsM-DESeeeejkPKbx9U_hJYfKPcS0XyR-5fJkFJixT1pvXRGEk6m-9zI7qwN4wzD9TsBlLOt2hRSJV237FKSXCuC_GVJjq6ag9SPKobH8k2nOsdMrg8AOSLWZbp2Jp_Ba2LAnLBjh8Vc1P0-GQgmD-2SJbwJMbWTc6pqyknIufF4zBAmoRL0ykq-LhwTseue5_Y5cz5tg9EHQB62ithi75Y_K5UmMvIYQeGNE4H6nKzCpxwy9cOBkSe6muDzPjnXEI2wZWzGgylpYisS4yEwJNKF-zeC0RisVxZPDLs72pC0VUN497dWaDde-FIwZItXNLeMJqv9SR8dj3KpTk_emWojkgI9e4hRQEqg-OENClUzaS_mAwDPjdAD4QGc1gxDcXJZbFAampbjEfVt6s9hNeDPzAuQNepPUNVDyEm-7nnSxsSGuvAqJLMZ2EOXikU8paN6WmZICoC3uaZvo39OBLn9cqeFOdTB3QhjRjcixWjLNEHZRNFlMtVIbJsWhP6nUOCG4jGvUYkfEbHzhbtRITrFT2JtHkx_-tKi_MntMcwdPZjfr9bSw8yXyzrqxKELXVQw3FWlUQCSzNH-FCg0Y5X4KQpUUTV_PuG4SpB5TJXKzUZOGeJQCTJeRHz2WnX4tnYCPg-El0-9dQehI3RZ4Qs6LN1ORtu14hElXFDUXNjTyOMVq4LlltZP7uRpYRBUMZQsvCjwQ-mJAd498nZKuS-ukRrwo0OsyfEUZQcaA90gHcgRTNrdTsTvw4FQtCs_rhJ3u5NMtXKVtPWCX-5TQ4SS6Et-WnQyR9C-SmVrOhiBNLik-1roXlD7aXUJJ-XLARkkErw-UKF6hPV9Ap8PgVqfi038tpBUW5lDliGw7Qbho3i2YumRZhN1se-os3WPMioeJGCvFlPctT4orAPEv588VHmr9T6JXKMsxFoOB7ujVUTRYCUSGCdETtOf-epqwcfi-RTEb_UceBn0NbIvjd_ "iot_hub_flow")

### Device Provisioning Service

![iot_provisioning_flow](https://www.plantuml.com/plantuml/png/0/fLTXRzCy4FwUNt5yeaCK8VI2z5OmaNMJjXVshYn1GaB8SfDhQfROmNRA2kA_SqwJDkhI9ktsePFivvwxUto-SzuRoxKjijJpp9sGETSy0rgp2ByzeByp7jqbMXKo7gjKwRTl_Sdaz1Jez-FLs0-lRlvlXmNXxUTnICYFV84oHktz8HSbSUGGB5Ana4dbPRKHd7zW_kHgQ-NS3trsS_RVnJY43My3r8Y5ZM_EBw0Aqdby2bTIME_hisym79u2_yM4iYpNQY6CK98YO54gK5ec23U28FHlGwy3raLL6DHCyWnPWjRh2-mCC2vH8_Zo-kM_m0ixh_JaUPwAY5k2i-o7jIIjb1ZPjGxTfANh7JHEn9lSBVCK0Zy0g-vGGe8ITOaUzN6mDpN33JCYaTmM6giidQSooOFijlAOPpnwzVgDrxBM8wIS6_DJwRZh6BUnU-Mar6fio_dLmTL7Hze7nTH4MapHsqdPKJaQ4s4ibPgfgPXLkOZOJAHkkIEz7kRj6Q_nkmfr3TYKwcv8cmUD4Oe5agRNdy8GKZ65jK7inBkjOjzBt5Ezl9YcmiohvFkAzi2gNQiAjakfQdjJ5pjmjI1rd_uD94AfpUJp6LIxSTayU_J46by5d_DbgdXC_xN1qKDQc6cLmLHJVz0mY59LnA0qNCG9dfGkZtJL-uGqcbn9WuzsvhGPWLGMckQKQ4ggKs5rycy5r21r1rSOUFSEg0jGxwfEus2svj775DTom_rammq38QEqA4SjQgsqEMbCpeSSTNsL8JtOxJW7Acwcycegi5KmBRUc8zr-BFjlKJdv9ysZMv6FEaVx28QGf6hAKqRFbbm9sfWhOzsKxcLC0yz457ylyEfegrH5sR7St0lTX3NUWHDpk9hC3EzpKL4MKjmpAoXqxzZhiEAbzKSnL2lhldL8jJSUVGVPER-hyZdSJkEF0erxlXnZQ_MIyLavTWutRbH4ATOZEOISAPrrFzJQg5wy6Aux2PiNqrutOHTHZ0iHuH6AEkFlXbAfY-W8-xBtkMyEFDuwGsDuilygRJy5tN6SSODl_XbSNllZON3fq_DkiBwwpo6OSusn7qN4_W6N9oTx-NisnwqIfhRYRrebK4z3FUFaqTlpIgNBWtf1OjpJzmQpkY1bOgQLilkjlbQXbcOHQYjc3elVlBQHjNLTMfx23dE7InXaP43FjP_2peK4Bk4ok2QrtbEVfW_Olm00 "iot_provisioning_flow")

## Porting the IoT Clients

In order to port the clients to a target platform the following items are required:

- Support for a C99 compiler.
- Types such as `uint8_t` must be defined.
- The target platform supports a stack of several kB (actual requirement depends on features being used and data sizes).
- An MQTT over TLS client supporting QoS 0 and 1 messages.

Optionally, the IoT services support MQTT tunneling over WebSocket Secure which allows bypassing firewalls where port 8883 is not open. Using WebSockets also allows usage of devices that must go through a WebProxy. Application developers are responsible with setting up the wss:// tunnel.

## API

### Connecting

The application code is required to initialize the TLS and MQTT stacks.
Two authentication schemes are currently supported: _X509 Client Certificate Authentication_ and _Shared Access Signature_ authentication. 

When X509 client authentication is used, the MQTT password field should be an empty string.

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

We recommend that the handling of incoming MQTT PUB messages is implemented by a chain-of-responsibility architecture. Each handler is passed the topic and will either accept and return a response, or pass it to the next handler.

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

### Retrying Operations

Retrying operations requires understanding two aspects: error evaluation (did the operation fail, should the operation be retried) and retry timing (how long to delay before retrying the operation). The IoT client library is supplying optional APIs for error classification and retry timing.

#### Error policy

The SDK will not handle protocol-level (WebSocket, MQTT, TLS or TCP) errors. The application-developer is expected to classify and handle errors the following way:

- Operations failing due to authentication errors should not be retried.
- Operations failing due to communication-related errors other than ones security-related (e.g. TLS Alert) may be retried.

Both IoT Hub and Provisioning services will use `MQTT CONNACK` as described in Section 3.2.2.3 of the [MQTT v3 specification](http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Table_3.1_-).

##### IoT Service Errors

APIs using `az_iot_status` report service-side errors to the client through the IoT protocols.

The following APIs may be used to determine if the status indicates an error and if the operation should be retried:

```C
az_iot_status status = response.status;
if (az_iot_is_success_status(status))
{
    // success case
}
else
{
    if (az_iot_is_retriable_status(status))
    {
        // retry
    }
    else
    {
        // fail
    }
}
```

#### Retry timing

Network timeouts and the MQTT keep-alive interval should be configured considering tradeoffs between how fast network issues are detected vs traffic overheads. [This document](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-mqtt-support#default-keep-alive-timeout) describes the recommended keep-alive timeouts as well as the minimum idle timeout supported by Azure IoT services.

For connectivity issues at all layers (TCP, TLS, MQTT) as well as cases where there is no `retry-after` sent by the service, we suggest using an exponential back-off with random jitter function. `az_iot_retry_calc_delay` is available in Azure IoT Common:

```C
// The previous operation took operation_msec.
// The application calculates random_msec between 0 and max_random_msec.

int32_t delay_msec = az_iot_retry_calc_delay(operation_msec, attempt, min_retry_delay_msec, max_retry_delay_msec, random_msec);
```

_Note 1_: The network stack may have used more time than the recommended delay before timing out. (e.g. The operation timed out after 2 minutes while the delay between operations is 1 second). In this case there is no need to delay the next operation.

_Note 2_: To determine the parameters of the exponential with back-off retry strategy, we recommend modeling the network characteristics (including failure-modes). Compare the results with defined SLAs for device connectivity (e.g. 1M devices must be connected in under 30 minutes) and with the available [IoT Azure scale](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-devguide-quotas-throttling) (especially consider _throttling_, _quotas_ and maximum _requests/connects per second_).

In the absence of modeling, we recommend the following default:

```C
    min_retry_delay_msec =   1000;
    max_retry_delay_msec = 100000;
    max_random_msec      =   5000;
```

For service-level errors, the Provisioning Service is providing a `retry-after` (in seconds) parameter:

```C
// az_iot_provisioning_client_received_topic_payload_parse was successful and created a az_iot_provisioning_client_register_response response

int32_t delay_ms;
if ( response.retry_after_seconds > 0 )
{
    delay_ms = response.retry_after_seconds;
}
else
{
    delay_ms = az_iot_retry_calc_delay(operation_msec, attempt, min_retry_delay_msec, max_retry_delay_msec, random_msec);
}
```

#### Suggested Retry Strategy

Combining the functions above we recommend the following flow:

![iot_retry_flow](https://www.plantuml.com/plantuml/png/0/bLN1QiCm3BtxAtGR3F83PPHsWR50QKkQdSO8Rk9cfguTRAVGZVtxx2u9JQEaQmuvw3tvINeMdXbBjQqEWfWzboNLz00kP1by4t3VCytsXLQLp4Cbb7vwcg_Nqocu_o8AvqcqMkAqJHA_XObZBkYHoPnfdFOoZnQEjD9K5epy4FB--051C8L89UbQgoCtN4akYpHc1JVMdVDNtI0ETOc4FC0b0M9cDQTRmO1fhRGXmqVu0Zg8RdBJ7UMYqgokOfpKj4V6QNsvZ8gi4auWpPcW9p86zhDfMTPvI94js8m9HszuC5RG10AWNobNzjpgwOp_en6VTqu8wCvhNFktL39ePmVtk2VhKcuwY19n5r5gNpfMNYKDDSkPj9mAW-dsMO3QW_3ky6aWss_S1AG2PJ_TzB8M9ZTcxD7NQfkkIgQfTGntpARACcRChXAT3Y-MjfjniQmEm7Uqc-6LVMJ8K1HhFKJHziphlKDYBVTwopgnfgrE41yQ9ZAsGlMaa6t0vpHs-GVnpho6h_hm_PWuAuEoWGYPkJV0FJA_ "iot_retry_flow")

When devices are using IoT Hub without Provisioning Service, we recommend attempting to rotate the IoT Credentials (SAS Token or X509 Certificate) on authentication issues.

_Note:_ Authentication issues observed in the following cases do not require credentials to be rotated:

- DNS issues (such as WiFi Captive Portal redirects)
- WebSockets Proxy server authentication
