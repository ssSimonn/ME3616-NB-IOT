/**
  ******************************************************************************
  * @file    me3616.c
  * @author  Simon Luk (simonluk@unidevelop.net)
  * @brief   This file provides driver functions to handle internal AT Command for
  *          GOSUNCN ME3616 NB-IoT Module Made by emakerzone
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

/*
				   ##### How to use this driver #####
==============================================================================
  [..]
     *** This Driver Based On STMicroelectronics HAL firmware functions ***
     ======================================================================

   (#) Configurate Header of Definations -> me3616.h
  	   (++) Modify the #define Line in the beginning of the "me3616.h" file. 
  	   		Such as UART peripheral, GPIO Port/Pin, Buffer Size. 
  	   		It depends on your MCU.


   (#) Configurate the interface between ME3616 and MCU -> me3616_if.c 
   		(In most case, you do not need to modify too much by ST MCU.)

  	   (++) Modify GPIO PowerOn, PowerOff, Reset Function

	   (++) Modify AT Send and Receive Function

  	   (++) Modify Error Callback Funcion


   (#) Configurate Interrupt Entry -> stm32l031_it.c (By this Demo)

   	   (++) add #include "me3616.h"

   	   (++) add Receive Function(me3616_if.c) to Interrupt Entry at UART IRQ Handler


   (#) Add me3616 to your project. such as -> main.c

	   (++) add #include "me3616.h", also Compiler Preprocessor Directories

	   (++) add ME3636_INIT() and ME3616_APP() where you start NB-IoT.

	   (++) program your NB-IoT functions at me3616_app.c

	   (++) Have Fun!

===============================================================================
*/

#include "me3616.h"

Me3616_DeviceType ME3616_Instance;

const char * const AT_Header = "AT";
const char * const AT_Set = "=";
const char * const AT_Read = "?";
const char * const AT_Test = "=?";
const char * const AT_End = "\r\n";

