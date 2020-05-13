# Azure SDK KeyVault Keys Library for Embedded C

Azure KeyVault Keys library (`az_keyvault_keys`) provides abstractions, and helpers for communicating with Azure KeyVault in the C programming language. This library follows the Azure SDK Design Guidelines for Embedded C.

Use the Azure KeyVault Keys library to work with Azure KeyVault Keys.

* Create Key
* Get Key
* Delete Key

## Getting Started

### Download the Sample

**Prerequisites**: You must have an [Azure subscription][azure_sub], [Azure KeyVault account][keyvault_account] (KeyVault), and [C compiler][c_compiler] to use this package.

If you need a KeyVault vault, you can use the Azure [Cloud Shell][cloud_shell_bash] to create one with this Azure CLI command:

```Bash
az keyvault create --resource-group <resource-group-name> --name <keyvault-account-name>  --location <azure-region>
```

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

While working with KeyVault, you might encounter transient failures caused by [rate limits][keyvault_rate_limits] enforced by the service, or other transient problems like network outages. For information about handling these types of failures, see [Retry pattern][azure_pattern_retry] in the Cloud Design Patterns guide, and the related [Circuit Breaker pattern][azure_pattern_circuit_breaker].

## Next Steps

### More Sample Code

TODO

### Additional Documentation

For more extensive documentation on Azure Storage service, see the [Azure KeyVault documentation][keyvault_docs] on docs.microsoft.com.

## Contributing
For details on contributing to this repository, see the [contributing guide][azure_sdk_for_c_contributing].

This project welcomes contributions and suggestions.  Most contributions require you to agree to a
Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us
the rights to use your contribution. For details, visit https://cla.microsoft.com.

When you submit a pull request, a CLA-bot will automatically determine whether you need to provide
a CLA and decorate the PR appropriately (e.g., label, comment). Simply follow the instructions
provided by the bot. You will only need to do this once across all repos using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

### Additional Helpful Links for Contributors  
Many people all over the world have helped make this project better.  You'll want to check out:

* [What are some good first issues for new contributors to the repo?](https://github.com/azure/azure-sdk-for-c/issues?q=is%3Aopen+is%3Aissue+label%3A%22up+for+grabs%22)
* [How to build and test your change][azure_sdk_for_c_contributing_developer_guide]
* [How you can make a change happen!][azure_sdk_for_c_contributing_pull_requests]
* Frequently Asked Questions (FAQ) and Conceptual Topics in the detailed [Azure SDK for Embedded C wiki](https://github.com/azure/azure-sdk-for-c/wiki).

### Community

* Chat with other community members [![Join the chat at https://gitter.im/azure/azure-sdk-for-c](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/azure/azure-sdk-for-c?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

### Reporting Security Issues and Security Bugs

Security issues and bugs should be reported privately, via email, to the Microsoft Security Response Center (MSRC) <secure@microsoft.com>. You should receive a response within 24 hours. If for some reason you do not, please follow up via email to ensure we received your original message. Further information, including the MSRC PGP key, can be found in the [Security TechCenter](https://www.microsoft.com/msrc/faqs-report-an-issue).

### License

Azure SDK for Embedded C is licensed under the [MIT](LICENSE) license.

<!-- LINKS -->
[azure_sdk_for_c_contributing]: https://github.com/Azure/azure-sdk-for-c/blob/master/CONTRIBUTING.md
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
[keyvault_account]: https://docs.microsoft.com/en-us/azure/key-vault/key-vault-manage-with-cli2
[keyvault_docs]: https://docs.microsoft.com/en-us/azure/key-vault/
[keyvault_overview]: https://docs.microsoft.com/en-us/azure/key-vault/key-vault-overview
[keyvault_rate_limits]: https://docs.microsoft.com/en-us/azure/key-vault/key-vault-ovw-throttling
