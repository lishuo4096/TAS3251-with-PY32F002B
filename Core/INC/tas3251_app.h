#ifndef __TAS3251_APP_H__
#define __TAS3251_APP_H__

#include "main.h"

#define Status_Normal 0
#define Status_Warning 1
#define Status_Fault 2

#define Format_I2S 0x00
#define Format_DSP 0x10
#define Format_RTJ 0x20
#define Format_LTJ 0x30

#define Word_Lengh_16bit 0x00
#define Word_Lengh_20bit 0x01
#define Word_Lengh_24bit 0x02
#define Word_Lengh_32bit 0x03

#define Channel_16bit 16
#define Channel_32bit 32

#define TDM_DISABLE 0
#define TDM_MODE_4 1
#define TDM_MODE_8 2

#define TDM_ROLE_0   0
#define TDM_ROLE_1   1
#define TDM_ROLE_2   2
#define TDM_ROLE_3   3

#define TAS3251_I2C_ADDR 0X4A

/*
数字音量控制寄存器
00000000：+24.0dB
00000001：+23.5 dB
…
00101111：+0.5dB
00110000：0.0dB
00110001：-0.5dB
...
11111110：-103dB
11111111：静音
*/

#define LeftChVol_Reg 0x3D
#define RightChVol_Reg 0x3E
/*
5-4AFMTR/W0
I2S 数据格式：这些位控制 DAC 操作的输入和输出音频接口格式。
00：I2S
01：DSP
10：RTJ
11：LTJ
1ALENR/W1
I2S 字长：这些位控制 DAC 操作所需的输入和输出音频接口采样字长度。
00：16 位
01：20 位
10：24 位
11：32 位
*/
#define InputFormat_Reg 0x28
/*
时钟移位：这些位控制输入和输出的音频帧中音频数据的偏移。偏移定义为从音频帧的
起始 (MSB) 到所需音频采样的起始位置的 SCLK 数。
00000000：偏移 = 0 SCLK（无偏移）
00000001：偏移 = 1 SCLK
00000010：偏移 = 2 SCLK
…
11111111：偏移 = 256 SCLK
I2S TDM模式下专用
*/
#define Clkshift_Reg 0x29

#define DACDataRouting_Reg 0x2A


#endif

