# Generates an index page for cataloging different versions of the Docs

[CmdletBinding()]
Param (
    $RepoRoot,
    $DocGenDir
)

$ServiceMapping = @{
    "core"="Core";
    "storage"="Storage";
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

Write-Verbose "Creating Index using service directory and package names from repo..."
$ServiceList = Get-ChildItem "$($RepoRoot)/sdk" -Directory -Exclude eng, mgmtcommon, template | Sort-Object
$YmlPath = "${DocOutDir}/api"
New-Item -Path $YmlPath -Name "toc.yml" -Force

$TargetServices = $ServiceList | Where-Object { $ServiceMapping.Contains($_.Name) }

Write-Verbose "Creating Index for client packages..."
foreach ($Dir in $TargetServices)
{
    # Generate a new top-level md file for the service
    New-Item -Path $YmlPath -Name "$($Dir.Name).md" -Force

    # Add service to toc.yml
    $ServiceName = If ($ServiceMapping.Contains($Dir.Name)) { $ServiceMapping[$Dir.Name] } Else { $Dir.Name }
    Add-Content -Path "$($YmlPath)/toc.yml" -Value "- name: $($ServiceName)`r`n  href: $($Dir.Name).md"

    $PkgList = Get-ChildItem $Dir.FullName -Directory -Exclude .vs, .vscode

    if (($PkgList | Measure-Object).count -eq 0)
    {
        continue
    }
    Add-Content -Path "$($YmlPath)/$($Dir.Name).md" -Value "# Client"
    Add-Content -Path "$($YmlPath)/$($Dir.Name).md" -Value "---"
    Write-Verbose "Operating on Client Packages for $($Dir.Name)"

    # Generate a new md file for each package in the service
    foreach ($Pkg in $PkgList)
    {
        if (Test-Path "$($pkg.FullName)\package.txt")
        {
            $ProjectName = Get-Content "$($pkg.FullName)\package.txt"
            Add-Content -Path "$($YmlPath)/$($Dir.Name).md" -Value "#### $($ProjectName)"
        }
    }
}


Write-Verbose "Creating Site Title and Navigation..."
New-Item -Path "${DocOutDir}" -Name "toc.yml" -Force
Add-Content -Path "${DocOutDir}/toc.yml" -Value "- name: Azure SDK for C APIs`r`n  href: api/`r`n  homepage: api/index.md"

Write-Verbose "Copying root markdowns"
Copy-Item "$($RepoRoot)/README.md" -Destination "${DocOutDir}/api/index.md" -Force
Copy-Item "$($RepoRoot)/CONTRIBUTING.md" -Destination "${DocOutDir}/api/CONTRIBUTING.md" -Force

Write-Verbose "Building site..."
& "${DocFxTool}" build "${DocOutDir}/docfx.json"

Copy-Item "${DocGenDir}/assets/logo.svg" -Destination "${DocOutDir}/_site/" -Force
Copy-Item "${DocGenDir}/assets/toc.yml" -Destination "${DocOutDir}/_site/" -Force