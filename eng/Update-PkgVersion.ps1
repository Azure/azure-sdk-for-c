<#
.SYNOPSIS
Bumps up package versions after release

.DESCRIPTION
This script bumps up package versions following conventions defined at https://github.com/Azure/azure-sdk/blob/master/docs/policies/releases.md#incrementing-after-release-cpp

.PARAMETER RepoRoot
The Root of the repo

.PARAMETER ServiceDirectory
The Name of the Service Directory

.PARAMETER PackageName
The Name of the Package

.PARAMETER PackageDirName
Used in the case where the package directory name is different from the package name. e.g in cognitiveservice packages

.PARAMETER NewVersionString
Use this to overide version incement logic and set a version specified by this parameter


.EXAMPLE
Updating package version for Azure.Core
Update-PkgVersion.ps1 -ServiceDirectory core -PackageName Azure.Core

Updating package version for Azure.Core with a specified verion
Update-PkgVersion.ps1 -ServiceDirectory core -PackageName Azure.Core -NewVersionString 2.0.5

Updating package version for Microsoft.Azure.CognitiveServices.AnomalyDetector
Update-PkgVersion.ps1 -ServiceDirectory cognitiveservices -PackageName Microsoft.Azure.CognitiveServices.AnomalyDetector -PackageDirName AnomalyDetector

#>

[CmdletBinding()]
Param (
    [ValidateNotNullOrEmpty()]
    [string] $RepoRoot = "${PSScriptRoot}/..",
    [Parameter(Mandatory=$True)]
    [string] $ServiceDirectory,
    [Parameter(Mandatory=$True)]
    [string] $PackageName,
    [string] $PackageDirName,
    [string] $NewVersionString
)

. ${PSScriptRoot}\common\scripts\SemVer.ps1

# Updated Version in csproj and changelog using computed or set NewVersionString
function Update-Version($Unreleased=$True, $ReplaceVersion=$False)
{
    Write-Verbose "New Version: $SemVer"
    Set-Content -Path $PackageVersionPath -Value $SemVer.ToString()

    # Increment Version in ChangeLog file
    & "${PSScriptRoot}/common/Update-Change-Log.ps1" -Version $SemVer.ToString() -ChangeLogPath $ChangelogPath -Unreleased $Unreleased -ReplaceVersion $ReplaceVersion
}

# Obtain Current Package Version
if ([System.String]::IsNullOrEmpty($PackageDirName)) {$PackageDirName = $PackageName}
$PackageVersionPath = Join-Path $RepoRoot "sdk" $ServiceDirectory $PackageDirName "version.txt"
$ChangelogPath = Join-Path $RepoRoot "sdk" $ServiceDirectory $PackageDirName "CHANGELOG.md"
$PackageVersion = Get-Content -Path $PackageVersionPath

if ([System.String]::IsNullOrEmpty($NewVersionString))
{
    $SemVer = [AzureEngSemanticVersion]::new($PackageVersion)
    Write-Verbose "Current Version: ${PackageVersion}"

    $SemVer.IncrementAndSetToPrerelease()
    Update-Version
}
else
{
    # Use specified VersionString
    $SemVer = [AzureEngSemanticVersion]::new($NewVersionString)
    Update-Version -Unreleased $False -ReplaceVersion $True
}