//AT Command Base on <模组 AT 指令手册 V1.8>
//Caution! These Strings below, MUST Be corresponded with AT_CMD_t at me3616.h.
//For further AT command, refer to GOSUNCN AT Command Manual.
const char * AT_CMD_String[]=
{	
	//模组信息识别指令
	"I",							//查询模组识别信息
	"+GMI",							//查询制造商名称
	"+CGMI",						//查询制造商名称
	"+GMM",							//查询模组 ID
	"+CGMM",						//查询模组 ID
	"+GMR",							//查询软件版本号
	"+CGMR",						//查询软件版本号
	"+GSN",							//查询产品序列号
	"+CGSN",						//查询产品相应的序列标识
	"+CIMI",						//查询国际移动台设备标识
	"+ZPCB",						//查询 PCB 号

	//通用命令
	"&F",							//恢复出厂设置
	"&V",                           //显示当前配置
	"Z",                         	//复位为缺省配置
    "Q",                          	//结果码抑制
    "E",                          	//回显命令
    "V",                         	//DCE 返回格式
    "+CFUN",                        //设置电话功能
    "+CMEE",                        //上报设备错误
    "+CME",                         //ERROR ME 错误结果码

    //串口控制指令
    "+IPR",                         //设定串口波特率
    "+CMUX",                        //串口多路复用
    "+IFC",                         //DTE-DCE 的本地流控
    "+ZCOMWRT",                     //串口升级写文件

    //SIM相关命令
    "+CLCK",                        //功能锁
    "+CPWD",                        //改变锁密码
    "+CPIN",                        //输入 PIN 码
    "+CRSM",                        //有限制的 SIM 访问
    "*MICCID",                      //读取 SIM 卡的 ICCID

    //网络服务相关命令
    "+CEREG",                       //EPS 网络注册状态
    "+COPS",                        //PLMN 选择
    "+CESQ",                        //信号强度查询
    "+CSQ",                         //信号强度查询
    "+CTZU",                        //自动获取网络时间开关
    "+CTZR",                        //时区报告开关
    "+CCLK",                        //时钟管理
    "*MSPCHSC",                     //设置扰码算法
    "*MFRCLLCK",                    //锁频点/物理小区
    "*MBAND",                       //查询当前 BAND 值
    "*MBSC",                        //锁 BAND
    "*MENGINFO",                    //查询当前网络状态和小区信息
    "*MNBIOTRAI",                   //主动释放 RRC 连接

    //低功耗相关命令
    "+CEDRXS",                      //eDRX 设置
    "+CEDRXRDP",                    //eDRX 动态参数读取
    "+CPSMS",                       //节电模式（PSM）设置
    "+ZSLR",                        //系统睡眠开关
    "+SETWAKETIME",                 //设置模组唤醒时间
    "*MNBIOTEVENT",                 //禁止/使能 PSM 状态主动上报
    "+ESOWKUPDELAY",                //设置数据延时  

    //分组域命令
    "*MCGDEFCONT",                  //设置默认的 PSD 连接设置
    "+CGCONTRDP",                   //读取 PDP 上下文参数
    "+IP",                          //自动拨号 IP 上报
    "+EGACT",                       //激活/去激活 PDN 上下文

    //硬件相关及扩展AT命令
    "+ZADC",                        //读取 ADC 管脚值
    "+ZRST",                        //模组复位
    "+ZTURNOFF",                    //关闭模组
    "+ZCONTLED",                    //状态指示信号控制功能
    "+PWRKEYSTA",                   //设置打开/关闭 POWERKEY 长拉低功能

    //域解释AT命令
    "+EDNS",                        //通过域名获取 IP 地址

    //TCP/IP相关AT命令
    "+ESOC",                        //创建一个 TCP/UDP
    "+ESOCON",                      //套接字连接到远程地址和端口
    "+ESOSEND",                     //发送数据
    "+ESOCL",                       //关闭套接字
    "+ESONMI",                      //套接字消息到达指示符
    "+ESOERR",                      //套接字错误指示器
    "+ESOSETRPT",                   //接收数据的显示形式
    "+ESOREADEN",                   //设置数据上来主动上报
    "+ESODATA",                     //数据到来主动上报
    "+ESOREAD",                     //读取数据
    "+ESOSENDRAW",                  //发送原始数据
    "+PING",                        //通过内置协议栈 ping 服务器

    //MQTT相关AT命令
    "+EMQNEW",                      //建立新的 MQTT
    "+EMQCON",                      //向 MQTT 服务器发送链接报文
    "+EMQDISCON",                   //断开与 MQTT 服务器的链接
    "+EMQSUB",                      //发送 MQTT 订阅报文
    "+EMQUNSUB",                    //发送 MQTT 取消订阅报文
    "+EMQPUB",                      //发送 MQTT 发布报文

    //CoAP相关AT命令
    "+ECOAPSTA",                    //创建一个 COAP 服务器
    "+ECOAPNEW",                    //创建一个 COAP 客户端
    "+ECOAPSEND",                   //COAP 客户端发送数据
    "+ECOAPDEL",                    //销毁 CoAP 客户端实例
    "+ECOAPNMI",                    //返回服务器端响应
 
     //HTTP/HTTPS相关命令
    "+EHTTPCREATE",                 //创建客户端 HTTP/HTTPS 实例
    "+EHTTPCON",                    //建立 HTTP/HTTPS 链接
    "+EHTTPDISCON",                 //关闭 HTTP/HTTPS 链接
    "+EHTTPDESTROY",                //释放创建的 HTTP/HTTPS 链接
    "+EHTTPSEND",                   //发送 HTTP/HTTPS 请求
    "+EHTTPNMIH",                   //从主机响应的头信息
    "+EHTTPNMIC",                   //从主机响应的内容信息
    "+EHTTPERR",                    //客户端连接的错误提示

    //电信 IOT 接入相关 AT 命令
    "+M2MCLINEW",                   //LWM2M Client 注册电信 IOT 平台
    "+M2MCLIDEL",                   //LWM2M Client 去注册电信 IOT 平台
    "+M2MCLISEND",                  //LWM2M Client 数据发送
    "+M2MCLIRECV",                  //LWM2M Client 数据上报
    "+M2MCLICFG",                   //数据发送和上报模式设置
 
    //IPERF 带宽测试
    "+IPERF",                       //IPERF 带宽测试

    //FOTA 相关指令
    "+FOTATV",                      //设置 FOTA 升级参数
    "+FOTACTR",                     //启动 WeFOTA 升级
    "+FOTAIND",                     //WeFOTA 升级状态报告

    //FTP 相关 AT 指令
    "+ZFTPOPEN",                    //启动文件服务
    "+ZFTPCLOSE",                   //关闭文件服务
    "+ZFTPSIZE",                    //获取 FTP 文件大小
    "+ZFTPGET",                     //文件下载
    "+ZFTPPUT",                     //文件上传命令

    //GPS 相关指令
    "+ZGMODE",                      //设置定位模式
    "+ZGURL",                       //设置 AGPS 服务器的 URL
    "+ZGAUTO",                      //设置 AGNSS 数据自动下载功能
    "+ZGDATA",                      //下载或查询 AGNSS 数据
    "+ZGRUN",                       //开启/关闭 GPS 服务
    "+ZGLOCK",                      //设置单次定位下使能锁定系统睡眠
    "+ZGTMOUT",                     //设置单次定位超时时间
    "+ZGRST",                       //重启 GPS
    "+ZGPSR",                       //使能/禁止+ZGPSR 上报
    "+ZGNMEA",                      //设置 GPS 数据 NMEA 上报格式

    //中国移动 OneNET 平台接入相关 AT 命令
    "+MIPLCREATE",                  //创建 OneNET instance
    "+MIPLDELETE",                  //删除 OneNET instance
    "+MIPLOPEN",                    //设备注册到 OneNET 平台
    "+MIPLCLOSE",                   //去注册 OneNET 平台
    "+MIPLADDOBJ",                  //创建一个 object（对象）
    "+MIPLDELOBJ",                  //删除一个 object（对象）
    "+MIPLUPDATE",                  //注册更新命令
    "+MIPLREAD",                    //OneNET 平台向模组发起 read 请求
    "+MIPLREADRSP",                 //模组响应平台的 READ 请求
    "+MIPLWRITE",                   //OneNET 平台向模组发起 write 请求
    "+MIPLWRITERSP",                //模组响应平台的 WRITE 请求
    "+MIPLEXECUTE",                 //OneNET 平台向模组发起 execute 请求
    "+MIPLEXEUTERSP",               //模组响应平台的 execute 请求
    "+MIPLOBSERVE",                 //OneNET 平台向模组发起 observe 请求
    "+MIPLOBSERVERSP",              //模组响应平台的 observe 请求
    "+MIPLDISCOVER",                //OneNET 平台向模组发起 discover 请求
    "+MIPLDISCOVERRSP",             //模组响应平台的 DISCOVER 请求
    "+MIPLPARAMETER",               //OneNET 平台向模组发起设置 parameter 请求
    "+MIPLPARAMETERRSP",            //模组响应平台的设置 paramete 请求
    "+MIPLNOTIFY",                  //模组向平台请求同步数据
    "+MIPLVER",                     //查询 OneNET SDK 版本号
    "+MIPLEVENT",                   //模组状态上报
    ""
};

