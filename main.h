#include "stm32f303x8.h"
#include <string.h>

/* Déclarations */
void startup(void);
void initClock(void);
void initTIM17(void);
void msDelay(uint16_t delay);
void usDelay(uint16_t delay);
void initU2(void);
void printU2(char* data);
void initVGA(void);
void initTIM2(void);
void initTIM3(void);
void initSPI(void);
void sendSPI(uint16_t data);
void TIM2_IRQHandler(void);

/*	Basic initialisation	*/
void startup(){
	//Basic
	initClock();
	initTIM17();
	initU2();
	initVGA();
	msDelay(500);
	printU2("Setup complete \n\r");
}

/* Maxing out the device clock */
void initClock(){
	// cpu 72MHz
	FLASH->ACR |= FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY_1;
	// APB1 (low)		: 36MHz
	// APB2 (high)	: 72MHz
	RCC->CR |= (RCC_CR_HSEBYP | RCC_CR_HSEON);
	while(!(RCC->CR & RCC_CR_HSERDY)){}
	RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;
	//HSE->PLL : 72 MHz
	RCC->CFGR |= (RCC_CFGR_PLLMUL12);	
	RCC->CFGR |= RCC_CFGR_PLLSRC_HSE_PREDIV;		
	RCC->CR |= RCC_CR_PLLON;		
	while(!(RCC->CR & RCC_CR_PLLRDY));
	RCC->CFGR |= RCC_CFGR_SW_PLL;		
}

/* Initialisation of TIM17 as base for msDelay and usDelay */
void initTIM17(){
	//for msDelay and usDelay
	RCC->APB2ENR |= RCC_APB2ENR_TIM17EN;
	TIM17->CR1 |= TIM_CR1_ARPE | TIM_CR1_OPM;
}

void msDelay(uint16_t delay){
	TIM17->CR1 &= ~TIM_CR1_CEN;
	TIM17->SR =0U;
	TIM17->CNT =0U;
	TIM17->PSC = 7199U;
	TIM17->ARR = delay*10U-1U;
	TIM17->CR1 |= TIM_CR1_CEN;
	while(!(TIM17->SR & TIM_SR_UIF)){}
}

void usDelay(uint16_t delay){
	TIM17->CR1 &= ~TIM_CR1_CEN;
	TIM17->SR =0U;
	TIM17->CNT =0U;
	TIM17->PSC = 71U;
	TIM17->ARR = delay-1U;
	TIM17->CR1 |= TIM_CR1_CEN;
	while(!(TIM17->SR & TIM_SR_UIF)){}
}

/* Initialisation of USART2 for printing function debugging */
void initU2(){
	//GPIOs
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
	GPIOA->MODER |= GPIO_MODER_MODER2_1 | GPIO_MODER_MODER15_1;
	GPIOA->AFR[0] |= 7U<<GPIO_AFRL_AFRL2_Pos;
	GPIOA->AFR[1] |= 7U<<GPIO_AFRH_AFRH7_Pos;
	//Usart @ 115200
	RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
	USART2->BRR = 312U;
	USART2->CR1 |= USART_CR1_TE | USART_CR1_UE; //USART_CR1_RE
}

void printU2(char* data){
	int i;
	int size = (int) strlen(data);
	for(i=0;i<size;i++){
		while(!(USART2->ISR & USART_ISR_TXE)){}
		USART2->TDR = data[i];
	}
}

/* VGA */
void initVGA(){
	//Timers initialisation
	initTIM2();
	initTIM3();
	//SPI initialisation
	initSPI();
	//Timers enable
	TIM3->CR1 |= TIM_CR1_CEN;
	TIM2->CR1 |= TIM_CR1_CEN;
}

