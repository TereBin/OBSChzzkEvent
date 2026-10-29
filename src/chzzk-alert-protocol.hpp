#pragma once

#include <QtGlobal>

namespace AlertProtocol {
inline constexpr quint16 port = 21345;
inline constexpr qint64 maxRequestLineBytes = 16 * 1024;
inline constexpr int requestTimeoutMs = 5000;
inline constexpr int responseDrainMs = 20;
inline constexpr int maxEventIdLength = 256;
inline constexpr int maxSessionLength = 128;
inline constexpr int maxPending = 100;
inline constexpr int maxBridges = 32;
inline constexpr qint64 bridgeTimeoutMs = 90000;
} // namespace AlertProtocol
