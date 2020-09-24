# Azure IoT Clients

This is the main page for the Azure SDK for Embedded C official IoT client libraries. 

Here you will find everything you need to get devices connected to Azure. 

## Table of Contents

- [Azure IoT Clients](#azure-iot-clients)
  - [Table of Contents](#table-of-contents)
  - [Getting Started](#getting-started)
    - [Docs](#docs)
    - [Build](#build)
    - [Samples](#samples)
    - [Prerequisites](#prerequisites)
  - [Key Features](#key-features)
  - [Examples](#examples)
    - [IoT Hub Client Initialization](#iot-hub-client-initialization)
    - [Properties](#properties)
    - [Telemetry](#telemetry)
    - [IoT Hub Client with MQTT Stack](#iot-hub-client-with-mqtt-stack)
  - [Need Help?](#need-help)
  - [Contributing](#contributing)
    - [License](#license)

## Getting Started

The Azure IoT Client library is created to facilitate connectivity to Azure IoT services alongside an MQTT and TLS stack of the user's choice. This means that this SDK is **NOT** a platform but instead is a true SDK library.

![Methods](./resources/embc_high_level_arch.png)

From a functional perspective, this means that the user's application code (not the SDK) calls directly to the MQTT stack of their choice. The SDK provides utilities (in the form of functions, default values, etc) which help make the connection and feature set easier. Some examples of those utilities include:

- Publish topics to which messages can be sent and subscription topics to which users can subscribe for incoming messages.
- Functions to parse incoming message topics which populate structs with crucial message information.
- Default values for MQTT connect keep alive and connection port.

A full list of features can be found in the doxygen docs listed below in [Docs](#docs).

**Note**: this therefore requires a different programming model as compared to the earlier version of the C SDK ([found here](https://github.com/Azure/azure-iot-sdk-c)). To better understand the responsibilities of the user application code and the SDK, please take a look at the [State Machine diagram](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/docs/iot/mqtt_state_machine.md) that explains the high-level architecture, SDK components, and a clear view of SDK x Application responsibilities.

### Docs

For API documentation, please see the doxygen generated docs [here][azure_sdk_for_c_doxygen_docs]. You can find the IoT specific docs by navigating to the **Files -> File List** section near the top and choosing any of the header files prefixed with `az_iot_`.

### Build

The Azure IoT library is compiled following the same steps listed on the root [README](https://github.com/Azure/azure-sdk-for-c/blob/master/README.md) documentation, under ["Getting Started Using the SDK"](https://github.com/Azure/azure-sdk-for-c/blob/master/README.md#getting-started-using-the-sdk).

The library targets made available via CMake are the following:

- `az::iot::hub` - For Azure IoT Hub features ([API documentation here][azure_sdk_for_c_doxygen_hub_docs])
- `az::iot::provisioning` - For Azure IoT Provisioning features ([API documentation here][azure_sdk_for_c_doxygen_provisioning_docs])

### Samples

[This page](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/samples/iot/README.md) explains samples for the Azure Embedded C SDK IoT Hub Client and the Provisioning Clients and how to use them.

 For step-by-step guides starting from scratch, you may refer to these documents:

- Linux: [How to Setup and Run Azure SDK for Embedded C IoT Hub Samples on Linux](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/samples/iot/docs/how_to_iot_hub_samples_linux.md)

- Windows: [How to Setup and Run Azure SDK for Embedded C IoT Hub Samples on Microsoft Windows](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/samples/iot/docs/how_to_iot_hub_samples_windows.md).

- ESP8266: [How to Setup and Run Azure SDK for Embedded C IoT Hub Client on Esp8266 NodeMCU](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/samples/iot/docs/how_to_iot_hub_esp8266_nodemcu.md)

  **Important Note on Linux and Windows Samples**: While Windows and Linux devices are not likely to be considered as constrained ones, these samples were created to make it simpler to test the Azure SDK for Embedded C libraries, debug and step through the code to learn about it, even without a real device. We understand not everyone will have a real device to test and - sometimes - these devices won't have debugging capabilities. 

For extra guidance, please feel free to watch our Deep Dive Video below which goes over building the SDK, running the samples, and the architecture of the samples.

[![Watch the video](./resources/deep_dive_screenshot.png)](https://youtu.be/qdb3QIq8msg)

### Prerequisites

For compiling the Azure SDK for Embedded C for the most common platforms (Windows and Linux), no further prerequisites are necessary.
Please follow the instructions in the [Getting Started](#Getting-Started) section above.
For compiling for specific target devices, please refer to their specific toolchain documentation.

## Key Features

&radic; feature available  &radic;* feature partially available (see Description for details)  &times; feature planned but not supported

Feature | Azure SDK for Embedded C | Description
---------|----------|---------------------
 [Send device-to-cloud message](https://docs.microsoft.com/azure/iot-hub/iot-hub-devguide-messages-d2c) | &radic; | Send device-to-cloud messages to IoT Hub with the option to add custom message properties.
 [Receive cloud-to-device messages](https://docs.microsoft.com/azure/iot-hub/iot-hub-devguide-messages-c2d) | &radic; | Receive cloud-to-device messages and associated properties from IoT Hub.
 [Device Twins](https://docs.microsoft.com/azure/iot-hub/iot-hub-devguide-device-twins) | &radic; | IoT Hub persists a device twin for each device that you connect to IoT Hub.  The device can perform operations like get twin document, subscribe to desired property updates.
 [Direct Methods](https://docs.microsoft.com/azure/iot-hub/iot-hub-devguide-direct-methods) | &radic; | IoT Hub gives you the ability to invoke direct methods on devices from the cloud.  
 [DPS - Device Provisioning Service](https://docs.microsoft.com/azure/iot-dps/) | &radic; | This SDK supports connecting your device to the Device Provisioning Service via, for example, [individual enrollment](https://docs.microsoft.com/azure/iot-dps/concepts-service#enrollment) using an [X.509 leaf certificate](https://docs.microsoft.com/azure/iot-dps/concepts-security#leaf-certificate).  
 Protocol | MQTT | The Azure SDK for Embedded C supports only MQTT.  
 Retry Policies | &radic;* | The Azure SDK for Embedded C provides guidelines for retries, but actual retries should be handled by the application.
 [IoT Plug and Play](https://docs.microsoft.com/azure/iot-pnp/overview-iot-plug-and-play) | &radic; | IoT Plug and Play Preview enables solution developers to integrate devices with their solutions without writing any embedded code.

## Examples

### IoT Hub Client Initialization

To use IoT Hub connectivity, the first action by a developer should be to initialize the
client with the `az_iot_hub_client_init()` API. Once that is initialized, you may use the
`az_iot_hub_client_get_user_name()` and `az_iot_hub_client_get_client_id()` to get the
user name and client id to establish a connection with IoT Hub.

An example use case is below.

```C
//FOR SIMPLICITY THIS DOES NOT HAVE ERROR CHECKING. IN PRODUCTION ENSURE PROPER ERROR CHECKING.

az_iot_hub_client my_client;
static az_span my_iothub_hostname = AZ_SPAN_LITERAL_FROM_STR("contoso.azure-devices.net");
static az_span my_device_id = AZ_SPAN_LITERAL_FROM_STR("contoso_device");

//Make sure to size the buffer to fit the user name (100 is an example)
static char my_mqtt_user_name[100];
static size_t my_mqtt_user_name_length;

//Make sure to size the buffer to fit the client id (16 is an example)
static char my_mqtt_client_id_buffer[16];
static size_t my_mqtt_client_id_length;

int main()
{
  //Get the default IoT Hub options
  az_iot_hub_client_options options = az_iot_hub_client_options_default();

  //Initialize the client with hostname, device id, and options
  az_iot_hub_client_init(&my_client, my_iothub_hostname, my_device_id, &options);

  //Get the MQTT user name to connect
  az_iot_hub_client_get_user_name(&my_client, my_mqtt_user_name,
                sizeof(my_mqtt_user_name), &my_mqtt_user_name_length);

  //Get the MQTT client id to connect
                sizeof(my_mqtt_client_id), &my_mqtt_client_id_length);

  //At this point you are free to use my_mqtt_client_id and my_mqtt_user_name to connect using
  //your MQTT client.
}
```

### Properties

Included in the Azure SDK for Embedded C are helper functions to form and manage properties for IoT Hub services. Implementation starts by using the `az_iot_message_properties_init()` API. The user is free to initialize using an empty, but appropriately sized, span to later append properties or an already populated span containing a properly formatted property buffer. "Properly formatted" properties follow the form `{key}={value}&{key}={value}`.

Below is an example use case of appending properties.

```C
//FOR SIMPLICITY THIS DOES NOT HAVE ERROR CHECKING. IN PRODUCTION ENSURE PROPER ERROR CHECKING.
void my_property_func()
{
  //Allocate a span to put the properties
  uint8_t property_buffer[64];
  az_span property_span = az_span_create(property_buffer, sizeof(property_buffer));
  
  //Initialize the property struct with the span
  az_iot_message_properties props;
  az_iot_message_properties_init(&props, property_span, 0);
  //Append properties
  az_iot_message_properties_append(&props, AZ_SPAN_FROM_STR("key"), AZ_SPAN_FROM_STR("value"));
  //At this point, you are able to pass the `props` to other APIs with property parameters.
}
```

Below is an example use case of initializing an already populated property span.

```C
//FOR SIMPLICITY THIS DOES NOT HAVE ERROR CHECKING. IN PRODUCTION ENSURE PROPER ERROR CHECKING.
static az_span my_prop_span = AZ_SPAN_LITERAL_FROM_STR("my_device=contoso&my_key=my_value");
void my_property_func()
{
  //Initialize the property struct with the span
  az_iot_message_properties props;
  az_iot_message_properties_init(&props, my_prop_span, az_span_size(my_prop_span));
  //At this point, you are able to pass the `props` to other APIs with property parameters.
}
```

### Telemetry

Telemetry functionality can be achieved by sending a user payload to a specific topic. In order to get the appropriate topic to which to send, use the `az_iot_hub_client_telemetry_get_publish_topic()` API. An example use case is below.

```C
//FOR SIMPLICITY THIS DOES NOT HAVE ERROR CHECKING. IN PRODUCTION ENSURE PROPER ERROR CHECKING.

static az_iot_hub_client my_client;
static az_span my_iothub_hostname = AZ_SPAN_LITERAL_FROM_STR("contoso.azure-devices.net");
static az_span my_device_id = AZ_SPAN_LITERAL_FROM_STR("contoso_device");

void my_telemetry_func()
{
  //Initialize the client to then pass to the telemetry API
  az_iot_hub_client_init(&my_client, my_iothub_hostname, my_device_id, NULL);

  //Allocate a char buffer with capacity large enough to put the telemetry topic.
  char telemetry_topic[64];
  size_t telemetry_topic_length;

  //Get the NULL terminated topic and put in telemetry_topic to send the telemetry
  az_iot_hub_client_telemetry_get_publish_topic(&my_client, NULL, telemetry_topic,
                                    sizeof(telemetry_topic), &telemetry_topic_length);
}
```

### IoT Hub Client with MQTT Stack

Below is an implementation for using the IoT Hub Client SDK. This is meant to guide users in incorporating their MQTT stack with the IoT Hub Client SDK. Note for simplicity reasons, this code will not compile. Ideally, guiding principles can be inferred from reading through this snippet to create an IoT solution.

```C
#include <az/core/az_result.h>
#include <az/core/az_span.h>
#include <az/iot/az_iot_hub_client.h>

az_iot_hub_client my_client;
static az_span my_iothub_hostname = AZ_SPAN_LITERAL_FROM_STR("<your hub fqdn here>");
static az_span my_device_id = AZ_SPAN_LITERAL_FROM_STR("<your device id here>");

//Make sure the buffer is large enough to fit the user name (100 is an example)
static char my_mqtt_user_name[100];

//Make sure the buffer is large enough to fit the client id (16 is an example)
static char my_mqtt_client_id[16];

//This assumes an X509 Cert. SAS keys may also be used.
static const char my_device_cert[]= "-----BEGIN CERTIFICATE-----abcdefg-----END CERTIFICATE-----";

static char telemetry_topic[128];
static char telemetry_payload[] = "Hello World";

void handle_iot_message(mqtt_client_message* msg);

int main()
{
  //Get the default IoT Hub options
  az_iot_hub_client_options options = az_iot_hub_client_options_default();

  //Initialize the client with hostname, device id, and options
  az_iot_hub_client_init(&my_client, my_iothub_hostname, my_device_id, &options);

  //Get the MQTT user name to connect
  az_iot_hub_client_get_user_name(&my_client, my_mqtt_user_name,
                sizeof(my_mqtt_user_name), NULL);

  //Get the MQTT client id to connect
  az_iot_hub_client_get_client_id(&my_client, my_mqtt_client_id,
                sizeof(my_mqtt_client_id), NULL);

  //Initialize MQTT client with necessary parameters (example params shown)
  mqtt_client my_mqtt_client;
  mqtt_client_init(&my_mqtt_client, my_iothub_hostname, my_mqtt_client_id);

  //Subscribe to c2d messages
  mqtt_client_subscribe(&my_mqtt_client, AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC);

  //Subscribe to device methods
  mqtt_client_subscribe(&my_mqtt_client, AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC);

  //Subscribe to twin patch topic
  mqtt_client_subscribe(&my_mqtt_client, AZ_IOT_HUB_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC);

  //Subscribe to twin response topic
  mqtt_client_subscribe(&my_mqtt_client, AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC);

  //Connect to the IoT Hub with your chosen mqtt stack
  mqtt_client_connect(&my_mqtt_client, my_mqtt_user_name, my_device_cert);

  //This example would run to receive any incoming message and send a telemetry message five times
  int iterations = 0;
  mqtt_client_message msg;
  while(iterations++ < 5)
  {
    if(mqtt_client_receive(&msg))
    {
      handle_iot_message(&msg);
    }

    send_telemetry_message();
  }

  //Disconnect from the IoT Hub
  mqtt_client_disconnect(&my_mqtt_client);

  //Destroy the mqtt client
  mqtt_client_destroy(&my_mqtt_client);
}

void send_telemetry_message()
{
  //Get the topic to send a telemetry message
  az_iot_hub_client_telemetry_get_publish_topic(&client, NULL, telemetry_topic, sizeof(telemetry_topic), NULL);

  //Send the telemetry message with the MQTT client
  mqtt_client_publish(telemetry_topic, telemetry_payload, AZ_HUB_CLIENT_DEFAULT_MQTT_TELEMETRY_QOS);
}

void handle_iot_message(mqtt_client_message* msg)
{
  //Initialize the incoming topic to a span
  az_span incoming_topic = az_span_create(msg->topic, msg->topic_len);

  //The message could be for three features so parse the topic to see which it is for
  az_iot_hub_client_method_request method_request;
  az_iot_hub_client_c2d_request c2d_request;
  az_iot_hub_client_twin_response twin_response;
  if (az_result_succeeded(az_iot_hub_client_methods_parse_received_topic(&client, incoming_topic, &method_request)))
  {
    //Handle the method request
  }
  else if (az_result_succeeded(az_iot_hub_client_c2d_parse_received_topic(&client, incoming_topic, &c2d_request)))
  {
    //Handle the c2d message
  }
  else if (az_result_succeeded(az_iot_hub_client_twin_parse_received_topic(&client, incoming_topic, &twin_response)))
  {
    //Handle the twin message
  }
}

```

## Need Help?

- File an issue via [Github Issues](https://github.com/Azure/azure-sdk-for-c/issues/new/choose).
- Check [previous questions](https://stackoverflow.com/questions/tagged/azure+c) or ask new ones on StackOverflow using
  the `azure` and `c` tags.

## Contributing

If you'd like to contribute to this library, please read the [contributing guide][azure_sdk_for_c_contributing] to learn more about how to build and test the code.

### License

Azure SDK for Embedded C is licensed under the [MIT][azure_sdk_for_c_license] license.

<!-- LINKS -->
[azure_sdk_for_c_contributing]: https://github.com/Azure/azure-sdk-for-c/blob/master/CONTRIBUTING.md
[azure_sdk_for_c_doxygen_docs]: https://azure.github.io/azure-sdk-for-c
[azure_sdk_for_c_doxygen_hub_docs]: https://azuresdkdocs.blob.core.windows.net/$web/c/docs/1.0.0-preview.2/az__iot__hub__client_8h.html
[azure_sdk_for_c_doxygen_provisioning_docs]: https://azuresdkdocs.blob.core.windows.net/$web/c/docs/1.0.0-preview.2/az__iot__provisioning__client_8h.html
[azure_sdk_for_c_license]: https://github.com/Azure/azure-sdk-for-c/blob/master/LICENSE
