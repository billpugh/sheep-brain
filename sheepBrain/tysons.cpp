#include "util.h"
#include <TimeLib.h>
#include "GPS.h"
#include "printf.h"
#include "tysons.h"

void getTimes(bool debug, time_t & time,   time_t &oTime, time_t &cTime) {
  time = adjustedNow();

  tmElements_t openAt;
  tmElements_t closeAt;
  breakTime(time, openAt);
  breakTime(time, closeAt);
  openAt.Minute = 0;
  openAt.Second = 0;
  closeAt.Minute = 0;
  closeAt.Second = 0;
  int thisMonth = month(time);
  int thisDay = day(time);
  if (debug) {
    myprintf(Serial, "isOpen %d/%d/%d %2d:%2d%2d\n", 
    thisMonth, thisDay, year(time), hour(time), minute(time), second(time));
  }
  if (thisMonth == 1 && thisDay == 6) {
    openAt.Hour = 9; closeAt.Hour = 12 + 7;
  } else if (thisMonth == 1 and thisDay == 1) {
    openAt.Hour = 10; closeAt.Hour = 6;
  } else if (thisMonth == 12)
    switch (thisDay) {
      case 15: {
          openAt.Hour = 9;
          closeAt.Hour = 12 + 10;
          break;
        }
      case 16: {
          openAt.Hour = 9;
          closeAt.Hour = 12 + 10;
          break;
        }
      case 17: {
          openAt.Hour = 9;
          closeAt.Hour = 12 + 10;
          break;
        }
      case 18: {
          openAt.Hour = 9;
          closeAt.Hour = 12 + 10;
          break;
        }
      case 19: {
          openAt.Hour = 9;
          closeAt.Hour = 12 + 11;
          break;
        }
      case 20: {
          openAt.Hour = 9;
          closeAt.Hour = 12 + 11;
          break;
        }
      case 21: {
          openAt.Hour = 9;
          closeAt.Hour = 12 + 11;
          break;
        }
      case 22: {
          openAt.Hour = 9;
          closeAt.Hour = 12 + 11;
          break;
        }
      case 23: {
          openAt.Hour = 9;
          closeAt.Hour = 12 + 11;
          break;
        }
      case 24: {
          openAt.Hour = 8;
          closeAt.Hour = 12 + 5;
          break;
        }
      case 25: {
          openAt.Hour = 10;
          closeAt.Hour = 9;
          break;
        }
      case 26: {
          openAt.Hour = 8;
          closeAt.Hour = 12 + 9;
          closeAt.Minute = 30;
          break;
        }
      case 27: {
          openAt.Hour = 10;
          closeAt.Hour = 12 + 9;
          closeAt.Minute = 30;
          break;
        }
      case 28: {
          openAt.Hour = 10;
          closeAt.Hour = 12 + 9;
          closeAt.Minute = 30;
          break;
        }
      case 29: {
          openAt.Hour = 10;
          closeAt.Hour = 12 + 9;
          closeAt.Minute = 30;
          break;
        }
      case 30: {
          openAt.Hour = 11;
          closeAt.Hour = 12 + 7;
          break;
        }
      case 31: {
          openAt.Hour = 10;
          closeAt.Hour = 12 + 6;
          break;
        }
      default : {
          if (weekday(time) == 1) {
            // Sunday
            openAt.Hour = 11; closeAt.Hour = 12 + 7;
          } else {
            openAt.Hour = 10; closeAt.Hour = 12 + 9; closeAt.Minute = 30;
          }
          break;
        }
    }
  else {
    if (weekday(time) == 1) {
      // Sunday
      openAt.Hour = 11; closeAt.Hour = 12 + 7;
    } else {
      openAt.Hour = 10; closeAt.Hour = 12 + 9; closeAt.Minute = 30;
    }
  }
  oTime = makeTime(openAt);
  cTime = makeTime(closeAt);
}

int minutesUntilOpen(bool debug) {
  time_t time, oTime, cTime;
  getTimes(debug, time, oTime, cTime);
  return (oTime - time) / 60;
}

bool isShutdown(bool debug) {
  if (timeStatus() != timeSet)
    return false;
  if (!MALL_SHEEP)
    return false;
  time_t time, oTime, cTime;
  getTimes(debug, time, oTime, cTime);
  int minutesBeforeOpening = (oTime - time) / 60;
  int minutesAfterClosing = -(cTime - time) / 60;
  if (debug)
    myprintf(Serial, "checking for shutdown, %d minutes before opening, %d minutes after closing\n",
             minutesBeforeOpening, minutesAfterClosing);
  if (isOpen(false))
    return false;
  if (minutesBeforeOpening > 60)
    return true;
  if (minutesAfterClosing > 120)
    return true;
  return false;
}

bool isOpen(bool debug) {
  if (timeStatus() != timeSet)
    return true;
  if (!MALL_SHEEP)
    return true;
  time_t time, oTime, cTime;
  getTimes(debug, time, oTime, cTime);

  if (debug) {
    myprintf(Serial, "open at: %d\n", oTime);
    myprintf(Serial, "close at: %d\n", cTime);
    myprintf(Serial, "now: %d\n", time);

    if (oTime  > time)

      myprintf(Serial, "open in %d minutes\n", (oTime  - time) / 60);

    else
      myprintf(Serial, "opened %d minutes ago\n", -(oTime  - time) / 60);
    if (cTime  > time)

      myprintf(Serial, "close in %d minutes\n", (cTime - time) / 60);

    else
      myprintf(Serial, "closed %d minutes ago\n", -(cTime - time) / 60);
  }
  const time_t onBeforeOpening = 0 * 60;
  const time_t onAfterClosing = 0 * 60;
  if (oTime - onBeforeOpening <= time && time <= cTime + onAfterClosing)
    return true;
  return false;

}
