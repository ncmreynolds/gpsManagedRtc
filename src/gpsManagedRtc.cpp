/*
 *	An Arduino library for GPS Managed RTC support
 *
 *	https://github.com/ncmreynolds/gpsManagedRtc
 *
 *	Released under LGPL-2.1 see https://github.com/ncmreynolds/gpsManagedRtc/LICENSE for full license
 *
 */
#ifndef gpsManagedRtc_cpp
#define gpsManagedRtc_cpp
#include "gpsManagedRtc.h"


gpsManagedRtc::gpsManagedRtc()	//Constructor function
{
}

gpsManagedRtc::~gpsManagedRtc()	//Destructor function
{
}

void ICACHE_FLASH_ATTR gpsManagedRtc::begin() {
	Wire.begin();
	if(debugStream != nullptr) {
		debugStream->print(F("Setting up RTC:"));
	}
	if(rtc.begin() == true) {
		_rtcInitialised = true;
		if(debugStream != nullptr) {
			debugStream->println(F("OK"));
		}
		rtc.set24Hour();  //Enable 24 hour mode for text RTC output
		rtc.disableTrickleCharge(); //Trickle Charger disabled
	} else {
		if(debugStream != nullptr) {
			debugStream->println(F("Failed"));
		}
	}
	if(debugStream != nullptr) {
		debugStream->print(F("Setting up GPS:"));
	}
	if(_gpsRxPin != -1) {
		Serial1.begin(_gpsBaud, SERIAL_8N1, _gpsRxPin, _gpsTxPin);
		if(debugStream != nullptr) {
			debugStream->println(F("OK"));
		}
	} else {
		if(debugStream != nullptr) {
			debugStream->print(F("Not configured"));
		}
	}
}

void ICACHE_FLASH_ATTR gpsManagedRtc::debug(Stream &debugDestination) {
	debugStream = &debugDestination;		//Set the stream used for the terminal
}

void ICACHE_FLASH_ATTR gpsManagedRtc::gpsPins(int rxPin, int txPin) {
	_gpsRxPin = rxPin;
	_gpsTxPin = txPin;
}

void ICACHE_FLASH_ATTR gpsManagedRtc::gpsBaud(uint32_t baud) {
	_gpsBaud = baud;
}

