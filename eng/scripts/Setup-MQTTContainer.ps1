param (
    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string] $AgentImage,
    [string] $ConfigFile
)

if ($IsLinux -and $AgentImage -match "ubuntu") {
    $dockerGpgDir = "/etc/apt/keyrings"
    $dockerGpgPath = "/etc/apt/keyrings/docker.gpg"
    $dockerGpgUrl = "https://download.docker.com/linux/ubuntu/gpg"
    $dockerListPath = "/etc/apt/sources.list.d/docker.list"

    sudo install -m 0755 -d $dockerGpgDir

    sudo curl -fsSL $dockerGpgUrl | sudo gpg --dearmor -o $dockerGpgPath

    sudo chmod a+r /etc/apt/keyrings/docker.gpg
    
    echo "deb [arch=$(dpkg --print-architecture) signed-by=$($dockerGpgPath)] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable" | sudo tee $dockerListPath > /dev/null

    sudo apt-get update

    sudo apt-get -y install docker-ce

    docker pull eclipse-mosquitto

    # Testing

    sudo apt install net-tools

    echo "Checking connections"

    sudo netstat -p

    # End of testing

    sudo docker run -d -p 50000:50000 -p 9001:9001 -v ${$ConfigFile}:/mosquitto/config/mosquitto.conf eclipse-mosquitto
}
