#include "main.h"

#define PVDD_ADC_CHANNEL        ADC_CHANNEL_1   // 根据实际硬件修改对应的 PVDD ADC 通道
#define PVDD_DROP_THRESHOLD_MV  15000UL            // PVDD电压低于15V视为掉电
#define HighSide_Resistor 100000UL //100K
#define LowSide_Resistor 10000UL //10K

volatile uint32_t pvdd_mv = 240000UL;               // 全局 PVDD 电压 (单位: mV)

#define FACTORY_VREFINT_ADDR    (0x1FFF0020UL)
/* 厂商校准值读不到时使用的典型值(mV) */
#define VREFINT_TYPICAL_MV      (1200U)
/* VrefInt 内部通路使能后的稳定时间(us): 与 LL_ADC_DELAY_VREFINT_STAB_US / 手册 TS_vrefint 一致 */
#define VREFINT_STAB_DELAY_US   (10U)
/* 温度传感器内部通路稳定时间(us): 与 LL_ADC_DELAY_TEMPSENSOR_STAB_US 一致 */
#define TEMPSENSOR_STAB_DELAY_US (10U)
/* 12bit 满量程码值 */
#define ADC_FULL_SCALE          (4095U)

/* 微秒级软件延时(写法与 HAL 内部 ADC_Enable()/温度传感器延时一致) */
static void ADC_Delay_Us(uint32_t Delay_Us)
{
    __IO uint32_t wait_loop_index = (Delay_Us * (SystemCoreClock / 1000000U));
    while (wait_loop_index != 0U) {
        wait_loop_index--;
    }
}
static uint16_t Hex_Literal_To_Dec(uint16_t hex_val)
{
    /*Puya将厂商校准值的十进制按位填入16进制位，所以需要转换*/
    uint16_t dec_val = 0;
    uint16_t multiplier = 1;

    // 循环提取千位、百位、十位、个位 (按 16 进制位 0x000F 提取)
    while (hex_val > 0) {
        uint8_t digit = hex_val & 0x000F; // 取出最低的 4 位 (一个 HEX 位)
        if (digit > 9) {
            // 防呆校验：如果某一位超过了 9 (出现了 A-F)，说明数据不合法，返回 0
            return 0; 
        }
        dec_val += digit * multiplier;
        multiplier *= 10;
        hex_val >>= 4; // 右移 4 位，准备处理下一个十六进制位
    }

    return dec_val;
}
// 2. 读取 1.2V Vrefint 校准值函数
static uint16_t Get_Factory_Vrefint_Value(void)
{
    // 1. 读取地址 0x1FFF0020 处的 32 位原始数据
    uint32_t reg_val = *(__IO uint32_t *)FACTORY_VREFINT_ADDR;
    
    // 2. 取出高 16 位
    uint16_t raw_hex_literal = (uint16_t)(reg_val >> 16);
    
    // 3. 将读取到的十六进制字面量按位转换为十进制数值
    uint16_t vref_cal_value = Hex_Literal_To_Dec(raw_hex_literal);
    
    return vref_cal_value;
}
/**
 * @brief  转换一次指定 ADC 通道, 返回 12bit 码值(0 表示通道非法或转换失败)
 * @param  channel: ADC_CHANNEL_0 ~ ADC_CHANNEL_10 (CH8=温度, CH9=VrefInt, CH10=1/3VCCA)
 * @note   每次调用都会把转换序列改写成"只有本次要转换的通道",
 *         不能用 HAL_ADC_ConfigChannel() 的累加行为来完成通道切换:
 *         它内部只做 CHSELR |= 通道, 从不清理其它通道, 于是 IN0 与 VrefInt 会一直同时在序列里,
 *         而一次 ADSTART 是从序号最小的通道(IN0)开始扫描, HAL_ADC_PollForConversion()
 *         又在第一个 EOC 就返回, 因此无论传哪个通道, 读到的都是 IN0 的电压。
 */
