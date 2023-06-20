param (
    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string] $AgentImage,
    [string] $ConfigFile
)

if ($IsLinux -and $AgentImage -match "ubuntu") {
    docker pull azsdkengsys.azurecr.io/eclipse-mosquitto:2.0.1

    sudo docker run -d -p 127.0.0.1:2883:2883 -p 127.0.0.1:9001:9001 -v ${$ConfigFile}:/mosquitto/config/mosquitto.conf azsdkengsys.azurecr.io/eclipse-mosquitto:2.0.1
    
    $count = 0

    sudo docker ps -a

    Start-Sleep -Milliseconds 2000

    while ($count -lt 3) {
        $container = sudo docker ps -a --filter "ancestor=azsdkengsys.azurecr.io/eclipse-mosquitto:2.0.1" --format "{{.Names}}"
    
        if ($container) {
            Write-Host "Container is running"

            # Debugging
            sudo docker container inspect $container
            Write-Host "<<<<<<<<<<< Printing out logs >>>>>>>>>>>>>"
            sudo docker logs $container

            break
        }
        else {
            Write-Host "Container is not running"
            Start-Sleep -Milliseconds 2000
            $count++
        }
    }

    if ($count -eq 3) {
        Write-Error "Container is not running"
        exit 1
    }
}
