<#
.SYNOPSIS
Bumps up sdk version after release

.DESCRIPTION
This script bumps up the sdk version found in az_version.h following conventions defined at https://github.com/Azure/azure-sdk/blob/master/docs/policies/releases.md#incrementing-after-release-c
We use the version number defined in AZ_SDK_VERSION_STRING, and then overwrite other #defines

.PARAMETER RepoRoot
The Root of the repo

.PARAMETER NewVersionString
Use this to overide version incement logic and set a version specified by this parameter


.EXAMPLE
Updating sdk version to next preview version
Update-SdkVersion.ps1

Updating sdk version with a specified verion
Update-SdkVersion.ps1 -NewVersionString 2.0.5

#>

[CmdletBinding()]
Param (
    [ValidateNotNullOrEmpty()]
    [string] $RepoRoot = "${PSScriptRoot}/..",
    [string] $NewVersionString
)

. ${PSScriptRoot}\common\scripts\SemVer.ps1

# Updated Version in version file and changelog using computed or set NewVersionString
function Update-Version([AzureEngSemanticVersion]$SemVer, $Unreleased=$True, $ReplaceVersion=$False)
{
    Write-Output "New Version: $($SemVer)"
    if ($SemVer.HasValidPrereleaseLabel() -ne $true){
        Write-Error "Invalid prerelease label"
        exit 1
    }

    if ($SemVer.IsPrerelease -eq $true){
        $PrereleaseDefine = "`#define AZ_SDK_VERSION_PRERELEASE `"$($SemVer.PrereleaseLabel).$($SemVer.PrereleaseNumber)`""
    }
    else{
        $PrereleaseDefine = "#undef AZ_SDK_VERSION_PRERELEASE"
    }

    (Get-Content -Path $PackageVersionPath -Raw) `
     -replace $VersionStringRegEx, "`${1}`"$($SemVer)`"" `
     -replace $VersionMajorRegEx,  "`${1}$($SemVer.Major)" `
     -replace $VersionMinorRegEx,  "`${1}$($SemVer.Minor)" `
     -replace $VersionPatchRegEx,  "`${1}$($SemVer.Patch)" `
     -replace $VersionPrereleaseRegEx,  $PrereleaseDefine |
     Set-Content -Path $PackageVersionPath -NoNewline

    # Increment Version in ChangeLog file
    & "${PSScriptRoot}/common/Update-Change-Log.ps1" -Version $SemVer.ToString() -ChangeLogPath $ChangelogPath -Unreleased $Unreleased -ReplaceVersion $ReplaceVersion
}

# Obtain Current Package Version
# We use the version number defined in AZ_SDK_VERSION_STRING, and then overwrite other #defines

$PackageVersionPath = Join-Path $RepoRoot "sdk\core\az_core\inc\az_version.h"
$ChangelogPath = Join-Path $RepoRoot "sdk\core\az_core\CHANGELOG.md"
$PackageVersionFile = Get-Content -Path $PackageVersionPath -Raw
$VersionStringRegEx = '(#define AZ_SDK_VERSION_STRING )"(([0-9]+)\.([0-9]+)\.([0-9]+)(\-[^\"\-]+)?)"';
$VersionMajorRegEx = '(#define AZ_SDK_VERSION_MAJOR )([0-9]+)';
$VersionMinorRegEx = '(#define AZ_SDK_VERSION_MINOR )([0-9]+)';
$VersionPatchRegEx = '(#define AZ_SDK_VERSION_PATCH )([0-9]+)';
$VersionPrereleaseRegEx = '(#(un)?def(ine)? AZ_SDK_VERSION_PRERELEASE) ?("[a-z0-9.]+")?';

$match = $PackageVersionFile -match $VersionStringRegEx
$PackageVersion = $Matches[2]

if ([System.String]::IsNullOrEmpty($NewVersionString))
{
    $SemVer = [AzureEngSemanticVersion]::new($PackageVersion)
    Write-Output "Current Version: ${PackageVersion}"

    $SemVer.IncrementAndSetToPrerelease()
    Update-Version -SemVer $SemVer
}
else
{
    # Use specified VersionString
    $SemVer = [AzureEngSemanticVersion]::new($NewVersionString)
    Update-Version -SemVer $SemVer -Unreleased $False -ReplaceVersion $True
}
