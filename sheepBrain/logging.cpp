
#include <SdFat.h>
#include <TimeLib.h>

#include "util.h"
#include "gps.h"
#include "printf.h"
#include "radio.h"
#include "comm.h"
#include "sound.h"

File logFile;

const boolean disableInterrupts = true;

void setupLogging() {
  char fileName[100];

  unsigned long now = millis();

  if (year() != 2018) {
    SD.mkdir("log/nodate");
    int i = 0;
    while (true) {
      sprintf(fileName, "log/nodate/%04d.csv", i);
      if (!SD.exists(fileName)) {
        logFile = SD.open(fileName, FILE_WRITE);
        myprintf(Serial, "Using logfile %s\n", fileName);
        return;
      }
      i = i + 1;
    }
  }
  sprintf(fileName, "log/%04d%02d%02d", year(), month(), day());
  SD.mkdir(fileName);
  sprintf(fileName, "log/%04d%02d%02d/%02d%02d%02d.csv", year(), month(), day(), hour(), minute(), second());
  if (SD.exists(fileName))
    myprintf(Serial, "Overwriting %s\n", fileName);
  else
    myprintf(Serial, "Using logfile %s\n", fileName);
  logFile = SD.open(fileName, FILE_WRITE);
  if (!logFile) {
    Serial.println("Attempt to open log file failed");
  }
}

unsigned long nextFlush = 0;
void optionalFlush() {
  unsigned long now = millis();
  if (nextFlush > now || musicPlayer.playingMusic)
    return;
  sendSubActivity(100);
  logFile.flush();
  nextFlush = now + 2*60000;
}

void requiredFlush() {
  logFile.flush();
  nextFlush =  millis() + 15000;
}

void myLogStart(enum PacketKind kind) {
  myprintf(logFile, "L, %d,%d, ", sheepNumber, kind);

  myprintf(logFile, "%d/%d/%02d %d:%02d:%02d,%d, ",
           month(), day(), year() % 100, hour(), minute(), second(),
           now());
  myprintf(logFile, "%d,%d, ", minutesUptime(), batteryVoltageRaw());
  logFile.print(GPS.latitudeDegrees, 7);
  logFile.print(",");
  logFile.print(GPS.longitudeDegrees, 7);
}


void updateLog(unsigned long timeNow) {
  quickGPSUpdate();
  if (disableInterrupts)
    noInterrupts();
  myLogStart(InfoPacket);

  logFile.println();
  optionalFlush();
  if (disableInterrupts)
    interrupts();


}


void radioLogStart(uint8_t sheepNum, SheepInfo & info, enum PacketKind kind) {
  time_t when = info.time;
  myprintf(logFile, "R, %d,%d, ", sheepNum, kind);

  myprintf(logFile, "%d/%d/%02d %d:%02d:%02d,%d, ",
           month(when), day(when), year(when) % 100,  hour(when), minute(when), second(when),
           when);
  myprintf(logFile, "%d,%d, ", info.uptimeMinutes, info.batteryVoltageRaw);
  logFile.print(info.latitude, 7);
  logFile.print(",");
  logFile.print(info.longitude, 7);

}

void logBoot() {

  getLastActivity();
  if (disableInterrupts)
    noInterrupts();

  myprintf(logFile, "B, %d,%d,%d,%d,%d\n", activityData.reboots, activityData.lastActivity,
           activityData.subActivity, activityData.secondsSinceBoot,  activityData.secondsSinceLastActivity);
  requiredFlush();
  if (disableInterrupts)  interrupts();

}
void logRadioUpdate(uint8_t sheepNum, SheepInfo & info) {
  if (disableInterrupts) noInterrupts();
  radioLogStart(sheepNum, info, RadioInfoPacket);

  logFile.println();
  optionalFlush();
  if (disableInterrupts)  interrupts();
}

void logRadioDistress(uint8_t sheepNum, time_t when, SheepInfo & info, char* buf) {
  if (disableInterrupts) noInterrupts();
  radioLogStart(sheepNum, info, RadioDistressPacket);
  myprintf(logFile, ", \"%s\"\n", buf);

  requiredFlush();
  if (disableInterrupts) interrupts();
}
void logDistress(const char *fmt, ... ) {
  char buf[500]; // resulting string limited to 256 chars
  va_list args;
  va_start (args, fmt );
  vsnprintf(buf, 500, fmt, args);
  va_end (args);
  Serial.print("Distress message ");
  Serial.println(buf);
  distressPacket(buf);
  if (logFile) {
    if (disableInterrupts) noInterrupts();
    myLogStart(DistressPacket);
    myprintf(logFile, "\"%s\"\n", buf);
    requiredFlush();
    if (disableInterrupts) interrupts();
  }

}