/*
MIT License

Copyright (c) 2021 M5Stack

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#ifndef __M5_AD9833_H__
#define __M5_AD9833_H__

#include "Wire.h"
#include "Arduino.h"

#define DDS_UNIT_I2CADDR 0x31

#define DDS_DESC_ADDR   0x10
#define DDS_MODE_ADDE   0x20
#define DDS_CTRL_ADDR   0x21
#define DDS_FREQ_ADDR   0x30
#define DDS_PHASE_ADDR  0x34

#define DDS_FMCLK 10000000

class DDSUnit
{
private:
    TwoWire *_pwire = nullptr; 
    void writeDDSReg(uint8_t addr, uint8_t data);
    void writeDDSReg(uint8_t addr, uint8_t* data, size_t size );
    uint8_t readDDSReg(uint8_t addr);
    void readDDSRegs(uint8_t addr,uint8_t* dataptr,uint8_t size);
public:

    enum DDSmode{
        kReservedMode = 0,
        kSINUSMode,
        kTRIANGLEMode,
        kSQUAREMode,
        kSAWTOOTHMode,//锯齿波不能调节频率相位，固定1.3k hz 0相位
        kDCMode,
    };

    DDSUnit(){};
    ~DDSUnit(){};
    int begin( TwoWire *p = &Wire1);

    void setFreq( uint8_t reg ,uint64_t freq );
    void setPhase( uint8_t reg ,uint32_t phase );
    void setFreqAndPhase( uint8_t freg, uint64_t freq, uint8_t preg, uint32_t phase );
    void setMode(DDSmode mode);
    void setCTRL(uint8_t ctrlbyte);

    void selectFreqReg(uint8_t num);
    void selectPhaseReg(uint8_t num);

    void quickOUT(DDSmode mode, uint64_t freq, uint32_t phase);//默认0
    void OUT(uint8_t freqnum,uint8_t phasenum);

    void setSleep(uint8_t level);//1/2 1模式不输出保持最后一帧电平，2关闭时钟
    void reset();
};

#endif