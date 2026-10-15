#pragma once
#include <memory>
#include "IAudioStream.h"

/// @brief Interface for playback of audio streams.
class IAudioStreamPlayer
{
public:
    virtual ~IAudioStreamPlayer() = 0;

    /// @brief Starts playback of the given audio stream.
    /// @param audioStream The stream to play.
    /// @return True if playback was successfully started, or false otherwise.
    virtual bool StartPlayback(std::unique_ptr<IAudioStream> audioStream) = 0;

    /// @brief Stops playback of the currently playing audio stream.
    virtual void StopPlayback() = 0;

    /// @brief DsRunner: sets the volume (0 to 127) for the next StartPlayback.
    virtual void SetVolume(u32 volume) = 0;
};

inline IAudioStreamPlayer::~IAudioStreamPlayer() { }
