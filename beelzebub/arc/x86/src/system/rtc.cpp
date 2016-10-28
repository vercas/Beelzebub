/*
    Copyright (c) 2015 Alexandru-Mihai Maftei. All rights reserved.


    Developed by: Alexandru-Mihai Maftei
    aka Vercas
    http://vercas.com | https://github.com/vercas/Beelzebub

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to
    deal with the Software without restriction, including without limitation the
    rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
    sell copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

      * Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimers.
      * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimers in the
        documentation and/or other materials provided with the distribution.
      * Neither the names of Alexandru-Mihai Maftei, Vercas, nor the names of
        its contributors may be used to endorse or promote products derived from
        this Software without specific prior written permission.


    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    WITH THE SOFTWARE.

    ---

    You may also find the text of this license in "LICENSE.md", along with a more
    thorough explanation regarding other files.
*/

#include <system/rtc.hpp>
#include <system/io_ports.hpp>

#define CURRENT_YEAR       2016
#define CURRENT_CENTURY    ((CURRENT_YEAR / 100) * 100)

using namespace Beelzebub;
using namespace Beelzebub::System;

enum class CmosPorts : uint16_t
{
    Address = 0x70,
    Data    = 0x71,
};

enum class CmosRegisters : uint8_t
{
    RtcSeconds                  = 0x00,
    RtcSecondsAlarm             = 0x01,
    RtcMinutes                  = 0x02,
    RtcMinutesAlarm             = 0x03,
    RtcHours                    = 0x04,
    RtcHoursAlarm               = 0x05,
    RtcDayOfWeek                = 0x06,
    RtcDay                      = 0x07,
    RtcMonth                    = 0x08,
    RtcYear                     = 0x09,
    StatusA                     = 0x0A,
    StatusB                     = 0x0B,
    StatusC                     = 0x0C,
    StatusD                     = 0x0D,
    DiagnosticStatus            = 0x0E,
    CmosShutdownStatus          = 0x0F,
    FloppyDiskDriveTypes        = 0x10,
    SystemConfigurationSettings = 0x11,
    HardDiskTypes               = 0x12,
    TypematicParameters         = 0x13,
    InstalledEquipment          = 0x14,
    BaseMemoryLowByte           = 0x15,
    BaseMemoryHighByte          = 0x16,
    ExtendedMemoryLowByte       = 0x17,
    ExtendedMemoryHighByte      = 0x18,
    Hdd0ExtendedType            = 0x19,
    Hdd1ExtendedType            = 0x1A,
};

static __hot uint8_t ReadRegister(uint8_t reg)
{
    Io::Out8((uint16_t)CmosPorts::Address, reg);

    return Io::In8((uint16_t)CmosPorts::Data);
}

static __hot uint8_t ReadRegister(CmosRegisters reg)
{
    return ReadRegister((uint8_t)reg);
}

static __hot bool ReadUpdateInProgressFlag()
{
    return 0 != (ReadRegister(CmosRegisters::StatusA) & 0x80);
}

/****************
    Rtc class
****************/

/*  Statics  */

uint8_t Rtc::Seconds;
uint8_t Rtc::Minutes;
uint8_t Rtc::Hours;
uint8_t Rtc::Day;
uint8_t Rtc::Month;
uint32_t Rtc::Year;

uint8_t Rtc::CenturyRegister;

uint8_t Rtc::RawSeconds;
uint8_t Rtc::RawMinutes;
uint8_t Rtc::RawHours;
uint8_t Rtc::RawDay;
uint8_t Rtc::RawMonth;
uint8_t Rtc::RawYear;

/*  Reading  */

void Rtc::Read()
{
    uint8_t const century_register = CenturyRegister;
    //  Meh.

    uint8_t century, lastCentury;

    uint8_t lastSeconds, lastMinutes, lastHours;
    uint8_t lastDay, lastMonth, lastYear;

    //  Note: This uses the "read registers until you get the same values twice
    //  in a row" technique to avoid getting dodgy/inconsistent values due to
    //  RTC updates.

    do
    {
        lastSeconds = RawSeconds;
        lastMinutes = RawMinutes;
        lastHours   = RawHours;
        lastDay     = RawDay;
        lastMonth   = RawMonth;
        lastYear    = RawYear;
        lastCentury = RawCentury;

        while (ReadUpdateInProgressFlag());
        //  Again.

        RawSeconds = ReadRegister(CmosRegisters::RtcSeconds);
        RawMinutes = ReadRegister(CmosRegisters::RtcMinutes);
        RawHours   = ReadRegister(CmosRegisters::RtcHours);
        RawDay     = ReadRegister(CmosRegisters::RtcDay);
        RawMonth   = ReadRegister(CmosRegisters::RtcMonth);
        RawYear    = ReadRegister(CmosRegisters::RtcYear);

        if (century_register != 0)
            RawCentury = ReadRegister(century_register);

    } while ((lastSeconds != RawSeconds) || (lastMinutes != RawMinutes)
          || (lastHours   != RawHours  ) || (lastDay     != RawDay    )
          || (lastMonth   != RawMonth  ) || (lastYear    != RawYear   )
          || (lastCentury != RawCentury));

    uint8_t statusB = ReadRegister(CmosRegisters::StatusB);

    //  Convert BCD to binary values, if necessary.

    if (0 == (statusB & 0x04))
    {
        Seconds =  (RawSeconds & 0x0F) +  ((RawSeconds       >> 4) * 10);
        Minutes =  (RawMinutes & 0x0F) +  ((RawMinutes       >> 4) * 10);
        Hours   = ((RawHours   & 0x0F) + (((RawHours & 0x70) >> 4) * 10)) | (RawHours & 0x80);
        Day     =  (RawDay     & 0x0F) +  ((RawDay           >> 4) * 10);
        Month   =  (RawMonth   & 0x0F) +  ((RawMonth         >> 4) * 10);
        Year    =  (RawYear    & 0x0F) +  ((RawYear          >> 4) * 10);

        if (century_register != 0)
            century = (RawCentury & 0x0F) + ((RawCentury >> 4) * 10);
    }

    //  Convert 12-hour value to 24-hour value. You know, the kind that sane people use.

    if (0 == (statusB & 0x02) && (Hours & 0x80))
        Hours = ((Hours & 0x7F) + 12) % 24;

    //  Attempt to compute year.

    if (century_register != 0)
        Year += century * 100;
    else
    {
        Year += CURRENT_CENTURY;

        if (Year < CURRENT_YEAR)
            Year += 100;
    }
}
