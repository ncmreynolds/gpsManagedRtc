/*
 * An example of mixing an RTC and GPS module to ensure the 'system time' on an ESP32 is always passably accurate
 * 
 *  https://github.com/ncmreynolds/gpsManagedRtc
 *
 *  Released under LGPL-2.1 see https://github.com/ncmreynolds/gpsManagedRtc/LICENSE for full license
 * 
 */

#include <gpsManagedRtc.h>

gpsManagedRtc managedTime;

void setup() {
  Serial.begin(115200);
  delay(3000);  //To give time to connect to the console on a USB CDC ESP32
  managedTime.debug(Serial);
  managedTime.gpsBaud(9600);
  managedTime.gpsPins(4 /* RX ie. connected to GPS TX pin*/,5 /* TX ie. connected to GPS RX pin*/);
  managedTime.configureTimeZone("GMT0BST,M3.5.0/1,M10.5.0");  //GB/London
  //managedTime.configureTimeZone("NZST-12NZDT,M9.5.0,M4.1.0/3"); //Pacific/Auckland
  managedTime.begin();
}

void loop() {
  managedTime.housekeeping();
}
