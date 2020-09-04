[CmdletBinding()]
Param (
    [ValidateNotNullOrEmpty()]
    [string] $Ref = 'master',

    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string] $Dependencies,

    [string] $TargetPath ="$env:TEMP/$([guid]::NewGuid())",

    [boolean] $CI
)

function OutputWarning {
    param([string] $Output)

    if ($CI) {
        Write-Host "##vso[task.logissue type=warning]$Output"
    } else {
        Write-Warning $Output
    }
}

function SetEnvironmentVariable {
    param(
        [string] $Name,
        [string] $Value
    )

    if ($CI) {
        Write-Host "##vso[task.setvariable variable=_$Name;issecret=true;]$($Value)"
        Write-Host "##vso[task.setvariable variable=$Name;]$($Value)"
    } else {
        Write-Verbose "Setting local environment variable: $Name = ***"
        Set-Item -Path "env:$Name" -Value $Value
    }
}


New-Item -ItemType Directory -Path $TargetPath -Force
pushd $TargetPath

git clone https://github.com/Microsoft/vcpkg .
git checkout (Get-Content "$PSScriptRoot/../vcpkg.ref.txt")

if ($IsWindows) {
    .\bootstrap-vcpkg.bat
} else {
    ./bootstrap-vcpkg.sh
}

./vcpkg install $Dependencies

SetEnvironmentVariable -Name Path -Value "$TargetPath;$env:PATH"
SetEnvironmentVariable -Name VCPKG_INSTALLATION_ROOT -Value $TargetPath

trap {
    # If there is an error popd out to the original context
    popd
}

popd