//Prefix string of Active Report. 
const char * AT_Report_String[] =
{    
	
    "*MATREADY",
    "+CFUN",
    "+CPIN",
    "+IP",
    "+ESONMI",
    "+ESODATA",
    "+EMQDISCON",
    "+EMQPUB",
    "+ECOAPNMI",
    "+M2MCLIRECV",
    "+M2MCLI",
    "+iperf",
    "+ZGPSR",
    "+MIPLEVENT",
    "+MIPLREAD",
    "+MIPLWRITE",
    "+MIPLOBSERVE",
    "+MIPLDISCOVER",
    "+MIPLPARAMETER",
    NULL
};

typedef void (* _AT_Report_Entry)(struct __Me3616_DeviceType * Me3616, char * pch, uint16_t len);

_AT_Report_Entry AT_Report_Entry[20] = 
{
	MATREADY_Callback,
	CFUN_Callback,
	CPIN_Callback,
	IP_Callback,
	ESONMI_Callback,
	ESODATA_Callback,
	EMQDISCON_Callback,
	EMQPUB_Callback,
	ECOAPNMI_Callback,
	M2MCLIRECV_Callback,
	M2MCLI_Callback,
	IPERF_Callback,
	ZGPSR_Callback,
	MIPLEVENT_Callback,
	MIPLREAD_Callback,
	MIPLWRITE_Callback,
	MIPLOBSERVE_Callback,
	MIPLDISCOVER_Callback,
	MIPLPARAMETER_Callback,
	NULL
};


#ifdef DEBUG_ME3616

extern DMA_HandleTypeDef hdma_usart2_tx;
/**
  * @brief  Print Debug message and forward Tx, Rx string.
  * @note   Use ME3616 Instance directly.
  * @param  ch: Debug string comes from.
  * @param  direction: Indicate string source: Tx, Rx, MCU_AT, APP.
  * @retval None.
  */
void DBG_Print(const char * ch, DBG_DIR_t direction)
{
    int16_t len = 0;
	uint32_t data = HAL_GetTick();
	if ( ch == NULL || *ch == NULL) return;

	//wait until DBG_UART ready.
	while( __HAL_UART_GET_FLAG(&DBG_UART, UART_FLAG_TC) == 0);

	switch (direction)
	{
		case DBG_DIR_RX:
		{
			len = sprintf((char *)ME3616_Instance.DBG_TxBuffer, "[%d][Rx]: %s\r\n", data, ch );
			break;
		}
		
		case DBG_DIR_TX:
		{
			len = sprintf((char *)ME3616_Instance.DBG_TxBuffer, "\r\n[%d][Tx]: %s\r\n", data, ch );
			break;
		}
        
		case DBG_DIR_AT:
		{
			len = sprintf((char *)ME3616_Instance.DBG_TxBuffer, "[%d][MCU_AT]: %s\r\n", data, ch );
			break;
		}
        
        case DBG_DIR_SDK:
		{
			len = sprintf((char *)ME3616_Instance.DBG_TxBuffer, "[%d][EasyIoT]: %s\r\n", data, ch );
			break;
		}
		default:
		{
			len = sprintf((char *)ME3616_Instance.DBG_TxBuffer, "[%d][APP]: %s\r\n", data, ch );
		}
	}
	
    //Halt if DBG_Print() has error.
	if(len <= 0) while(1);
    
	while(hdma_usart2_tx.State != HAL_DMA_STATE_READY);
	HAL_UART_Transmit_DMA(&DBG_UART, ME3616_Instance.DBG_TxBuffer, len);
    while(hdma_usart2_tx.State != HAL_DMA_STATE_READY);
    while(__HAL_UART_GET_FLAG(&DBG_UART, UART_FLAG_TC) == 0);
}

void State_Hex2Str(char *sDest, uint32_t uSrc)
{
	sprintf(sDest, "%x", uSrc);
}

#endif




 __weak void ME3616_ErrorHandler(char *file, int line, char * pch)
{
	UNUSED(file);
	UNUSED(line);
	DBG_Print(pch,  DBG_DIR_AT);
	Set_Sys_State(&ME3616_Instance, SYS_STATE_ERR);
	//Halt and do nothing for this Demo.	
	while(1);
}

__INLINE bool Get_Sys_State(Me3616_DeviceType * Me3616, SYS_State_t mask)
{
	if(Me3616->Sys_State & mask)
	{
		return true;
	}
	else
	{
		return false;
	}
}

__INLINE void Set_Sys_State(Me3616_DeviceType * Me3616, SYS_State_t mask)
{
	Me3616->Sys_State |= mask;
}
__INLINE void Clear_Sys_State(Me3616_DeviceType * Me3616, SYS_State_t mask)
{
	Me3616->Sys_State &= ~mask;
}

