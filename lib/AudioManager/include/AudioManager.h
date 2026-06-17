// AudioManager.h
// Controls sequences of sound playback using WavTrigger as output
#pragma once

#include "AudioManagerListener.h"
#include "Sounds.h"
#include <stdint.h>
#include <WavListener.h>
#include <WavTrigger.h>

class AudioManagerListener;
class HardwareSerial;

enum class TrackType {
   SoundEffect,    // Normal sound effects that can be ducked
   Callout,        // Important callouts that duck other tracks
   BackgroundMusic // Background music that can be ducked
};

struct SoundEntry {
   TrackNumber_t trackNumber;
   unsigned long playTime;
};

class AudioManager : protected WavListener {
 public:
   AudioManager(HardwareSerial& serialPort, AudioManagerListener* listener = nullptr);
   virtual ~AudioManager();

   void setup();
   void update(); // Call in main loop to advance sequence

   void playSoundEffect(SoundId soundEffectNum, unsigned long currentTime, uint32_t optionalValue = 0);
   // Other sound management methods can be added here

   // Play a track polyphonically (does not stop other tracks)
   void loopTrack(TrackNumber_t trackId);
   void playTrack(TrackNumber_t trackId);
   void playCallout(TrackNumber_t trackId); // Queues callout for sequential playback
   void playBackgroundMusic(TrackNumber_t trackId);

   // Callout queue management
   void flushCallouts(); // Clear callout queue and stop any playing callout

   // Play a track solo (stops all other tracks)
   void playSolo(TrackNumber_t trackId);

   // Listener interface for debugging/testing
   void setListener(AudioManagerListener* listener);
   AudioManagerListener* getListener() const;

   // WavTrigger passthroughs for setup/loop management
   void stopAllTracks();

   int getNumVoices() const;
   int getNumTracks() const;
   int getPlayingTrack(int voiceNum) const;
   bool isTrackPlaying(TrackNumber_t trackId) const;

   bool isQueueEmpty() const;

   void queueTrack(TrackNumber_t track, unsigned long playTime);

 protected:
   void onTrackReport(uint16_t trackNum, uint8_t voice, bool isPlaying) override;

   WavTrigger* wavTrigger_;
   AudioManagerListener* listener_ = nullptr;

   // Track state for debugging
   static constexpr unsigned MAX_TRACKS = 128;

   static constexpr unsigned SOUND_QUEUE_SIZE = 30;
   static constexpr unsigned CALLOUT_QUEUE_SIZE = 10;
   void duckTracksForCallout();
   void unduckTracks();
   void playNextCallout(bool wasPlaying = false); // Internal: play next callout from queue

   SoundEntry soundQueue_[SOUND_QUEUE_SIZE];
   TrackNumber_t calloutQueue_[CALLOUT_QUEUE_SIZE]; // Queue of callout track IDs
   unsigned calloutQueueHead_ = 0;                  // Index to write next callout
   unsigned calloutQueueTail_ = 0;                  // Index to read next callout
   // bool isCalloutPlaying_ = false;                  // True if a callout is currently playing
   volatile TrackNumber_t calloutPlaying_ = 0;

};
