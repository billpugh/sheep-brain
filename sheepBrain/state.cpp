#include "util.h"
#include "touchSensors.h"
#include "sound.h"
#include "soundFile.h"
#include "state.h"
#include "printf.h"

unsigned long nextAmbientSound = 10000;
unsigned long timeEnteredCurrentState = 0;

unsigned long notInTheMoodUntil = 0;

BoredState boredState;
AttentiveState attentiveState;
ReadyToRideState readyToRideState;
RidingState ridingState;
NotInTheMoodState notInTheMoodState;
ViolatedState violatedState;

SheepState * currentSheepState = &boredState;


uint16_t secondsSinceEnteredCurrentState() {
  return (millis() - timeEnteredCurrentState) / 1000;
}
boolean maybeRiding() {

  return touchDuration(BACK_SENSOR) > 8200
         && (touchDuration(LEFT_SENSOR) > 2550 || touchDuration(RIGHT_SENSOR) > 2550 ||  touchDuration(RUMP_SENSOR) > 2550 )
         || touchDuration(RUMP_SENSOR) > 9500;
}

boolean definitivelyRiding() {

  return touchDuration(BACK_SENSOR) > 12200
         && (touchDuration(LEFT_SENSOR) > 3550 && touchDuration(RIGHT_SENSOR) > 3550 ||  touchDuration(RUMP_SENSOR) > 3550 )
         || touchDuration(RUMP_SENSOR) > 9500;
}

boolean isIgnored() {
  return untouchDuration(BACK_SENSOR) > 15000
         && untouchDuration(HEAD_SENSOR) > 15000
         && untouchDuration(RUMP_SENSOR) > 10000
         && untouchDuration(LEFT_SENSOR) > 10000
         && untouchDuration(RIGHT_SENSOR) > 10000;
}

boolean isTouched() {
  return  touchDuration(BACK_SENSOR) > 400
          ||  touchDuration(RUMP_SENSOR) > 400  ||  touchDuration(HEAD_SENSOR) > 500;
}

boolean qualityTouch() {
  return  touchDuration(BACK_SENSOR) > 17000
          ||  touchDuration(RUMP_SENSOR) > 25000  ||  touchDuration(HEAD_SENSOR) > 17000
          ||  touchDuration(BACK_SENSOR) + touchDuration(RUMP_SENSOR) +  touchDuration(HEAD_SENSOR) > 30000;
}

int privateTouches = 0;
unsigned long lastPrivateTouch = 0;

void becomeViolated() {
  notInTheMoodUntil = max(notInTheMoodUntil, millis() + 1000 * (30 + random(30, 60) + random(20, 100)));
}

void updateState(unsigned long now) {
  if (touchDuration(PRIVATES_SENSOR) > 2000 && now > lastPrivateTouch + 4000
      && !(currentSoundPriority == 4 && currentSoundFile != NULL)) {
    lastPrivateTouch = now;
    privateTouches++;
    myprintf(Serial, "inappropriate touch duration %d, num inappropriate touches = %d\n",
             touchDuration(PRIVATES_SENSOR), privateTouches);
    myprintf(Serial, "current value = %d, stable value = %d\n",
             cap.filteredData((uint8_t) PRIVATES_SENSOR), stableValue[PRIVATES_SENSOR]);
    inappropriateTouchSounds.playSound(now, false);

    if (privateTouches >= 2 && currentSheepState != &violatedState) {
      becomeViolated();
      currentSheepState = &violatedState;
      timeEnteredCurrentState = now;
      return;
    }
  }

  if (secondsSinceEnteredCurrentState() < 2)
    return;

  SheepState * newState = currentSheepState->update();
  if (newState != currentSheepState) {
    // got an update

    myprintf(Serial, "State changed from %s to %s\n", currentSheepState->name, newState->name);
    myprintf(Serial, "   %d secs, %6d %6d %6d %6d %6d\n",
             secondsSinceEnteredCurrentState(),
             combinedTouchDuration(HEAD_SENSOR),
             combinedTouchDuration(BACK_SENSOR),
             combinedTouchDuration(LEFT_SENSOR),
             combinedTouchDuration(RIGHT_SENSOR),
             combinedTouchDuration(RUMP_SENSOR));
    if (!musicPlayer.playingMusic ||
        newState->sounds.priority >= currentSoundPriority) {
      newState->sounds.playSound(now, false);
      nextAmbientSound = now + 12000 + random(1, 15000);
    }
    currentSheepState = newState;
    timeEnteredCurrentState = now;
  }
  //Serial.println("update state finished");
}


boolean SheepState::playSound(unsigned long now, boolean ambientNoise) {
  if (sounds.playSound(now, ambientNoise)) {
    return true;
  }
  return baaSounds.playSound(now, ambientNoise);
}

SheepState * BoredState::update() {
  if (secondsSinceEnteredCurrentState() > 15)
    privateTouches = 0;
  if (isTouched()) {
    if (random(100) < 20) {
      int notInTheMood =  (random(30, 60) + random(20, 100));
      myprintf(Serial, "not in the mood for %d seconds\n", notInTheMood);
      notInTheMoodUntil = millis() + 1000 * notInTheMood;

    }
    return &attentiveState;
  }
  return this;
}


SheepState * AttentiveState::update() {
  if (isIgnored())
    return &boredState;
  if (definitivelyRiding()) {
    becomeViolated();
    return &violatedState;
  }
  if (secondsSinceEnteredCurrentState() > 40 && !musicPlayer.playingMusic
      && millis() < notInTheMoodUntil)
    return &notInTheMoodState;
  if (qualityTouch() && secondsSinceEnteredCurrentState() > 15 && !musicPlayer.playingMusic) {
    if (millis() < notInTheMoodUntil)
      return &notInTheMoodState;
    return &readyToRideState;
  }
  return this;
}

SheepState * ReadyToRideState::update() {
  if (musicPlayer.playingMusic)
    return this;
  if (maybeRiding() && secondsSinceEnteredCurrentState() > 20 )
    return &ridingState;

  if (!qualityTouch() && secondsSinceEnteredCurrentState() > 20) {
    if (isTouched())
      return &attentiveState;
    return &boredState;
  }

  return this;
}

SheepState * RidingState::update() {
  if (!maybeRiding() && secondsSinceEnteredCurrentState() > 20 && !musicPlayer.playingMusic) {

    endOfRideSounds.playSound(millis(), false);

    if (qualityTouch())
      return &readyToRideState;

    if (isTouched())
      return &attentiveState;

    return &boredState;
  }
  return this;
}


SheepState * NotInTheMoodState::update() {
  if (definitivelyRiding()) {
    becomeViolated();
    return &violatedState;
  }
  if (musicPlayer.playingMusic)
    return this;
    
  if (millis() > notInTheMoodUntil || secondsSinceEnteredCurrentState() > 30) {
    if (qualityTouch()) {
      if (notInTheMoodUntil > 20000)
        notInTheMoodUntil -= 20000;
      return &attentiveState;
    }
    if (isTouched())
      return &attentiveState;
    return &boredState;
  }
  return this;
}


SheepState * ViolatedState::update() {
  if (secondsSinceEnteredCurrentState() > 60 && !isTouched() && !musicPlayer.playingMusic)  {
    return &boredState;
  }
  return this;
}


