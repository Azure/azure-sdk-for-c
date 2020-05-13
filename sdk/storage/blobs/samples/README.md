---
page_type: sample
languages:
  - c
products:
  - azure
  - azure-storage
  - azure-storage-blobs
urlFragment: storage-blob-samples
---

# Azure Storage Blob Samples client Library for Embedded C
This document explains samples and how to use them.

## Key Concepts
Key concepts are explained in detail [here][SDK_README_KEY_CONCEPTS].

# Samples Azure Storage Blob APIs
This document describes how to use samples and what is done in each sample.

## Getting Started
Getting started explained in detail [here][SDK_README_GETTING_STARTED].

## Examples
   Following section document various examples.

1. [Basic Examples][samples_basic]: Create storage blob client. Upload blob.

## Troubleshooting
When interacting with blobs using this C client library, errors returned by the service correspond to the same HTTP
status codes returned for [REST API][error_codes] requests. For example, if you try to retrieve a container or blob that
doesn't exist in your Storage Account, a `404` error is returned, indicating `Not Found`

## Next Steps
Start using Storage blob C SDK in your solutions. Our SDK details could be found at [SDK README][BLOB_SDK_README]. 

###  Additional Documentation
For more extensive documentation on Azure Storage blob, see the [API reference documentation][storageblob_rest].

## Contributing

If you'd like to contribute to this library, please read the [contributing guide][azure_sdk_for_c_contributing] to learn more about how to build and test the code.

### License

Azure SDK for Embedded C is licensed under the [MIT][azure_sdk_for_c_license] license.

<!-- LINKS -->
[azure_sdk_for_c_contributing]: https://github.com/Azure/azure-sdk-for-c/blob/master/CONTRIBUTING.md
[azure_sdk_for_c_license]: https://github.com/Azure/azure-sdk-for-c/blob/master/LICENSE
[BLOB_SDK_README]: ../README.md
[SDK_README_CONTRIBUTING]:../README.md#contributing
[SDK_README_GETTING_STARTED]: ../README.md#getting-started
[SDK_README_KEY_CONCEPTS]: ../README.md#key-concepts
[samples_basic]: src/blobs_client_example.c
[storageblob_rest]: https://docs.microsoft.com/en-us/rest/api/storageservices/blob-service-rest-api
[error_codes]: https://docs.microsoft.com/rest/api/storageservices/blob-service-error-codes

![Impressions](https://azure-sdk-impressions.azurewebsites.net/api/impressions/azure-sdk-for-c%2Fsdk%2Fstorage%2Fblobs%2Fsrc%2Fsamples%2FREADME.png)
