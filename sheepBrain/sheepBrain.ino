#include <Adafruit_SleepyDog.h>


#include <Wire.h>
#include <TimeLib.h>
#include "util.h"
#include "sound.h"
#include "printf.h"
#include "touchSensors.h"
#include "soundFile.h"
#include "slave.h"
#include "state.h"
#include "GPS.h"
#include "radio.h"
#include "logging.h"
#include "avdweb_SAMDtimer.h"


extern  void checkForCommand(unsigned long now);


uint8_t sheepNumber = 14;
SheepInfo infoOnSheep[NUMBER_OF_SHEEP];

SheepInfo & getSheep(int s) {
  if (s < 1 || s > NUMBER_OF_SHEEP) {
    myprintf(Serial, "invalid request for sheep %d\n", s);
    return infoOnSheep[0];
  }
  return infoOnSheep[s - 1];
}


//const uint8_t SHDN_MAX9744 = 10;

unsigned long nextPettingReport;

boolean debugTouch = false;
SoundCollection boredSounds(0);
SoundCollection ridingSounds(2);
SoundCollection readyToRideSounds(2);

SoundCollection attentiveSounds(1);
SoundCollection notInTheMoodSounds(1);
SoundCollection violatedSounds(1);
SoundCollection baaSounds(0);

// not a state
SoundCollection endOfRideSounds(3);
SoundCollection inappropriateTouchSounds(4);
SoundCollection seperatedSounds(1);




unsigned long setupFinished = 0;
uint16_t minutesUptime() {
  return ( millis() - setupFinished) / 1000 / 60;
}

unsigned long nextTestDistress = 0xffffffff; // disable test distreess

volatile unsigned long lastInterrupt = 0;
volatile  long longestInterval = 0;
volatile unsigned int maxAvail = 0;
volatile int maxInterruptTime = 0;

unsigned long updateGPSLatency() {
  unsigned long now_us = millis();
  if (lastInterrupt != 0) {
    long diff = now_us - lastInterrupt;
    if (longestInterval < diff) {
      longestInterval = diff;
    }
  }
  lastInterrupt = now_us;
  int a = Serial1.available();
  if (maxAvail < a)
    maxAvail = a;
  return now_us;
}
void ISR_GPS(struct tc_module *const module_inst)
{
  unsigned long start = micros();
  if (read_gps_in_interrupt) {

    updateGPSLatency();
    while (true) {
      char c = GPS.read();
      if (!c) break;
      c = GPS.read();
      if (!c) break;
      c = GPS.read();
      if (!c) break;
    }
  }
  if (musicPlayerReady)
    musicPlayer.feedBuffer();
  unsigned long end = micros();
  long diff = end - start;
  if (maxInterruptTime < diff)
    maxInterruptTime = diff;

}

SAMDtimer IRS_timer5 = SAMDtimer(5, ISR_GPS, 55000, 0);

void setup() {
  sheepNumber = 1;
  memset(&infoOnSheep, 0, sizeof (infoOnSheep));
  pinMode(LED_BUILTIN, OUTPUT);
  //  pinMode(SHDN_MAX9744, OUTPUT);
  //  digitalWrite(SHDN_MAX9744, LOW);
  Wire.begin();
  randomSeed(analogRead(0));
  Serial.begin(115200);
  if (true) while (!Serial && millis() < 5000) {
      digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
      setupDelay(200);                     // wait for a second
      digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
      setupDelay(200);
    }
  int countdownMS = Watchdog.enable(14000);
  myprintf(Serial, "Watchdog set, %d ms timeout\n", countdownMS);

  for (int i = 1; i < 10; i++) {
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    setupDelay(20);                     // wait for a second
    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    setupDelay(20);
    Serial.println(i);
  }

  if (setupRadio())
    Serial.println("radio found");
  else
    Serial.println("radio not found!");

  if (useGPS) {
    if (setupGPS())
      Serial.println("GPS found");
    else
      logDistress("GPS not found!");
  } else
    Serial.println("Skipping GPS");



  if (!setupSD()) {
    Serial.println("setupSD failed");
    Serial.println("huh");
    logDistress("SD card not found");
    while (true) {
      digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
      setupDelay(1000);                     // wait for a second
      digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
      setupDelay(1000);

    }
  }

  File configFile = SD.open("config.txt");
  if (!configFile) {
    while (true) {
      logDistress("no config.txt file");
      while (true) {
        digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
        setupDelay(1000);                     // wait for a second
        digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
        setupDelay(1000);
      }
    }
  }
  sheepNumber = configFile.parseInt();
  myprintf(Serial, "I am sheep %d\n", sheepNumber);
  configFile.close();

  setupLogging();
  myprintf(logFile, "Start of log for sheep %d\n", sheepNumber);

  if (useSound) {


    //    digitalWrite(SHDN_MAX9744, HIGH);
    //    setupDelay(10);
    //    pinMode(SHDN_MAX9744, INPUT);
    //    setupDelay(100);
    setupSound();
    Serial.println("Sound set up, examining sound files");

    boredSounds.load("BORED");
    ridingSounds.load("RIDNG");
    readyToRideSounds.load("RDRID");
    endOfRideSounds.load("EORID");
    attentiveSounds.load("ATTNT");
    notInTheMoodSounds.load("NMOOD");
    violatedSounds.load("VIOLT");
    inappropriateTouchSounds.load("INAPP");
    seperatedSounds.load("SEPRT");

    baaSounds.load("baa");
    baaSounds.playSound(millis(), false);

  } else
    Serial.println("Skipping sound");

  if (useTouch) {
    Serial.println("Setting up touch");
    setupTouch();
    Serial.println("touch set up");
  } else
    Serial.println("Skipping touch");

  if (useSlave) {
    Serial.println("Setting up comm");
    setupComm();
  } else
    Serial.println("Skipping comm");

  //discardGPS();
  setupFinished = millis();
  myprintf(Serial, "%dms set up time\n", setupFinished);
  myprintf(logFile, "sheep %d ready\n", sheepNumber);

  updateLog(setupFinished);
  quickGPSUpdate();
  if (useGPSinterrupts) {
    lastInterrupt = setupFinished;
    read_gps_in_interrupt = true;
    IRS_timer5.enableInterrupt(1);
  }
  nextPettingReport = setupFinished + 2000;

}


