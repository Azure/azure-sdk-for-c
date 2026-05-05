# Content Return Types

These are **fundamentally different representations** and should NOT be treated as equivalent.

## Text Format (`contentReturnType=text`)

Human-readable language-specific code representation of the API surface:

- **Use when:** User wants readable, structured code with language-appropriate syntax highlighting
- **Format:** Language-specific code (C#, Java, Python, TypeScript, etc.)
- **File extension:** Language-appropriate (`.cs`, `.java`, `.py`, `.ts`, `.go`, etc.)
- **Formatting rule:** Apply language-appropriate pretty-printing (indentation, line breaks, proper code structure) to the retrieved content before saving.

### Best For

- Code review and visual inspection
- Understanding public API signatures
- Documentation and examples
- Comparing API surfaces across versions

## Code File Format (`contentReturnType=codefile`)

JSON-structured code representation of the API surface:

- **Use when:** User needs structured API metadata, diagnostics, or machine-readable format
- **Format:** JSON with standardized APIView schema containing:
  - Package metadata (name, version, language)
  - API tokens (classes, methods, properties, etc.)
  - Diagnostics and validation warnings
  - Version information
- **File extension:** `.json`
- **Formatting rule:** Apply JSON pretty-printing with proper indentation (standard 2 or 4 spaces) to the retrieved content before saving.

### Best For

- Programmatic analysis and processing
- API metadata extraction
- Validation and diagnostics
- Integration with automated tools
