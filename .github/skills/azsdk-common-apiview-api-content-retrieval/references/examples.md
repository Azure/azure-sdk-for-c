# Examples

## Canonical Example: C# Storage Package

**Query:** "Show me the API surface for Azure.Storage.Blobs v12.14.0"

**Steps:**

1. Call `azsdk_apiview_get_review_url` with:
   - `package = Azure.Storage.Blobs`
   - `language = C#`
   - `version = 12.14.0`

2. Call `azsdk_apiview_get_content` with:
   - `apiViewUrl = <URL from step 1>`
   - `contentReturnType = text` (for language-specific code)

3. Save retrieved content:
   - **Filename:** `Azure.Storage.Blobs-12.14.0.cs` (C# extension for syntax highlighting)
   - **Formatting:** Apply C# code formatting (indentation, line breaks, proper structure)
   - **Location:** Workspace root

4. **Display result:**
   - File location and quick access link
   - Summary of main namespaces (e.g., `Azure.Storage.Blobs`, `Azure.Storage.Blobs.Models`, `Azure.Storage.Blobs.Specialized`)
   - Key classes (e.g., `BlobContainerClient`, `BlobClient`)
   - Notable public methods and properties

## Example: Java Package (Latest Version)

**Query:** "Get the API surface for Azure Storage Blob SDK Java"

1. Call `azsdk_apiview_get_review_url` with:
   - `package = azure-storage-blob`
   - `language = Java`
   - `version` = omitted (gets latest)

2. Call `azsdk_apiview_get_content` with:
   - `apiViewUrl = <URL from step 1>`
   - `contentReturnType = text`

3. Save as `azure-storage-blob-{detected-version}.java`

## Example: JSON Code File Format

**Query:** "Retrieve the APIView code file for Python SDK Azure Identity in JSON format"

1. Call `azsdk_apiview_get_review_url` with:
   - `package = azure-identity`
   - `language = Python`

2. Call `azsdk_apiview_get_content` with:
   - `apiViewUrl = <URL from step 1>`
   - `contentReturnType = codefile` (JSON structure with metadata)

3. Extract payload and save:
    - Keep only the CodeFile JSON payload.
    - Strip envelope keys like `operation_status`, `result`, and tool metadata if present.
    - Save as `azure-identity-{version}.json` with JSON pretty-printing.

4. Display: File location
