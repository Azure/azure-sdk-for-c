# Provisioning CA Certificates to Devices using TrustBundle

In general, Azure IoT Edge devices do not have server certificates signed by authorities trusted by 
the leaf device. To allow the device to connect securely to an Azure IoT Edge device, new 
certificates must be installed within the Trusted Root Certification Authority (CA) store of the 
leaf device. 

The TrustBundle feature in Azure Device Provisioning Service feature allows IoT administrators to 
manage the device CA certificate store remotely.

Also see https://azure.github.io/iot-identity-service/

## Application Architecture

_For security reasons, it is recommended to install certificates using a separate, elevated 
privilege process instead of granting more permissions to the Provisioning or Hub application 
components:_

![trust_bundle_application](https://www.plantuml.com/plantuml/png/XPF1Ri8m38RlUGghf-rG7c124mYGXiO13NQQTWYjAH4XHabQf4sy-vpGGQkrx5RP_lpRlzEHyzBwyg35rie3GhAW4oojgfJ60XFu5W0VIqkLSegCCWLCw70aWyP_XjHBkMb6pa8SPRQN1NUQQQoanxpHBdHZQ8BMgwtAE0jpmnDeZJR2cQOoluXEiL8PGajxXJO4e_ASri3g4HEvz78Z7QkkRUc2Q5DZvSdMkp0u_Yejwp8-6SCRKLo4uxESfsw75fH9_QjwovsRWftBm9JpLqKjdOSARLW37j3Buh4Mq8epj0LLWpbajtOkAjqr0ePfsdkUgqMXwi-f-Z18q-VU4_N4BqpRmBkbFSRsCSF0TBfud_Z7NM68AQkANInBkXr9dc2Xp9xfa_8xy3iUE5rDNo-qfsDaM-ucujsXYwNrzNgVvK1qDXy8D3a41I56_Cb_w0y0 "trust_bundle_application")


The Certificate Installer component must be able to persist the last TrustBundle version (etag) as 
well as the list of installed certificates. When a new version of the TrustBundle is received, 
the Certificate Installer must first remove previous TrustBundle certificates that are no longer 
part of the current TrustBundle then install new certificates from the TrustBundle. 

__Important:__ Accidentally deleting the Azure IoT CA root on the device will result in permanent 
Azure IoT service connectivity loss.

## Public API

```C
az_iot_provisioning_client_registration_state.trust_bundle
```

## Sample

The `paho_iot_provisioning_csr_trustbundle_sample` demonstrates how to use the SDK public API as well as parse the 
TrustBundle to extract the PEM certificates. Device Provisioning Service configuration is required
to enable and configure a TrustBundle for the individual or group device enrollments.

### Example output

```
SUCCESS:        Trust Bundle received:
                        [1] CA Certificate data:
                         -----BEGIN CERTIFICATE-----(...)
                        Subject: CN=root-contoso, OU=contoso, O=contoso, C=16148
                        Thumbprint: 5C57EB7EED8E278B1EAE278AAE1C6083EC2861B0
                        Issuer: CN=root-contoso, OU=contoso, O=contoso, C=16148
                TrustBundle ID: myTrustBundle1
                TrustBundle eTag: \"1f00c05d-0000-0600-0000-623a4b200000\"
...
