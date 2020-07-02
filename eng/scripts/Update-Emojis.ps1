<#
.SYNOPSIS
Replace emoji :<name>: expressions with \emoji :<name>: syntax that Doxygen's Markdown renderer can use

.DESCRIPTION
This script serches recursively for files with the given expression and replaces
emojis using a regular expression.

.PARAMETER Path
Path to search. The default is the current working directory (.\)

.Parameter Filter
File filter to search. The default is *.md

#>

[CmdletBinding()]
Param (
    [string] $Path = '.\',
    [string] $Filter = '*.md'
)

# Fetch a list of available emojis from the GitHub API
$availableEmojis = ((Invoke-WebRequest 'https://api.github.com/emojis' ).Content | ConvertFrom-Json -AsHashtable)

$files = Get-ChildItem -Path $Path -Filter $Filter -Recurse

foreach($file in $files) {
    Write-Verbose "Replacing content in $file"
    $fileContent = Get-Content $file

    $emojiCandidates = [regex]::Matches($fileContent, ':([a-zA-Z0-9_-]+?):') | ForEach-Object { $_.Groups[1].Value }

    $matchSet = @{ }

    foreach($match in $emojiCandidates) {
        if (-not $matchSet.ContainsKey($match) -and $availableEmojis.ContainsKey($match)) {
            $matchSet[$match] = $true
        }
    }

    foreach($key in $matchSet.Keys) {
        Write-Verbose "Replacing $key"
        $fileContent = $fileContent.Replace(":$($key):", "\emoji :$($key):")
    }

    if ($matchSet.Keys.Count -gt 0) {
        Set-Content $file -Value $fileContent
    }
}