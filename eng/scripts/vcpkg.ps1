[CmdletBinding()]
Param (
    [Parameter()]
    [ValidateNotNullOrEmpty()]
    [string] $Ref = 'master',

    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string] $Dependencies,

    [Parameter()]
    [string] $TargetPath ="$env:TEMP/$([guid]::NewGuid())",

    [Parameter()]
    [switch] $devOpsLogging = ($null -ne $env:SYSTEM_TEAMPROJECTID)
)

function SetEnvironmentVariable {
    param(
        [string] $Name,
        [string] $Value
    )

    if ($devOpsLogging) {
        Write-Host "##vso[task.setvariable variable=$Name]$($Value)"
    } else {
        Write-Verbose "Setting local environment variable: $Name = ***"
        Set-Item -Path "env:$Name" -Value $Value
    }
}

try {
    New-Item -ItemType Directory -Path $TargetPath -Force
    Push-Location $TargetPath

    git clone https://github.com/Microsoft/vcpkg .
    git checkout (Get-Content "$PSScriptRoot/../vcpkg-ref.txt")

    if ($IsWindows) {
        .\bootstrap-vcpkg.bat
        .\vcpkg.exe install $Dependencies.Split(' ')
    } else {
        ./bootstrap-vcpkg.sh
        ./vcpkg install $Dependencies.Split(' ')
    }

    SetEnvironmentVariable -Name Path -Value "$TargetPath;$env:PATH"
    SetEnvironmentVariable -Name VCPKG_INSTALLATION_ROOT -Value $TargetPath

} finally {
    Pop-Location
}
