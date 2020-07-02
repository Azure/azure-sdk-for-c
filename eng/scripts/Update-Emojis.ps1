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

$files = Get-ChildItem -Path $Path -Filter $Filter -Recurse

foreach($file in $files) {
    Write-Verbose "Replacing content in $file"
    $fileContent = Get-Content $file
    $fileContent -replace '(^|[^:])(:[a-zA-Z0-9_-]+?:)($|[^:])', '$1\emoji $2$3' | Set-Content -Path $file
}