unsigned long lastReport = 0;


const boolean TRACE = false;

unsigned long lastLoop = 0;

int32_t prevLat = -1;
int32_t prevLong = -1;
const uint16_t REPORT_INTERVAL = 15000;
void loop() {
  Watchdog.reset();
  interrupts();
  SheepInfo & me = getSheep();
  me.time = now();
  if (totalYield > 0) {
    myprintf(Serial, "total yield %dms\n", totalYield);
    totalYield = 0;
  }

  me.uptimeMinutes = minutesUptime();
  me.batteryVoltageRaw =  batteryVoltageRaw();
  me.errorCodes = 0;
  unsigned long now = millis();
  if ((now / 500) % 2 == 1)
    digitalWrite(LED_BUILTIN, HIGH);
  else
    digitalWrite(LED_BUILTIN, LOW);


  if (nextTestDistress < now) {
    nextTestDistress = now + random(150000, 170000);
    logDistress("This is a test distress message from sheep %d, next test in %d seconds",
                sheepNumber, (nextTestDistress - now) / 1000);
  }
  if (useGPS) {
    quickGPSUpdate();
    updateGPS(now);
  }

  if (useSound && playSound)
    updateSound(now);

  if (useGPS && !useGPSinterrupts) {
    quickGPSUpdate();
  }
  if (useRadio)
    updateRadio();

  if (useGPS && !useGPSinterrupts) {
    quickGPSUpdate();
  }
  if (lastReport + REPORT_INTERVAL < now ) {
    myprintf(Serial, "%d State %s, %f volts,  %d minutes uptime, %d GPS fixes, %2d:%02d:%02d\n",
             now, currentSheepState->name,
             batteryVoltage(),  minutesUptime(), fixCount,
             hour(), minute(), second());
    myprintf(Serial, "  %dms interrupt interval, %dus max interrupt time, %d max avail\n",
             longestInterval, maxInterruptTime, maxAvail);


    if (currentSoundFile != NULL)
      myprintf(Serial, "  playing %s\n", currentSoundFile -> name);
    else if (musicPlayer.playingMusic)
      Serial.println("  Playing unknown sound");
    else
      myprintf(Serial, "next ambient sound in %d ms\n", nextAmbientSound - now);

    myprintf(Serial, "  GPS readings %d good, %d bad\n", total_good_GPS, total_bad_GPS);
    if (getGPSFixQuality)
      myprintf(Serial, "  Fix quality = %d, Satellites = %d\n", GPS.fixquality, GPS.satellites);
    myprintf(Serial, "  Location = %d, %d; longest gps void = %dms,  Feet from man = ",
             latitudeDegreesAvg, longitudeDegreesAvg, longestGPSvoid);
    Serial.println(distanceFromMan);
    if (now < 10000)
      longestGPSvoid = 0;


    if (prevLat != -1) {
      float speed = 1000 * distance_between_fixed_in_feet(latitudeDegreesAvg, longitudeDegreesAvg, prevLat, prevLong)
                    / (now - lastReport);
      Serial.print("  Speed: ");
      Serial.print(speed, 2);
      Serial.println(" ft/sec");
    }
    prevLat = latitudeDegreesAvg;
    prevLong = longitudeDegreesAvg;


    longestInterval = 0;
    maxAvail = 0;
    maxInterruptTime = 0;
    time_t BRC_time = BRC_now();

    if (false)
      myprintf(Serial, "BRC time: %2d: %02d: %02d\n", hour(BRC_time), minute(BRC_time), second(BRC_time));

    lastReport = now;
    if (useLog) {
      if (TRACE)
        Serial.println("Updating log");
      updateLog(now);
      if (TRACE)
        Serial.println("Log updated");
    }
  }


  if (useGPS && !useGPSinterrupts) {
    quickGPSUpdate();
  }
  if (useTouch) {

    if (TRACE)
      Serial.println("updateTouchData");
    unsigned long startTouch = micros();
    updateTouchData(now, false);
    //Serial.println(micros()-startTouch);
    if (debugTouch) {
      Serial.print("TD ");
      for (int i = 0; i < numTouchSensors; i++)
        myprintf(Serial, " %3d %3d ", stableValue[i], sensorValue((TouchSensor)i));
      Serial.println();
    }
  }

  if (useGPS && !useGPSinterrupts) {
    quickGPSUpdate();
  }

  if (doUpdateState && now > 10000 && useTouch) {
    if (TRACE) Serial.println("updateState");
    updateState(now);
    if (useSlave)
      sendComm(now);


    for (int i = 0; i < numTouchSensors; i++) {
      if (((newTouched >> i) & 1 ) != 0) {
        myprintf(Serial, "touched %d - %d\n", i, sensorValue((enum TouchSensor)i));
        //playFile(" %d.mp3", i);
      }
    }
    if (false && nextPettingReport < now) {
      nextPettingReport = now + 250;
      for (int i = 0; i < numPettingSensors; i++) {

        float confidence = 0.0;
        /// int i = 7;
        float hz = detectPetting(i, 64, &confidence);
        digitalWrite(LED_BUILTIN, hz > 0.0);
        Serial.print(hz);
        Serial.print(" ");
        //       Serial.print(confidence);
        //      Serial.print(" ");
      }
      Serial.println();
    }
  }

  if (useGPS && !useGPSinterrupts) {
    quickGPSUpdate();
  }

  if (useSound && playSound) {
    if (TRACE) Serial.println("checkForNextAmbientSound");
    checkForNextAmbientSound(now);
  }

  if (useGPS && !useGPSinterrupts) {
    quickGPSUpdate();
  }
  //  if (TRACE) Serial.println("checkForCommand");
  //  checkForCommand(now);

  //yield(10);
}

