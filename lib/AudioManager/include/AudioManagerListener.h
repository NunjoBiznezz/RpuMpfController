// AudioManagerListener.h
// Interface for receiving AudioManager track events
#pragma once

#include <stdint.h>

class AudioManagerListener {
public:
    virtual ~AudioManagerListener() {}
    virtual void onTrackStarted(uint16_t trackId) = 0;
    virtual void onTrackStopped(uint16_t trackId) = 0;
};
