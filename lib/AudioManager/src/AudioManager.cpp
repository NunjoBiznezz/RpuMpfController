// AudioManager.cpp
// Implementation of AudioManager for WavTrigger sound sequences
#include "AudioManager.h"
#include "Sounds.h"
#include <Arduino.h>
#include <WavListener.h>
#include <WavTrigger.h>

constexpr AudioGain_t UNITY_GAIN = WavTrigger::UNITY_GAIN; // 0 dB nominal volume
constexpr AudioGain_t MAX_GAIN = WavTrigger::MAX_GAIN;     // +10 dB
constexpr AudioGain_t MIN_GAIN = WavTrigger::MIN_GAIN;     // -70 dB

// Ducking parameters
// Having a slight delay on ducking/unducking helps smooth transitions
constexpr uint32_t DUCK_DOWN_TIME = 200;
constexpr uint32_t DUCK_UP_TIME = 500;

// Duck all non-callout tracks by setting volume to ducked level
constexpr AudioGain_t DUCKED_VOLUME_DB = -15; // Absolute volume level when ducked


AudioManager::AudioManager(HardwareSerial& serialPort, AudioManagerListener* listener) : listener_(listener) {
   for (unsigned index = 0; index < SOUND_QUEUE_SIZE; index++) {
      soundQueue_[index].trackNumber = EMPTY_SLOT;
      soundQueue_[index].playTime = 0;
   }

   for (unsigned index = 0; index < CALLOUT_QUEUE_SIZE; index++) {
      calloutQueue_[index] = EMPTY_SLOT;
   }
   calloutQueueHead_ = 0;
   calloutQueueTail_ = 0;
   calloutPlaying_ = EMPTY_SLOT;

   this->wavTrigger_ = new WavTrigger(serialPort);
}

AudioManager::~AudioManager() {
   wavTrigger_->setListener(nullptr);
   delete wavTrigger_;
}

void AudioManager::playTrack(TrackNumber_t trackId) {
   wavTrigger_->trackPlayPoly(trackId);

   // If a callout is playing, immediately duck this track
   if (calloutPlaying_ != EMPTY_SLOT) {
      // wavTrigger_->trackFade(trackId, DUCKED_VOLUME_DB, 0, true);
      wavTrigger_->trackGain(trackId, DUCKED_VOLUME_DB);
   } else {
      wavTrigger_->trackGain(trackId, UNITY_GAIN);
   }
}

void AudioManager::loopTrack(TrackNumber_t trackNumber) {
   wavTrigger_->trackPlayPoly(trackNumber);
   wavTrigger_->trackLoop(trackNumber, true);
   // If a callout is playing, immediately duck this track
   if (calloutPlaying_ != EMPTY_SLOT) {
      wavTrigger_->trackGain(trackNumber, DUCKED_VOLUME_DB);
   } else {
      wavTrigger_->trackGain(trackNumber, UNITY_GAIN);
   }
}

void AudioManager::playCallout(TrackNumber_t trackNumber) {
   wavTrigger_->update();

   // Add to queue
   unsigned nextHead = (calloutQueueHead_ + 1) % CALLOUT_QUEUE_SIZE;
   if (nextHead != calloutQueueTail_) { // Queue not full
      calloutQueue_[calloutQueueHead_] = trackNumber;
      calloutQueueHead_ = nextHead;
   }

   // If no callout is currently playing, start this one
   if (calloutPlaying_ == EMPTY_SLOT) {
      playNextCallout();
   }
}

void AudioManager::playBackgroundMusic(TrackNumber_t trackNumber) {
   wavTrigger_->trackPlayPoly(trackNumber);
   wavTrigger_->trackLoop(trackNumber, true);
   // If a callout is playing, immediately duck this track
   if (calloutPlaying_ != EMPTY_SLOT) {
      wavTrigger_->trackGain(trackNumber, DUCKED_VOLUME_DB);
   } else {
      wavTrigger_->trackGain(trackNumber, UNITY_GAIN);
   }
}

void AudioManager::playSolo(TrackNumber_t trackNumber) {
   wavTrigger_->trackPlaySolo(trackNumber);
}

void AudioManager::update() {
   unsigned long currentTime = millis();

   wavTrigger_->update();

   // Play any enrties in the souns queue
   for (unsigned index = 0; index < SOUND_QUEUE_SIZE; index++) {
      SoundEntry& entry = soundQueue_[index];
      if (entry.trackNumber != 0) {
         if (currentTime >= entry.playTime) {
            this->playTrack(entry.trackNumber);
            entry.trackNumber = EMPTY_SLOT; // Clear entry
         }
      }
   }
}