void ICACHE_FLASH_ATTR gpsManagedRtc::housekeeping() {
	bool timeSet = false;
	if(Serial1.available()) {
		_gpsUpdateTimer = millis();
		while(Serial1.available() > 0 && millis() - _gpsUpdateTimer < 50) {
			gps.encode(Serial1.read());
		}
	}
	if(gps.date.isValid()) {
	  _gpsDay = gps.date.day();
	  _gpsMonth = gps.date.month();
	  _gpsYear = gps.date.year();
	}
	if(gps.time.isValid()) {
	  _gpsSeconds = gps.time.second();
	  _gpsMinutes = gps.time.minute();
	  _gpsHours = gps.time.hour();
	}
	if(_gpsInitialised == false && gps.date.isValid() && gps.time.isValid()) {
		if(debugStream != nullptr) {
			debugStream->println(F("GPS got fix"));
		}
		_gpsInitialised = true;
	}
	if(millis() - _updateTimer > _updateInterval || timeIsGood() == false) {
		_updateTimer = millis();
		if(_rtcInitialised == true) {
			if(rtc.updateTime()) { //Updates the time variables from RTC
				_rtcSeconds = rtc.getSeconds();
				_rtcMinutes = rtc.getMinutes();
				_rtcHours = rtc.getHours();
				_rtcDay = rtc.getDate();
				_rtcMonth = rtc.getMonth();
				_rtcYear = rtc.getYear();
			}
			if(gpsTimeIsGood()) {
				if(_rtcInitialised == true && (rtcTimeIsGood() == false || _preferGps == true) && rtcOutOfSyncWithGps()) {
					setRtcFromGps();
				}
				if(_preferGps == true) {
					if(systemOutOfSyncWithGps()) {
						setSystemTimeFromGps();
						timeSet = true;
					}
				}
			}
			if(rtcTimeIsGood() == true && timeSet == false) {
				if(systemOutOfSyncWithRtc()) {
					setSystemTimeFromRtc();
					timeSet = true;
				}
			}
		} else {
			if(gpsTimeIsGood()) {
				if(systemOutOfSyncWithGps()) {
					setSystemTimeFromGps();
					timeSet = true;
				}
			}
		}
		if(debugStream != nullptr) {
			if(timeIsGood())
			{
				time_t now;
				time(&now);
				if(_timezone != nullptr) {	//This horrible bodge exists because mktime and various time functions only work in local time, not UTC!
					setenv("TZ", "GMT0", 1);
					tzset();
				}
				tm *gmTimeInfo = localtime(&now);		//UTC
				if(_timezone != nullptr) {
					setenv("TZ", _timezone, 1);
					tzset();
				}
				tm *localTimeInfo = localtime(&now);	//Including DST and time zone adjustments
				debugStream->print(F("RTC(UTC) - "));
				if(_rtcYear > _earliestValidYear) {
					printTime(_rtcHours, _rtcMinutes, _rtcSeconds, _rtcDay, _rtcMonth, _rtcYear);
				} else {
					debugStream->print(F("XX:XX:XX XX/XX/XXXX"));
				}
				debugStream->print(F(" GPS(UTC) - "));
				if(_gpsYear > _earliestValidYear) {
					printTime(_gpsHours, _gpsMinutes, _gpsSeconds, _gpsDay, _gpsMonth, _gpsYear);
				} else {
					debugStream->print(F("XX:XX:XX XX/XX/XXXX"));
				}
				debugStream->printf_P(PSTR(" Sat:%02u HDOP:%.1f SYS(UTC) - "),gps.satellites.value(), gps.hdop.hdop());
				if(gmTimeInfo->tm_year +1900 > _earliestValidYear) {
					printTime(gmTimeInfo->tm_hour, gmTimeInfo->tm_min, gmTimeInfo->tm_sec, gmTimeInfo->tm_mday, gmTimeInfo->tm_mon + 1,  gmTimeInfo->tm_year +1900);
				} else {
					debugStream->print(F("XX:XX:XX XX/XX/XXXX"));
				}
				debugStream->print(F(" SYS(Local) - "));
				if(localTimeInfo->tm_year +1900 > _earliestValidYear) {
					printTime(localTimeInfo->tm_hour, localTimeInfo->tm_min, localTimeInfo->tm_sec, localTimeInfo->tm_mday, localTimeInfo->tm_mon + 1,  localTimeInfo->tm_year +1900);
				} else {
					debugStream->print(F("XX:XX:XX XX/XX/XXXX"));
				}
				debugStream->print(F("\r\n"));
			} else {
				debugStream->println(F("Time not yet set."));
			}
		}
	}
}

bool ICACHE_FLASH_ATTR gpsManagedRtc::timeIsGood() {
	return gpsTimeIsGood() || rtcTimeIsGood();
}

bool ICACHE_FLASH_ATTR gpsManagedRtc::gpsTimeIsGood() {
	return _gpsInitialised == true && gps.date.isValid() && gps.time.isValid() && gps.satellites.value() >= _gpsSatellitesThreshold && gps.hdop.hdop() <= _gpsHdopThreshold && _gpsYear > _earliestValidYear && _gpsYear < _latestValidYear;	//Some cheap GPS seems to start reporting valid time at _latestValidYear even without a good fix!
}
bool ICACHE_FLASH_ATTR gpsManagedRtc::rtcTimeIsGood() {
	return _rtcInitialised == true && _rtcYear > _earliestValidYear && _gpsYear < _latestValidYear;
}
void ICACHE_FLASH_ATTR gpsManagedRtc::setRtcFromGps() {
	if(debugStream != nullptr) {
		debugStream->print(F("Updating RTC from GPS:"));
	}
	if(
		rtc.setSeconds(_gpsSeconds) && 
		rtc.setMinutes(_gpsMinutes) && 
		rtc.setHours(_gpsHours) && 
		rtc.setDate(_gpsDay) && 
		rtc.setMonth(_gpsMonth) && 
		rtc.setYear(_gpsYear)) {
		if(debugStream != nullptr) {
			debugStream->println(F("OK"));
		}
	} else {
		if(debugStream != nullptr) {
			debugStream->println(F("failed"));
		}
	}
}

