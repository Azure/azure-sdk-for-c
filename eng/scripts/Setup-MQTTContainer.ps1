param (
    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string] $AgentImage,
    [string] $ConfigFile
)

if ($IsLinux -and $AgentImage -match "ubuntu") {
    $FullCurrentPath = Get-Location | Resolve-Path
    
    # Setup certificates
    $CAPass = 1122

    # CA Generation
    Invoke-Expression "sudo openssl genrsa -passout pass:$($CAPass) -des3 -out ca.key 2048"
    Invoke-Expression "sudo openssl req -new -x509 -days 1826 -key ca.key -out ca.pem -subj `"/O=CA-cert/CN=127.0.0.1`" -passin pass:$($CAPass)"

    # Server Certificate Generation
    Invoke-Expression "sudo openssl genrsa -out server.key 2048"
    Invoke-Expression "sudo openssl req -new -out server.csr -key server.key -subj `"/O=SERVER-cert/CN=127.0.0.1`""
    Invoke-Expression "sudo openssl x509 -req -in server.csr -CA ca.pem -CAkey ca.key -CAcreateserial -out server.pem -days 360 -passin pass:$($CAPass)"

    # Client Certificate Generation
    Invoke-Expression "sudo openssl genrsa -out client-key.pem 2048"
    Invoke-Expression "sudo openssl req -new -out client.csr -key client-key.pem -subj `"/O=CLIENT-cert/CN=127.0.0.1`""
    Invoke-Expression "sudo openssl x509 -req -in client.csr -CA ca.pem -CAkey ca.key -CAcreateserial -out client.pem -days 360 -passin pass:$($CAPass)"

    # Setting Permissions
    Invoke-Expression "sudo chmod a+r $($FullCurrentPath)/server.key"
    Invoke-Expression "sudo chmod a+r $($FullCurrentPath)/client.pem"
    Invoke-Expression "sudo chmod a+r $($FullCurrentPath)/client-key.pem"
    Invoke-Expression "sudo chmod a+r $($FullCurrentPath)/ca.key"

    # Copying Certificates to the right location
    Invoke-Expression "mkdir certs"
    $FullPathCerts = (Join-Path $FullCurrentPath "certs/")
    Invoke-Expression "sudo cp $($FullCurrentPath)/ca.pem $($FullPathCerts)/ca.pem"
    Invoke-Expression "sudo cp $($FullCurrentPath)/server.pem $($FullPathCerts)/server.pem"
    Invoke-Expression "sudo cp $($FullCurrentPath)/server.key $($FullPathCerts)/server.key"

    docker pull azsdkengsys.azurecr.io/eclipse-mosquitto:2.0.1

    $FullPathConfigFile = (Join-Path $FullCurrentPath $ConfigFile)

    Invoke-Expression "sudo docker run -d -p 127.0.0.1:8883:8883 -p 127.0.0.1:2883:2883 --mount type=bind,src=$($FullPathConfigFile),dst=/mosquitto/config/mosquitto.conf --mount type=bind,src=$($FullPathCerts),dst=/mosquitto/config/certs azsdkengsys.azurecr.io/eclipse-mosquitto:2.0.1"

    Start-Sleep -Milliseconds 2000

    while ($count -lt 3) {
        $container = sudo docker ps -a --filter "ancestor=azsdkengsys.azurecr.io/eclipse-mosquitto:2.0.1" --format "{{.Names}}"
    
        if ($container) {
            Write-Host "Container started."

            Write-Host "Container initialization logs:"
            sudo docker logs $container

            break
        }
        else {
            Write-Host "Container starting..."
            Start-Sleep -Milliseconds 2000
            $count++
        }
    }

    if ($count -eq 3) {
        Write-Error "Container failed to start."
        exit 1
    }
  
  # Updating the location of the CA, client, and server certificates in the test
  $platformTestFile = Get-Content -Path $FullCurrentPath/sdk/tests/platform/test_az_mqtt5_policy.c
  $platformTestFile = $platformTestFile -replace "#define TEST_CERTIFICATE_PATH `"`"", "#define TEST_CERTIFICATE_PATH `"$($FullCurrentPath)/ca.pem`""
  $platformTestFile = $platformTestFile -replace "#define TEST_CLIENT_CERTIFICATE_PATH `"`"", "#define TEST_CLIENT_CERTIFICATE_PATH `"$($FullCurrentPath)/client.pem`""
  $platformTestFile = $platformTestFile -replace "#define TEST_CLIENT_KEY_PATH `"`"", "#define TEST_CLIENT_KEY_PATH `"$($FullCurrentPath)/client-key.pem`""

  Set-Content -Path $FullCurrentPath/sdk/tests/platform/test_az_mqtt5_policy.c -Value $platformTestFile
}