void initTIM2(){
	//GPIO
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
	GPIOA->AFR[0] |= (1U<<GPIO_AFRL_AFRL0_Pos)|(1U<<GPIO_AFRL_AFRL1_Pos);
	GPIOA->MODER |= GPIO_MODER_MODER0_1|GPIO_MODER_MODER1_1;
	GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR0_0|GPIO_OSPEEDER_OSPEEDR0_1|GPIO_OSPEEDER_OSPEEDR1_0|GPIO_OSPEEDER_OSPEEDR1_1;
	//RCC
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
	TIM2->CNT = 0U;
	TIM2->PSC = 1U;
	TIM2->ARR = 831U;
	TIM2->CR1 |= TIM_CR1_ARPE|TIM_CR1_URS;
	//PWM outputs
	TIM2->CCR1 = 752U;
	TIM2->CCMR1 |= TIM_CCMR1_OC1PE | (6U<<TIM_CCMR1_OC1M_Pos);
	TIM2->CCR2 = 697U;
	TIM2->CCMR1 |= TIM_CCMR1_OC2PE | (7U<<TIM_CCMR1_OC2M_Pos);
	TIM2->CCER |= TIM_CCER_CC1E|TIM_CCER_CC2E;
	TIM2->EGR |= TIM_EGR_UG;
	//Master
	TIM2->CR2 |= TIM_CR2_MMS_1;
	//Interrupt
	TIM2->DIER |= TIM_DIER_UIE;
	NVIC_EnableIRQ(TIM2_IRQn);
}

void initTIM3(){
	//GPIO
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
	GPIOA->AFR[0] |= (2U<<GPIO_AFRL_AFRL6_Pos)|(2U<<GPIO_AFRL_AFRL7_Pos);
	GPIOA->MODER |= GPIO_MODER_MODER6_1|GPIO_MODER_MODER7_1;
	GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR6_0|GPIO_OSPEEDER_OSPEEDR6_1|GPIO_OSPEEDER_OSPEEDR7_0|GPIO_OSPEEDER_OSPEEDR7_1;
	//RCC
	RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
	TIM3->CNT = 0U;
	TIM3->PSC = 0U;
	TIM3->ARR = 508U;
	TIM3->CR1 |= TIM_CR1_ARPE|TIM_CR1_URS;
	//PWM outputs
	TIM3->CCR1 = (483U);
	TIM3->CCMR1 |= TIM_CCMR1_OC1PE | (6U<<TIM_CCMR1_OC1M_Pos);
	TIM3->CCR2 = (509U-28U);
	TIM3->CCMR1 |= TIM_CCMR1_OC2PE | (7U<<TIM_CCMR1_OC2M_Pos);
	TIM3->CCER |= TIM_CCER_CC1E|TIM_CCER_CC2E;
	TIM3->EGR |= TIM_EGR_UG;
	//Slave
	TIM3->SMCR |= TIM_SMCR_TS_0|(7U<<TIM_SMCR_SMS_Pos);
}

void initSPI(){//9MHz transfers
	//GPIO
	RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
	GPIOB->AFR[0] |= (5U<<GPIO_AFRL_AFRL5_Pos);
	GPIOB->MODER |= GPIO_MODER_MODER5_1;
	GPIOB->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR5_0|GPIO_OSPEEDER_OSPEEDR5_1;
	//SPI
	RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
	SPI1->CR1 = 0xC304U;
	SPI1->CR1 |= (1U<<SPI_CR1_BR_Pos);//|(SPI_CR2_NSSP);
	SPI1->CR2 = 0x0F04U;
	//SPI1->CR1 |= SPI_CR1_SPE;//Enable SPI1
	//DMA
	//RCC->AHBENR |= RCC_AHBENR_DMA1EN;
	//...
	SPI1->CR1 |= SPI_CR1_SPE;
}

void sendSPI(uint16_t data){
	//SPI1->CR1 |= SPI_CR1_SPE;
	//while(!(SPI1->SR & SPI_SR_TXE)){}
	SPI1->DR = data;
	while(!(SPI1->SR & SPI_SR_TXE)){}
	//while(!(SPI1->SR & SPI_SR_BSY)){}
	//SPI1->DR = data;
	//SPI1->CR1 &= ~SPI_CR1_SPE;
}
