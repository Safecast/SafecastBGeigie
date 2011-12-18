/*
   The bGeigie-mini
   A device for car-borne radiation measurement (aka Radiation War-driving).

   Copyright (c) 2011, Robin Scheibler aka FakuFaku, Christopher Wang aka Akiba
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
   DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
   ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <SD.h>
#include <chibi.h>
#include <limits.h>
#include <EEPROM.h>
#include <GPS.h>
#include <InterruptCounter.h>

#define TIME_INTERVAL 5000
#define NX 12
#define DEST_ADDR 0x1234
#define CHANNEL 20
#define TX_ENABLED 1
#define LED_ENABLED 0
#define PRINT_BUFSZ 80
#define AVAILABLE 'A'          // indicates geiger data are ready (available)
#define VOID      'V'          // indicates geiger data not ready (void)
#define BMRDD_EEPROM_ID 100
#define BMRDD_ID_LEN 3

static const int chipSelect = 10;
static const int radioSelect = A3;
static const int sdPwr = 4;

unsigned long shift_reg[NX] = {0};
unsigned long reg_index = 0;
unsigned long total_count = 0;
int str_count = 0;
char geiger_status = VOID;

// This is the data file object that we'll use to access the data file
File dataFile; 

// GPS object
GPS *gps;

// the line buffer for serial receive and send
static char line[LINE_SZ];

static char msg1[] PROGMEM = "SD init...\n";
static char msg2[] PROGMEM = "Card failure...\n";
static char msg3[] PROGMEM = "Card initialized\n";
static char msg4[] PROGMEM = "Error: Log file cannot be opened.\n";
static char msg5[] PROGMEM = "Device Id: ";

char filename[13];      // placeholder for filename
char hdr[6] = "BMRDD";  // header for sentence
char dev_id[BMRDD_ID_LEN+1];  // device id
char ext_log[] = ".log";
char ext_bak[] = ".bak";
char fileHeader[] = "# NEW LOG\n# format=1.2.0\n";
 
// State variables
int gps_init_acq = 0;
int log_created = 0;

/**************************************************************************/
/*!

*/
/**************************************************************************/
void setup()
{
  char tmp[25];
  
  // init serial
  Serial.begin(9600);

  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(chipSelect, OUTPUT);
  digitalWrite(chipSelect, HIGH);     // disable SD card chip select 
  
  pinMode(radioSelect, OUTPUT);
  digitalWrite(radioSelect, HIGH);    // disable radio chip select
  
  pinMode(sdPwr, OUTPUT);
  digitalWrite(sdPwr, LOW);           // turn on SD card power
  
  // Create pulse counter on INT1
  interruptCounterSetup(1, TIME_INTERVAL);

  // Create GPS object using default Serial connection
  gps = new GPS(&Serial, line);
  
  // set device id
  pullDevId();

  strcpy_P(tmp, msg5);
  Serial.print(tmp);
  Serial.println(dev_id);

  // print free RAM. we should try to maintain about 300 bytes for stack space
  Serial.println(FreeRam());

#if TX_ENABLED
  // init chibi on channel normally 20
  chibiInit();
  chibiSetChannel(CHANNEL);
#endif

  // turn SD card on
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);
  
  strcpy_P(tmp, msg1);
  Serial.print(tmp);
  
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    // don't do anything more:
    int printOnce = 1;
    while(1) 
    {
      if (printOnce) {
        printOnce = 0;
        strcpy_P(tmp, msg2);
        Serial.print(tmp);
      }
    }
  }
  strcpy_P(tmp, msg3);
  Serial.print(tmp);

  // set initial state of Geiger to void
  geiger_status = VOID;
  str_count = 0;
  
  // print free RAM. we should try to maintain about 300 bytes for stack space
  Serial.println(FreeRam());

  // And now Start the Pulse Counter!
  interruptCounterReset();

  // Starting now!
  Serial.println("Starting now!");
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
void loop()
{
  char tmp[25];

  // update gps
  gps->update();

  // generate CPM every TIME_INTERVAL seconds
  if (interruptCounterAvailable())
  {
    if (gps->available())
    {
      unsigned long cpm=0, cpb=0;
      byte line_len;

      // obtain the count in the last bin
      cpb = interruptCounterCount();

      // reset the pulse counter
      interruptCounterReset();

      // insert count in sliding window and compute CPM
      shift_reg[reg_index] = cpb;     // put the count in the correct bin
      reg_index = (reg_index+1) % NX; // increment register index
      cpm = cpm_gen();                // compute sum over all bins

      // update the total counter
      total_count += cpb;
      
      // set status of Geiger
      if (str_count < NX)
      {
        geiger_status = VOID;
        str_count++;
      } else if (cpm == 0) {
        geiger_status = VOID;
      } else {
        geiger_status = AVAILABLE;
      }
      
      // generate timestamp. only update the start time if 
      // we printed the timestamp. otherwise, the GPS is still 
      // updating so wait until its finished and generate timestamp
      memset(line, 0, LINE_SZ);

      line_len = gps_gen_timestamp(line, shift_reg[reg_index], cpm, cpb);


      if (gps_init_acq == 0 && gps->getData()->status[0] == AVAILABLE)
      {
        // flag GPS acquired
        gps_init_acq = 1;
        // Create the filename for that drive
        strcpy(filename, dev_id);
        strcat(filename, "-");
        strncat(filename, gps->getData()->date+2, 2);
        strncat(filename, gps->getData()->date, 2);
        // print some comment line to mark beginning of new log
        strcpy(filename+8, ext_log);
        dataFile = SD.open(filename, FILE_WRITE);
        if (dataFile)
        {
          Serial.print("Filename: ");
          Serial.println(filename);
          dataFile.print(fileHeader);
          dataFile.close();
        }
        else
        {
          char tmp[40];
          strcpy_P(tmp, msg4);
          Serial.print(tmp);
        }
        // write to backup file as well
        strcpy(filename+8, ext_bak);
        dataFile = SD.open(filename, FILE_WRITE);
        if (dataFile)
        {
          dataFile.print(fileHeader);
          dataFile.close();
        }
        else
        {
          char tmp[40];
          strcpy_P(tmp, msg4);
          Serial.print(tmp);
        }
      }
      
      // turn on SD card power and delay a bit to initialize
      //digitalWrite(sdPwr, LOW);
      //delay(10);
      
      // init the SD card see if the card is present and can be initialized:
      //while (!SD.begin(chipSelect));

      if (gps_init_acq == 0)
      {
        Serial.print("No GPS: ");
        Serial.println(line);
#if TX_ENABLED
        // send out wirelessly. first wake up the radio, do the transmit, then go back to sleep
        chibiSleepRadio(0);
        delay(10);
        chibiTx(DEST_ADDR, (byte *)line, LINE_SZ);
        chibiSleepRadio(1);
#endif
      }
      else
      {
        // dump data to SD card
        strcpy(filename+8, ext_log);
        dataFile = SD.open(filename, FILE_WRITE);
        if (dataFile)
        {
          Serial.println(line);
          dataFile.print(line);
          dataFile.print("\n");
          dataFile.close();

  #if TX_ENABLED
          // send out wirelessly. first wake up the radio, do the transmit, then go back to sleep
          chibiSleepRadio(0);
          delay(10);
          chibiTx(DEST_ADDR, (byte *)line, LINE_SZ);
          chibiSleepRadio(1);
  #endif
        }
        else
        {
          char tmp[40];
          strcpy_P(tmp, msg4);
          Serial.print(tmp);
        }   
        
        // write to backup file as well
        strcpy(filename+8, ext_bak);
        dataFile = SD.open(filename, FILE_WRITE);
        if (dataFile)
        {
          dataFile.print(line);
          dataFile.print("\n");
          dataFile.close();
        }
        else
        {
          char tmp[40];
          strcpy_P(tmp, msg4);
          Serial.print(tmp);
        }
        
        //turn off sd power        
        //digitalWrite(sdPwr, HIGH); 
      }
    }
  }
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
byte gps_gen_timestamp(char *buf, unsigned long counts, unsigned long cpm, unsigned long cpb)
{
  byte len;
  byte chk;

  gps_t *ptr = gps->getData();
  
  memset(buf, 0, LINE_SZ);
  sprintf(buf, "$%s,%s,20%s-%s-%sT%s:%s:%sZ,%ld,%ld,%ld,%c,%s,%s,%s,%s,%s,%s,%s,%s",  \
              hdr, \
              dev_id, \
              ptr->datetime.year, ptr->datetime.month, ptr->datetime.day,  \
              ptr->datetime.hour, ptr->datetime.minute, ptr->datetime.second, \
              cpm, \
              cpb, \
              total_count, \
              geiger_status, \
              ptr->lat, ptr->lat_hem, \
              ptr->lon, ptr->lon_hem, \
              ptr->altitude, \
              ptr->status, \
              ptr->precision, \
              ptr->quality);
   len = strlen(buf);
   buf[len] = '\0';

   // generate checksum
   chk = GPS::checksum(buf+1, len);

   // add checksum to end of line before sending
   if (chk < 16)
     sprintf(buf + len, "*0%X", (int)chk);
   else
     sprintf(buf + len, "*%X", (int)chk);

   return len;
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
unsigned long cpm_gen()
{
   unsigned int i;
   unsigned long c_p_m = 0;
   
   // sum up
   for (i=0 ; i < NX ; i++)
     c_p_m += shift_reg[i];
   
   return c_p_m;
}

void pullDevId()
{
  for (int i=0 ; i < BMRDD_ID_LEN ; i++)
  {
    dev_id[i] = (char)EEPROM.read(i+BMRDD_EEPROM_ID);
    if (dev_id[i] < '0' || dev_id[i] > '9')
    {
      dev_id[i] = '0';
      EEPROM.write(i+BMRDD_EEPROM_ID, dev_id[i]);
    }
  }
  dev_id[BMRDD_ID_LEN] = NULL;
}