uint16_t Get_ADC_Code(uint16_t channel){
    static uint16_t Last_Channel = 0xFFFFU;//上一次转换的通道
    uint32_t ADC_Temp = 0U;
    uint8_t  i = 0U;

    // 1. 通道合法性检查: PY32F002B 只有 CH0~CH10
    if (channel > ADC_CHANNEL_10) {
        return 0; // 通道非法
    }

    // 2. 重写转换序列, 只保留本次要转换的通道(直接写 CHSELR 即可, 此时 ADC 处于停止状态)
    ADC1->CHSELR = ADC_CHSELR_CHANNEL(channel);

    // 3. 内部通道(VrefInt/温度传感器)的内部通路需要单独使能, 使能后要等参考稳定
    if (channel == ADC_CHANNEL_VREFINT) {
        ADC->CCR |= ADC_CCR_VREFEN;
        if (Last_Channel != channel) {
            ADC_Delay_Us(VREFINT_STAB_DELAY_US);
        }
    }
    else if (channel == ADC_CHANNEL_TEMPSENSOR) {
        ADC->CCR |= ADC_CCR_TSEN;
        if (Last_Channel != channel) {
            ADC_Delay_Us(TEMPSENSOR_STAB_DELAY_US);
        }
    }

    // 4. 转换两次: 第一次丢弃(切换通道后采样网络要重新建立), 第二次才是有效值
    //    采样时间由 hadc1.Init.SamplingTimeCommon 统一决定, 见 Studio_ADC1_Init()
    for (i = 0U; i < 2U; i++) {
        if (HAL_ADC_Start(&hadc1) != HAL_OK) {
            return 0; // 启动失败
        }
        if (HAL_ADC_PollForConversion(&hadc1, 10) != HAL_OK) {//超时10ms
            HAL_ADC_Stop(&hadc1);
            return 0; // 转换超时
        }
        ADC_Temp = HAL_ADC_GetValue(&hadc1);
        HAL_ADC_Stop(&hadc1);
    }

    Last_Channel = channel;
    return (uint16_t)ADC_Temp;
    
}
// 3. 用 VrefInt 反推 VCCA(ADC 参考电压)的实际毫伏值
//    原理: (VrefInt厂商校准毫伏值 / VCCA) = (VrefInt的ADC码值 / 4095)
//          即 VCCA = VrefInt毫伏值 * 4095 / VrefInt码值
uint32_t Get_VCCA_mV(void)
{
    uint32_t vref_mV   = 0U;
    uint32_t vref_code = 0U;
    uint8_t  i = 0U;

    // 1. 厂商校准值(mV), 读不到就按典型值 1200mV
    vref_mV = Get_Factory_Vrefint_Value();
    if (vref_mV == 0U) {
        vref_mV = VREFINT_TYPICAL_MV;
    }

    // 2. 多次采样求平均, 抑制单次噪声(内部参考本身很稳, 主要滤掉数字噪声)
    for (i = 0U; i < 8U; i++) {
        vref_code += Get_ADC_Code(ADC_CHANNEL_VREFINT);
    }
    vref_code /= 8U;

    if (vref_code == 0U) {
        return 0; // 转换失败
    }

    return (vref_mV * ADC_FULL_SCALE) / vref_code;
}
// 4. 码值 -> 毫伏值: Vx = 码值 * VCCA / 4095
uint32_t Get_ADC_mV(uint16_t channel)
{
    uint32_t vcca_mV  = 0U;
    uint32_t adc_code = 0U;
    uint8_t  i = 0U;

    vcca_mV = Get_VCCA_mV();
    if (vcca_mV == 0U) {
        return 0;
    }

    for (i = 0U; i < 4U; i++) {
        adc_code += Get_ADC_Code(channel);
    }
    adc_code /= 4U;

    // 最大 4095 * 3300 = 13513500, 不会溢出 uint32
    return (adc_code * vcca_mV) / ADC_FULL_SCALE;
}
/* 分压还原比例: PVDD = 引脚电压 * (上/下)
 * (100K + 10K) / 10K = 11 倍 */
#define PVDD_DIV_RATIO_NUM   (HighSide_Resistor + LowSide_Resistor)
#define PVDD_DIV_RATIO_DEN   (LowSide_Resistor)

/**
 * @brief  读取 PVDD 电源电压 (mV), 结果同步更新到全局 pvdd_mv
 * @note   PVDD 经 100K/10K 分压后接入 PVDD_ADC_CHANNEL。
 *         100K 上拉电阻源阻抗较高, hadc1.Init.SamplingTimeCommon 必须
 *         选足够长的采样时间(建议 239.5 周期), 否则读数会明显偏低。
 * @retval PVDD 电压 mV; ADC 转换失败时返回 0, pvdd_mv 保持上一次有效值
 */
uint32_t Get_PVDD_mV(void)
{
    uint32_t pin_mv = 0U;
    uint32_t pvdd   = 0U;

    /* 内部已完成 4 次采样平均, 并用 VrefInt 反推实际 VCCA 修正参考误差 */
    pin_mv = Get_ADC_mV(PVDD_ADC_CHANNEL);
    if (pin_mv == 0U) {
        return 0U;              /* 转换失败: 不覆盖上次有效结果 */
    }

    /* 分压还原: pin_mv 最大约 3300 * 110000 = 3.63e8, uint32 不会溢出 */
    pvdd = (pin_mv * PVDD_DIV_RATIO_NUM) / PVDD_DIV_RATIO_DEN;

    if (pvdd > 65535U) {        /* 上限保护, 匹配全局 pvdd_mv 的 uint16_t */
        pvdd = 65535U;
    }

    pvdd_mv = (uint16_t)pvdd;
    return pvdd;
}

/**
 * @brief  掉电判断: PVDD 低于 PVDD_DROP_THRESHOLD_MV 视为掉电
 * @retval 1 = 已掉电, 0 = 正常 (尚未成功读过电压时也返回 0)
 */
bool PVDD_Is_PowerDrop(void)
{
    return ((pvdd_mv != 0U) && (pvdd_mv < PVDD_DROP_THRESHOLD_MV)) ? 1U : 0U;
}
