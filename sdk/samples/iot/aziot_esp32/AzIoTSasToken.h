#ifndef AZIOTSASTOKEN
#define AZIOTSASTOKEN

#include <Arduino.h>
#include <az_iot_hub_client.h>
#include <az_span.h>

class AzIoTSasToken
{
public:
  AzIoTSasToken(
      az_iot_hub_client* client,
      az_span deviceKey,
      az_span signatureBuffer,
      az_span sasTokenBuffer);
  void Generate(unsigned int expiryTimeInMinutes);
  bool IsExpired();
  az_span Get();

private:
  az_iot_hub_client* client;
  az_span deviceKey;
  az_span signatureBuffer;
  az_span sasTokenBuffer;
  az_span sasToken;
  uint32_t expirationUnixTime;
};

#endif // AZIOTSASTOKEN
