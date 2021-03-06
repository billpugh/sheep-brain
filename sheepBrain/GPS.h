#ifndef _GPS_H
#define _GPS_H

#include "Adafruit_GPS.h"

#include <TimeLib.h>

extern Adafruit_GPS GPS;
extern uint32_t fixCount;
extern unsigned long lastGPSReading;
extern boolean haveFixNow();
extern boolean setupGPS();
extern boolean discardGPS();
extern void quickGPSUpdate();
extern void updateGPS();
extern void logGPS(unsigned long now);
extern time_t BRC_now();
extern uint8_t adjustedHour();
extern time_t adjustedNow();


extern boolean inCorral(int sheep) ;
extern boolean isClose(int sheep);
extern boolean isInFriendlyTerritory() ;
extern float distanceFromMan;
extern float distanceFromCoral;
extern float howCrowded();

extern float distance_between_fixed_in_feet(int32_t lat1, int32_t long1, int32_t lat2, int32_t long2) ;
extern volatile boolean read_gps_in_interrupt;
extern int32_t latitudeDegreesAvg;
extern int32_t longitudeDegreesAvg;
extern long longestGPSvoid;
extern uint16_t total_good_GPS;
extern uint16_t total_bad_GPS;
#endif
