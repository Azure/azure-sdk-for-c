command -v zip >/dev/null 2>&1 || { echo >&2 "Please install zip."; exit 1; }

git clone https://github.com/Azure/azure-sdk-for-c sdkrepo
mkdir azure-sdk-for-c

mkdir -p azure-sdk-for-c/azure/core
cp -r sdkrepo/sdk/inc/azure/core/* azure-sdk-for-c/azure/core
mkdir -p azure-sdk-for-c/azure/iot/
cp -r sdkrepo/sdk/inc/azure/iot/* azure-sdk-for-c/azure/iot

cp -r sdkrepo/sdk/src/azure/core/* azure-sdk-for-c/
cp -r sdkrepo/sdk/src/azure/iot/* azure-sdk-for-c/
cp sdkrepo/sdk/src/azure/platform/az_platform_stub.c azure-sdk-for-c/

zip -r9 azure-sdk-for-c azure-sdk-for-c/

rm -rf azure-sdk-for-c
rm -rf sdkrepo/
