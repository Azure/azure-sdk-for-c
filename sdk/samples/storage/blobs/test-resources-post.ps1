# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# This script is used to set up a block blob for testing storage samples. The
# sample assumes that the blob is in place before execution.

param (
  [hashtable] $DeploymentOutputs
)

$storageAccountName = $DeploymentOutputs['ACCOUNT_NAME']
$context = New-AzStorageContext -StorageAccountName $storageAccountName

New-Item -Type File -Name "TestBlob" -Force
Set-AzStorageBlobContent -File "TestBlob" -Container $DeploymentOutputs['CONTAINER_NAME'] -Context $context