void AudioManager::setListener(AudioManagerListener* listener) {
   listener_ = listener;
}

AudioManagerListener* AudioManager::getListener() const {
   return listener_;
}

void AudioManager::setup() {
   wavTrigger_->setListener(this);
   wavTrigger_->start();
   delay(100); // Allow time for startup
   wavTrigger_->setReporting(true);
   // delay(100);
   wavTrigger_->stopAllTracks();
   // delay(100);
}

void AudioManager::stopAllTracks() {
   wavTrigger_->stopAllTracks();
}

int AudioManager::getNumVoices() const {
   return this->wavTrigger_->getNumVoices();
}

int AudioManager::getNumTracks() const {
   return this->wavTrigger_->getNumTracks();
}

int AudioManager::getPlayingTrack(int voiceNum) const {
   return wavTrigger_->getPlayingTrack(voiceNum);
}

bool AudioManager::isTrackPlaying(TrackNumber_t trackNumber) const {
   return wavTrigger_->isTrackPlaying(trackNumber);
}

void AudioManager::onTrackReport(TrackNumber_t trackNumber, uint8_t voice, bool isPlaying) {
   // Track stopped - check if it was a callout before clearing
   if (!isPlaying && (calloutPlaying_ == trackNumber)) {
      playNextCallout(true);
   }

   // Notify listener
   if (listener_) {
      if (isPlaying) {
         listener_->onTrackStarted(trackNumber);
      } else {
         listener_->onTrackStopped(trackNumber);
      }
   }
}

