[CmdletBinding()]
Param (
    [Parameter()]
    [ValidateNotNullOrEmpty()]
    [string] $Ref = (Get-Content "$PSScriptRoot/../vcpkg-commit.txt"),

    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string] $Dependencies,

    [Parameter()]
    [ValidateNotNullOrEmpty()]
    [string] $VcpkgPath = "$PSScriptRoot/../../vcpkg"
)

$initialDirectory = Get-Location

try {
    Write-Host "Cloning into: $VcpkgPath"
    git clone https://github.com/Microsoft/vcpkg $VcpkgPath
    Set-Location $VcpkgPath
    git fetch --tags
    Write-Host "Checking-out: $Ref"
    git checkout $Ref

    if ($IsWindows) {
        .\bootstrap-vcpkg.bat
        .\vcpkg.exe install $Dependencies.Split(' ')
    } else {
        ./bootstrap-vcpkg.sh
        ./vcpkg install $Dependencies.Split(' ')
    }
} finally {
    Set-Location $initialDirectory
}