__INLINE AT_CMD_t Get_Last_AT_CMD(Me3616_DeviceType * Me3616)
{
	return Me3616->AT_Info.LastCMDBase;
}

__INLINE void Set_Last_AT_CMD_None(Me3616_DeviceType * Me3616)
{
	Me3616->AT_Info.LastCMDBase = AT_CMD_NONE;
}

__INLINE AT_Action_t Get_Last_AT_Action(Me3616_DeviceType * Me3616)
{
	return Me3616->AT_Info.LastCMDAction;
}

__INLINE AT_State_t Get_AT_State(Me3616_DeviceType * Me3616)
{
	return Me3616->AT_Info.At_State;
}

__INLINE void Set_AT_Info(Me3616_DeviceType * Me3616, AT_CMD_t at_cmd, AT_Action_t at_action, AT_State_t at_state)
{
	if(at_state != AT_STATE_IGNORE)
	{
		Me3616->AT_Info.At_State = at_state;
	}
	if(at_action != AT_ACTION_IGNORE) 
	{
		Me3616->AT_Info.LastCMDAction = at_action;
	}
	if(at_cmd != AT_CMD_IGNORE) 
	{
		Me3616->AT_Info.LastCMDBase = at_cmd;
	}
}

/**
  * @brief  Establist AT command and send to ME3616.
  * @param  Me3616: Instance of Me3616.
  * @param  at_cmd: AT Command refer by AT_CMD_t
  * @param  at_action: Parameter type commands refer by 3GPP
  * @param  override: if true, This Function will force send AT Command out
  						with out consider AT state, timout, and command response.
  * @param  pch: while at_action is AT_SET, follow command strings.
  * @retval true for send success. false for fail.
  */
bool ME3616_Send_AT_Command(Me3616_DeviceType * Me3616, AT_CMD_t at_cmd, AT_Action_t at_action, bool override, char * pch)
{
	bool res = 0;
    int16_t len = 0;
	//Check NULL pointer
	if((at_action == AT_SET) && (pch == NULL)) ME3616_ErrorHandler(__FILE__, __LINE__, "Send_AT_Command() has a NULL CMD Pointer.");

	Set_Sys_State(Me3616, SYS_STATE_BUSY);

	//clear last CMD string
	memset(Me3616->TxBuffer, 0, Me3616->TxStringLen);
	switch( at_action )
	{
		case AT_BASE:
		{
			len = sprintf( (char * )Me3616->TxBuffer, "%s%s%s", AT_Header, AT_CMD_String[at_cmd], AT_End);
			if(len <= 0) ME3616_ErrorHandler(__FILE__, __LINE__, "AT Command Fault.");		
			break;
		}
		case AT_SET:
		{
			len = sprintf( (char * )Me3616->TxBuffer, "%s%s%s%s%s", AT_Header, AT_CMD_String[at_cmd], AT_Set, pch, AT_End);
			if(len <= 0) ME3616_ErrorHandler(__FILE__, __LINE__, "AT Command Fault.");		
			break;
		}
		case AT_READ:
		{
			len = sprintf( (char * )Me3616->TxBuffer, "%s%s%s%s", AT_Header, AT_CMD_String[at_cmd], AT_Read, AT_End);
			if(len <= 0) ME3616_ErrorHandler(__FILE__, __LINE__, "AT Command Fault.");
			break;
		}
		case AT_TEST:
		{
			len = sprintf( (char * )Me3616->TxBuffer, "%s%s%s%s", AT_Header, AT_CMD_String[at_cmd], AT_Test, AT_End);
			if(len <= 0) ME3616_ErrorHandler(__FILE__, __LINE__, "AT Command Fault.");
			break;
		}
		default:
		{
			//Set_AT_State(Me3616, at_class, at_cmd, at_action, AT_STATE_ATERR);
			ME3616_ErrorHandler(__FILE__, __LINE__, "Unknow AT Action.");
		}
	}
	Me3616->TxStringLen = len;
	
	//Ignore previous AT state, force send AT command 
	if(override == true)
	{
		Set_AT_Info(Me3616,  at_cmd, at_action, AT_STATE_SEND);
		Me3616->TxDataLastTime = HAL_GetTick();
		res = UART_AT_Send(Me3616);
	}
	else
	{
		//wait last AT Command response a result.
		if(Wait_AT_SendReady(Me3616) == false) return false;

		//AT Status ERR and Timout MUST BE clear before send a new AT CMD.
		if(Me3616->AT_Info.At_State == AT_STATE_ATOK || Me3616->AT_Info.At_State == AT_STATE_NONE)
		{
			
			Set_AT_Info(Me3616,  at_cmd, at_action, AT_STATE_SEND);
			Me3616->TxDataLastTime = HAL_GetTick();
			res = UART_AT_Send(Me3616);
		}
		else
		{
			ME3616_ErrorHandler(__FILE__, __LINE__, "AT Send fault. Previous AT State has not clear.");
		}
	}

	//wait AT response
	if (res == true ) 
	{
		//AT Send successed, wait response.
		Wait_AT_Response(Me3616);
		return true;
	}
	else
	{	
		ME3616_ErrorHandler(__FILE__, __LINE__, "AT Send fault. UART Failure.");
		return false;
	}
}

