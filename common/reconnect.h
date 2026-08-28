#ifndef RECONNECT_H
#define RECONNECT_H

namespace Reconnect {

constexpr int kHeartbeatIntervalMs = 1000;
constexpr int kWatchdogTimeoutMs   = 3000;
constexpr int kInitialDelayMs      = 1000;
constexpr int kMaxDelayMs          = 16000;

inline int nextDelayMs(int currentDelayMs)
{
    if (currentDelayMs < kInitialDelayMs)
        return kInitialDelayMs;

    const int doubled = currentDelayMs * 2;
    return doubled > kMaxDelayMs ? kMaxDelayMs : doubled;
}

} // namespace Reconnect

#endif // RECONNECT_H
