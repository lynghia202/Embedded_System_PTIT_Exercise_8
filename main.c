#include "stm32f10x.h"
#include <stdio.h>

static void delay_ms(uint32_t ms){
    SysTick->LOAD = SystemCoreClock/1000 - 1;
    SysTick->VAL  = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
    for(uint32_t i=0;i<ms;i++){
        while((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)==0);
    }
    SysTick->CTRL = 0;
}

static void USART1_Init(void){
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);
    GPIO_InitTypeDef gpio;
	
    // PA9  = TX (AF push-pull)
    gpio.GPIO_Pin   = GPIO_Pin_9;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &gpio);

    // PA10 = RX (floating input)
    gpio.GPIO_Pin   = GPIO_Pin_10;
    gpio.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);

    USART_InitTypeDef us;
    us.USART_BaudRate            = 9600;
    us.USART_WordLength          = USART_WordLength_8b;
    us.USART_StopBits            = USART_StopBits_1;
    us.USART_Parity              = USART_Parity_No;
    us.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    us.USART_Mode                = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART1, &us);
    USART_Cmd(USART1, ENABLE);
}

/* printf redirection -> USART1 */
int fputc(int ch, FILE *f){
		(void)f;
    while((USART1->SR & USART_SR_TXE)==0);
    USART1->DR = (uint16_t)ch;
    return ch;
}

static void ADC1_Init_CH0(void){
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1, ENABLE);

    // PA0 analog input
    GPIO_InitTypeDef gpio;
    gpio.GPIO_Pin  = GPIO_Pin_0;
    gpio.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &gpio);

    RCC_ADCCLKConfig(RCC_PCLK2_Div6);

    ADC_InitTypeDef adc;
    adc.ADC_Mode               = ADC_Mode_Independent;
    adc.ADC_ScanConvMode       = DISABLE;
    adc.ADC_ContinuousConvMode = DISABLE;          // đo t?ng l?n trong v?ng l?p
    adc.ADC_ExternalTrigConv   = ADC_ExternalTrigConv_None;
    adc.ADC_DataAlign          = ADC_DataAlign_Right;
    adc.ADC_NbrOfChannel       = 1;
    ADC_Init(ADC1, &adc);

    ADC_Cmd(ADC1, ENABLE);

    // Calibrate
    ADC_ResetCalibration(ADC1);
    while(ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1));
}

static uint16_t ADC1_Read_CH0(void){
    ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_239Cycles5);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
    return ADC_GetConversionValue(ADC1);
}

int main(void){
    SystemInit();
    USART1_Init();
    ADC1_Init_CH0();

    printf("\r\n--- LDR ADC demo (PA0, Vref=3.3V) ---\r\n");

    while(1){
        uint16_t raw = ADC1_Read_CH0();            // 0..4095
        float voltage = (3.3f * raw) / 4095.0f;    // Vref = 3.3V
        float percent = ((4095.0f - raw) * 100.0f) / 4095.0f;
        printf("RAW=%4u  V=%.3f V  Light=%.1f %%\r\n", raw, voltage, percent);
        delay_ms(200);
    }
}
