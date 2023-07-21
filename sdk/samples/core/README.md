---
page_type: sample
languages:
  - c
products:
  - azure
urlFragment: core-samples
---

# Azure Core Samples client Library for Embedded C

This document explains samples and how to use them.

## Key Concepts

## Getting Sarted
needed in settings.json for vscode (or these set in cmake)

```json
"cmake.configureEnvironment": {
    "VCPKG_ROOT": "<path to vcpkg>",
    "VCPKG_DEFAULT_TRIPLET": "x64-linux"
  },
  "cmake.configureSettings": {
    "WARNINGS_AS_ERRORS" : "ON",
    "TRANSPORT_CURL" : "OFF",
    "UNIT_TESTING" : "OFF",
    "UNIT_TESTING_MOCKS" : "OFF",
    "TRANSPORT_PAHO" : "OFF",
    "PRECONDITIONS" : "ON",
    "LOGGING" : "ON",
    "TRANSPORT_MOSQUITTO" : "ON",
    "CMAKE_TOOLCHAIN_FILE" : "<path to vcpkg.cmake>",
    "AZ_MQTT_TRANSPORT_IMPL" : "MOSQUITTO"
  },
```

## Examples

## Troubleshooting

## Next Steps

### Additional Documentation

## Contributing

This project welcomes contributions and suggestions. Find [more contributing][SDK_README_CONTRIBUTING] details here.

<!-- LINKS -->
[SDK_README_CONTRIBUTING]: https://github.com/Azure/azure-sdk-for-c/blob/main/CONTRIBUTING.md

![Impressions](https://azure-sdk-impressions.azurewebsites.net/api/impressions/azure-sdk-for-c%2Fsdk%2Fcore%2Fcore%2Fsamples%2FREADME.png)
