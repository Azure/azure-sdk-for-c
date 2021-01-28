[CmdletBinding()]
Param (
    [Parameter()]
    [ValidateNotNullOrEmpty()]
    [string] $TargetRef = 'master'
)

if ((Get-Command npx | Measure-Object).Count -eq 0) { 
    Write-Error "Could not locate npx. Install NodeJS (includes npm and npx) https://nodejs.org/en/download/"
    exit 1
}

$initialDirectory = Get-Location
try { 
    Set-Location "$PSScriptRoot/../../.."

    # Lists names of files that were in some way changed between the 
    # current ref and $TargetRef. Excludes files that were deleted to
    # prevent errors in Resolve-Path
    $changedFiles = git diff --diff-filter=d --name-only $TargetRef `
        | Resolve-Path `
        | Join-String -Separator ' '
    
    Write-Host "npx cspell --config .\cspell.json $changedFiles"
    Invoke-Expression "npx cspell --config .\cspell.json $changedFiles"
} finally {
    Set-Location $initialDirectory
}