bool ICACHE_FLASH_ATTR gpsManagedRtc::rtcOutOfSyncWithGps() {
	int32_t difference = compareTimes(_rtcSeconds, _rtcMinutes, _rtcHours, _rtcDay, _rtcMonth, _rtcYear, _gpsSeconds, _gpsMinutes, _gpsHours, _gpsDay, _gpsMonth, _gpsYear);
	if(abs(difference) > _timeUpdateThreshold) {
		if(debugStream != nullptr) {
			debugStream->print(F("RTC ("));
			printTime(_rtcHours, _rtcMinutes, _rtcSeconds, _rtcDay, _rtcMonth, _rtcYear);
			debugStream->print(F(") out of sync with GPS ("));
			printTime(_gpsHours, _gpsMinutes, _gpsSeconds, _gpsDay, _gpsMonth, _gpsYear);
			debugStream->printf_P(PSTR(") by %is\r\n"), difference);
		}
		return true;
	}
	return false;
}

bool ICACHE_FLASH_ATTR gpsManagedRtc::systemOutOfSyncWithGps() {
	if(_timezone != nullptr) {	//This horrible bodge exists because mktime and various time functions only work in local time, not UTC!
		setenv("TZ", "GMT0", 1);
		tzset();
	}
	getLocalTime(&_currentTimeInfo);
	if(_timezone != nullptr) {
		setenv("TZ", _timezone, 1);
		tzset();
	}
	int32_t difference = compareTimes(_currentTimeInfo.tm_sec, _currentTimeInfo.tm_min, _currentTimeInfo.tm_hour, _currentTimeInfo.tm_mday, _currentTimeInfo.tm_mon + 1, _currentTimeInfo.tm_year +1900, _gpsSeconds, _gpsMinutes, _gpsHours, _gpsDay, _gpsMonth, _gpsYear);
	if(abs(difference) > _timeUpdateThreshold) {
		if(debugStream != nullptr) {
			debugStream->print(F("System time ("));
			printTime(_currentTimeInfo.tm_hour, _currentTimeInfo.tm_min, _currentTimeInfo.tm_sec, _currentTimeInfo.tm_mday, _currentTimeInfo.tm_mon + 1, _currentTimeInfo.tm_year +1900);
			debugStream->print(F(") out of sync with GPS ("));
			printTime(_gpsHours, _gpsMinutes, _gpsSeconds, _gpsDay, _gpsMonth, _gpsYear);
			debugStream->printf_P(PSTR(") by %is\r\n"), difference);
		}
		return true;
	}
	return false;
}

bool ICACHE_FLASH_ATTR gpsManagedRtc::systemOutOfSyncWithRtc() {
	if(_timezone != nullptr) {	//This horrible bodge exists because mktime and various time functions only work in local time, not UTC!
		setenv("TZ", "GMT0", 1);
		tzset();
	}
	getLocalTime(&_currentTimeInfo);
	if(_timezone != nullptr) {
		setenv("TZ", _timezone, 1);
		tzset();
	}
	int32_t difference = compareTimes(_currentTimeInfo.tm_sec, _currentTimeInfo.tm_min, _currentTimeInfo.tm_hour, _currentTimeInfo.tm_mday, _currentTimeInfo.tm_mon + 1, _currentTimeInfo.tm_year +1900, _rtcSeconds, _rtcMinutes, _rtcHours, _rtcDay, _rtcMonth, _rtcYear);
	if(abs(difference) > _timeUpdateThreshold) {
		if(debugStream != nullptr) {
			debugStream->print(F("System time ("));
			printTime(_currentTimeInfo.tm_hour, _currentTimeInfo.tm_min, _currentTimeInfo.tm_sec, _currentTimeInfo.tm_mday, _currentTimeInfo.tm_mon + 1, _currentTimeInfo.tm_year +1900);
			debugStream->print(F(") out of sync with RTC ("));
			printTime(_rtcHours, _rtcMinutes, _rtcSeconds, _rtcDay, _rtcMonth, _rtcYear);
			debugStream->printf_P(PSTR(") by %is\r\n"), difference);
		}
		return true;
	}
	return false;
}

