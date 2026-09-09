// ============================================================================
// APPS: known developer tools — src/system/data/apps.inc
// Hand-curated (small, stable) — edit directly, unlike ai/data .inc files
// which are generator-owned. Included ONLY by app_detect.c.
//
// 14 AppSlot rows: (name, altProcs, displayName, note).
//   name     = canonical executable/bundle name (lowercase when CLI).
//   altProcs = space-separated alternative proc basenames (desktop app
//              binaries, versioned CLI names); NULL when none.
// Detection is name-based (PATH/bundle/process); see ;;INTENTION in
// app_detect.c for scope boundaries. "flags" is pending owner detail.
// ============================================================================
static const AppSlot kKnownApps[] = {

    {
        "opencode",
        NULL,
        "opencode",
        "agentic coding CLI + desktop (opencode.ai)",
    },
    {
        "codex",
        NULL,
        "codex",
        "OpenAI Codex CLI",
    },
    {
        "claude",
        NULL,
        "Claude Code",
        "Anthropic Claude Code CLI",
    },
    {
        "t3",
        "T3 Code",
        "T3 Code",
        "agent harness control plane (pingdotgg/t3code); npx t3 or desktop app",
    },
    {
        "cursor",
        "cursor-agent",
        "Cursor",
        "Cursor editor app; canonical CLI is cursor-agent",
    },
    {
        "hermes",
        NULL,
        "Hermes",
        "Nous Research Hermes tooling / API client",
    },
    {
        "nous",
        NULL,
        "Nous Research",
        "Nous Research CLI / provider tooling",
    },
    {
        "grok",
        NULL,
        "Grok",
        "xAI Grok Build CLI",
    },
    {
        "gemini",
        NULL,
        "Gemini",
        "Google Gemini CLI",
    },
    {
        "aider",
        NULL,
        "Aider",
        "pair-programming CLI (aider.chat)",
    },
    {
        "goose",
        NULL,
        "Goose",
        "Block Goose coding agent",
    },
    {
        "cline",
        NULL,
        "Cline",
        "Cline coding agent CLI",
    },
    {
        "qwen-code",
        NULL,
        "Qwen Code",
        "Alibaba Qwen Code CLI",
    },
    {
        "continue",
        NULL,
        "Continue",
        "Continue.dev agent CLI (npm)",
    },
};