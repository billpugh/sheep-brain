#ifndef _STATE_H
#define _STATE_H
#include "soundFile.h"


enum State {
  Bored,
  Welcoming,
  Riding,
  ReadyToRide,
  NotInTheMood,
  Violated,
};

extern unsigned long timeEnteredCurrentState;

extern void updateState(unsigned long now);
class SheepState {
  public:
    virtual SheepState * update() = 0;
    const char * const name;
    const enum State state;
    SoundCollection & sounds;
    SheepState(enum State st, const char * n, SoundCollection & s) : state(st), name(n), sounds(s) {};
    boolean playSound(unsigned long now, boolean ambientNoise);

};

extern SheepState * currentSheepState;


class BoredState : public SheepState {
 public:
    BoredState() : SheepState(Bored, "Bored", boredSounds) {};
    SheepState * update();

};

class AttentiveState : public SheepState {
   public:
    AttentiveState() : SheepState(Welcoming, "Attentive", welcomingSounds) {};
    SheepState * update();
};

class ReadyToRideState : SheepState {
   public:
    ReadyToRideState() : SheepState(ReadyToRide, "Ready to ride", ridingSounds) {};
    SheepState * update();
};

class RidingState : public SheepState {
   public:
    RidingState() : SheepState(Riding, "Riding", ridingSounds) {};

    SheepState * update();
};

class NotInTheMoodState : public SheepState {
   public:
    NotInTheMoodState() : SheepState(NotInTheMood, "Not in the mood", boredSounds) {};
    SheepState * update();
};


class ViolatedState : public SheepState {
   public:
    ViolatedState() : SheepState(Violated, "Violated", boredSounds) {};
    SheepState * update();
};

#endif
