# Embedded default resources

**Covers:** The authoritative `.enigmaxml` and gzip resources, how the C++ byte arrays are generated, and their expected SHA-256 values.
**Read when:** Touching `src/defaults/`, `resources/defaults/`, or `scripts/generate_embedded_defaults.py`.
**Confidence:** confirmed; the hashes are the check.

Keep the raw `.enigmaxml` files as authoritative, inspectable source artifacts.
They are Finale-generated files with intentional CRLF endings; preserve their
exact bytes even though `git diff --check` reports carriage returns as trailing
whitespace. Use the committed deterministic `gzip -n -9` files as the inputs to
the resource-generation script.

Generate and commit the C++ byte arrays under `src/defaults/` with
`scripts/generate_embedded_defaults.py`; do not generate them during a normal
CMake build and do not edit them by hand. Run the script with `--check` to detect
stale output. Do not wrap the XML in ZIP or MUSX containers. Tests should cover
inflation, expected byte counts or hashes, platform selection, musxdom parsing,
and the presence of required option instances.

Expected SHA-256 values:

| Resource | SHA-256 |
| --- | --- |
| macOS EnigmaXML | `cebcc5af8d625979e1baa11c7350a1fc1cbb8475c776bdb5c34aea059e9a9120` |
| macOS gzip | `c58e69ab810451f7b295b3fe1e5545f9e1dd9d064b10e84c9253fe7a90a1ff66` |
| Windows EnigmaXML | `b151b38bd48580db7dd64a73b1364323936391abb19d74e424f27d35070fd2cb` |
| Windows gzip | `745444c37c44c13b17c72e1c6aad9f05e3e04ac2ab04bce027a2f55850201a5f` |
