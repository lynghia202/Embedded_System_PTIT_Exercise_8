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

    RCC_ADCCLKConfig(RCC_PCLK2_Div6);//Cau hinh Clock cho ADC: Chia tan so Clock APB2 (PCLK2, thuong là 72MHz) cho 6.
									// f_ADC: 72MHz / 6 = 12 MHz (Dam bao duoi gioi han 14MHz).

    ADC_InitTypeDef adc;
    adc.ADC_Mode               = ADC_Mode_Independent; // Che do hoat dong: ADC1 hoat dong doc lap (Khong ghep doi voi ADC khac).
    adc.ADC_ScanConvMode       = DISABLE; // Che do Quet: Tat. Chi doc 1 kenh duy nhat (PA0).
    adc.ADC_ContinuousConvMode = DISABLE;  // Che do Chuyen doi Lien tuc: Tat. ADC k tu dong bat dau chuyen doi moi ngay sau khi xong.        
    adc.ADC_ExternalTrigConv   = ADC_ExternalTrigConv_None; // Kich hoat ben ngoai: KHONG su dung su kien Timer hay chan ngoai de bat dau chuyen doi.
    adc.ADC_DataAlign          = ADC_DataAlign_Right; // Canh chinh Du lieu: Gia tri RAW 12-bit duoc luu o ben phai (LSB) cua thanh ghi 16-bit.
    adc.ADC_NbrOfChannel       = 1; // So luong Kenh: Dat la 1 vi ta chi doc kenh 0 (PA0).

    ADC_Init(ADC1, &adc); // Khoi tao ADC1 voi cac tham so da cau hinh.

    ADC_Cmd(ADC1, ENABLE);

    // Hieu chinh
    ADC_ResetCalibration(ADC1); // Reset lai gia tri hieu chinh cu.
    while(ADC_GetResetCalibrationStatus(ADC1)); // Cho doi qua trinh Reset hieu chinh hoan tat.
    ADC_StartCalibration(ADC1); // Bat dau qua trinh hieu chinh tu dong.
    while(ADC_GetCalibrationStatus(ADC1)); // Cho doi qua trinh hieu chinh moi hoan tat (dam bao do chinh xac).
}

static uint16_t ADC1_Read_CH0(void){
    // Cau hinh kenh ADC (ADC_Channel_0) va thoi gian giu mau (Sample Time).
    ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_239Cycles5); 

    // Ra lenh cho ADC bat dau chuyen doi (do dien ap) bang phan mem.
    ADC_SoftwareStartConvCmd(ADC1, ENABLE); 

    // Ky thuat Polling: CPU dung lai o day va LIEN TUC kiem tra 
    // cho den khi co EOC (End Of Conversion - Hoan tat do) duoc bat len.
    while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET); 

    // Doc gia tri RAW (0-4095) tu thanh ghi cua ADC.
    // Lenh nay tu dong xoa co EOC.
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