void ICACHE_FLASH_ATTR gpsManagedRtc::setSystemTimeFromGps() {
	if(_timezone != nullptr) {	//This horrible bodge exists because mktime and various time functions only work in local time, not UTC!
		setenv("TZ", "GMT0", 1);
		tzset();
	}
	if(debugStream != nullptr) {
		debugStream->print(F("Updating System time from GPS:"));
	}
	struct tm newTimeOfDay;
	newTimeOfDay.tm_year = _gpsYear - 1900;
	newTimeOfDay.tm_mon = _gpsMonth - 1;
	newTimeOfDay.tm_mday = _gpsDay;
	newTimeOfDay.tm_hour = _gpsHours;
	newTimeOfDay.tm_min = _gpsMinutes;
	newTimeOfDay.tm_sec = _gpsSeconds;
	time_t t = mktime(&newTimeOfDay);
	struct timeval now = { .tv_sec = t };
	if(settimeofday(&now, NULL) == 0) {						
		if(debugStream != nullptr) {
			debugStream->println(F("OK"));
		}
	} else {
		if(debugStream != nullptr) {
			debugStream->println(F("Failed"));
		}
	}
	if(_timezone != nullptr) {
		setenv("TZ", _timezone, 1);
		tzset();
	}
}

void ICACHE_FLASH_ATTR gpsManagedRtc::setSystemTimeFromRtc() {
	if(_timezone != nullptr) {	//This horrible bodge exists because mktime and various time functions only work in local time, not UTC!
		setenv("TZ", "GMT0", 1);
		tzset();
	}
	if(debugStream != nullptr) {
		debugStream->print(F("Updating System time from RTC:"));
	}
	struct tm newTimeOfDay;
	newTimeOfDay.tm_year = _rtcYear - 1900;
	newTimeOfDay.tm_mon = _rtcMonth - 1;
	newTimeOfDay.tm_mday = _rtcDay;
	newTimeOfDay.tm_hour = _rtcHours;
	newTimeOfDay.tm_min = _rtcMinutes;
	newTimeOfDay.tm_sec = _rtcSeconds;
	time_t t = mktime(&newTimeOfDay);
	struct timeval now = { .tv_sec = t };
	if(settimeofday(&now, NULL) == 0) {						
		if(debugStream != nullptr) {
			debugStream->println(F("OK"));
		}
	} else {
		if(debugStream != nullptr) {
			debugStream->println(F("Failed"));
		}
	}
	if(_timezone != nullptr) {
		setenv("TZ", _timezone, 1);
		tzset();
	}
}

int64_t ICACHE_FLASH_ATTR gpsManagedRtc::compareTimes(uint8_t sec1, uint8_t min1, uint8_t hr1, uint8_t day1, uint8_t mon1, uint16_t yr1, uint8_t sec2, uint8_t min2, uint8_t hr2, uint8_t day2, uint8_t mon2, uint16_t yr2) {
	struct tm time1;
	struct tm time2;
	time1.tm_hour = hr1,
    time1.tm_min = min1,
    time1.tm_sec = sec1,
    time1.tm_mday = day1;
	time1.tm_mon = mon1 - 1;
	time1.tm_year = yr1 - 1900;
	time2.tm_hour = hr2,
    time2.tm_min = min2,
    time2.tm_sec = sec2,
    time2.tm_mday = day2;
	time2.tm_mon = mon2 - 1;
	time2.tm_year = yr2 - 1900;
	if(_timezone != nullptr) {	//This horrible bodge exists because mktime and various time functions only work in local time, not UTC!
		setenv("TZ", "GMT0", 1);
		tzset();
	}
	time_t epoch1 = mktime(&time1);
	time_t epoch2 = mktime(&time2);
	if(_timezone != nullptr) {
		setenv("TZ", _timezone, 1);
		tzset();
	}
	return difftime(epoch1, epoch2);
}

void ICACHE_FLASH_ATTR gpsManagedRtc::printTime(uint8_t hr, uint8_t min, uint8_t sec, uint8_t day, uint8_t mon, uint16_t yr) {
	debugStream->printf_P(PSTR("%02u:%02u:%02u %02u/%02u/%04u"), hr, min, sec, day, mon, yr);
}

void ICACHE_FLASH_ATTR gpsManagedRtc::configureTimeZone(const char *tz)
{
	if(debugStream != nullptr) {
		debugStream->print(F("Setting timezone:"));
		debugStream->println(tz);
	}
	_timezone = new char[strlen(tz) + 1];
	strlcpy(_timezone, tz, strlen(tz) + 1);
	setenv("TZ",_timezone,1);
	tzset();
}
void ICACHE_FLASH_ATTR gpsManagedRtc::configureTimeZone(String tz)
{
	configureTimeZone(tz.c_str());
}
void ICACHE_FLASH_ATTR gpsManagedRtc::preferRtc()
{
	_preferGps = false;
}
void ICACHE_FLASH_ATTR gpsManagedRtc::preferGps()
{
	_preferGps = true;
}


#endif
