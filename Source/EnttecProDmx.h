#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <string>
#include <vector>
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

namespace hitlaunchdmx
{

inline constexpr int kDmxUniverseSize = 512;

class EnttecProDmx : private juce::HighResolutionTimer
{
public:
    static constexpr int kSendRateHz = 40;

    EnttecProDmx();
    ~EnttecProDmx() override;

    bool connect();
    void disconnect();
    bool isConnected() const { return connected.load(); }

    int  scanDevices();
    int  numDevices() const  { return numDevicesDetected; }

    juce::String getStatusText() const;

    void setChannel(int channel1to512, juce::uint8 value);
    void setBlackout(bool enabled) { blackout.store(enabled); }

    void  setStrobeHz(float hz) { strobeHz.store(hz); }
    float getStrobeHz() const   { return strobeHz.load(); }

private:
    void hiResTimerCallback() override;

    bool openPort(const std::string& devicePath);
    bool sendDmxFrame();
    int  sendPacket(int label, const unsigned char* data, int length);
    int  receivePacket(int label, unsigned char* data, unsigned int expectedLength);
    bool readByte(unsigned char& out);
    void closePort();

    std::atomic<bool>  connected { false };
    std::atomic<bool>  blackout  { false };
    std::atomic<float> strobeHz  { 0.0f };
    std::uint64_t      txFrame   { 0 };
    int numDevicesDetected { 0 };

    static constexpr int kDataLen = 513;
    std::array<unsigned char, kDataLen> dmxData {};
    std::array<unsigned char, kDataLen> blackoutData {};

    int serialFd { -1 };
    std::string selectedDevicePath;
    int firmwareMajor { 0 }, firmwareMinor { 0 };
    int refreshRate { 0 };
    juce::String lastError;
    juce::CriticalSection dataLock;
};

}  // namespace hitlaunchdmx
