//
// Created by shinbokuow on 2020/7/29.
//

#include "uart.h"
#include "sysctl.h"
#include <sbi/sbi_console.h>

#define __UART_BRATE_CONST 16

volatile uart_t *const uart[3] =
	{
		(volatile uart_t *)UART1_BASE_ADDR,
		(volatile uart_t *)UART2_BASE_ADDR,
		(volatile uart_t *)UART3_BASE_ADDR};

void uart_init(uart_device_number_t channel)
{
	sysctl_clock_enable(SYSCTL_CLOCK_UART1 + channel);
	sysctl_reset(SYSCTL_RESET_UART1 + channel);
}

void uart_configure(uart_device_number_t channel, uint32_t baud_rate, uart_bitwidth_t data_width, uart_stopbit_t stopbit, uart_parity_t parity)
{
	/*
	configASSERT(data_width >= 5 && data_width <= 8);
	if(data_width == 5)
	{
		configASSERT(stopbit != UART_STOP_2);
	} else
	{
		configASSERT(stopbit != UART_STOP_1_5);
	}
	 */

	uint32_t stopbit_val = stopbit == UART_STOP_1 ? 0 : 1;
	uint32_t parity_val = 0;
	switch(parity)
	{
	case UART_PARITY_NONE:
		parity_val = 0;
		break;
	case UART_PARITY_ODD:
		parity_val = 1;
		break;
	case UART_PARITY_EVEN:
		parity_val = 3;
		break;
	default:
		//configASSERT(!"Invalid parity");
		break;
	}

	//uint32_t freq = sysctl_clock_get_freq(SYSCTL_CLOCK_APB0);
	uint32_t freq = 195000000;
	//printf("freq = %u\r\n", freq);
	uint32_t divisor = freq / baud_rate;
	//printf("divisor = %u\r\n", divisor);
	uint8_t dlh = divisor >> 12;
	//printf("dlh = %d\r\n", dlh);
	uint8_t dll = (divisor - (dlh << 12)) / __UART_BRATE_CONST;
	//printf("dll = %d\r\n", dll);
	uint8_t dlf = divisor - (dlh << 12) - dll * __UART_BRATE_CONST;
	//printf("dlf = %d\r\n", dlf);
	//printf("const = %d\r\n", __UART_BRATE_CONST);

	/* Set UART registers */

	uart[channel]->LCR |= 1u << 7;
	uart[channel]->DLH = dlh;
	uart[channel]->DLL = dll;
	uart[channel]->DLF = dlf;
	uart[channel]->LCR = 0;
	uart[channel]->LCR = (data_width - 5) | (stopbit_val << 2) | (parity_val << 3);
	uart[channel]->LCR &= ~(1u << 7);
	uart[channel]->IER |= 0x80; /* THRE */
	uart[channel]->FCR = UART_RECEIVE_FIFO_1 << 6 | UART_SEND_FIFO_8 << 4 | 0x1 << 3 | 0x1;
}

int uart_channel_putc(char c, uart_device_number_t channel)
{
	while(uart[channel]->LSR & (1u << 5))
		continue;
	uart[channel]->THR = c;
	return c & 0xff;
}

int uart_channel_getc(uart_device_number_t channel)
{
	/* If received empty */
	if(!(uart[channel]->LSR & 1))
		return EOF;
	else
		return (char)(uart[channel]->RBR & 0xff);
}

int uart_send_data(uart_device_number_t channel, const char *buffer, size_t buf_len)
{
	int g_write_count = 0;
	while(g_write_count < buf_len)
	{
		uart_channel_putc(*buffer++, channel);
		g_write_count++;
	}
	return g_write_count;
}

int uart_receive_data(uart_device_number_t channel, char *buffer, size_t buf_len)
{
	size_t i = 0;
	for(i = 0; i < buf_len; i++)
	{
		if(uart[channel]->LSR & 1)
			buffer[i] = (char)(uart[channel]->RBR & 0xff);
		else
			break;
	}
	return i;
}

void uart_set_receive_trigger(uart_device_number_t channel, uart_receive_trigger_t trigger)
{
	uart[channel]->IER |= 0x01;
	uart[channel]->SRT = trigger;
}