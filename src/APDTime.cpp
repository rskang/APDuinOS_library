/* APDuinOS Library
 * Copyright (C) 2012 by Gy�rgy Schreiber
 *
 * This file is part of the APDuinOS Library
 *
 * This Library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the APDuinOS Library.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * APDTime.cpp
 *
 *  Created on: Mar 27, 2012
 *      Author: George Schreiber
 *
 * APDTime implements APDuino timekeeping with RTC hardware or software clock.
 * It is based on the RTCLib from JeeLabs - http://news.jeelabs.org/code/
 *
 * Credits:
 * * RTCLib - Copyright JeeLabs - http://news.jeelabs.org/code/
 * * NTP syncing is based on NTP2RTC - code in the public domain - http://www.arduino.cc/playground/Main/DS1307OfTheLogshieldByMeansOfNTP
 *   original credits:
 *      "based upon Udp NTP Client, by Michael Margolis, mod by Tom Igoe
 *       uses the RTClib from adafruit (based upon Jeelabs)
 *       Thanx!
 *       mod by Rob Tillaart, 10-10-2010"
 *
 */

#include "APDTime.h"

APDTime::APDTime() {
	// TODO Auto-generated constructor stub
  initBlank();
}

APDTime::APDTime(boolean bRTC) {
  initBlank();
  if (bRTC) {
    if (this->pRTC == NULL) {
      this->pRTC = new RTC_DS1307();

      if (this->pRTC != NULL) {
        // not powering from A2&3
        //pinMode(A3, OUTPUT);  //digitalWrite(A3, HIGH);
        //pinMode(A2, OUTPUT);  //digitalWrite(A2, LOW);

        Wire.begin();
        pRTC->begin();

        if (! pRTC->isrunning()) {
          SerPrintP("RTC is NOT running (no hw?)...");
          free(this->pRTC);
          this->pRTC = NULL;
//          SerPrintP("RTC HW NEEDS TESTING!\n");
          //RTC.adjust(DateTime(__DATE__, __TIME__));
        } else {
          // we have RTC
          SerPrintP("HW RTC OK..."); //pRTC->now());
        }
      } else {
          SerPrintP("RTC alloc fail...");
          pRTC = NULL;
      }
    }else{
        SerPrintP("RTC already init...");
    }
  } else {
      // millis based already by default
      if (pRTCm == NULL) SerPrintP("ERROR - SW clock should be running!\n");
  }

}

APDTime::~APDTime() {
	// TODO Auto-generated destructor stub
}

void APDTime::initBlank() {
  rollovers = 0;
  start_time = millis();
#ifdef DEBUG
  SerPrintP("APDTime Starting at "); Serial.print(start_time,DEC); SerPrintP(" millis...\n");
#endif
  pRTC = NULL;
  pRTCm = new RTC_Millis();
  if (pRTCm) {
      SerPrintP("Starting SW clock...");
      // TODO check & load time from APDLOG.TXT last message, if exists
      pRTCm->begin(DateTime(__DATE__, __TIME__));           // SW clock always start @ last build time - adjusted later if RTC | NTP
      PrintDateTime(pRTCm->now());
      SerPrintP("\n");
  } else {
      SerPrintP("Seems SW clock setup failed. :(\n");
  }
  memcpy(timeServer,0,4*sizeof(byte));
  localPort = 0;
  pUdp = NULL;
//#ifdef DEBUG
  SerPrintP("SW DateTime: ");
  PrintDateTime(this->now());
  SerPrintP("\n");
//#endif
}

DateTime APDTime::now() {
  if (this->pRTCm != NULL && this->pRTC->isrunning()) {
      return this->pRTC->now();
  } else if (this->pRTCm != NULL) {
      return this->pRTCm->now();
  }
  return DateTime(1970,01,01,0,0,0);            // otherwise
}

// returns time printed to the char array. no checks, you must make sure the ptr to the char array is valid
// and that the buffer is long enough to hold the result (otherwise bad things WILL happen)
char *APDTime::nowS(char *strbuf) {
  DateTime now = this->now();
  sprintf(strbuf,"%04d-%02d-%02d %02d:%02d:%02d",now.year(),now.month(),now.day(),now.hour(),now.minute(),now.second());
  return strbuf;
}

unsigned long APDTime::getUpTime() {
  return (millis() - this->start_time);
}

char *APDTime::getUpTimeS(char *psz_uptime) {
  unsigned long ut = this->getUpTime() / 1000;
  int updays = (ut / 3600) / 24;
  int uphours = (ut / 3600) % 24;
  int upmins = (ut % 3600) / 60;
  int upsec = (ut % 3600) % 60;
  sprintf(psz_uptime,"%03dd%02dh%02dm%02ds",updays,uphours,upmins,upsec);
  return psz_uptime;
}

