# Upgrade MQTT Container

This article describes needed steps to upgrade the `eclipse-mosquitto` container
in the engineering system. 

## Prerequisites

These steps can be run on Windows, Linux, or Mac. Verification must be done in
Linux.

* [Azure CLI](https://learn.microsoft.com/cli/azure/install-azure-cli)
* [Docker](https://docs.docker.com/engine/install/) 

## Steps

1. Ensure that Docker engine is running
1. Pull image from docker hub `docker pull eclipse-mosquitto:<version>` where `<version>` is the version you want to upgrade to
1. Perform local verification to ensure that the new image works. Run `eng/scripts/Setup-MQTTContainer.ps1` from the root of the SDK specifying `$AgentImage` as `"ubuntu"` and `$ConfigFile` as `"eng/config/mosquitto.conf"`. Make sure to comment out and change references to the previous version.

1. Log into the ACR instance using `az acr login --name azsdkengsys`
1. Tag the existing image `docker tag eclipse-mosquitto:<version> azsdkengsys.azurecr.io/eclipse-mosquitto:<version>`
1. Push the new image `docker push azsdkengsys.azurecr.io/eclipse-mosquitto:<version>`
1. Update `Setup-MQTTContainer.ps1` replacing old versions of `azsdkengsys.azurecr.io/eclipse-mosquitto:<old-version>` with the new version `azsdkengsys.azurecr.io/eclipse-mosquitto:<version>`

If you don't have push permissions to the container, reach out to your 
Engineering System contact.
