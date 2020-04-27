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
`az_iot_hub_client_get_user_name()` and `az_iot_hub_client_client_id_get()` to get the
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
  az_iot_hub_client_client_id_get(&my_client, my_mqtt_client_id, 
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

For details on contributing to this repository, see the [contributing guide](https://github.com/Azure/azure-sdk-for-c/blob/master/CONTRIBUTING.md).

This project welcomes contributions and suggestions. Most contributions require you to agree to a Contributor License
Agreement (CLA) declaring that you have the right to, and actually do, grant us the rights to use your contribution. For
details, visit https://cla.microsoft.com.

When you submit a pull request, a CLA-bot will automatically determine whether you need to provide a CLA and decorate
the PR appropriately (e.g., label, comment). Simply follow the instructions provided by the bot. You will only need to
do this once across all repositories using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact
[opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

### Additional Helpful Links for Contributors

Many people all over the world have helped make this project better.  You'll want to check out:

* [What are some good first issues for new contributors to the repo?](https://github.com/azure/azure-sdk-for-c/issues?q=is%3Aopen+is%3Aissue+label%3A%22up+for+grabs%22)
* [How to build and test your change](CONTRIBUTING.md#developer-guide)
* [How you can make a change happen!](CONTRIBUTING.md#pull-requests)
* Frequently Asked Questions (FAQ) and Conceptual Topics in the detailed [Azure SDK for C wiki][azure_sdk_for_c_wiki].

### Community

* Chat with other community members [![Join the chat at https://gitter.im/azure/azure-sdk-for-c](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/azure/azure-sdk-for-c?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

### Reporting security issues and security bugs

Security issues and bugs should be reported privately, via email, to the Microsoft Security Response Center (MSRC) <secure@microsoft.com>. You should receive a response within 24 hours. If for some reason you do not, please follow up via email to ensure we received your original message. Further information, including the MSRC PGP key, can be found in the [Security TechCenter](https://www.microsoft.com/msrc/faqs-report-an-issue).

### License

Azure IoT SDK for Embedded C is licensed under the [MIT](LICENSE) license.
