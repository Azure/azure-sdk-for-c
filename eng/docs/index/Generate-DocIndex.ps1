# Generates an index page for cataloging different versions of the Docs

[CmdletBinding()]
Param (
    $RepoRoot,
    $DocGenDir
)

$ServiceMapping = @{
    "core"="Core";
    "iot"="IoT";
}

Write-Verbose "Name Reccuring paths with variable names"
$DocFxTool = "${RepoRoot}/docfx/docfx.exe"
$DocOutDir = "${RepoRoot}/docfx_project"

Write-Verbose "Initializing Default DocFx Site..."
& "${DocFxTool}" init -q -o "${DocOutDir}"

Write-Verbose "Copying template and configuration..."
New-Item -Path "${DocOutDir}" -Name "templates" -ItemType "directory"
Copy-Item "${DocGenDir}/templates/*" -Destination "${DocOutDir}/templates" -Force -Recurse
Copy-Item "${DocGenDir}/docfx.json" -Destination "${DocOutDir}/" -Force

$YmlPath = "${DocOutDir}/api"
New-Item -Path $YmlPath -Name "toc.yml" -Force

Write-Verbose "Creating Index for client packages..."
foreach ($ServiceKey in $ServiceMapping.Keys | Sort-Object)
{
    # Generate a new top-level md file for the service
    New-Item -Path $YmlPath -Name "$($ServiceKey).md" -Force

    # Add service to toc.yml
    $ServiceName = $ServiceMapping[$ServiceKey]
    $CmakeContent = Get-Content "sdk/src/azure/$ServiceKey/CMakeLists.txt" -Raw
    $ProjectName = if ($CmakeContent -match 'project\s.*\(([\w].*?)\s') {
        $Matches[1]
    } else {
        "Azure SDK for Embedded C"
    }

    Add-Content -Path "$($YmlPath)/toc.yml" -Value "- name: $($ServiceName)`r`n  href: $($ServiceKey).md"

    Add-Content -Path "$($YmlPath)/$($ServiceKey).md" -Value "# Client"
    Add-Content -Path "$($YmlPath)/$($ServiceKey).md" -Value "---"
    Add-Content -Path "$($YmlPath)/$($ServiceKey).md" -Value "### $ProjectName"
    Write-Verbose "Operating on Client Packages for $($ServiceKey)"
}

Write-Verbose "Creating Site Title and Navigation..."
New-Item -Path "${DocOutDir}" -Name "toc.yml" -Force
Add-Content -Path "${DocOutDir}/toc.yml" -Value "- name: Azure SDK for C APIs`r`n  href: api/`r`n  homepage: api/index.md"

Write-Verbose "Copying root markdowns"
Copy-Item "$($RepoRoot)/README.md" -Destination "${DocOutDir}/api/index.md" -Force
Copy-Item "$($RepoRoot)/CONTRIBUTING.md" -Destination "${DocOutDir}/api/CONTRIBUTING.md" -Force

Write-Verbose "Building site..."
& "${DocFxTool}" build "${DocOutDir}/docfx.json"

Copy-Item "${DocGenDir}/assets/logo.svg" -Destination "${DocOutDir}/" -Force
Copy-Item "${DocGenDir}/assets/toc.yml" -Destination "${DocOutDir}/" -Force
Copy-Item "${DocGenDir}/assets/logo.svg" -Destination "${DocOutDir}/_site/" -Force
Copy-Item "${DocGenDir}/assets/toc.yml" -Destination "${DocOutDir}/_site/" -Force