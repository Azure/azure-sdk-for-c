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
    Invoke-Expression "sudo openssl req -new -x509 -days 1826 -key ca.key -out ca.pem -subj `"/O=CA-cert/CN=localhost`" -passin pass:$($CAPass)"

    # Server Certificate Generation
    Invoke-Expression "sudo openssl genrsa -out server.key 2048"
    Invoke-Expression "sudo openssl req -new -out server.csr -key server.key -subj `"/O=SERVER-cert/CN=localhost`""
    Invoke-Expression "sudo openssl x509 -req -in server.csr -CA ca.pem -CAkey ca.key -CAcreateserial -out server.pem -days 360 -passin pass:$($CAPass)"

    # Client Certificate Generation
    Invoke-Expression "sudo openssl genrsa -out client-key.pem 2048"
    Invoke-Expression "sudo openssl req -new -out client.csr -key client-key.pem -subj `"/O=CLIENT-cert/CN=localhost`""
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

    Write-Host "START OF DEBUGGING LOGS -----------------------------"
    Write-Host "Output of ls: "
    Invoke-Expression "ls"
    Write-Host "Output of ls certs: "
    Invoke-Expression "ls certs/"
    Write-Host "Value of FullCurrentPath: $($FullCurrentPath)"
    Write-Host "Value of FullPathConfigFile: $($FullPathConfigFile)"

    Write-Host "END OF DEBUGGING LOGS -----------------------------"

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

    Write-Host "Printing /etc/hosts"
    Invoke-Expression "sudo cat /etc/hosts"

    Write-Host "Installing mosquitto clients..."
    Invoke-Expression "sudo apt install mosquitto-clients"

    Write-Host "Publishing messages to the broker on localhost"
    Invoke-Expression "mosquitto_pub -h localhost -p 8883 -t testing -m `"MESSAGE TESTING`" --cafile /mnt/vss/_work/1/s/ca.pem --key /mnt/vss/_work/1/s/client-key.pem --cert /mnt/vss/_work/1/s/client.pem"

    # Get the docker container id number and print the latest logs from the container itself
    $ContainerID = Invoke-Expression "sudo docker ps -a --filter `"ancestor=azsdkengsys.azurecr.io/eclipse-mosquitto:2.0.1`" --format `"{{.ID}}`""
    Invoke-Expression "sudo docker logs $($containerID)"

    #Write-Host "Publishing to the broker on 127.0.0.1"
    #Invoke-Expression "mosquitto_pub -h 127.0.0.1 -p 8883 -t testing -m `"MESSAGE TESTING`" --cafile /mnt/vss/_work/1/s/ca.pem --key /mnt/vss/_work/1/s/client-key.pem --cert /mnt/vss/_work/1/s/client.pem"
    

}
