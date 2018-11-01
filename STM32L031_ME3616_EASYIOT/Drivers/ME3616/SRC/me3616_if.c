/**
  ******************************************************************************
  * @file    me3616_if.c
  * @author  Simon Luk (simonluk@unidevelop.net)
  * @brief   This file provide the interface and callback between ME3616 and MCU
  *          for GOSUNCN ME3616 NB-IoT Module Made by emakerzone
  *
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2018 Simon Luk </center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of Simon Luk nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */



#include "me3616.h"
extern DMA_HandleTypeDef hdma_usart1_tx;

void ME3616_IF_ErrorHandler(char *file, int line, char * pch)
{
	UNUSED(file);
	UNUSED(line);
	DBG_Print(pch,  DBG_DIR_AT);
	Set_Sys_State(&ME3616_Instance, SYS_STATE_ERR);
	//Halt and do nothing for this Demo.
	while(1);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	DBG_Print("UART ErrorCallback.", DBG_DIR_AT);
	
	//Halt and do nothing for this Demo.
	while(1);
}

/**
  * @brief  Power off ME3616.
  * @param  Me3616: Instance of Me3616.
  * @retval None.
  */
void ME3616_PowerOff(Me3616_DeviceType * Me3616)
{
	HAL_GPIO_WritePin(ME3616_Power_Port, ME3616_Power_Pin, GPIO_PIN_SET);
}

/**
  * @brief  Power on ME3616.
  * @param  Me3616: Instance of Me3616.
  * @param  delay_ticks: simulates power button push down time.
  * @retval None.
  */
void ME3616_PowerOn(Me3616_DeviceType * Me3616, uint32_t delay_ticks)
{
	HAL_GPIO_WritePin(ME3616_Power_Port, ME3616_Power_Pin, GPIO_PIN_SET);
	HAL_Delay(delay_ticks);
	HAL_GPIO_WritePin(ME3616_Power_Port, ME3616_Power_Pin, GPIO_PIN_RESET);
}

/**
  * @brief  reset ME3616.
  * @param  Me3616: Instance of Me3616.
  * @param  delay_ticks: simulates reset button push down time.
  * @retval None.
  */
void ME3616_Reset(Me3616_DeviceType * Me3616, uint32_t	delay_ticks)
{
	HAL_GPIO_WritePin(ME3616_Reset_Port, ME3616_Reset_Pin, GPIO_PIN_SET);
	HAL_Delay(delay_ticks);
	HAL_GPIO_WritePin(ME3616_Reset_Port, ME3616_Reset_Pin, GPIO_PIN_RESET);
}



/**
  * @brief  Init UART Character Match .
  * @param  Me3616: Instance of Me3616.
  * @param  huart: for ME3616_UART.
  * @retval None.
  */
void Init_UART_CM(UART_HandleTypeDef * huart)
{
    //Enable the Character Match for Usart2
    //Disable ME3616 UART
    __HAL_UART_DISABLE(huart);
	//HAL_UART_AbortReceive_IT(huart);
    
    //Clear the Character Match flag
	__HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_CMF); 
    
    //Clear ADD[7:0] at [31:24] of CR2
	huart->Instance->CR2 &= ~USART_CR2_ADD;    
    
    //Set '\n' to ADD[7:0]
    huart->Instance->CR2 |= (uint32_t)('\n') << USART_CR2_ADD_Pos;

    //Enable IT for the Character Match
	__HAL_UART_ENABLE_IT(huart, UART_IT_CM); 
    
    //Enable ME3616 UART
    __HAL_UART_ENABLE(huart);

    //Disable IT for Error, for non-stop UART->DMA transfer.
//	__HAL_UART_DISABLE_IT(huart, UART_IT_ERR);
}


/**
  * @brief  before send AT CMD, wait at state ready
  * @param  Me3616: Instance of Me3616.
  * @retval true for ready, false for timeout.
  */
bool Wait_AT_SendReady(Me3616_DeviceType * Me3616)
{
	//By Polling in this Demo. You can overwrite this by Timer or RTOS task.
	uint32_t start_time = 0;
	start_time = HAL_GetTick();
	
	while(1)
	{
		//Time out or Receive AT ERROR
		if((HAL_GetTick() - start_time) > ME3616_SEND_TIMOUT)
		{
			Set_AT_Info(Me3616, AT_CMD_IGNORE, AT_ACTION_IGNORE, AT_STATE_TIMEOUT);
			return false;
		}
		
		else if(Get_AT_State(Me3616) != AT_STATE_SEND)
		{
			return true;
		}
	}
}


