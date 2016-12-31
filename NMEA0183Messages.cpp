/* 
NMEA0183Messages.cpp

2015 Copyright (c) Kave Oy, www.kave.fi  All right reserved.

Author: Timo Lappalainen

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-
  1301  USA
*/

#include <NMEA0183Messages.h>
#include <cstdlib>
#include <cmath>
#include <cstdio>

const double pi=3.1415926535897932384626433832795;
const double kmhToms=1000.0/3600.0; 
const double knToms=1852.0/3600.0; 
const double degToRad=pi/180.0; 
const double msTokmh=3600.0/1000.0;
const double msTokn=3600.0/1852.0;

//*****************************************************************************
void NMEA0183AddChecksum(char* msg) {
  unsigned int i=1; // First character not included in checksum
  uint8_t chkSum = 0;
  char ascChkSum[4];
  uint8_t tmp;

  while (msg[i] != '\0') {
    chkSum ^= msg[i];
    i++;
  }
  ascChkSum[0] ='*';
  tmp = chkSum/16;
  ascChkSum[1] = tmp > 9 ? 'A' + tmp-10 : '0' + tmp;
  tmp = chkSum%16;
  ascChkSum[2] = tmp > 9 ? 'A' + tmp-10 : '0' + tmp;
  ascChkSum[3] = '\0';
  strcat(msg, ascChkSum);
}

//*****************************************************************************
double LatLonToDouble(const char *data, const char sign) {
  double val=atof(data);
  double deg=floor(val/100);
  
  val=deg+(val-deg*100.0)/60.0;
  if ( sign=='S' || sign=='W' ) val=-val;
  
  return val;
}

//*****************************************************************************
double NMEA0183GPTimeToSeconds(const char *data) {
  double val=atof(data);
  double hh=floor(val/10000);
  double mm=floor((val-hh*10000)/100);
  
  val=hh*3600+mm*60+(val-hh*10000.0-mm*100);
 
  return val;
}

time_t NMEA0183GPSDateTimetotime_t(const char *dateStr, const char *timeStr) {
  struct tm TimeElements;
  char StrCvt[3]="00";

    if (timeStr!=0 && strlen(timeStr)>=6) {
      StrCvt[0]=timeStr[0]; StrCvt[1]=timeStr[1]; 
      TimeElements.tm_hour=atoi(StrCvt);
      StrCvt[0]=timeStr[2]; StrCvt[1]=timeStr[3]; 
      TimeElements.tm_min=atoi(StrCvt);
      StrCvt[0]=timeStr[4]; StrCvt[1]=timeStr[5]; 
      TimeElements.tm_sec=atoi(StrCvt)+30;
    } else {
      TimeElements.tm_sec=0;
      TimeElements.tm_min=0;
      TimeElements.tm_hour=0;
    }
    
    if (dateStr!=0 && strlen(dateStr)==6) {
      StrCvt[0]=dateStr[0]; StrCvt[1]=dateStr[1]; 
      TimeElements.tm_mday=atoi(StrCvt);
      StrCvt[0]=dateStr[2]; StrCvt[1]=dateStr[3]; 
      TimeElements.tm_mon=atoi(StrCvt);
      StrCvt[0]=dateStr[4]; StrCvt[1]=dateStr[5]; 
      TimeElements.tm_year=atoi(StrCvt)+30;
    } else {
      TimeElements.tm_mday=0;
      TimeElements.tm_mon=0;
      TimeElements.tm_year=0;
    }
    
    return mktime(&TimeElements);
} 

//*****************************************************************************
// $GPGGA,182435.00,6023.20859,N,02219.99442,E,2,10,0.9,4.0,M,20.6,M,5.0,0120*4D
bool NMEA0183ParseGGA_nc(const tNMEA0183Msg &NMEA0183Msg, double &GPSTime, double &Latitude, double &Longitude,
                      int &GPSQualityIndicator, int &SatelliteCount, double &HDOP, double &Altitude, double &GeoidalSeparation,
                      double &DGPSAge, int &DGPSReferenceStationID) {
  bool result=( NMEA0183Msg.FieldCount()>=14 );
  
  if ( result ) {
    GPSTime=NMEA0183GPTimeToSeconds(NMEA0183Msg.Field(0));
    Latitude=LatLonToDouble(NMEA0183Msg.Field(1),NMEA0183Msg.Field(2)[0]);
    Longitude=LatLonToDouble(NMEA0183Msg.Field(3),NMEA0183Msg.Field(4)[0]);
    GPSQualityIndicator=atoi(NMEA0183Msg.Field(5));
    SatelliteCount=atoi(NMEA0183Msg.Field(6));
    HDOP=atof(NMEA0183Msg.Field(7));
    Altitude=atof(NMEA0183Msg.Field(8));
    // Check units of antenna altitude NMEA0183Msg.Field(9)
    GeoidalSeparation=atof(NMEA0183Msg.Field(10));
    // Check units of GeoidalSeparation NMEA0183Msg.Field(11)
    DGPSAge=atof(NMEA0183Msg.Field(12));
    DGPSReferenceStationID=atoi(NMEA0183Msg.Field(13));
  }
    
  return result;
}

