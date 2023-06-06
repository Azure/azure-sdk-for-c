param (
    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string] $AgentImage,
    [string] $ConfigFile
)

if ($IsLinux -and $AgentImage -match "ubuntu") {
    docker pull eclipse-mosquitto

    sudo docker run -d -p 1883:1883 -p 9001:9001 -v ${$ConfigFile}:/mosquitto/config/mosquitto.conf eclipse-mosquitto

    # Testing

    sudo apt install net-tools

    echo "Checking connections"

    sudo netstat -p

    sudo netstat -apn | grep 1883

    docker ps
}
