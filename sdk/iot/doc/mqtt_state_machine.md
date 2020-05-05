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

![iot_hub_flow](https://www.plantuml.com/plantuml/svg/0/dLTjRzis4FxENt7VbWnpLmHBWHf5KCK9vgp9uiOgCA0e15gwM4GaKYMff5wn_pvIBvRjY7fnFXY1U7VFmuV7uzr7fQdKUPeGefuOpwYaAPXl6k4d0VDtISEdjHGvZsOY4VB31s-nkBW0ytjxC_Fyc_k_sysM_iENcMyiR-apyWWb-Sz05853Qn648c9yNQ6K2ykhJu6tk3duSkNvNd1_-TVrp8ScxexH3fTWTdzr3HL9Y_WBt72coTTVly5aSW7qtu097SJvAWWJXbm7p2nFOUbx3pvfNuLMlpNg85Se0qvJ93thxX0rJaHwreiGMPQma6ecU91-rsPLS8wXuV_F-TltC2jimSjrF8MCAlKYP0HC0QQPtbJYks9iHJEItMTscIQmz9QWnHDoM2D7MNXD1p5zPQAeAimKMtEgSuamBS9JmvOCCJ_OVvpIyEpy7MdvrasW5eRqcvGfun9iNsFQYDMYTCriHsGV4qnHomrn8hHdhAuTIwxvAc4g3hJ8M6WuRSD2I3ci44mIHl3okm2LhrGesShUEYs0yWqKJc_8E8nMKIeJkx7PsTNOBOLdKNdQ5KKieO97bcXJESOwj5FSsbgITwXZ4QcnHDFIRHpPVgTUmkwW4bKckC8Z3if_ONnqyXcVVSp1CWnrxHVZ2CePrM5y1Dy-lrwonBtOnJSIOPxMfNNlMCbeZvv4E8vW8ymWTWxp_c8YqbHmQ0Hrl68Dvxh4Tcsh1LDYYEpPlaftdtqV4hPgG2FYWgsODdNTOLT7hzkdds3tHfDaPVecY4orNHjhKvXZogC4vRHeRznTYwr_fbSpzwYA76qVW8Vg0oo-NDxUBEUGeb9qZKN44P6w2w03d0iD9Y98J8mEaGEpCqXdpZb1gv0zmEue5jNANKQpTjjGrPCsLZxNiKdydgEoxOCLAZGv0BLhWT4yJjEZv7EVsWvq-Q45lTVtyEwKgsNQxlJaJfljLIdGd5QToUT-yse5YwTjT0xqFdX6kI8r_hpsGKXOUFvitkXNZXl2Xtc-Wj9zwfeffZ21-diib7PC4LlR29zPY7PEC1Ysq_Hjfwd8i4PxKZQc0Vx8MDd51vW9Kv0rQuKzR8HjX4kgyDqV9_CxRpPPpZtJpPvi8Nw3PKmbHbuOccBp2JUdfmEZFk6cdA38s4Ptqd9OcHGPIirGRIjnNpxlJ9HzjXQp-hTzO7jjxjfQVoOF1EiBOLN8EzJOhKYTKkoNhZxnn_Se1_iTsqSSkhlskt-TEtTyUFCjG2MrtcGuqAFZ15-RLwSrnFGPfJBts9O3hwcYmBXfIX-fkOQaOt0rI2XDXE37-TltWPQvCWzT88NGtLvVApg5MsWF-z7QuhVMCRowtt2IqesTH5-OZatnD6FfV7btIiX7v56U9ly3 "iot_hub_flow")

### Device Provisioning Service

![iot_provisioning_flow](https://www.plantuml.com/plantuml/svg/0/hLTHRziu37xNhz3RaiCo70xRW0L3CJSjq6xhDTTum81X41IBSOJOaYV9MRFX_lient6onedjP7q82fFyI7waAEgTTTouF4q8iGkfCcvuIl6R0_gJKFoxvD4YDZfNOgGJRTw-3SRZYmkAlnzlHq5uU_zllTeaVFWq2b7p8r-24c38_-YY08wy1ekqa2eklQm5awiFB1ZVFlbo5OG1kxzyVpqAwHPtHsad5ZQwlxgcPP3UupDwgwGZN_xwIWU32yf_CAaTounUIYkrmWXOb4XGZadK6z9963o4f46j2Ie6nt8BXYcU0cchs0Pr4uIWG_hvrTzlAC_TlAILPraY8-xGYgNV72fhfI1o5HsjiOj03g1vIkyp_vadT19CgDCBK3G61QRG7dRmSpHVossXRsMikCiDOCxUOP5o9xidwSnJ7ltpwZMf-TOof8nR-rqRGJhe7cKLKwimzwkv2MvJI2p199_QoCWfa8EG3Ura89RMOUyLN6tWcDEPZDXC9YWYvvdNPrpf7chRIg7AzzaHdLeV1YAGIzymT_-58Ktab5OMmjFtK4PFcf1R4f3bqqJQ-JfzSf9LVIQbof-4BCavH7-9i6ss0teRZZStma-Al1PqZpzTqb8gYZ7TOlhYnMDXWodADk9Ae62LQ2w6-z1qPdHAfmRdZw6zAD6veDhGQn53lt1xXhlMnJTWCwqi_167dgSXLThHNGXCEC4AODys-DWBgXtfEYidD_ELpYrfbyjJioPdM_r8fra2hf6pVOiTbYeXhOGzcqSHMEkxOCPbWgE_VwAFtMuNIF_gDuzI-GB5IBFTMKFsRJP7g8XXJzh0rxSSpAhajcR6DEHrtFhuh2dMfl5k1wv1XurxqP0EQ9peAKyOxdoPbYYOQ-kQzg3AM0fc8oIwbVAwtylooMAk-_qWz0k99HqtYy5waM7mcCaICbVoaJc9ePDpJlOQk-5UQ8LI_b-Dv2wZO6TpxcJd_PPpeRFTzd19SOxrgVuDE3m4VEO55d0l4YU1x7GPIlBa-46eGtdnOa1nlSDsmzhEyschGt48IC1IHd02fTuOZ5c_lKvo3O_El-K4EnEci4vnVFBu300iIhJmskTGuSytzbUQaIxBl6-CIsxXzSlUpLqm6ank0bpGUzL0UKudSsv01CM5z05M_NvN-0Mirkz7SlgOtpk4E2A3xTZ7YmnvcN6CaHnvcU000BD40X_nnOcivyYcO4PhTzpgInbaOHQ1SNBcTU4hgRjUVpkkjM-paXNQIz405_JlqitnAkQAtapk-eIymzxCq-GN "iot_provisioning_flow")

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
if(az_failed(az_iot_hub_client_sas_get_signature(client, unix_time + 3600, signature, &signature)));
{
    // error.
}

// Application will Base64Encode the HMAC256 of the az_span_ptr(signature) containing az_span_size(signature) bytes with the Shared Access Key.

if(az_failed(az_iot_hub_client_sas_get_password(client, base64_hmac_sha256_signature, NULL, password, password_size, &password_length)))
{
    // error.
}
```

Recommended defaults:
    - MQTT Keep-Alive Interval:  `AZ_IOT_DEFAULT_MQTT_CONNECT_KEEPALIVE_SECONDS`
    - MQTT Clean Session: false.

#### MQTT Clean Session

We recommend to always use [Clean Session](http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718030) false when connecting to IoT Hub.
Connecting with Clean Session true will remove all enqueued C2D messages.

### Subscribe to topics

Each service requiring a subscription implements a function similar to the following:

_Example:_

```C

if(az_failed(az_iot_hub_client_c2d_get_subscribe_topic_filter(client, topic, topic_size, NULL))
{
    // error.
}
```

__Note:__ If the MQTT stack allows, it is recommended to subscribe prior to connecting.

### Sending APIs

Each action (e.g. send telemetry, request twin) is represented by a separate public API.
The application is responsible for filling in the MQTT payload with the format expected by the service.

_Example:_

```C
if(az_failed(az_iot_hub_client_telemetry_get_publish_topic(client, NULL, topic, topic_size, NULL)))
{
    // error.
}
```

__Note:__ To limit overheads, when publishing, it is recommended to serialize as many MQTT messages within the same TLS record. This feature may not be available on all MQTT/TLS/Sockets stacks.

### Receiving APIs

We recommend that the handling of incoming MQTT PUB messages is implemented by a chain-of-responsibility architecture. Each handler is passed the topic and will either accept and return a response, or pass it to the next handler.

_Example:_

```C
    az_iot_hub_client_c2d_request c2d_request;
    az_iot_hub_client_method_request method_request;
    az_iot_hub_client_twin_response twin_response;

    //az_span received_topic is filled by the application.

    if (az_succeeded(az_iot_hub_client_c2d_parse_received_topic(client, received_topic, &c2d_request)))
    {
        // This is a C2D message: 
        //  c2d_request.properties contain the properties of the message.
        //  the MQTT message payload contains the data.
    }
    else if (az_succeeded(ret = az_iot_hub_client_methods_parse_received_topic(client, received_topic, &method_request)))
    {
        // This is a Method request:
        //  method_request.name contains the method
        //  method_request.request_id contains the request ID that must be used to submit the response using az_iot_hub_client_methods_response_get_publish_topic()
    }
    else if (az_succeeded(ret = az_iot_hub_client_twin_parse_received_topic(client, received_topic, &twin_response)))
    {
        // This is a Twin operation.
        switch (twin_response.response_type)
        {
            case AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_GET:
                // This is a response to a az_iot_hub_client_twin_document_get_publish_topic.
                break;
            case AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_DESIRED_PROPERTIES:
                // This is received as the Twin desired properties were changed using the service client.
                break;
            case AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_REPORTED_PROPERTIES:
                // This is a response received after patching the reported properties using az_iot_hub_client_twin_patch_get_publish_topic().
                break;
            default:
                // error.
        }
    }
```

__Important:__ C2D messages are not enqueued until the device establishes the first MQTT session (connects for the first time to IoT Hub). The C2D message queue is preserved (according to the per-message time-to-live) as long as the device connects with Clean Session `false`.

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

![iot_retry_flow](https://www.plantuml.com/plantuml/svg/0/bPNDQkCm4CVlUegvBHIy3n1AQ3RBie7G13TxMefHd6agwaf6abEwfUzUsObSoHedsnnya7xpd-_enbYkRVDSCVCaPCqrVmPtP17U6BZV3ru-xRLgv6wkAgMlhsVhzNGAxhjSp6URnUgMnkus-P_vnf5BVa2vGqrZlsQBfODMciizidV6_bxTGvPDOQtLGHYXf91xTWmeF08Vo1lhX8z4ZdjXBEhY9nv4YHxgY6-nVOvM2xwj451hfKt7UES3dT15A59eBr8yS54r6dr2dS4mcc5QgNbdTXv9LKfUbKtbOYjsMF7NL6C0f0gyhWDR8iyU20jA4rJzO09j7fT3cq3cI5ChQV1xPvBn1tkQdKk6_5yXbEqgzjhT1pbH4t2hPDQN5_wlO_Ba8EqQKJKIZYRaCfw6aAlMKp7Nk4Df1Q_CcF-KXD7-4UoN6ZbYtoxK1AG2PHzHGzbVitTWB6f7Yo_KflZTR9t9NLEMQ0mxhRw_-DpwpvpTUR6gKNFhbA8C_Jf713lDGYj7_Wd4Ujx-NDV9-wZH9D5hKnjCdFSyjQ_HULY4w28jHzHImkcvpGegIImJNSTB6pJA9FKStvVZrEMOrNx0sfV5pz1menowsbek94Xy2KRK3T-DUxdSq_W1 "iot_retry_flow")

When devices are using IoT Hub without Provisioning Service, we recommend attempting to rotate the IoT Credentials (SAS Token or X509 Certificate) on authentication issues.

_Note:_ Authentication issues observed in the following cases do not require credentials to be rotated:

- DNS issues (such as WiFi Captive Portal redirects)
- WebSockets Proxy server authentication


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