bool Check_Response(Me3616_DeviceType * Me3616, char *pch, uint16_t len)
{
	//Waiting a command response?
	if( Get_AT_State(Me3616) == AT_STATE_SEND)
	{
		//check incoming string is AT OK?
		if(!strncmp(pch, "OK", 2))
		{
			Set_AT_Info(Me3616, AT_CMD_IGNORE, AT_ACTION_IGNORE, AT_STATE_ATOK);
			DBG_Print("AT OK Confirmed.",  DBG_DIR_AT);
			AT_ResultReport(Me3616, true);
			return true;
		}
		
		else if(!strncmp(pch, "ERROR", 5))
		{	
			//Command feedback Error With +CMEE = 0
			Set_AT_Info(Me3616, AT_CMD_IGNORE, AT_ACTION_IGNORE, AT_STATE_ATERR);
			DBG_Print("AT ERROR Confirmed.",  DBG_DIR_AT);
			AT_ResultReport(Me3616, false);
			return true;
		}
		else if(!strncmp(pch, "+CME ERROR", 10))
		{	
			//Command feedback Error with +CMEE = 1 or 2
			Set_AT_Info(Me3616, AT_CMD_IGNORE, AT_ACTION_IGNORE, AT_STATE_ATERR);
			DBG_Print("AT ERROR Confirmed.",  DBG_DIR_AT);
			CME_Callback(Me3616, pch, len);
			return true;
		}
		else
		{
			//Command Response Before AT OK/ERROR
			Command_Response(Me3616, pch, len);
		}
	}
	//Active Response or other unknow response.
	else
	{
		//without state of AT_Send, but receive a string.
		Active_Report(Me3616, pch, len);
	}
	return false;
}

void RxHandler(Me3616_DeviceType * Me3616, char *p_Buff, uint16_t len)
{
	Me3616->RxDataLastTime = HAL_GetTick();
	
	Set_Sys_State(Me3616, SYS_STATE_INCOMMING_NEW_AT_STRING);
	Check_Response(Me3616, p_Buff, len);
	Clear_Sys_State(Me3616, SYS_STATE_INCOMMING_NEW_AT_STRING);
	Set_Sys_State (Me3616, SYS_STATE_READY);
}


void ME3616_String_Receive(Me3616_DeviceType * Me3616)
{
    //Load previous positions of string in RxBuff. for easier to porting to another system.
    char * const pBuff = (char *)Me3616->RxBuffer;
	char * const pBuffBorder = (char *)Me3616->RxBuffer + ME3616_RX_BUFFER_SIZE - 1;
    char * pBegin = (char *)Me3616->RxBuffer + Me3616->RxStringBegin;
    char * pEnd = (char *)Me3616->RxBuffer + Me3616->RxStringEnd;
	char * const pVaildBuff = (char *)Me3616->RxVaildString;
	uint16_t uLength = 0;
	
	//delay 30 ticks for waitting DMA transfer complete.
    //if system have havey duty on DMA, you should consider adjust Rx buffer and this time of delay.
	HAL_Delay(30);
	
	//Ensure pointers are legal.
	if(pEnd > pBuff + ME3616_RX_BUFFER_SIZE - 1) 
	{
		ME3616_IF_ErrorHandler(__FILE__, __LINE__, "UART Rx Buffer pointer illegal.");	
	}

	//Scan whole RxBuffer, in a definite length.
	for(uint16_t i = 0; i < ME3616_RX_BUFFER_SIZE - 1; i++)
	{
        //Out of Buffer by one string, length of a single string > Buffer size.
        if( i >= ME3616_RX_BUFFER_SIZE - 1) ME3616_IF_ErrorHandler(__FILE__, __LINE__, "UART Receive out of buffer.");

        //pick a char from RxBuff
		switch( *pEnd )
		{
			//the end of the string found.
			case '\n':
			{
				//judge the string whether segmented.
				if( pBegin <= pEnd)
				{ 
					//get the length of string.
                    uLength =  pEnd - pBegin + 1;
                    
					//Copy and Send string to RxVaildBuff, directly.
					memcpy(pVaildBuff, pBegin, uLength);
					
                    //add '\0' to end the string. overwrite the bottom CR LF
                    memset(pVaildBuff  + uLength - 2, 0, 2);
					
					//Clear this string in RxBuff.
					memset(pBegin, 0,  uLength);
				}
				//String is segmented.
				else
				{
					//copy two parts to RxVaildString
					//from RxStringBegin to buffer bottom
					strcpy(pVaildBuff, pBegin);
					
					//clear this string in RxBuffer.
					memset(pBegin, 0,  pBuffBorder - pBegin + 1);
					
					//copy from buffer header to RxStringEnd
					memcpy(pVaildBuff + ( pBuffBorder - pBegin + 1 ), pBuff, pEnd - pBuff + 1);

					//add '\0' to end the string. overwrite the bottom CR LF
					memset(pVaildBuff + ( pBuffBorder - pBegin) + (pEnd - pBuff), 0, 2);

					//clear
 					memset(pBuff, 0, pEnd - pBuff + 1); 
				}

				//get the length of Vailded String.
                uLength = strlen(pVaildBuff);

				//if Vailded String has only beginning of CR LF, ignore this string. and prepare next string.
                if( *pVaildBuff == '\0') 
                {
                    if( pEnd < pBuffBorder)
					{
				        pEnd++;
						pBegin = pEnd;
					}
					else
					{
						pEnd = pBuff;
						pBegin = pBuff;
					}
                    break;
                }
                else
                {
                    //Send the vailded string to RxHandler
                    RxHandler(Me3616, pVaildBuff, uLength);
					
                    //Prepare to receive next string 
					if( pEnd < pBuffBorder)
					{
						pEnd++;
						pBegin = pEnd;
					}
					else
					{
						pEnd = pBuff;
						pBegin = pBuff;
					}
                    break;
                }
			}

		//RxBuff empty, store position.
        case '\0':
			{
                pBegin = pEnd;

				Me3616->RxStringBegin = pBegin - pBuff;
				Me3616->RxStringEnd = pEnd - pBuff;
				return;
			}
			
			default:
			{
                //point to next char
				if(pEnd < pBuffBorder)
				{
                    pEnd++;
				}
				else
				{
                    pEnd = pBuff;
				}
			}				
		}  
	}
}

