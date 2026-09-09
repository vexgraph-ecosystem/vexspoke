// ============================================================================
// CAPTURE TOOLS: known capture-capable apps — src/system/data/capture_tools.inc
// Hand-curated (small, stable) — edit directly. Included ONLY by
// capture_tool.c (Rule 3 slot-record co-location).
//
// 15 CaptureSlot rows: (slug, displayName, procKey, driverDir, kind, note).
//   procKey != NULL  -> process row; probed via ProcessProbe_isRunning.
//   driverDir != NULL -> driver row (CoreAudio HAL plugin); probed via
//                        ProcessProbe_isDriverLoaded(dir, slug).
// kind: CAPTURE_KIND_SCREEN / STREAM / AUDIO.
// ============================================================================
static const CaptureSlot kCaptureTools[] = {

    {
        "obs",
        "OBS Studio",
        "obs",
        NULL,
        CAPTURE_KIND_SCREEN,
        "recording + streaming; window title shows state when entitled",
    },
    {
        "quicktime-player",
        "QuickTime Player",
        "QuickTime Player",
        NULL,
        CAPTURE_KIND_SCREEN,
        "macOS-native recorder (ScreenCaptureKit)",
    },
    {
        "screenshot",
        "Screenshot",
        "screencaptureui",
        NULL,
        CAPTURE_KIND_SCREEN,
        "macOS Cmd+Shift+5 capture utility",
    },
    {
        "screen-studio",
        "Screen Studio",
        "Screen Studio",
        NULL,
        CAPTURE_KIND_SCREEN,
        "polished mac screen recorder",
    },
    {
        "cleanshot-x",
        "CleanShot X",
        "CleanShot X",
        NULL,
        CAPTURE_KIND_SCREEN,
        "capture + annotation suite",
    },
    {
        "kap",
        "Kap",
        "Kap",
        NULL,
        CAPTURE_KIND_SCREEN,
        "open-source GIF/screen recorder",
    },
    {
        "loom",
        "Loom",
        "Loom",
        NULL,
        CAPTURE_KIND_SCREEN,
        "async video + screen sharing",
    },
    {
        "screenflow",
        "ScreenFlow",
        "ScreenFlow",
        NULL,
        CAPTURE_KIND_SCREEN,
        "screencast editor",
    },
    {
        "camtasia",
        "Camtasia",
        "Camtasia 2023",
        NULL,
        CAPTURE_KIND_SCREEN,
        "versioned bundle name may grow (e.g. Camtasia 2025)",
    },
    {
        "filmage",
        "Filmage",
        "Filmage",
        NULL,
        CAPTURE_KIND_SCREEN,
        "video editor + screen recorder",
    },
    {
        "zoom",
        "Zoom",
        "zoom.us",
        NULL,
        CAPTURE_KIND_STREAM,
        "Zoom meetings / presentations",
    },
    {
        "voice-memos",
        "Voice Memos",
        "Voice Memos",
        NULL,
        CAPTURE_KIND_AUDIO,
        "macOS-native audio capture",
    },
    {
        "loopback",
        "Loopback",
        "Loopback",
        NULL,
        CAPTURE_KIND_AUDIO,
        "Rogue Amoeba virtual audio device",
    },
    {
        "blackhole",
        "BlackHole",
        NULL,
        "/Library/Audio/Plug-Ins/HAL",
        CAPTURE_KIND_AUDIO,
        "virtual audio driver (not a process)",
    },
    {
        "soundflower",
        "Soundflower",
        NULL,
        "/Library/Audio/Plug-Ins/HAL",
        CAPTURE_KIND_AUDIO,
        "legacy virtual audio driver (not a process)",
    },
};