param (
    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string] $AgentImage,
    [string] $ConfigFile
)

if ($IsLinux -and $AgentImage -match "ubuntu") {
    docker pull azsdkengsys.azurecr.io/eclipse-mosquitto:2.0.1

    $CurrentPath = Get-Location

    $FullPathConfigFile = Join-Path $CurrentPath $ConfigFile | Resolve-Path

    Write-Host "DEBUG FullPathConfigFile: $FullPathConfigFile"
    Write-Host "DEBUG ConfigFile: $ConfigFile"
    Write-Host "DEBUG CurrentPath: $CurrentPath"

    sudo docker run -d -p 127.0.0.1:2883:2883 -p 127.0.0.1:9001:9001 --mount type=bind,src=/mnt/vss/_work/1/s/eng/config/mosquitto.conf,dst=/mosquitto/config/mosquitto.conf azsdkengsys.azurecr.io/eclipse-mosquitto:2.0.1

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