// TODO return whatever udp would return
void APDTime::setupNTPSync(int UDPPort, byte *TimeServer, int iTZ, int iDST ) {
  // check if not in use...
  SerPrintP("SETUP UDP 4 NTP\n");
  if (pUdp==NULL) {
    //Ethernet.begin(mac, ip, );     // For when you are directly connected to the Internet.
    memcpy(timeServer,TimeServer,4);
    dst = iDST;
    time_zone = iTZ;
    localPort = UDPPort;
    pUdp = new EthernetUDP;
    if (pUdp != NULL) {
      if (pUdp->begin(localPort)) {
          SerPrintP("UDP networking prepared for NTP.\n");
      } else {
          SerPrintP("UDP network setup for NTP FAILED.\n");
          free(pUdp);
          pUdp = NULL;
      }
    }else{
        SerPrintP("UDP Networking initialization failed.\n");
    }
  }
}


///////////////////////////////////////////
//
// NTP2RTC MISC
//
void APDTime::PrintDateTime(DateTime t)
{
    //char datestr[24] = "";
    char datestr[32] = "";
    // TODO implement format string, input by user
    sprintf(datestr, "%04d-%02d-%02d %02d:%02d:%02d", t.year(), t.month(), t.day(), t.hour(), t.minute(), t.second());
    Serial.print(datestr);
}


/* * send an NTP request to the time server at the given address
 * NTP syncing is based on NTP2RTC - code in the public domain - http://www.arduino.cc/playground/Main/DS1307OfTheLogshieldByMeansOfNTP
 *   original credits:
 *      "based upon Udp NTP Client, by Michael Margolis, mod by Tom Igoe
 *       uses the RTClib from adafruit (based upon Jeelabs)
 *       Thanx!
 *       mod by Rob Tillaart, 10-10-2010""
*/
unsigned long APDTime::sendNTPpacket(byte *address)
{
  unsigned long ulret = 0;
  if (this->pUdp != NULL) {
    SerPrintP("PREPARING NTP PACKET...\n");
    memset(pb, 0, NTP_PACKET_SIZE);		// blank packet
    // Initialize values needed to form NTP request
    // (see URL above for details on the packets)
    pb[0] = 0b11100011;   // LI, Version, Mode
    pb[1] = 0;     // Stratum, or type of clock
    pb[2] = 6;     // Polling Interval
    pb[3] = 0xEC;  // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    pb[12]  = 49;
    pb[13]  = 0x4E;
    pb[14]  = 49;
    pb[15]  = 52;
    SerPrintP("SENDING NTP PACKET...\n");
    // all NTP fields have been given values, now
    // you can send a packet requesting a timestamp:
  #if ARDUINO >= 100
    // IDE 1.0 compatible:
    if (this->pUdp->beginPacket(address, 123)) {
    	this->pUdp->write(pb,NTP_PACKET_SIZE);
    	this->pUdp->endPacket();
    } else {
    	SerPrintP("ERR STARTING UDP.\n");
    }
  #else
    pUdp->sendPacket( pb,NTP_PACKET_SIZE,  address, 123); //NTP requests are to port 123
  #endif
  } else {
      SerPrintP("NO UDP\n");
  }
  return ulret;
}


void APDTime::adjust(DateTime dt) {
	SerPrintP("ADJUST:");
  if (this->pRTC != NULL && this->pRTC->isrunning()) {
      SerPrintP("RTC,");
      pRTC->adjust(dt);
  }
  SerPrintP("SW.");
  pRTCm->adjust(dt);
  SerPrintP("DONE.\n");
}


