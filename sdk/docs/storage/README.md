# Azure SDK Storage Blobs Library for Embedded C

Azure Storage Blobs library (`az_storage_blobs`) provides abstractions, and helpers for communicating with Azure Storage Blobs in the C programming language. This library follows the Azure SDK Design Guidelines for Embedded C.

Use the Azure SDK Storage Blobs Library for Embedded C to work with Azure Storage Blobs.

* Upload blobs

## Getting Started

The Azure Storage Blobs Client library is created to facilitate connectivity to Azure Storage Blobs services using the HTTP stack of the user's choice. This means that this SDK is **NOT** a platform but instead is a true SDK library.

### Docs

For API documentation, please see the doxygen generated docs [here][azure_sdk_for_c_doxygen_docs]. You can find the Storage Blobs specific docs by navigating to the **Files -> File List** section near the top and choosing any of the header files prefixed with `az_storage_blobs`.

### Build

The Azure Storage Blobs library is compiled following the same steps listed on the root [README][azure_sdk_for_c_readme] documentation, under ["Getting Started Using the SDK"][azure_sdk_for_c_readme_getting_started].

The library targets made available via CMake are the following:

- `az::storage::blobs` - For Azure Storage Blobs features ([API documentation here][azure_sdk_for_c_doxygen_docs])

### Samples

This [page][samples_storage_blobs_readme] explains samples for the Azure Embedded C SDK Storage Blobs Client.


### Prerequisites

For compiling the Azure SDK for Embedded C for the most common platforms (Windows and Linux), no further prerequisites are necessary.
Please follow the instructions in the [Getting Started](#Getting-Started) section above.
For compiling for specific target devices, please refer to their specific toolchain documentation.

## Key Concepts

### Authentication

The embedded C SDK currently supports SAS based authentication.  See [this page][storage_access_control_sas] for information on creating SAS tokens.
The client credential should be set to `AZ_CREDENTIAL_ANONYMOUS` when using SAS tokens.

### Creating the Storage Client

To use the storage client the first action is to initialize the client with `az_storage_blobs_blob_client_init`.
```C
  az_storage_blobs_blob_client client;
  az_storage_blobs_blob_client_options options = az_storage_blobs_blob_client_options_default();

  az_storage_blobs_blob_client_init(
          &client, az_span_from_str(getenv(URI_ENV)), AZ_CREDENTIAL_ANONYMOUS, &options);
```

### Uploading a blob

Once the client is created it can be used to upload blobs.
```C
  az_result const blob_upload_result = az_storage_blobs_blob_upload(
      &client, &az_context_app, content_to_upload, NULL, &http_response)
```

### Retry Policy

While working with Storage, you might encounter transient failures caused by [rate limits][storage_rate_limits] enforced by the service, or other transient problems like network outages. For information about handling these types of failures, see [Retry pattern][azure_pattern_retry] in the Cloud Design Patterns guide, and the related [Circuit Breaker pattern][azure_pattern_circuit_breaker].

## Need Help

- File an issue via [Github Issues](https://github.com/Azure/azure-sdk-for-c/issues/new/choose).

### Additional Documentation

For more extensive documentation on Azure Storage service, see the [Azure Storage documentation][storage_docs] on docs.microsoft.com.

## Contributing

If you'd like to contribute to this library, please read the [contributing guide][azure_sdk_for_c_contributing] to learn more about how to build and test the code.

### License

Azure SDK for Embedded C is licensed under the [MIT][azure_sdk_for_c_license] license.

<!-- LINKS -->
[azure_pattern_circuit_breaker]: https://docs.microsoft.com/azure/architecture/patterns/circuit-breaker
[azure_pattern_retry]: https://docs.microsoft.com/azure/architecture/patterns/retry
[azure_sdk_for_c_contributing]: ../../../CONTRIBUTING.md
[azure_sdk_for_c_contributing_developer_guide]: ../../../CONTRIBUTING.md#developer-guide
[azure_sdk_for_c_contributing_pull_requests]: ../../../CONTRIBUTING.md#pull-requests
[azure_sdk_for_c_doxygen_docs]: https://azure.github.io/azure-sdk-for-c
[azure_sdk_for_c_license]: ../../../LICENSE
[azure_sdk_for_c_readme]: ../../../README.md
[azure_sdk_for_c_readme_getting_started]:../../../README.md#getting-started-using-the-sdk
[samples_storage_blobs_readme]: ../../samples/storage/blobs/README.md
[storage_access_control_sas]: https://docs.microsoft.com/en-us/rest/api/storageservices/delegate-access-with-shared-access-signature
[storage_account_create]: https://docs.microsoft.com/en-us/azure/storage/common/storage-account-create?tabs=azure-portal
[storage_blobs]: https://docs.microsoft.com/en-us/azure/storage/blobs/storage-blobs-overview
[storage_docs]: https://docs.microsoft.com/azure/storage/
[storage_rate_limits]: https://docs.microsoft.com/en-us/azure/storage/blobs/scalability-targets
[storage_overview]: https://docs.microsoft.com/en-us/azure/storage/blobs/storage-blobs-introduction
