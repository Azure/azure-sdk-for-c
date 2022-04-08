# Using Azure IoT Device Provisioning Certificate Signing Requests

Device Provisioning Service can be configured to provision operational X509 
certificates to IoT devices. These operational certificates must be used to authenticate with the 
allocated IoT Hub. An onboarding authentication mechanism (either SAS token or X509 Client 
Certificate) is still required to authenticate the device with DPS.

## Certificate Signing Request Flow

![dpsCSR](https://www.plantuml.com/plantuml/png/ZPFVJy8m4CVVzrVSeoumJOmF4aCOGzGO331CJ8mFPJsWSRQpxKJ-Uwyhc3cNu5j_tETxtxjh1yOoRSlt77XXadGUZF1JP0ZTFlXL3svHecOM8GnLuLP2IPQ78TmaoTayZU4DTzs22Tt29Xg9D92Wjc8bCYUJYhKKSWrp_2ZHM2YDspN5sqIdQG_YqcvGMY_bqJHEEx2OJa1fI791j_1yUNxb9ELICSqfY1GpZToHB3S1ITLjnib3Cu_6jJ0MDPpubRTsppimL7pN--ENUdVQAkkFQWrm5IWjRBj81Lnt_mbKTTP2crLGX_KENg8eUcsQsvJByMpqs05U7RZAZgjxRmU9oMonW94mcP8ICrjBVlSqR1tLqGm4TliLlHSxHW1-5O8c6nsoax-ARvgJYe9pNDa0_Dq5S4LFayw1OYmJ4kYASIvyCk0_CBq6PfIC0fJS0TXMwdzBRFYiTOaG63EHbnf_sLriX05wZ0MG6JI6qKoLp7VExvRWVMPVLQl9myJcMFrtMELg6mG3YgIl_mq0 "dpsCSR")

## Public API

```C
// Building the DPS register MQTT payload containing the operational X509 Certificate Signing Request:
az_iot_provisioning_client_payload_options options;
az_iot_provisioning_client_payload_options_default();

options.certificate_signing_request = AZ_SPAN_FROM_BUFFER(CSR_IN_PEM_FORMAT);

az_iot_provisioning_client_register_get_request_payload(
    dps_client,
    NULL,
    &options,
    uint8_t* mqtt_payload,
    size_t mqtt_payload_size,
    size_t* out_mqtt_payload_length)

// Perform DPS registration.
// [...]

az_iot_provisioning_client_register_response register_response; // parsed during registration.

if (az_span_size(register_response->registration_state.issued_client_certificate) > 0)
{
    // issued_client_certificate contains the CA signed *operational* certificate in PEM format.
}

```

## Sample

The `paho_iot_provisioning_csr_trustbundle_sample` demonstrates how to use the public API to submit a certificate 
signing request then display the provisioned device certificate that can be used to connect to Azure IoT Hub (using any of the `paho_iot_hub_*` samples using X509 authentication).

### 1. Generate a CSR

1. Generate either an ECC or RSA key-pair

```
# Generate an ECC P-256 keypair:
openssl ecparam -out key.pem -name prime256v1 -genkey

# Generate an RSA 4096 bit keypair:
openssl genrsa -out key.pem 4096
```

2. Generate a PKCS#10 Certificate Signing Request (CSR) from the keypair:

Below, replace `my-device-id` with the device ID.

_Important:_ The device ID *must* match the one configured for the registration ID. Devices are not allowed to submit requests with Subject Certificate Name other than the configured device ID.

```
openssl req -new -key key.pem -out request.csr -subj '/CN=my-device-id'
```

Additionally, to read the information from a CSR file use:

```
openssl req -noout -text -in request.csr
```

For more information on OpenSSL CSR support, see https://www.openssl.org/docs/man1.1.1/man1/openssl-req.html

### 2. Set the environment for CSR sample

```sh
export AZ_IOT_PROVISIONING_ID_SCOPE=0ne?????
export AZ_IOT_PROVISIONING_REGISTRATION_ID=<registration_id>
# Onboarding Certificate and Key:
export AZ_IOT_DEVICE_X509_CERT_PEM_FILE_PATH=path_to_certificate_store.pem
# Operational Certificate Signing Request:
export AZ_IOT_DEVICE_X509_OPERATIONAL_CSR_PEM_FILE_PATH=path_to_csr_request.pem
```