void Active_Report(Me3616_DeviceType * Me3616, char *pch, uint16_t len)
{
	const char  ** String_P = NULL;
	uint8_t Entry_num = 0;
	if(Me3616 == NULL || pch == NULL || *pch == NULL) return;

	for(String_P = AT_Report_String; *String_P != NULL;	 String_P++, Entry_num++)
	{
		if(!strncmp(pch, *String_P, strlen(*String_P) ))
		{
			//String Match
			AT_Report_Entry[Entry_num](Me3616, pch, len);
			return;
		}
	}
	if(*String_P == NULL)UnknowActiveReport_Callback(Me3616, pch, len);

	return;
}

bool ME3616_Init(Me3616_DeviceType * Me3616, UART_HandleTypeDef * AT_huart, DMA_HandleTypeDef * DmaTx, DMA_HandleTypeDef * DmaRx)
{
    uint32_t start_time = 0;

	__set_PRIMASK(1);
	
	Set_Sys_State(Me3616, SYS_STATE_POWERON);

	
	memset(Me3616->IPv4, 0, ME3616_IPV4_SIZE);
	memset(Me3616->IPv6, 0, ME3616_IPV6_SIZE);
	Set_AT_Info(Me3616, AT_CMD_IGNORE, AT_ACTION_IGNORE, AT_STATE_NONE);

	//Init Buffer and pointer
	Me3616->RxStringBegin = 0;
	Me3616->RxStringEnd = 0;
		
	memset(Me3616->RxVaildString, 0, ME3616_RX_BUFFER_SIZE);
	memset(Me3616->RxBuffer, 0, ME3616_RX_BUFFER_SIZE);
	memset(Me3616->TxBuffer, 0, ME3616_TX_BUFFER_SIZE);

	Me3616->UartDevice = AT_huart;
	Me3616->UartDMA_Tx = DmaTx;
	Me3616->UartDMA_Rx = DmaRx;
    
	__set_PRIMASK(0);
    
    Init_UART_CM(Me3616->UartDevice);

	#ifdef DEBUG_ME3616
	Init_UART_CM(&DBG_UART);
	HAL_UART_Receive_IT(&DBG_UART, Me3616->DBG_RxBuffer, ME3616_DBG_RX_BUFFER_SIZE -1);
	#endif
	
	HAL_UART_Receive_DMA(Me3616->UartDevice, Me3616->RxBuffer, ME3616_RX_BUFFER_SIZE);
	
	

	ME3616_PowerOn(Me3616, 1000);
    HAL_Delay(1000);
	ME3616_Reset(Me3616, 1000);



	
    while( 1 )
    { 
		//wait for IPv6 address assigned
		if(Get_Sys_State(Me3616, SYS_STATE_IPV6) == true) 
		{
			//wait ME3616 Module fully Idle, ready to send a new command.
			HAL_Delay(5000);
			Set_Sys_State (Me3616, SYS_STATE_READY);
			return true;
		}
		else if((HAL_GetTick() - start_time) > ME3616_BOOT_TIMOUT)
		{
			//Some SIM Card does not support IPv6
			if(Get_Sys_State(Me3616, SYS_STATE_IPV4) == true)
			{
				Set_Sys_State(Me3616, SYS_STATE_READY);
				return true;
			}

			Set_Sys_State(Me3616, SYS_STATE_ERR);
			//Timeout without IPv4 and IPv6 address.
			return false;
		}
    HAL_Delay(2000);        
    }
}

void Hex2Str(char *sDest, const char *sSrc, int nSrcLen )  
{  
    int  i;  
    char szTmp[3];  
  
    for( i = 0; i < nSrcLen; i++ )  
    {
		sprintf( szTmp, "%02X", (unsigned char) sSrc[i] );  
        memcpy( &sDest[i * 2], szTmp, 2 );  
    }  
    return ;  
}

//十六进制字符串转换为字节流  
bool HexStrToByte(unsigned char* dest, const char* source, int sourceLen)  
{
    short i;
    unsigned char highByte, lowByte;
	
		if((sourceLen % 2) != 0)
			return false;

    for (i = 0; i < sourceLen; i += 2)
    {
        highByte = toupper(source[i]);
        lowByte  = toupper(source[i + 1]);
			
		if((lowByte < 0x30 && lowByte > 0x46) || (highByte < 0x30 && highByte > 0x46))
			return false;

        if (highByte > 0x39)
            highByte -= 0x37;
        else
            highByte -= 0x30;

        if (lowByte > 0x39)
            lowByte -= 0x37;
        else
            lowByte -= 0x30;
  
        dest[i / 2] = (highByte << 4) | lowByte;
    }  
    return true;
}