void AudioManager::playSoundEffect(SoundId soundEffectNum, unsigned long currentTime, uint32_t optionalValue) {
   switch (soundEffectNum) {
   case SoundId::ROLLOVER:
   case SoundId::DT_SKILL_SHOT:
   case SoundId::ROLLOVER_SKILL_SHOT:
   case SoundId::SU_SKILL_SHOT:
   case SoundId::LEFT_SPINNER:
   case SoundId::RIGHT_SPINNER:
   case SoundId::DROP_TARGET:
   case SoundId::BALL_OVER:
      queueTrack(static_cast<TrackNumber_t>(SoundId::THOUSAND_POINT), currentTime);
      queueTrack(static_cast<TrackNumber_t>(SoundId::TEN_POINT), currentTime + 75);
      break;
   case SoundId::LEFT_INLANE:
      for (unsigned count = 0; count < (unsigned)optionalValue; count++) {
         queueTrack(static_cast<TrackNumber_t>(SoundId::TEN_THOUSAND_POINT), currentTime + 200 * count);
         queueTrack(static_cast<TrackNumber_t>(SoundId::TEN_POINT), currentTime + 75 + (200 * count));
      }
      break;
   case SoundId::RIGHT_INLANE:
      for (int count = 0; count < 6; count++) {
         queueTrack((count < 3) ? static_cast<TrackNumber_t>(SoundId::TEN_THOUSAND_POINT) : static_cast<TrackNumber_t>(SoundId::POP_BUMPER_HIT),
                    currentTime + 200 * count);
         queueTrack(static_cast<TrackNumber_t>(SoundId::TEN_POINT), currentTime + 75 + (200L * count));
      }
      break;
   case SoundId::SAUCER_HIT_5K:
      for (int count = 0; count < 5; count++) {
         queueTrack(static_cast<TrackNumber_t>(SoundId::TEN_THOUSAND_POINT), currentTime + 200L * count);
         queueTrack(static_cast<TrackNumber_t>(SoundId::TEN_POINT), currentTime + 75 + (200L * count));
      }
      break;
   case SoundId::SAUCER_HIT_30K:
      for (int count = 0; count < 3; count++) {
         queueTrack(static_cast<TrackNumber_t>(SoundId::ADD_BONUS), currentTime + 200L * count);
         queueTrack(static_cast<TrackNumber_t>(SoundId::TEN_POINT), currentTime + 75 + (200L * count));
      }
      break;
   case SoundId::SAUCER_HIT_20K:
      for (int count = 0; count < 2; count++) {
         queueTrack(static_cast<TrackNumber_t>(SoundId::ADD_BONUS), currentTime + 200 * count);
         queueTrack(static_cast<TrackNumber_t>(SoundId::TEN_POINT), currentTime + 75 + (200L * count));
      }
      break;
   case SoundId::SAUCER_HIT_10K:
      for (int count = 0; count < 1; count++) {
         queueTrack(static_cast<TrackNumber_t>(SoundId::ADD_BONUS), currentTime + 200 * count);
         queueTrack(static_cast<TrackNumber_t>(SoundId::TEN_POINT), currentTime + 75 + (200L * count));
      }
      break;
   case SoundId::RIGHT_OUTLANE:
      for (int count = 0; count < 5; count++) {
         queueTrack(static_cast<TrackNumber_t>(SoundId::TEN_THOUSAND_POINT), currentTime + 200L * count);
         queueTrack(static_cast<TrackNumber_t>(SoundId::TEN_POINT), currentTime + 75 + (200L * count));
      }
      break;
   case SoundId::TOP_BUMPER_HIT:
   case SoundId::BOTTOM_BUMPER_HIT:
      queueTrack(static_cast<TrackNumber_t>(SoundId::POP_BUMPER_HIT), currentTime);
      queueTrack(static_cast<TrackNumber_t>(SoundId::TEN_POINT), currentTime + 75);
      break;
   case SoundId::SHOOT_AGAIN:
   case SoundId::PLAYER_1_UP:
   case SoundId::PLAYER_2_UP:
   case SoundId::PLAYER_3_UP:
   case SoundId::PLAYER_4_UP:
      queueTrack(static_cast<TrackNumber_t>(SoundId::ADD_BONUS), currentTime);
      queueTrack(static_cast<TrackNumber_t>(SoundId::TEN_THOUSAND_POINT), currentTime + 75);
      queueTrack(static_cast<TrackNumber_t>(SoundId::TEN_POINT), currentTime + 175);
      break;
   case SoundId::BONUS_COUNT:
      // case SoundId::2X_BONUS_COUNT:
      // case SoundId::3X_BONUS_COUNT:
      // case SoundId::4X_BONUS_COUNT:
      // case SoundId::5X_BONUS_COUNT:
      queueTrack(static_cast<TrackNumber_t>(SoundId::TEN_THOUSAND_POINT), currentTime);
      queueTrack(static_cast<TrackNumber_t>(SoundId::TEN_POINT), currentTime + 75);
      break;
   case SoundId::UPPER_SLING:
   case SoundId::EXTRA_BALL:
   case SoundId::TILT_WARNING:
      queueTrack(static_cast<TrackNumber_t>(SoundId::POP_BUMPER_HIT), currentTime); // (uint8_t)SoundId::POP_BUMPER_HIT
      queueTrack(static_cast<TrackNumber_t>(SoundId::TEN_POINT), currentTime + 75);
      break;
   // case SoundId::10PT_SWITCH:
   case SoundId::MATCH_SPIN:
   case SoundId::LOWER_SLING:
      queueTrack(static_cast<TrackNumber_t>(SoundId::HUNDRED_POINT), currentTime);
      queueTrack(static_cast<TrackNumber_t>(SoundId::TEN_POINT), currentTime + 75);
      break;
   case SoundId::DROP_TARGET_CLEAR_1:
   case SoundId::DROP_TARGET_CLEAR_2:
   case SoundId::DROP_TARGET_CLEAR_3:
   case SoundId::DROP_TARGET_CLEAR_4:
   case SoundId::DROP_TARGET_CLEAR_5:
      queueTrack(static_cast<TrackNumber_t>(SoundId::ADD_BONUS), currentTime);
      queueTrack(static_cast<TrackNumber_t>(SoundId::TEN_POINT), currentTime + 75);
      break;
   case SoundId::FIRST_SU_SWITCH_HIT:
   case SoundId::SECOND_SU_SWITCH_HIT:
   case SoundId::THIRD_SU_SWITCH_HIT:
   case SoundId::FOURTH_SU_SWITCH_HIT:
   case SoundId::FIFTH_SU_SWITCH_HIT:
      queueTrack(static_cast<TrackNumber_t>(SoundId::TEN_THOUSAND_POINT), currentTime);
      queueTrack(static_cast<TrackNumber_t>(SoundId::TEN_POINT), currentTime + 75);
      break;
   case SoundId::ADD_CREDIT:
   case SoundId::GAME_OVER:
      queueTrack(static_cast<TrackNumber_t>(SoundId::ADD_BONUS), currentTime);
      queueTrack(static_cast<TrackNumber_t>(SoundId::TEN_THOUSAND_POINT), currentTime + 75);
      queueTrack(static_cast<TrackNumber_t>(SoundId::THOUSAND_POINT), currentTime + 150);
      queueTrack(static_cast<TrackNumber_t>(SoundId::HUNDRED_POINT), currentTime + 225);
      queueTrack(static_cast<TrackNumber_t>(SoundId::ADD_BONUS), currentTime + 325);
      queueTrack(static_cast<TrackNumber_t>(SoundId::TEN_THOUSAND_POINT), currentTime + 400);
      queueTrack(static_cast<TrackNumber_t>(SoundId::THOUSAND_POINT), currentTime + 475);
      queueTrack(static_cast<TrackNumber_t>(SoundId::HUNDRED_POINT), currentTime + 550);
      queueTrack(static_cast<TrackNumber_t>(SoundId::TEN_POINT), currentTime + 650);
      break;
   case SoundId::ADD_PLAYER_1:
   case SoundId::ADD_PLAYER_2:
   case SoundId::ADD_PLAYER_3:
   case SoundId::ADD_PLAYER_4:
   case SoundId::RESCUE_FROM_THE_DEEP:
   case SoundId::TRIDENT_INTRO:
      queueTrack(static_cast<TrackNumber_t>(SoundId::HUNDRED_POINT), currentTime);
      queueTrack(static_cast<TrackNumber_t>(SoundId::THOUSAND_POINT), currentTime + 75);
      queueTrack(static_cast<TrackNumber_t>(SoundId::TEN_THOUSAND_POINT), currentTime + 150);
      queueTrack(static_cast<TrackNumber_t>(SoundId::ADD_BONUS), currentTime + 225);
      queueTrack(static_cast<TrackNumber_t>(SoundId::HUNDRED_POINT), currentTime + 325);
      queueTrack(static_cast<TrackNumber_t>(SoundId::THOUSAND_POINT), currentTime + 400);
      queueTrack(static_cast<TrackNumber_t>(SoundId::TEN_THOUSAND_POINT), currentTime + 475);
      queueTrack(static_cast<TrackNumber_t>(SoundId::ADD_BONUS), currentTime + 550);
      queueTrack(static_cast<TrackNumber_t>(SoundId::TEN_POINT), currentTime + 650);
      break;

   default:
      break;
   }
}

