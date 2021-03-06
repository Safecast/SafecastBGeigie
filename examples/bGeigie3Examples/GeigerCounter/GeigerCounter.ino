/*
   HardwareCounter.ino
   Example of using the Hardware Counter library

   This example will compute the one minute number of counts using a 12 bins
   sliding window, updating the value every 5 seconds.  Obviously, one has to
   wait one full minute before having a correct value for the last minute.

   Created 2011, Robin Scheibler aka FakuFaku
   This example is in the public domain.
*/

#include <HardwareCounter.h>
#include <bg3_pins.h>
#include <bg_sensors.h>
#include <SD.h>
#include <SPI.h>

#define DELAY 5000
#define NBINS 12

static HardwareCounter counter(counts, DELAY);  // The hardware counter object
static unsigned long bins[NBINS] = {0};         // the sliding window
static int index = 0;                           // the current bin index

void setup()
{

  // start serial
  Serial.begin(57600);

  // output some nice message
  Serial.println("Start counting counts!");
  delay(100);

  bg_hvps_pwr_config();
  bg_hvps_on();

  bgs_sensors_init(sense_pwr, batt_sense, temp_sense, hum_sense, hv_sense);

  // Start the counter
  counter.start();

}

void loop()
{
  // if counter result available
  if (counter.available())
  {
    int cpb = counter.count();    // fetch value of counter
    int i;
    int cpm = 0;
    bins[index] = cpb;
    for (i = 0 ; i < NBINS ; i++) // compute sum of all bins
      cpm += bins[i];
    index = (index+1)%NBINS;      // update bin index

    // print out result
    Serial.print("Counts in last 5 seconds: ");
    Serial.print(cpb);
    Serial.print(" and last minute: ");
    Serial.print(cpm);
    Serial.print(" (HV == ");
    Serial.print(bgs_read_hv());
    Serial.println(")");

    // restart the counter
    counter.start();
  }
}