/* ntpSync() will sync hw/sw clock to a network time server
 * NTP syncing is based on NTP2RTC - code in the public domain - http://www.arduino.cc/playground/Main/DS1307OfTheLogshieldByMeansOfNTP
 *   original credits:
 *      "based upon Udp NTP Client, by Michael Margolis, mod by Tom Igoe
 *       uses the RTClib from adafruit (based upon Jeelabs)
 *       Thanx!
 *       mod by Rob Tillaart, 10-10-2010"
 *
 * integrated into APDuinOS by George Schreiber 27-03-2012
*/
void APDTime::ntpSync()
{
	if (pUdp == NULL) {
		SerPrintP("\nNO UDP OBJ!");
		return;
	}
#ifdef DEBUG
  SerPrintP("\nNTP SYNC!\n");
#endif
  if (this->pRTCm != NULL && timeServer[0] != 0) {
//#ifdef DEBUG
    SerPrintP("\nSW:");
    PrintDateTime(pRTCm->now());
    if (pRTC != NULL && pRTC->isrunning()) {
        SerPrintP("\nHW:");
        PrintDateTime(pRTC->now());
    }
    SerPrintP(".NTP REQ:");
    //delay(5);
    SerDumpIP(timeServer);
    SerPrintP(".\n");
    //delay(10);
//#endif

    // send an NTP packet to a time server
    this->sendNTPpacket(timeServer);
//#ifdef DEBUG
    SerPrintP("Packet sent.\n");
//#endif
    // wait to see if a reply is available
    //delay(3000);                // TODO iso loop and check if there is a response, bail out after soft timeout
    for (int i =0; i < 1000; i++) {	// approx 10 secs.
    	if (!pUdp->available()) {
    		delay(10);
    	} else {
    		break;
    	}
    }

    if ( pUdp->available() ) {
#ifdef DEBUG
       SerPrintP("Comm...");
#endif
      // read the packet into the buffer
  #if ARDUINO >= 100
      pUdp->read(pb, NTP_PACKET_SIZE);      // New from IDE 1.0,
  #else
      pUdp->readPacket(pb, NTP_PACKET_SIZE);
  #endif
#ifdef DEBUG
       SerPrintP("Processing NTP response ...");
#endif
      // NTP contains four timestamps with an integer part and a fraction part
      // we only use the integer part here
      unsigned long t1, t2, t3, t4;
      t1 = t2 = t3 = t4 = 0;
      for (int i=0; i< 4; i++)
      {
        t1 = t1 << 8 | pb[16+i];
        t2 = t2 << 8 | pb[24+i];
        t3 = t3 << 8 | pb[32+i];
        t4 = t4 << 8 | pb[40+i];
      }

      // part of the fractional part
      // could be 4 bytes but this is more precise than the 1307 RTC
      // which has a precision of ONE second
      // in fact one byte is sufficient for 1307
      float f1,f2,f3,f4;
      f1 = ((long)pb[20] * 256 + pb[21]) / 65536.0;
      f2 = ((long)pb[28] * 256 + pb[29]) / 65536.0;
      f3 = ((long)pb[36] * 256 + pb[37]) / 65536.0;
      f4 = ((long)pb[44] * 256 + pb[45]) / 65536.0;

      // NOTE:
      // one could use the fractional part to set the RTC more precise
      // 1) at the right (calculated) moment to the NEXT second!
      //    t4++;
      //    delay(1000 - f4*1000);
      //    RTC.adjust(DateTime(t4));
      //    keep in mind that the time in the packet was the time at
      //    the NTP server at sending time so one should take into account
      //    the network latency (try ping!) and the processing of the data
      //    ==> delay (850 - f4*1000);
      // 2) simply use it to round up the second
      //    f > 0.5 => add 1 to the second before adjusting the RTC
      //   (or lower threshold eg 0.4 if one keeps network latency etc in mind)
      // 3) a SW RTC might be more precise, => ardomic clock :)
#ifdef DEBUG
       SerPrintP("NTP->UNIX...");
#endif
      // convert NTP to UNIX time, differs seventy years = 2208988800 seconds
      // NTP starts Jan 1, 1900
      // Unix time starts on Jan 1 1970.
      const unsigned long seventyYears = 2208988800UL;
      t1 -= seventyYears;
      t2 -= seventyYears;
      t3 -= seventyYears;
      t4 -= seventyYears;

      /*
      Serial.println("T1 .. T4 && fractional parts");
      PrintDateTime(DateTime(t1)); Serial.println(f1,4);
      PrintDateTime(DateTime(t2)); Serial.println(f2,4);
      PrintDateTime(DateTime(t3)); Serial.println(f3,4);
      */
      PrintDateTime(DateTime(t4)); Serial.println(f4,4);

      // Adjust timezone and DST
      t4 += (( time_zone + dst )* 3600L);     // Notice the L for long calculations!
      t4 += 1;               // adjust the delay(1000) at begin of loop!
      if (f4 > 0.4) t4++;    // adjust fractional part, see above
      this->adjust(DateTime(t4));

//#ifdef DEBUG
      if (this->pRTC != NULL) {
        SerPrintP("RTC after : ");
        PrintDateTime(pRTC->now());
      }
      if (this->pRTC != NULL) {
        SerPrintP("RTC after : ");
        PrintDateTime(pRTC->now());
      }
      SerPrintP("APDTime will give time:");
      PrintDateTime(this->now());

      SerPrintP("\ndone ...\n");
//#endif
    }
    else
    {
      SerPrintP("No UDP available ...");
    }
  } else {
      Serial.println("No RTC. Sorry.");
  }
}
