/* 
NMEA0183.h

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

#ifndef _tNMEA0183_H_
#define _tNMEA0183_H_
#include <Arduino.h>
#include <NMEA0183Msg.h>

#define MAX_OUT_BUF 3


extern "C" typedef void(*msgHdlType)(const tNMEA0183Msg &NMEA0183Msg, void* args);

class tNMEA0183
{
  protected:
    HardwareSerial *port;
    int MsgCheckSumStartPos;
    char MsgInBuf[MAX_NMEA0183_MSG_LEN];
    char MsgOutBuf[MAX_OUT_BUF][MAX_NMEA0183_MSG_LEN];
    int MsgOutIdx;
    int MsgInPos;
    int MsgOutPos;
    bool MsgInStarted;
    bool MsgOutStarted;
    uint8_t SourceID;  // User defined ID for this message handler
    bool BlockingOnRead;

    // Handler callback
    msgHdlType MsgHandler;
    void* MsgHdlArgs;
    int nextOutIdx(int idx);

  public:
    tNMEA0183();
    void Begin(HardwareSerial *_port, uint8_t _SourceID=0, unsigned long _baud=4800, bool blockOnRead=false);
    void SetMsgHandler(msgHdlType _MsgHandler, void* args=0);

    /* For incoming messages */
    void ParseMessages();
    bool GetMessage(tNMEA0183Msg &NMEA0183Msg);

    /* For outgoing messages */
    bool SendMessage(const char *buf);
    void kick();
};

#endif
