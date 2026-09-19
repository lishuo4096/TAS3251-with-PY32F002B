#include "main.h"
#include "tas3251_app.h"

#define SwapDataLR false
#define Input_Format Format_I2S
#define Channel_Length Channel_32bit
#define Input_Word_Length Word_Lengh_24bit

#define TDM_MODE TDM_MODE_8
#define TDM_ROLE TDM_ROLE_3

#if (TDM_MODE == TDM_DISABLE)
    #define CLOCK_SHIFT_VAL   0

#elif (TDM_MODE == TDM_MODE_4)
    #if (TDM_ROLE == TDM_ROLE_0 || TDM_ROLE == TDM_ROLE_1)
        #define CLOCK_SHIFT_VAL   ((TDM_ROLE * 2) * Channel_Length)
    #else
        #error "TDM4 模式下 TDM_ROLE 仅支持 ROLE_0 与 ROLE_1"
    #endif

#elif (TDM_MODE == TDM_MODE_8)
    #if (TDM_ROLE >= TDM_ROLE_0 && TDM_ROLE <= TDM_ROLE_3)
        #define CLOCK_SHIFT_VAL   ((TDM_ROLE * 2) * Channel_Length)
    #else
        #error "TDM8 模式下 TDM_ROLE 仅支持 ROLE_0 至 ROLE_3"
    #endif

#else
    #error "不支持的 TDM 模式配置"
#endif

uint8_t TAS3251_Status = Status_Normal;

// 单个寄存器读写函数
HAL_StatusTypeDef I2C_WriteReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t value)
{
    return HAL_I2C_Mem_Write(&hi2c1, 
                             (uint16_t)(dev_addr << 1), 
                             (uint16_t)reg_addr, 
                             I2C_MEMADD_SIZE_8BIT, 
                             &value, 
                             1, 
                             10);
}
uint16_t I2C_ReadReg(uint8_t dev_addr, uint8_t reg_addr)
{
    uint8_t temp = 0;
    HAL_I2C_Mem_Read(&hi2c1,
                   (uint16_t)(dev_addr << 1), 
                   (uint16_t)reg_addr, 
                    I2C_MEMADD_SIZE_8BIT, 
                    &temp, 
                    1, 
                    10);
    return temp;
}
/*
w 90 00 00 # Go to page 0
w 90 7f 00 # Go to book 0
w 90 02 00 # Enable device
TAS3251寄存器默认配置就可以跑，会自动识别输入音频时钟并配置内部PLL进行工作
最高支持96KHZ24BIT，默认格式为 I2S格式，左右对齐需要寄存器配置
如需TDM功能，可以写入其他寄存器
*/
void TAS3251_Init(void){
    HAL_GPIO_WritePin(GPIOB,DAC_MUTE_PIN,GPIO_PIN_RESET);//DAC静音
    HAL_GPIO_WritePin(GPIOA,AMP_RESET_PIN,GPIO_PIN_RESET);//功放停止工作
    
    I2C_WriteReg(TAS3251_I2C_ADDR,0x00,0x00);//切换至page0
    I2C_WriteReg(TAS3251_I2C_ADDR,0x7f,0x00);//切换至book0
    I2C_WriteReg(TAS3251_I2C_ADDR,0x02,0x10);//DAC待机模式
    I2C_WriteReg(TAS3251_I2C_ADDR,0x02,0x90);//DSP复位
    HAL_Delay(10);
 
    #if SwapDataLR
    I2C_WriteReg(TAS3251_I2C_ADDR,DACDataRouting_Reg,0x22);
    #else
    //I2C_WriteReg(TAS3251_I2C_ADDR,DACDataRouting_Reg,0x11);//默认配置就是不交换
    #endif
    
    TAS3251_Set_Input_Format(Input_Format,Input_Word_Length);//配置输入格式
    
    #if (TDM_MODE != TDM_DISABLE)
    I2C_WriteReg(TAS3251_I2C_ADDR, Clkshift_Reg, (uint8_t)CLOCK_SHIFT_VAL);
    #endif
    
    I2C_WriteReg(TAS3251_I2C_ADDR,0x08,0X10);//使能PCM到TPA MUTE控制(我也不知道干什么用的)
    I2C_WriteReg(TAS3251_I2C_ADDR,LeftChVol_Reg,0X30);//左声道数字音量0db
    I2C_WriteReg(TAS3251_I2C_ADDR,RightChVol_Reg,0X30);//右声道数字音量0db
    I2C_WriteReg(TAS3251_I2C_ADDR,0x02,0x00);//DSP开始上电并发送数据
    TAS3251_Set_Vol_dB(0);
    HAL_GPIO_WritePin(GPIOB,DAC_MUTE_PIN,GPIO_PIN_SET);//DAC取消静音
    HAL_GPIO_WritePin(GPIOA,AMP_RESET_PIN,GPIO_PIN_SET);//功放工作
    
    
}