bool AudioManager::isQueueEmpty() const {
   for (unsigned i = 0; i < SOUND_QUEUE_SIZE; ++i) {
      if (soundQueue_[i].trackNumber != EMPTY_SLOT) {
         return false;
      }
   }
   return true;
}

void AudioManager::queueTrack(TrackNumber_t soundIndex, unsigned long playTime) {
   // Find an empty slot in the sound queue
   for (unsigned i = 0; i < SOUND_QUEUE_SIZE; ++i) {
      if (soundQueue_[i].trackNumber == EMPTY_SLOT) {
         soundQueue_[i].trackNumber = soundIndex;
         soundQueue_[i].playTime = playTime;
         break;
      }
   }
}

void AudioManager::duckTracksForCallout() {
   for (unsigned voice = 0; voice < WavTrigger::MAX_NUM_VOICES; voice++) {
      auto track = wavTrigger_->getPlayingTrack(voice);
      if (EMPTY_SLOT != track) {
         // Apply ducked volume (originalGain already stored when track started)
         wavTrigger_->trackFade(track, DUCKED_VOLUME_DB, DUCK_DOWN_TIME, false); // Fade to ducked volume
      }
   }
}

void AudioManager::unduckTracks() {
   // Restore volume for all ducked tracks
   for (unsigned voice = 0; voice < WavTrigger::MAX_NUM_VOICES; voice++) {
      uint16_t track = wavTrigger_->getPlayingTrack(voice);
      if (EMPTY_SLOT != track) {
         // Restore to original volume
         wavTrigger_->trackFade(track, UNITY_GAIN, DUCK_UP_TIME,
                               false); // Fade to original volume
      }
   }
}

void AudioManager::playNextCallout(bool wasPlaying) {
   // Check if there's a callout in the queue
   if (calloutQueueTail_ != calloutQueueHead_) {
      calloutPlaying_ = calloutQueue_[calloutQueueTail_];
      calloutQueueTail_ = (calloutQueueTail_ + 1) % CALLOUT_QUEUE_SIZE;

      // Duck other tracks and play the callout
      duckTracksForCallout();
      wavTrigger_->trackPlayPoly(calloutPlaying_);
   } else {
      if (wasPlaying) {
         unduckTracks();
      }
      calloutPlaying_ = EMPTY_SLOT;
   }
}

void AudioManager::flushCallouts() {
   wavTrigger_->update();

   // Clear the queue
   calloutQueueHead_ = 0;
   calloutQueueTail_ = 0;

   if (calloutPlaying_ != EMPTY_SLOT) {
      // Stop the currently playing callout
      wavTrigger_->trackStop(calloutPlaying_);
      calloutPlaying_ = EMPTY_SLOT;

      // Unduck all tracks immediately
      unduckTracks();
   }
}
