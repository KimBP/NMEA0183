/* 
NMEA0183.cpp

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

#include <NMEA0183.h>

tNMEA0183::tNMEA0183()
: port(0), MsgCheckSumStartPos(-1), MsgOutIdx(0),
  MsgInPos(0), MsgOutPos(0),
  MsgInStarted(false), MsgOutStarted(false),
  SourceID(0),
  MsgHandler(0), MsgHdlArgs(0)
{
  for(int i=0; i < MAX_OUT_BUF;i++) {
    MsgOutBuf[i][0]='\0';
  }
}

//*****************************************************************************
void tNMEA0183::Begin(HardwareSerial *_port, uint8_t _SourceID, unsigned long _baud) {
  SourceID=_SourceID;
  port=_port;
  port->begin(_baud);
//*****************************************************************************
void tNMEA0183::SetMsgHandler(msgHdlType _MsgHandler, void* args)
{
	MsgHandler = _MsgHandler;
	MsgHdlArgs = args;
}

//*****************************************************************************
void tNMEA0183::ParseMessages() {
  tNMEA0183Msg NMEA0183Msg;
  
    while (GetMessage(NMEA0183Msg)) {
      if (MsgHandler!=0) MsgHandler(NMEA0183Msg,MsgHdlArgs);
    }
}

//*****************************************************************************
bool tNMEA0183::GetMessage(tNMEA0183Msg &NMEA0183Msg) {
  bool result=false;

  while (port->available() > 0 && !result) {
    int NewByte=port->read();
      if (NewByte=='$' || NewByte=='!') { // Message start
        MsgInStarted=true;
        MsgInPos=0;
        MsgInBuf[MsgInPos]=NewByte;
        MsgInPos++;
      } else if (MsgInStarted) {
        MsgInBuf[MsgInPos]=NewByte;
        if (NewByte=='*') MsgCheckSumStartPos=MsgInPos;
        MsgInPos++;
        if (MsgCheckSumStartPos>0 and MsgCheckSumStartPos+3==MsgInPos) { // We have full checksum and so full message
            MsgInBuf[MsgInPos]=0; // add null termination
          if (NMEA0183Msg.SetMessage(MsgInBuf)) {
            NMEA0183Msg.SourceID=SourceID;
            result=true;
          }
          MsgInStarted=false;
          MsgInPos=0;
          MsgCheckSumStartPos=-1;  
        }
        if (MsgInPos>=MAX_NMEA0183_MSG_LEN) { // Too many chars in message. Start from beginning
          MsgInStarted=false;
          MsgInPos=0;
          MsgCheckSumStartPos=-1;  
        }
      }
  }
  
  return result;
}

//*****************************************************************************
int tNMEA0183::nextOutIdx(int idx)
{
  idx++;
  if (idx >= MAX_OUT_BUF) {
    idx = 0;
  }
  return idx;
}

//*****************************************************************************
void tNMEA0183::kick() {
  if (MsgOutBuf[MsgOutIdx][0] != '\0') {
    MsgOutStarted = true;
    while(port->availableForWrite() > 0) {
      port->write(MsgOutBuf[MsgOutIdx][MsgOutPos]);
      MsgOutPos++;
      if (MsgOutBuf[MsgOutIdx][MsgOutPos] == '\0') {
        // Done with this message - clear it and prepare for next
        MsgOutBuf[MsgOutIdx][0] = '\0';
	MsgOutIdx = nextOutIdx(MsgOutIdx);
        MsgOutPos=0;
        if (MsgOutBuf[MsgOutIdx][0] == '\0') {
          MsgOutStarted=false;
        }
        return;
      }
    }
  }
}

//*****************************************************************************
bool tNMEA0183::SendMessage(const char *buf) {

  if(strlen(buf) >= MAX_NMEA0183_MSG_LEN)
    return false;

  bool result=true;
  int bufIdx = MsgOutIdx;
  if(MsgOutStarted) {
    result=false;
    do {
      bufIdx = nextOutIdx(bufIdx);
      if (MsgOutBuf[bufIdx][0] != '\0') {
        result=true; // Yep it is free
      }
    } while ((result==false) && (bufIdx != MsgOutIdx) );
  }
  if (result==true) {
    strcpy(&MsgOutBuf[bufIdx][0],buf);
    kick();
  }
  return result;
}


