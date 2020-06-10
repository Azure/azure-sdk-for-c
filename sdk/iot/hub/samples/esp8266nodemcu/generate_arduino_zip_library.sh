command -v zip >/dev/null 2>&1 || { echo >&2 "Please install zip."; exit 1; }

git clone https://github.com/Azure/azure-sdk-for-c sdkrepo
mkdir azure-sdk-for-c

cp -r sdkrepo/sdk/core/az_core/inc/* azure-sdk-for-c/
cp -r sdkrepo/sdk/core/az_core/internal/* azure-sdk-for-c/
cp -r sdkrepo/sdk/core/az_core/src/* azure-sdk-for-c/
cp -r sdkrepo/sdk/platform/noplatform/src/* azure-sdk-for-c/

cp -r sdkrepo/sdk/iot/hub/inc/* azure-sdk-for-c/
cp -r sdkrepo/sdk/iot/hub/src/* azure-sdk-for-c/
cp -r sdkrepo/sdk/iot/common/inc/* azure-sdk-for-c/
cp -r sdkrepo/sdk/iot/common/src/* azure-sdk-for-c/

zip -r9 azure-sdk-for-c azure-sdk-for-c/

rm -rf azure-sdk-for-c
rm -rf sdkrepo/