/*****************************************************************************
                        AT Command Callback Functions
******************************************************************************/
__weak void AT_ResultReport(Me3616_DeviceType * Me3616, bool result)
{
    if(result == true)
    {
        DBG_Print("OK", DBG_DIR_RX);
    }
    else
    {
        DBG_Print("ERROR", DBG_DIR_RX);
    }
}
__weak void Command_Response(Me3616_DeviceType * Me3616, char * pch, uint16_t len)
{
	DBG_Print(pch, DBG_DIR_RX);
}
__weak void MATREADY_Callback(Me3616_DeviceType * Me3616, char * pch, uint16_t len)
{
	char str_state[12] = {0};
			
	DBG_Print("MATREADY Below:",  DBG_DIR_AT);
	DBG_Print(pch, DBG_DIR_RX);

	if(strstr(pch, "1") != NULL)
	{
		Set_Sys_State(Me3616, SYS_STATE_MATREADY);
	}
	else
	{
		Clear_Sys_State(Me3616, SYS_STATE_MATREADY);
	}
	DBG_Print("Sys_State Changed. New state is:", DBG_DIR_AT);
	State_Hex2Str(str_state, Me3616->Sys_State);
	DBG_Print(str_state, DBG_DIR_AT);
}
__weak void CME_Callback(Me3616_DeviceType * Me3616, char * pch, uint16_t len)
{
	DBG_Print("CME Below:",  DBG_DIR_AT);
	DBG_Print(pch, DBG_DIR_RX);

}
__weak void CFUN_Callback(Me3616_DeviceType * Me3616, char * pch, uint16_t len)
{
	char str_state[12] = {0};
		
	DBG_Print("CFUN Below:",  DBG_DIR_AT);
	DBG_Print(pch, DBG_DIR_RX);

	if(strstr(pch, "1") != NULL)
	{
		Set_Sys_State(Me3616, SYS_STATE_CFUN);
	}
	else
	{
		Clear_Sys_State(Me3616, SYS_STATE_CFUN);
	}
	DBG_Print("Sys_State Changed. New state is:",  DBG_DIR_AT);
	State_Hex2Str(str_state, Me3616->Sys_State);
	DBG_Print(str_state, DBG_DIR_AT);
}
__weak void CPIN_Callback(Me3616_DeviceType * Me3616, char * pch, uint16_t len)
{
	char str_state[12] = {0};
	
	DBG_Print("CPIN Below:",  DBG_DIR_AT);
	DBG_Print(pch, DBG_DIR_RX);
	
	if(strstr(pch, "READY") != NULL)
	{
		Set_Sys_State(Me3616, SYS_STATE_CPIN);
	}
	else
	{
		//CPIN not in Ready state, check Manual.
		Clear_Sys_State(Me3616, SYS_STATE_CPIN);
	}
	DBG_Print("Sys_State Changed. New state is:", DBG_DIR_AT);
	State_Hex2Str(str_state, Me3616->Sys_State);
	DBG_Print(str_state, DBG_DIR_AT);
}
__weak void IP_Callback(Me3616_DeviceType * Me3616, char * pch, uint16_t len)
{	
	char str_state[12] = {0};
	
	DBG_Print("IP Below:",  DBG_DIR_AT);
	DBG_Print(pch, DBG_DIR_RX);
	
	if(strstr(pch + 5, ".") != NULL)
	{
		//This is an IPv4 String, update sys state
		memcpy(Me3616->IPv4, pch + 5, len - 4);
		Set_Sys_State(Me3616, SYS_STATE_IPV4);
	}
	else if (strstr(pch + 5, ":") != NULL)
	{
		//this is an IPv6 String, update sys state
		memcpy(Me3616->IPv6, pch + 5, len - 5);
        Set_Sys_State(Me3616, SYS_STATE_IPV6);
	}
	else
	{
		//Unknow IP String, clear IP setting
		memset(Me3616->IPv4, 0, ME3616_IPV4_SIZE);
		memset(Me3616->IPv6, 0, ME3616_IPV6_SIZE);
		Clear_Sys_State(Me3616, SYS_STATE_IPV4);
		Clear_Sys_State(Me3616, SYS_STATE_IPV6);
	}
	DBG_Print("Sys_State Changed. New state is:",  DBG_DIR_AT);
	State_Hex2Str(str_state, Me3616->Sys_State);
	DBG_Print(str_state, DBG_DIR_AT);
}
__weak void ESONMI_Callback(Me3616_DeviceType * Me3616, char * pch, uint16_t len)
{
	DBG_Print("ESONMI Below:",  DBG_DIR_AT);
	DBG_Print(pch, DBG_DIR_RX);
}
__weak void ESODATA_Callback(Me3616_DeviceType * Me3616, char * pch, uint16_t len)
{
	DBG_Print("ESODATA Below:",  DBG_DIR_AT);
	DBG_Print(pch, DBG_DIR_RX);
}
__weak void EMQDISCON_Callback(Me3616_DeviceType * Me3616, char * pch, uint16_t len)
{
	DBG_Print("EMQDISCON Below:",  DBG_DIR_AT);
	DBG_Print(pch, DBG_DIR_RX);
}
__weak void EMQPUB_Callback(Me3616_DeviceType * Me3616, char * pch, uint16_t len)
{
	DBG_Print("EMQPUB Below:",  DBG_DIR_AT);
	DBG_Print(pch, DBG_DIR_RX);
}
__weak void ECOAPNMI_Callback(Me3616_DeviceType * Me3616, char * pch, uint16_t len)
{
	DBG_Print("ECOAPNMI Below:",  DBG_DIR_AT);
	DBG_Print(pch, DBG_DIR_RX);
}
__weak void M2MCLI_Callback(Me3616_DeviceType * Me3616, char * pch, uint16_t len)
{
	char str_state[12] = {0};
	DBG_Print("M2MCLI Below:",  DBG_DIR_AT);
	DBG_Print(pch, DBG_DIR_RX);


	if(strstr(pch, "deregister success") != NULL)
	{
		Clear_Sys_State(Me3616, SYS_STATE_LWM_REGISTER_SUCCESS);
		Clear_Sys_State(Me3616, SYS_STATE_LWM_OBSERVE_SUCCESS);
		
		DBG_Print("Sys_State Changed. New state is:",  DBG_DIR_AT);
		State_Hex2Str(str_state, Me3616->Sys_State);
		DBG_Print(str_state,  DBG_DIR_AT);
			
	}
	else if(strstr(pch, "register success") != NULL)
	{
		Set_Sys_State(Me3616, SYS_STATE_LWM_REGISTER_SUCCESS);
		
		DBG_Print("Sys_State Changed. New state is:",  DBG_DIR_AT);
		State_Hex2Str(str_state, Me3616->Sys_State);
		DBG_Print(str_state,  DBG_DIR_AT);	
	}
	else if(strstr(pch, "observe success") != NULL)
	{
		Set_Sys_State(Me3616, SYS_STATE_LWM_OBSERVE_SUCCESS);
		
		DBG_Print("Sys_State Changed. New state is:",  DBG_DIR_AT);
		State_Hex2Str(str_state, Me3616->Sys_State);
		DBG_Print(str_state,  DBG_DIR_AT);
	}
	else if (strstr(pch, "notify success") != NULL)
	{
		Set_Sys_State(Me3616, SYS_STATE_LWM_NOTIFY_SUCCESS);
				
		DBG_Print("Sys_State Changed. New state is:",  DBG_DIR_AT);
		State_Hex2Str(str_state, Me3616->Sys_State);
		DBG_Print(str_state,  DBG_DIR_AT);
	}
}
__weak void M2MCLIRECV_Callback(Me3616_DeviceType * Me3616, char * pch, uint16_t len)
{
	DBG_Print("M2MCLIRECV Below:",  DBG_DIR_AT);
	DBG_Print(pch,  DBG_DIR_RX);
}
__weak void IPERF_Callback(Me3616_DeviceType * Me3616, char * pch, uint16_t len)
{
	DBG_Print("IPERF Below:",  DBG_DIR_AT);
	DBG_Print(pch,  DBG_DIR_RX);
}
__weak void ZGPSR_Callback(Me3616_DeviceType * Me3616, char * pch, uint16_t len)
{
	DBG_Print("ZGPSR Below:",  DBG_DIR_AT);
	DBG_Print(pch,  DBG_DIR_RX);
}
__weak void MIPLEVENT_Callback(Me3616_DeviceType * Me3616, char * pch, uint16_t len)
{
	DBG_Print("MIPLEVENT Below:",  DBG_DIR_AT);
	DBG_Print(pch,  DBG_DIR_RX);
}
__weak void MIPLREAD_Callback(Me3616_DeviceType * Me3616, char * pch, uint16_t len)
{
	DBG_Print("MIPLREAD Below:",  DBG_DIR_AT);
	DBG_Print(pch,  DBG_DIR_RX);
}
__weak void MIPLWRITE_Callback(Me3616_DeviceType * Me3616, char * pch, uint16_t len)
{
	DBG_Print("MIPLWRITE Below:",  DBG_DIR_AT);
	DBG_Print(pch,  DBG_DIR_RX);
}
__weak void MIPLOBSERVE_Callback(Me3616_DeviceType * Me3616, char * pch, uint16_t len)
{
	DBG_Print("MIPLOBSERVE Below:",  DBG_DIR_AT);
	DBG_Print(pch,  DBG_DIR_RX);
}
__weak void MIPLDISCOVER_Callback(Me3616_DeviceType * Me3616, char * pch, uint16_t len)
{
	DBG_Print("MIPLDISCOVER Below:",  DBG_DIR_AT);
	DBG_Print(pch, DBG_DIR_RX);
}
__weak void MIPLPARAMETER_Callback(Me3616_DeviceType * Me3616, char * pch, uint16_t len)
{
	DBG_Print("MIPLPARAMETER Below:",  DBG_DIR_AT);
	DBG_Print(pch, DBG_DIR_RX);
}
__weak void UnknowActiveReport_Callback(Me3616_DeviceType * Me3616, char * pch, uint16_t len)
{
    DBG_Print("UnknowActiveReport Below:",  DBG_DIR_AT);
	DBG_Print(pch, DBG_DIR_RX);
}

