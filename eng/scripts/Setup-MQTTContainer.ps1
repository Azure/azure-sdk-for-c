param (
    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string] $AgentImage,
    [string] $ConfigFile
)

if ($IsLinux -and $AgentImage -match "ubuntu") {
    docker pull azsdkengsys.azurecr.io/eclipse-mosquitto:2.0.1

    Write-Host "Following is the config file path: $ConfigFile"

    # Get the current path
    $currentPath = Get-Location

    # Print the current path
    Write-Host "Current path: $currentPath"

    # Get the files and folders in the current directory
    $filesAndFolders = Get-ChildItem -Path $currentPath

    # Print the files and folders
    Write-Host "Files and folders in current directory:"
    foreach ($item in $filesAndFolders) {
        Write-Host $item.Name
    }

    Write-Host "Run the docker run command to start the container"

    $fullPathConfigFile = "$currentPath/$ConfigFile"

    Write-Host "Full path of the config file: $fullPathConfigFile"

    #sudo docker run -d -p 127.0.0.1:2883:2883 -p 127.0.0.1:9001:9001 -v ${$fullPathConfigFile}:/mosquitto/config/mosquitto.conf azsdkengsys.azurecr.io/eclipse-mosquitto:2.0.1
    sudo docker run -d -p 127.0.0.1:2883:2883 -p 127.0.0.1:9001:9001 --mount type=bind,src=$fullPathConfigFile,dst=/mosquitto/config/mosquitto.conf azsdkengsys.azurecr.io/eclipse-mosquitto:2.0.1

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