void checkForNextAmbientSound(unsigned long now) {
  if (!useSound || !playSound)
    return;
  if (now < nextAmbientSound)
    return;
  if (musicPlayer.playingMusic || soundPlayedRecently(now)) {
    nextAmbientSound = now + 3000;
    return;
  }

  if (random(3) == 0) {
    if (baaSounds.playSound(now, true))
      nextAmbientSound = now + 3000; //random(5000, 14000);
    else
      nextAmbientSound = now + 3000;
  } else {
    if (currentSheepState -> playSound(now, true))
      nextAmbientSound = now + 3000; // random(12000, 30000);
    else
      nextAmbientSound = now + 3000;
  }

}
unsigned long nextCommandCheck = 0;
void checkForCommand(unsigned long now) {
  if (nextCommandCheck > now)
    return;
  nextCommandCheck = now + 200;
  if (Serial && Serial.available()) {
    char c = Serial.read();
    if (c != '\n') {
      Serial.println();
      Serial.print("Got " );
      Serial.println(c);
      switch (c) {

        case 'a':
          playFile("baa%d.mp3", 1 + random(8));
          break;
        case 'w':
          attentiveSounds.playSound(now, false);
          break;
        case 'r':
          ridingSounds.playSound(now, false);
          break;
        case 'b':
          boredSounds.playSound(now, false);
          break;
        case 'd':
          Serial.println(musicPlayer.startPlayingFile("drumbone.mp3"));
          break;
        case 's':
          musicPlayer.stopPlaying();
          break;
        case 't':
          dumpTouchData();
          debugTouch = !debugTouch;
          break;

        // if we get an 'p' on the serial console, pause/unpause!
        case 'p':
          if (! musicPlayer.paused()) {
            Serial.println("Paused");
            musicPlayer.pausePlaying(true);
          } else {
            Serial.println("Resumed");
            musicPlayer.pausePlaying(false);
          }
          break;

        case '+':
        case '-':
          if (c == '+')
            thevol++;
          else
            thevol--;
          if (thevol > 63) thevol = 63;
          if (thevol < 0) thevol = 0;

          setVolume(thevol);
          break;
      }
    }
  }
}


