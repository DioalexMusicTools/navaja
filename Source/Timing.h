#pragma once
#include <algorithm>
#include <array>

namespace dc23 {
struct NoteDivision { const char* label; float quarterNotes; };
inline constexpr std::array<NoteDivision, 7> noteDivisions {{
    {"1/2", 2.0f}, {"1/4", 1.0f}, {"1/8 D", 0.75f},
    {"1/8", 0.5f}, {"1/8 T", 1.0f/3.0f},
    {"1/16", 0.25f}, {"1/32", 0.125f}
}};
// Host BPM -> musical delay in milliseconds. Fallback handled by caller.
inline float syncedEchoMilliseconds(int index, float bpm) noexcept {
    index = std::clamp(index, 0, int(noteDivisions.size()) - 1);
    return 60000.0f * noteDivisions[static_cast<size_t>(index)].quarterNotes
         / std::clamp(bpm, 30.0f, 300.0f);
}
} // namespace dc23
