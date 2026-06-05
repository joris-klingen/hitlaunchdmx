#pragma once

#include <juce_core/juce_core.h>
#include <array>
#include <atomic>
#include <optional>

namespace hitlaunchdmx
{

struct MidiLogEntry
{
    enum class Kind : juce::uint8 { NoteOn, NoteOff };
    Kind           kind     { Kind::NoteOn };
    juce::uint8    channel  { 1 };
    juce::uint8    pitch    { 0 };
    juce::uint8    velocity { 0 };
};

class MidiLog
{
public:
    static constexpr int kCapacity = 256;

    MidiLog() = default;
    ~MidiLog() = default;

    void push(const MidiLogEntry& e) noexcept
    {
        const int w    = writeIndex.load(std::memory_order_relaxed);
        const int next = (w + 1) % kCapacity;
        if (next == readIndex.load(std::memory_order_acquire))
            return;
        buffer[w] = e;
        writeIndex.store(next, std::memory_order_release);
    }

    std::optional<MidiLogEntry> pop() noexcept
    {
        const int r = readIndex.load(std::memory_order_relaxed);
        if (r == writeIndex.load(std::memory_order_acquire))
            return std::nullopt;
        const auto e = buffer[r];
        readIndex.store((r + 1) % kCapacity, std::memory_order_release);
        return e;
    }

private:
    std::array<MidiLogEntry, kCapacity> buffer {};
    std::atomic<int> readIndex  { 0 };
    std::atomic<int> writeIndex { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiLog)
};

}  // namespace hitlaunchdmx
