$Language = "c"
$PackageRepository = "C"
$packagePattern = "*.json"
$MetadataUri = ""

# Parse out package publishing information given a vcpkg format.
function Get-c-PackageInfoFromPackageFile ($pkg, $workingDirectory) 
{
  $packageInfo = Get-Content -Raw -Path $pkg | ConvertFrom-JSON
  $packageArtifactLocation = (Get-ItemProperty $pkg).Directory.FullName
  $releaseNotes = ""
  $readmeContent = ""

  $pkgVersion = $packageInfo.version

  $changeLogLoc = @(Get-ChildItem -Path $packageArtifactLocation -Recurse -Include "CHANGELOG.md")[0]
  if ($changeLogLoc)
  {
    $releaseNotes = Get-ChangeLogEntryAsString -ChangeLogLocation $changeLogLoc -VersionString $pkgVersion
  }
  
  $readmeContentLoc = @(Get-ChildItem -Path $packageArtifactLocation -Recurse -Include "README.md")[0]
  if ($readmeContentLoc) 
  {
    $readmeContent = Get-Content -Raw $readmeContentLoc
  }

  return New-Object PSObject -Property @{
    PackageId      = 'azure-sdk-for-c'
    PackageVersion = $pkgVersion
    # Artifact info is always considered deployable for C because it is not
    # deployed anywhere. Dealing with duplicate tags happens downstream in
    # CheckArtifactShaAgainstTagsList
    Deployable     = $true
    ReleaseNotes   = $releaseNotes
  }
}

# Stage and Upload Docs to blob Storage
function Publish-c-GithubIODocs ($DocLocation, $PublicArtifactLocation)
{
    # The documentation publishing process for C differs from the other
    # languages in this file because this script is invoked for the whole SDK
    # publishing. It is not, for example, invoked once per service publishing.
    # There is a similar situation for other language publishing steps above...
    # Those loops are left over from previous versions of this script which were
    # used to publish multiple docs packages in a single invocation.
    $pkgInfo = Get-Content $DocLocation/package-info.json | ConvertFrom-Json
    $releaseTag = RetrieveReleaseTag "C" $PublicArtifactLocation 
    Upload-Blobs -DocDir $DocLocation -PkgName 'docs' -DocVersion $pkgInfo.version -ReleaseTag $releaseTag
}
