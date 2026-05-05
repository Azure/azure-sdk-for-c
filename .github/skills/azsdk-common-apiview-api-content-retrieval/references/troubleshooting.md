# Troubleshooting

## Review URL not found

**Problem:** `azsdk_apiview_get_review_url` returns no result or 404 error

**Solutions:**
1. Confirm package name spelling and exact casing (e.g., `Azure.Storage.Blobs` not `azure-storage-blobs`)
2. Verify language is supported (C#, Java, Python, TypeScript, Go, etc.)
3. Check that version exists in APIView (e.g., `12.14.0` not `12.14`)
4. Try without version to get latest release

## Language ambiguous

**Problem:** Package name alone doesn't clearly indicate which SDK language is needed

**Solution:** Ask the user for clarification before retrying `get-review-url`:
- "Which SDK are you interested in? (C#, Java, Python, TypeScript, Go)"
- Use the user's explicit choice in the API call

## MCP unavailable

**Problem:** `azure-sdk-mcp` server is not connected or not responding

**Critical Rule:** Do NOT switch to browser-first or manual lookup workflows.

**Action:**
1. Stop the retrieval attempt
2. Report the blocker: "azure-sdk-mcp server is unavailable. Please ensure the MCP server is connected and retry."
3. Do not attempt alternative methods
