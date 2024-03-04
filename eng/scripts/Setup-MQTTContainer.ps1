param (
    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string] $AgentImage,
    [string] $ConfigFile
)

if ($IsLinux -and $AgentImage -match "ubuntu") {
    # Setup certificates
    $CAPass = 1122

    # CA Generation
    sudo openssl genrsa -passout pass:$CAPass -des3 -out ca.key 2048
    sudo openssl req -new -x509 -days 1826 -key ca.key -out ca.pem -subj /O=CA-cert/CN=127.0.0.1 -passin pass:$CAPass

    # Server Certificate Generation
    sudo openssl genrsa -out server.key 2048
    sudo openssl req -new -out server.csr -key server.key -subj /O=SERVER-cert/CN=127.0.0.1
    sudo openssl x509 -req -in server.csr -CA ca.pem -CAkey ca.key -CAcreateserial -out server.pem -days 360 -passin pass:$CAPass

    # Client Certificate Generation
    sudo openssl genrsa -out client-key.pem 2048
    sudo openssl req -new -out client.csr -key client-key.pem -subj /O=CLIENT-cert/CN=127.0.0.1
    sudo openssl x509 -req -in client.csr -CA ca.pem -CAkey ca.key -CAcreateserial -out client.pem -days 360 -passin pass:$CAPass

    # Setting Permissions
    sudo chmod a+r (Resolve-Path server.key)
    sudo chmod a+r (Resolve-Path client.pem)
    sudo chmod a+r (Resolve-Path client-key.pem)
    sudo chmod a+r (Resolve-Path ca.key)

    # Copying Certificates to the right location
    $certsPath = New-Item -ItemType Directory -Path certs
    sudo cp (Resolve-Path ca.pem) $certsPath/ca.pem
    sudo cp (Resolve-Path server.pem) $certsPath/server.pem
    sudo cp (Resolve-Path server.key) $certsPath/server.key

    docker pull azsdkengsys.azurecr.io/eclipse-mosquitto:2.0.1

    Invoke-Expression "sudo docker run -d -p 127.0.0.1:8883:8883 -p 127.0.0.1:2883:2883 --mount type=bind,src=$(Resolve-Path $ConfigFile),dst=/mosquitto/config/mosquitto.conf --mount type=bind,src=$certsPath,dst=/mosquitto/config/certs azsdkengsys.azurecr.io/eclipse-mosquitto:2.0.1"

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
}
