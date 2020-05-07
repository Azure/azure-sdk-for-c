# Azure IoT Clients for Embedded C

Official Embedded C clients for Azure IoT Hub and Device Provisioning Service.

## Getting started

TBD

### Prerequisites

TBD

## Examples

### IoT Hub Client

To use IoT Hub connectivity, the first action by a developer should be to initialize the
client with the `az_iot_hub_client_init()` API. Once that is initialized, you may use the
`az_iot_hub_client_get_user_name()` and `az_iot_hub_client_get_client_id()` to get the
user name and client id to establish a connection with IoT Hub.

An example use case is below.

```C
//FOR SIMPLICITY THIS DOES NOT HAVE ERROR CHECKING. IN PRODUCTION ENSURE PROPER ERROR CHECKING.

az_iot_hub_client my_client;
static az_span my_iothub_hostname = AZ_SPAN_LITERAL_FROM_STR("constoso.azure-devices.net");
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
  az_iot_hub_client_get_client_id(&my_client, my_mqtt_client_id, 
                sizeof(my_mqtt_client_id), &my_mqtt_client_id_length);

  //At this point you are free to use my_mqtt_client_id and my_mqtt_user_name to connect using
  //your MQTT client.
}
```

### Properties

Included in the IoT SDK are helper functions to form and manage properties for IoT Hub services. Implementation starts by using the `az_iot_hub_client_properties_init()` API. The user is free to intitialize using an empty, but appropriately sized, span to later append properties or an already populated span containing a properly formated property buffer. "Properly formatted" properties follow the form `{key}={value}&{key}={value}`.

Below is an example use case of appending properties.

```C
//FOR SIMPLICITY THIS DOES NOT HAVE ERROR CHECKING. IN PRODUCTION ENSURE PROPER ERROR CHECKING.
void my_property_func()
{
  //Allocate a span to put the properties
  uint8_t property_buffer[64];
  az_span property_span = az_span_init(property_buffer, sizeof(property_buffer));
  
  //Initialize the property struct with the span
  az_iot_hub_client_properties props;
  az_iot_hub_client_properties_init(&props, property_span, 0);
  //Append properties
  az_iot_hub_client_properties_append(&props, AZ_SPAN_FROM_STR("key"), AZ_SPAN_FROM_STR("value"));
  //At this point, you are able to pass the `props` to other API's with property parameters.
}
```

Below is an example use case of initializing an already populated property span.

```C
//FOR SIMPLICITY THIS DOES NOT HAVE ERROR CHECKING. IN PRODUCTION ENSURE PROPER ERROR CHECKING.
static az_span my_prop_span = AZ_SPAN_LITERAL_FROM_STR("my_device=contoso&my_key=my_value");
void my_property_func()
{
  //Initialize the property struct with the span
  az_iot_hub_client_properties props;
  az_iot_hub_client_properties_init(&props, my_prop_span, az_span_size(my_prop_span));
  //At this point, you are able to pass the `props` to other API's with property parameters.
}
```

### Telemetry

Telemetry functionality can be achieved by sending a user payload to a specific topic. In order to get the appropriate topic to which to send, use the `az_iot_hub_client_telemetry_get_publish_topic()` API. An example use case is below.

```C
//FOR SIMPLICITY THIS DOES NOT HAVE ERROR CHECKING. IN PRODUCTION ENSURE PROPER ERROR CHECKING.

static az_iot_hub_client my_client;
static az_span my_iothub_hostname = AZ_SPAN_LITERAL_FROM_STR("constoso.azure-devices.net");
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

## Need help?

* File an issue via [Github Issues](https://github.com/Azure/azure-sdk-for-c/issues/new/choose).
* Check [previous questions](https://stackoverflow.com/questions/tagged/azure+c) or ask new ones on StackOverflow using
  the `azure` and `c` tags.

## Contributing

If you'd like to contribute to this library, please read the [contributing guide][azure_sdk_for_c_contributing] to learn more about how to build and test the code.

### License

Azure SDK for Embedded C is licensed under the [MIT][azure_sdk_for_c_license] license.

<!-- LINKS -->
[azure_sdk_for_c_contributing]: https://github.com/Azure/azure-sdk-for-c/blob/master/CONTRIBUTING.md
[azure_sdk_for_c_license]: https://github.com/Azure/azure-sdk-for-c/blob/master/LICENSE