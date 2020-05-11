# Azure SDK Storage Blobs Library for Embedded C

Azure Storage Blobs library (`az_storage_blobs`) provides abstractions, and helpers for communicating with Azure Storage Blobs in the C programming language. This library follows the Azure SDK Design Guidelines for Embedded C.

Use the Azure SDK Storage Blobs Library for Embedded C to work with Azure Storage Blobs.

* Upload blobs
* Create and modify blob metadata

## Getting Started

### Install the Package

TODO link to the vcpkg

### Authenticate the Client

TODO

### Get Credentials

TODO

### Create Client

TODO

## Key Concepts

TODO

## Examples

TODO

## Troubleshooting

### General

TODO

### Retry Policy

While working with Storage, you might encounter transient failures caused by [rate limits][storage_rate_limits] enforced by the service, or other transient problems like network outages. For information about handling these types of failures, see [Retry pattern][azure_pattern_retry] in the Cloud Design Patterns guide, and the related [Circuit Breaker pattern][azure_pattern_circuit_breaker].

## Next Steps

### More Sample Code

TODO

### Additional Documentation

For more extensive documentation on Azure Storage service, see the [Azure Storage documentation][storage_docs] on docs.microsoft.com.

## Contributing

If you'd like to contribute to this library, please read the [contributing guide][azure_sdk_for_c_contributing] to learn more about how to build and test the code.

### License

Azure SDK for Embedded C is licensed under the [MIT][azure_sdk_for_c_license] license.

<!-- LINKS -->
[azure_sdk_for_c_contributing]: https://github.com/Azure/azure-sdk-for-c/blob/master/CONTRIBUTING.md
[azure_sdk_for_c_license]: https://github.com/Azure/azure-sdk-for-c/blob/master/LICENSE
[azure_sdk_for_c_contributing_developer_guide]: https://github.com/Azure/azure-sdk-for-c/blob/master/CONTRIBUTING.md#developer-guide
[azure_sdk_for_c_contributing_pull_requests]: https://github.com/Azure/azure-sdk-for-c/blob/master/CONTRIBUTING.md#pull-requests
[azure_cli]: https://docs.microsoft.com/cli/azure
[azure_pattern_circuit_breaker]: https://docs.microsoft.com/azure/architecture/patterns/circuit-breaker
[azure_pattern_retry]: https://docs.microsoft.com/azure/architecture/patterns/retry
[azure_portal]: https://portal.azure.com
[azure_sub]: https://azure.microsoft.com/free/
[c_compiler]: https://visualstudio.microsoft.com/vs/features/cplusplus/
[cloud_shell]: https://docs.microsoft.com/azure/cloud-shell/overview
[cloud_shell_bash]: https://shell.azure.com/bash
[storage_account_create]: https://docs.microsoft.com/en-us/azure/storage/common/storage-account-create?tabs=azure-portal
[storage_blobs]: https://docs.microsoft.com/en-us/azure/storage/blobs/storage-blobs-overview
[storage_docs]: https://docs.microsoft.com/azure/storage/
[storage_overview]: https://docs.microsoft.com/en-us/azure/storage/blobs/storage-blobs-introduction
[storage_rate_limits]: https://docs.microsoft.com/en-us/azure/storage/blobs/scalability-targets