/**
  * @brief  Handle MCU to send a string
  * @param  Me3616: Instance of Me3616.
  * @retval true for success, false for fail.
  */
bool UART_AT_Send(Me3616_DeviceType * Me3616)
{
	uint16_t len = 0;
	len = strlen((char *)(Me3616->TxBuffer));
	
	//Tx string, out of TxBuffer
	if(ME3616_TX_BUFFER_SIZE -1 < len) ME3616_IF_ErrorHandler(__FILE__, __LINE__, "UART Send out of buffer.");

    DBG_Print((char *)(Me3616->TxBuffer), DBG_DIR_TX);
    
	//Wait until transmit is idle
	while(Me3616->UartDMA_Tx->State != HAL_DMA_STATE_READY);
	
	if(HAL_UART_Transmit_DMA(&ME3616_UART, Me3616->TxBuffer, len) == HAL_OK)
	{
        //Wait until transmit is idle
        while(Me3616->UartDMA_Tx->State != HAL_DMA_STATE_READY);
        while(__HAL_UART_GET_FLAG (Me3616->UartDevice, UART_FLAG_TC) == 0);
		return true;
	}
	else
    {
        while(__HAL_UART_GET_FLAG (Me3616->UartDevice, UART_FLAG_TC) == 0);
		DBG_Print("UART_AT_Send() DMA send failed.", DBG_DIR_AT);
		return false;
    }
}


/**
  * @brief  after send AT CMD, wait at state OK
  * @param  Me3616: Instance of Me3616.
  * @retval true for ready, false for timeout.
  */
bool Wait_AT_Response(Me3616_DeviceType * Me3616)
{
	//By Polling in this Demo. You can overwrite this by Timer or RTOS task.
	uint32_t start_time = 0;
	start_time = HAL_GetTick();

	while(1)
	{
		//Time out or Receive AT ERROR
		if((HAL_GetTick() - start_time) > ME3616_RECEIVE_TIMOUT)
		{
			Set_AT_Info(Me3616, AT_CMD_IGNORE, AT_ACTION_IGNORE, AT_STATE_TIMEOUT);
			return false;
		}
		
		else if((Get_AT_State(Me3616) == AT_STATE_ATOK) || (Get_AT_State(Me3616) == AT_STATE_ATERR))
		{
			return true;
		}
	}
}


void DBG_Forward(Me3616_DeviceType * Me3616)
{
	uint16_t len = 0;

	len = strlen((char *)Me3616->DBG_RxBuffer);
    
    DBG_Print((char *)(Me3616->DBG_RxBuffer), DBG_DIR_TX);
    
	//Wait until transmit is idle
	while(Me3616->UartDMA_Tx->State != HAL_DMA_STATE_READY);

	if(HAL_UART_Transmit_DMA(Me3616->UartDevice, Me3616->DBG_RxBuffer, len) != HAL_OK)
	{
		DBG_Print("DBG_Forward to ME3616 Fail.", DBG_DIR_AT);
	}
    
    //Wait until transmit is idle
    while(Me3616->UartDMA_Tx->State != HAL_DMA_STATE_READY);
    while(__HAL_UART_GET_FLAG(Me3616->UartDevice, UART_FLAG_TC) == 0);
    
    memset(Me3616->DBG_RxBuffer, 0, ME3616_DBG_RX_BUFFER_SIZE -1);

	HAL_UART_AbortReceive_IT (&DBG_UART);
	
}

void HAL_UART_AbortReceiveCpltCallback(UART_HandleTypeDef * huart)
{
    if(huart == &DBG_UART)
    {
        while(huart->RxState != HAL_UART_STATE_READY);
		if(HAL_UART_Receive_IT(&DBG_UART, ME3616_Instance.DBG_RxBuffer, ME3616_DBG_RX_BUFFER_SIZE -1) != HAL_OK)
		{
			DBG_Print("DBG_Forward restart receive Fail.", DBG_DIR_AT);
		}
		
	__HAL_UART_CLEAR_FLAG(&DBG_UART, UART_FLAG_CMF);
    }

}


/**
  * @brief  when receive one or more new string, call Receive function to handle
  * @param  Me3616: Instance of Me3616.
  * @retval None.
  */
void UART_AT_Receive(Me3616_DeviceType * Me3616)
{
	ME3616_String_Receive(Me3616);
    __HAL_UART_CLEAR_FLAG(Me3616->UartDevice, UART_FLAG_CMF);
}

