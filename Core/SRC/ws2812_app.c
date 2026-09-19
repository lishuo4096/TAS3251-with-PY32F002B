#include "main.h"

#define logic_high true
#define logic_low false

#define WS2812_PORT      GPIOB//PA5为WS2812 DATA引脚
#define WS2812_PIN      WS2812_DATA_PIN

// 快速 GPIO 高/低电平设置（直接写 BSRR 寄存器，仅需 1~2 个周期）
#define WS2812_HIGH()    (WS2812_PORT->BSRR = WS2812_PIN)
#define WS2812_LOW()     (WS2812_PORT->BRR  = WS2812_PIN)
#define WS2812_Lightup() WS2812_LOW()
// __NOP() 延时宏定义
#define NOP1()   __NOP()
#define NOP2()   __NOP(); __NOP()
#define NOP4()   NOP2(); NOP2()
#define NOP8()   NOP4(); NOP4()

static inline void WS2812_Send_Bit(bool bit)
{
    if (bit) 
    {
        // ----- 发送 1 码 -----
        WS2812_HIGH();
        // 高电平维持约 800ns (24MHz 下需要约 19 个周期)
        NOP8(); NOP8(); NOP2(); 
        WS2812_LOW();
        // 低电平维持约 450ns
        NOP8(); NOP2();
    } 
    else 
    {
        // ----- 发送 0 码 -----
        WS2812_HIGH();
        // 高电平维持约 300ns (24MHz 下需要约 7 个周期)
        NOP4(); NOP2();
        WS2812_LOW();
        // 低电平维持约 850ns
        NOP8(); NOP8(); NOP2();
    }
}

static const uint8_t POW_Table[256] = {//255 * pow(k,1.8) k∈0~1的结果
    0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,2,
    2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,6,
    6,6,7,7,8,8,8,9,9,10,10,10,11,11,12,12,
    13,13,14,14,15,15,16,16,17,17,18,18,19,19,20,21,
    21,22,22,23,24,24,25,26,26,27,28,28,29,30,30,31,
    32,32,33,34,35,35,36,37,38,38,39,40,41,41,42,43,
    44,45,46,46,47,48,49,50,51,52,53,53,54,55,56,57,
    58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,
    74,75,76,77,78,79,80,81,82,83,84,86,87,88,89,90,
    91,92,93,95,96,97,98,99,100,102,103,104,105,107,108,109,
    110,111,113,114,115,116,118,119,120,122,123,124,126,127,128,129,
    131,132,134,135,136,138,139,140,142,143,145,146,147,149,150,152,
    153,154,156,157,159,160,162,163,165,166,168,169,171,172,174,175,
    177,178,180,181,183,184,186,188,189,191,192,194,195,197,199,200,
    202,204,205,207,208,210,212,213,215,217,218,220,222,224,225,227,
    229,230,232,234,236,237,239,241,243,244,246,248,250,251,253,255
};
void WS2812_Send_Data(uint8_t WS2812_Num,uint8_t Red,uint8_t Green,uint8_t Blue){
    //发送数据顺序为GRB
    uint8_t i,j;
    uint8_t dat;
    __disable_irq();
    for(j = 0;j < WS2812_Num;j++){
    //Green
    dat = Green;
    for(i = 0; i < 8; i++){
        if(dat & 0x80) WS2812_Send_Bit(logic_high); // 提取最高位，判断发1码还是0码
        else           WS2812_Send_Bit(logic_low);
        dat <<= 1; // 左移一位，准备发送下一位
    }
    //Red
    dat = Red;
    for(i = 0; i < 8; i++){
        if(dat & 0x80) WS2812_Send_Bit(logic_high);
        else           WS2812_Send_Bit(logic_low);
        dat <<= 1;
    }
    //Blue
    dat = Blue;
    for(i = 0; i < 8; i++){
        if(dat & 0x80) WS2812_Send_Bit(logic_high);
        else           WS2812_Send_Bit(logic_low);
        dat <<= 1;
    }
    }
    __enable_irq();
    WS2812_LOW();
    for (uint32_t delay = 0; delay < 1000; delay++) {//延时80us左右，点亮灯珠
        __NOP();
    }
}
void HSV2RGB(uint8_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b) {
    uint8_t region,p,q,t;
    uint16_t remainder;
    if (s == 0) { // 灰度
        *r = *g = *b = v;
        return;
    }
    region = h / 43; // 将 H 分为 6 个区域，每个区域约 43 度
    remainder = (h - (region * 43)) * 6; // 计算当前区域内的偏移量
    p = (v * (255 - s)) >> 8;
    q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;
    switch (region) {
        case 0: *r = v; *g = t; *b = p; break;
        case 1: *r = q; *g = v; *b = p; break;
        case 2: *r = p; *g = v; *b = t; break;
        case 3: *r = p; *g = q; *b = v; break;
        case 4: *r = t; *g = p; *b = v; break;
        default: *r = v; *g = p; *b = q; break;
    }
}