//*****************************************************************************
// $GPRMC,092348.00,A,6035.04228,N,02115.15472,E,0.01,272.61,060815,7.2,E,D*34
bool NMEA0183ParseRMC_nc(const tNMEA0183Msg &NMEA0183Msg, double &GPSTime, double &Latitude, double &Longitude,
                      double &TrueCOG, double &SOG, unsigned long &DaysSince1970, double &Variation, time_t *DateTime) {
  bool result=( NMEA0183Msg.FieldCount()>=11 );
  
  if ( result ) {
    time_t lDT;

    GPSTime=NMEA0183GPTimeToSeconds(NMEA0183Msg.Field(0));
    Latitude=LatLonToDouble(NMEA0183Msg.Field(2),NMEA0183Msg.Field(3)[0]);
    Longitude=LatLonToDouble(NMEA0183Msg.Field(4),NMEA0183Msg.Field(5)[0]);
    SOG=atof(NMEA0183Msg.Field(6))*knToms;
    TrueCOG=atof(NMEA0183Msg.Field(7))*degToRad;
    
    lDT=NMEA0183GPSDateTimetotime_t(NMEA0183Msg.Field(8),0)+floor(GPSTime);
    DaysSince1970=0; // Deprecated for now
    //DaysSince1970=elapsedDays(lDT);
    if (DateTime!=0) *DateTime=lDT;
    Variation=atof(NMEA0183Msg.Field(9))*degToRad; if (NMEA0183Msg.Field(10)[0]=='W') Variation=-Variation;
  }

  return result;
}
//*****************************************************************************
// $GPVTG,89.34,T,81.84,M,0.00,N,0.01,K*24
bool NMEA0183ParseVTG_nc(const tNMEA0183Msg &NMEA0183Msg, double &TrueCOG, double &MagneticCOG, double &SOG) {
  bool result=( NMEA0183Msg.FieldCount()>=8 );
  
  if ( result ) {
    TrueCOG=atof(NMEA0183Msg.Field(0))*degToRad;
    MagneticCOG=atof(NMEA0183Msg.Field(2))*degToRad;
    if (NMEA0183Msg.Field(6)[0]!=0) {  // km/h is valid
      SOG=atof(NMEA0183Msg.Field(6))*kmhToms;
    } else {
      SOG=atof(NMEA0183Msg.Field(4))*knToms;
    }
  }

  return result;
}

//*****************************************************************************
// Helper to avoid enabling floating point support
int sprintfDouble2(char* msg, double val)
{
  int valInt = val;
  int valFrag;

  val -= valInt;
  valFrag = val * 100;

  if (valFrag < 10) {
	  return sprintf(msg,"%d.0%d",valInt, valFrag);
  } else {
	  return sprintf(msg,"%d,%d", valInt, valFrag);
  }
}

//*****************************************************************************
bool NMEA0183BuildVTG(char* msg, const char Src[], double TrueCOG, double MagneticCOG, double SOG)
{
  char scratch[20];

  msg[0] = '$';
  msg[1] = Src[0];
  msg[2] = Src[1];
  strcpy(&msg[3],"VTG,");
  TrueCOG /= degToRad;
  if (TrueCOG < 360) {
    sprintfDouble2(scratch, TrueCOG);
    strcat(msg,scratch);
  }
  strcat(msg,",T,");
  MagneticCOG /= degToRad;
  if (MagneticCOG < 360) {
	sprintfDouble2(scratch, MagneticCOG);
    strcat(msg,scratch);
  }
  strcat(msg,",M,");
  if (SOG >= 0.00) {
     sprintfDouble2(scratch, SOG*msTokn);
     strcat(msg, scratch);
     strcat(msg,",N,,K");
  } else {
     strcat(msg, ",N,,K,");
  }

  NMEA0183AddChecksum(msg);
  return true;
}

//*****************************************************************************
// $xxDPT,76.1,0.0,100*1B
bool NMEA0183ParseDPT_nc(const tNMEA0183Msg &NMEA0183Msg,double &depth, double &offset, double &maxRange) {
  maxRange = 0;

  switch (NMEA0183Msg.FieldCount() ) {
  case 3:
	  maxRange = atof(NMEA0183Msg.Field(2));
	  // Fall through
  case 2:
	  depth = atof(NMEA0183Msg.Field(0));
	  offset = atof(NMEA0183Msg.Field(1));
	  return true;
	  break;
  default:
	  return false;
  }
}


//*****************************************************************************
// $HEHDT,244.71,T*1B
bool NMEA0183ParseHDT_nc(const tNMEA0183Msg &NMEA0183Msg,double &TrueHeading) {
  bool result=( NMEA0183Msg.FieldCount()>=2 );
  if ( result ) {
    TrueHeading=atof(NMEA0183Msg.Field(0))*degToRad;
  }

  return result;
}

//*****************************************************************************
// !AIVDM,1,1,,B,177KQJ5000G?tO`K>RA1wUbN0TKH,0*5C
// PkgCnt (1)
// PkgNmb (1)
// SeqMessageId (empty)
// Radio Channel Code (B): A/B or 1/2
// Payload - 6bit encoded
// Fillbits (0)
bool NMEA0183ParseVDM_nc(const tNMEA0183Msg &NMEA0183Msg, 
			uint8_t &pkgCnt, uint8_t &pkgNmb,
			unsigned int &seqMessageId, char &channel,
			unsigned int &length, char *bitstream,
			unsigned int &fillBits)
{
  bool result=( NMEA0183Msg.FieldCount()>=6);

  if ( result ) {
    unsigned int payloadLen = NMEA0183Msg.FieldLen(4);
    if ( payloadLen > length)
      return false;
    length = payloadLen;
    memcpy(bitstream, NMEA0183Msg.Field(4), length);
    fillBits=atoi(NMEA0183Msg.Field(5));
    seqMessageId=atoi(NMEA0183Msg.Field(2));
    pkgNmb=atoi(NMEA0183Msg.Field(1));
    pkgCnt=atoi(NMEA0183Msg.Field(0));
    channel = *NMEA0183Msg.Field(3);
    if (channel == '1') channel = 'A';
    if (channel == '2') channel = 'B';
  }

  return result;
}

