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
Updating package version for azure-core
Update-PkgVersion.ps1 -ServiceDirectory core -PackageName core

Updating package version for azure-core with a specified verion
Update-PkgVersion.ps1 -ServiceDirectory core -PackageName core -NewVersionString 2.0.5

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

# Updated Version in version file and changelog using computed or set NewVersionString
function Update-Version([AzureEngSemanticVersion]$SemVer, $Unreleased=$True, $ReplaceVersion=$False)
{
    Write-Verbose "New Version: $SemVer"
    if ($SemVer.HasValidPrereleaseLabel() -ne $true){
        Write-Error "Invalid prerelease label"
        exit 1
    }

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
    Update-Version -SemVer $SemVer
}
else
{
    # Use specified VersionString
    $SemVer = [AzureEngSemanticVersion]::new($NewVersionString)
    Update-Version -SemVer $SemVer -Unreleased $False -ReplaceVersion $True
}
