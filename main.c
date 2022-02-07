#include "main.h"
static uint16_t line1[20]={0xAAAA,0x0U,0xAAAA,0x0U,0xAAAA,0x0U,0xAAAA,0x0U,0xAAAA,0x0U,0xFFFF,0x0U,0xFFFF,0x0U,0xFFFF,0x0U,0xFFFF,0x0U,0xFFFF,0x0U};
static uint16_t line2[20]={0U,0xAAAA,0x0U,0xAAAA,0x0U,0xAAAA,0x0U,0xAAAA,0x0U,0xAAAA,0x0U,0xFFFF,0x0U,0xFFFF,0x0U,0xFFFF,0x0U,0xFFFF,0x0U,0xFFFF};
static uint16_t blankLine[20]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
void printLine(uint16_t data[20]);

int main(){
	startup();	
	while(1){
		
	}
}

void printLine(uint16_t data[20]){
	for(int i=0;i<20;i++){
			sendSPI(data[i]);
		}
	for(int i=20;i<26;i++){
			sendSPI(0U);
		}
}

void TIM2_IRQHandler(){
	static int lineCounter=0;
	
	if(TIM2->DIER & TIM_SR_UIF){
	if(lineCounter<480){
		if(lineCounter&1){
			printLine(line1);
		}
		else{
			printLine(line2);
		}
		
	}
	else{
		printLine(blankLine);
	}
	lineCounter++;
	if(lineCounter>=509){
		lineCounter=0;
	}
	TIM2->DIER &= TIM_SR_UIF;
	}
}