void TAS3251_Set_Vol_dB(int8_t Volume){//输入为db
    //db转二进制
    uint8_t temp = 0;
        if (Volume == -128) {
        temp = 0xFF;                // 静音
    } else if (Volume >= 24) {
        temp = 0x00;                // 上限 +24dB
    } else if (Volume <= -103) {
        temp = 0xFE;                // 下限 -103dB
    } else {
        // reg = 48 - Volume*2
        // 注意：Volume 为负时，48 - 2*Volume 会大于 48，正好落在 -x dB 区间
        temp = (uint8_t)(48 - (Volume * 2));
    }
    I2C_WriteReg(TAS3251_I2C_ADDR,LeftChVol_Reg,temp);
    I2C_WriteReg(TAS3251_I2C_ADDR,RightChVol_Reg,temp);
}


void TAS3251_Set_Input_Format(uint8_t format, uint8_t word_len)
{
    uint8_t temp;
    temp = (format & 0x30) | (word_len & 0x03);
    I2C_WriteReg(TAS3251_I2C_ADDR, InputFormat_Reg, temp);
}

void AMP_Fault_Handler(void){
    //TAS3251报错为低电平有效
    if(!HAL_GPIO_ReadPin(GPIOA,AMP_FAULT_PIN)){
        TAS3251_Status = Status_Fault;
        HAL_GPIO_WritePin(GPIOB,DAC_MUTE_PIN,GPIO_PIN_RESET);//DAC静音
        HAL_GPIO_WritePin(GPIOA,AMP_RESET_PIN,GPIO_PIN_RESET);//关闭功放
        WS2812_Send_Data(1,127,0,0);//红色
        HAL_Delay(5000);//延时5000ms，如果结束后还是报错，那就再循环
        HAL_GPIO_WritePin(GPIOB,DAC_MUTE_PIN,GPIO_PIN_SET);//DAC播放
        HAL_GPIO_WritePin(GPIOA,AMP_RESET_PIN,GPIO_PIN_SET);//启动功放
    }
    else if((TAS3251_Status != Status_Fault) && ((!HAL_GPIO_ReadPin(GPIOA,AMP_CLIP_PIN)) || PVDD_Is_PowerDrop())){
        TAS3251_Status = Status_Warning;
    }
    else {
        TAS3251_Status = Status_Normal;
    }
    
    if(TAS3251_Status == Status_Normal){
        WS2812_Send_Data(1,0,32,0);//显示绿色
        HAL_GPIO_WritePin(GPIOB,DAC_MUTE_PIN,GPIO_PIN_SET);//DAC播放
        HAL_GPIO_WritePin(GPIOA,AMP_RESET_PIN,GPIO_PIN_SET);//启动功放
    }
    else if(TAS3251_Status == Status_Warning){
        WS2812_Send_Data(1,127,64,0);//显示黄色
    }
    else{
        WS2812_Send_Data(1,127,0,0);
    }
    if(PVDD_Is_PowerDrop()){
        HAL_GPIO_WritePin(GPIOB,DAC_MUTE_PIN,GPIO_PIN_RESET);//DAC静音
        HAL_GPIO_WritePin(GPIOA,AMP_RESET_PIN,GPIO_PIN_RESET);//关闭功放
    }
}

