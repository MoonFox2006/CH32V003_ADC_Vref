#include "debug.h"

static uint16_t vref;

static void Init_PVD() {
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    PWR_PVDLevelConfig(PWR_PVDLevel_4V4);
    PWR_PVDCmd(ENABLE);
}

static void Init_ADC(void) {
    GPIO_InitTypeDef GPIO_InitStructure = { 0 };
    ADC_InitTypeDef  ADC_InitStructure = { 0 };

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_GPIOC, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div8);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    ADC_DeInit(ADC1);
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &ADC_InitStructure);

    ADC_Calibration_Vol(ADC1, ADC_CALVOL_50PERCENT);
    ADC_Cmd(ADC1, ENABLE);

    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1)) {}
    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1)) {}

    ADC_RegularChannelConfig(ADC1, ADC_Channel_Vrefint, 1, ADC_SampleTime_241Cycles);
    {
        uint32_t sum = 0;

        for (uint8_t i = 0; i <= 10; ++i) {
            ADC_SoftwareStartConvCmd(ADC1, ENABLE);
            while (! ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC)) {}
            if (i) { // Skip first value
                sum += ADC_GetConversionValue(ADC1);
            }
        }
        vref = sum / 10;
    }
}

static uint16_t Get_ADC(uint8_t ch) {
    ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_241Cycles);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    while (! ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC)) {}
    return ADC_GetConversionValue(ADC1);
}

int main(void) {
    uint16_t vcc;

    SystemCoreClockUpdate();

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

    Init_PVD();
    Delay_Init();
    USART_Printf_Init(115200);

    Delay_Ms(1000);

    if (PWR_GetFlagStatus(PWR_FLAG_PVDO)) // Below 4.4 V
        vcc = 330; // 3.3 V
    else
        vcc = 500; // 5.0 V
    printf("VCC = %u.%02u V\n", vcc / 100, vcc % 100);

    Init_ADC();
    printf("ADC initialized, Vref = %u\n", vref);

    while (1) {
        uint16_t adc, v, vr;

        adc = Get_ADC(ADC_Channel_2); // PC4
        v = adc * vcc / 1023;
        vr = adc * 120 / vref;
        printf("ADC = %u, %u.%02u V (%u.%02u V unref)\n", adc, vr / 100, vr % 100, v / 100, v % 100);
        Delay_Ms(500);
    }
}
