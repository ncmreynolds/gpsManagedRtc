/*
 *	An Arduino library for GPS Managed RTC support
 *
 *	https://github.com/ncmreynolds/gpsManagedRtc
 *
 *	Released under LGPL-2.1 see https://github.com/ncmreynolds/gpsManagedRtc/LICENSE for full license
 *
 */
#ifndef gpsManagedRtc_h
#define gpsManagedRtc_h
#include <Arduino.h>
#include <RV-3028-C7.h>
#include <TinyGPS++.h>

class gpsManagedRtc	{

	public:
		gpsManagedRtc();														//Constructor function
		~gpsManagedRtc();														//Destructor function
		void gpsPins(int rxPin, int txPin = -1);								//GPS pins. BEWARE TX->RX and RX->TX classic gotcha on pinout. TX pin is optional
		void gpsBaud(uint32_t baud);											//GPS Baud rate
		void configureTimeZone(const char *tz);									//Set a time zone
		void configureTimeZone(String tz);										//Time zones must be in IANA format eg. "GMT0BST,M3.5.0/1,M10.5.0", see https://raw.githubusercontent.com/nayarsystems/posix_tz_db/master/zones.csv for a list
		void begin();															//Start the gpsManagedRtc
		bool timeIsGood();														//Is the system time good?
		void debug(Stream &);													//Start debugging on a stream
		void housekeeping();													//Do updates, must be run frequently
	protected:
	private:
		Stream *debugStream = nullptr;											//The stream used for the debugging
		uint32_t updateTimer = 0;												//Utility timer for the houskeeping function
		uint32_t updateInterval = 10E3;											//How often to consider updating the system time or RTC
		uint32_t timeUpdateThreshold = 1;										//Number of seconds to allow to drift before correcting
		//RTC
		RV3028 rtc;
		bool rtcInitialised = false;
		uint8_t rtcSeconds = 0;
		uint8_t rtcMinutes = 0;
		uint8_t rtcHours = 0;
		uint8_t rtcWeekday = 0;
		uint8_t rtcDay = 0;
		uint8_t rtcMonth = 0;
		uint16_t rtcYear = 0;
		//GPS
		TinyGPSPlus gps;
		bool gpsInitialised = false;
		uint8_t gpsSeconds = 0;
		uint8_t gpsMinutes = 0;
		uint8_t gpsHours = 0;
		uint8_t gpsWeekday = 0;
		uint8_t gpsDay = 0;
		uint8_t gpsMonth = 0;
		uint16_t gpsYear = 0;
		uint32_t GPSBaud = 9600;
		int gpsRxPin = -1;
		int gpsTxPin = -1;
		uint32_t gpsUpdateTimer = 0;
		uint8_t gpsSatellitesThreshold = 8;
		float gpsHDOPThreshold = 1.2;
		//General
		struct tm currentTimeInfo;
		char* _timezone = nullptr;
		bool preferGps = true;
		int64_t compareTimes(uint8_t sec1, uint8_t min1, uint8_t hr1, uint8_t day1, uint8_t mon1, uint16_t yr1, uint8_t sec2, uint8_t min2, uint8_t hr2, uint8_t day2, uint8_t mon2, uint16_t yr2);
		bool gpsTimeIsGood();
		bool rtcOutOfSyncWithGps();
		void setRtcFromGps();
		bool systemOutOfSyncWithGps();
		bool systemOutOfSyncWithRtc();
		void setSystemTimeFromGps();
		void setSystemTimeFromRtc();
		void printTime(uint8_t hr, uint8_t min, uint8_t sec, uint8_t day, uint8_t mon, uint16_t yr);
};
#endif
