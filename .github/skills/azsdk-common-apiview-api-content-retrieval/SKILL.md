---
name: azsdk-common-apiview-api-content-retrieval
license: MIT
metadata:
  version: "1.0.0"
  distribution: shared
description: "Retrieve APIView API surface content for a package/version. **UTILITY SKILL**. WHEN: \"show me the API surface\", \"retrieve API content\", \"give me the CodeFile\". DO NOT USE FOR: APIView URL resolution only or APIView metadata lookups. INVOKES: azure-sdk-mcp:azsdk_apiview_get_review_url, azure-sdk-mcp:azsdk_apiview_get_content."
compatibility:
  requires: "azure-sdk-mcp server"
---

# APIView Surface Retrieval

**Prerequisites:** `azure-sdk-mcp` server required.

Use MCP APIView tools as the only supported path for API surface retrieval.

## Hard Rules

- MCP-first and MCP-only for retrieval.
- Do not read local artifacts unless user explicitly asks for local files.
- Do not switch to browser/manual lookup for retrieval.
- If MCP is unavailable, stop and report the blocker.

## MCP Tools

| Tool | Purpose |
|------|---------|
| `azure-sdk-mcp:azsdk_apiview_get_review_url` | Resolve APIView review URL from package/language (version optional) |
| `azure-sdk-mcp:azsdk_apiview_get_content` | Retrieve API surface content (text or codefile format) from the resolved URL |

## Steps

1. **Read Referenced Guidance** — Read linked files under `Reference Materials` before making retrieval decisions.
2. **Check for Direct URL** — If user provided an APIView URL directly, skip to step 5. Otherwise, continue to step 3.
3. **Parse Query Inputs** — Extract package name (and optionally version) from the user query. Infer language when clear; otherwise ask one concise clarification.
4. **Resolve Review URL** — Call `azsdk_apiview_get_review_url` with package, language, and version if provided (skip if URL was provided in step 2).
5. **Choose Content Format** — Determine which format user needs:
   - **Text format (`text`)** — Human-readable API signatures for code review and visual inspection
   - **Code file format (`codefile`)** — JSON structure for programmatic analysis and tooling
   - See [Content Formats](references/content-formats.md) for detailed comparison and use cases
6. **Fetch API Surface** — Call `azsdk_apiview_get_content` with `apiViewUrl=<result from step 4 or user-provided URL>` and chosen `contentReturnType`.
7. **Extract, Format, and Save Content** — Save a nicely formatted artifact to the workspace (or user-specified directory) using `{package}-{version}.{ext}` naming.
  - Follow [Content Formats](references/content-formats.md) for format-specific extraction and formatting rules.
  - Follow [Examples](references/examples.md) for canonical filename and save behavior.
8. **Return Result** — Link the resulting file

## Reference Materials

- [Content Formats](references/content-formats.md) — Detailed comparison of text vs. code file formats
- [Examples](references/examples.md) — Canonical examples and common scenarios
- [Troubleshooting](references/troubleshooting.md) — Solutions for common issues