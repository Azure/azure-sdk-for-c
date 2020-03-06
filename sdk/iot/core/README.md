# Azure IoT SDK for Embedded C

Official Embedded C libraries for Azure IoT.

## Getting started

TBD

### Prerequisites

TBD

## Examples

### Telemetry

Telemetry functionality can be achieved by sending a user payload to a specific topic. In order to get the appropriate topic to which to send, use the `az_iot_hub_client_telemetry_publish_topic_get()` API. An example use case is below.

```C
static az_iot_hub_client my_client;
static az_span my_iothub_hostname = AZ_SPAN_LITERAL_FROM_STR("constoso.azure-devices.net");
static az_span my_device_id = AZ_SPAN_LITERAL_FROM_STR("contoso_device");

void my_telemetry_func()
{
  //Get the default options for the client
  az_iot_hub_options my_options = az_iot_hub_client_options_default();

  //Initialize the client to then pass to the telemetry API
  az_iot_hub_client_init(&my_client, my_iothub_hostname, my_device_id, &my_options);

  //Allocate a span to put the telemetry topic
  uint8_t telemetry_topic_buffer[64];
  az_span topic_span = AZ_SPAN_FROM_BUFFER(telemetry_topic_buffer);

  //Get the NULL terminated topic and put in topic_span to send the telemetry
  az_iot_hub_client_telemetry_publish_topic_get(&client, NULL, topic_span, &topic_span);
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