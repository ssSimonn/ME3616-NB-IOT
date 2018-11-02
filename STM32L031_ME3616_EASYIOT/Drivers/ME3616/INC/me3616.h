	/**
	  ******************************************************************************
	  * @file	 me3616.h
	  * @author  Simon Luk (simonluk@unidevelop.net)
	  * @brief	 This file is the header of me3616.c 
	  *          provides driver functions to handle internal AT Command for
	  * 		 GOSUNCN ME3616 NB-IoT Module Made by emakerzone
	  *
	  ******************************************************************************
	  * @attention
	  *
	  * <h2><center>&copy; COPYRIGHT(c) 2018 Simon Luk </center></h2>
	  *
	  * Redistribution and use in source and binary forms, with or without modification,
	  * are permitted provided that the following conditions are met:
	  *   1. Redistributions of source code must retain the above copyright notice,
	  * 	 this list of conditions and the following disclaimer.
	  *   2. Redistributions in binary form must reproduce the above copyright notice,
	  * 	 this list of conditions and the following disclaimer in the documentation
	  * 	 and/or other materials provided with the distribution.
	  *   3. Neither the name of Simon Luk nor the names of its contributors
	  * 	 may be used to endorse or promote products derived from this software
	  * 	 without specific prior written permission.
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

#ifndef __ME3616_H__
#define __ME3616_H__

#ifdef __cplusplus
    extern "C" {
#endif 
     
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>

#include "main.h"
#include "stm32l0xx_hal.h"


//use DBG_Print() to foward Tx and Rx and print inner debug message, using DBG_UART.
#define DEBUG_ME3616
   

#define ME3616_UART                     huart2 
extern UART_HandleTypeDef				ME3616_UART;

#define DBG_UART						hlpuart1
extern UART_HandleTypeDef				DBG_UART;

#define ME3616_Power_Port				POWER_ON_EN_GPIO_Port
#define ME3616_Power_Pin				POWER_ON_EN_Pin

#define ME3616_Reset_Port				NB_RST_EN_GPIO_Port
#define ME3616_Reset_Pin				NB_RST_EN_Pin

//A Buffer for ME3616, from MCU to ME3616
#define ME3616_TX_BUFFER_SIZE 			200

//A Buffer for ME3616, from ME3616 to MCU
#define ME3616_RX_BUFFER_SIZE           200

//A Buffer for DBG, from MCU to PC
#define ME3616_DBG_TX_BUFFER_SIZE 		200

//A Buffer for DBG, From PC to MCU
#define ME3616_DBG_RX_BUFFER_SIZE 		200

//Wait ME3616 boot up
#define ME3616_BOOT_TIMOUT              40000

//Wait until AT_State ready to Send
#define ME3616_SEND_TIMOUT              5000

//Wait ME3616 response OK / ERROR
#define ME3616_RECEIVE_TIMOUT           10000

//Buffer size to store IP address
#define ME3616_IPV4_SIZE                18
#define ME3616_IPV6_SIZE                42


//AT command set.
//Caution! These enum defines below, MUST Be corresponded with AT_CMD_String[] at me3616.c.
//For further AT command, refer to GOSUNCN AT Command Manual.
typedef enum {
    //模组信息识别指令
    AT_CMD_MODULE_I,						//查询模组识别信息
	AT_CMD_MODULE_GMI,                      //查询制造商名称
	AT_CMD_MODULE_CGMI,						//查询制造商名称
	AT_CMD_MODULE_GMM,                      //查询模组 ID
	AT_CMD_MODULE_CGMM,						//查询模组 ID
	AT_CMD_MODULE_GMR,                      //查询软件版本号
	AT_CMD_MODULE_CGMR,						//查询软件版本号
	AT_CMD_MODULE_GSN,                      //查询产品序列号
	AT_CMD_MODULE_CGSN,						//查询产品相应的序列标识
	AT_CMD_MODULE_CIMI,						//查询国际移动台设备标识
	AT_CMD_MODULE_ZPCBS,                    //查询 PCB 号
	
    //通用命令
    AT_CMD_COMMON_F,                        //恢复出厂设置
	AT_CMD_COMMON_V,                        //显示当前配置
	AT_CMD_COMMON_ATZ,                      //复位为缺省配置
    AT_CMD_COMMON_ATQ,                      //结果码抑制
    AT_CMD_COMMON_ATE,                      //回显命令
    AT_CMD_COMMON_ATV,                      //DCE 返回格式
    AT_CMD_COMMON_CFUN,                     //设置电话功能
    AT_CMD_COMMON_CMEE,                     //上报设备错误
    AT_CMD_COMMON_CME,                      //ERROR ME 错误结果码

    //串口控制指令
    AT_CMD_SERIAL_IPR,                      //设定串口波特率
    AT_CMD_SERIAL_CMUX,                     //串口多路复用
    AT_CMD_SERIAL_IFC,                      //DTE-DCE 的本地流控
    AT_CMD_SERIAL_ZCOMWRT,                  //串口升级写文件

    //SIM相关命令
    AT_CMD_SIM_CLCK,                        //功能锁
    AT_CMD_SIM_CPWD,                        //改变锁密码
    AT_CMD_SIM_CPIN,                        //输入 PIN 码
    AT_CMD_SIM_CRSM,                        //有限制的 SIM 访问
    AT_CMD_SIM_MICCID,                      //读取 SIM 卡的 ICCID

    //网络服务相关命令
    AT_CMD_NETWORK_CEREG,                   //EPS 网络注册状态
    AT_CMD_NETWORK_COPS,                    //PLMN 选择
    AT_CMD_NETWORK_CESQ,                    //信号强度查询
    AT_CMD_NETWORK_CSQ,                     //信号强度查询
    AT_CMD_NETWORK_CTZU,                    //自动获取网络时间开关
    AT_CMD_NETWORK_CTZR,                    //时区报告开关
    AT_CMD_NETWORK_CCLK,                    //时钟管理
    AT_CMD_NETWORK_MSPCHSC,                 //设置扰码算法
    AT_CMD_NETWORK_MFRCLLCK,                //锁频点/物理小区
    AT_CMD_NETWORK_MBAND,                   //查询当前 BAND 值
    AT_CMD_NETWORK_MBSC,                    //锁 BAND
    AT_CMD_NETWORK_MENGINFO,                //查询当前网络状态和小区信息
    AT_CMD_NETWORK_MNBIOTRAI,               //主动释放 RRC 连接

    //低功耗相关命令
    AT_CMD_LOWPOWER_CEDRXS,                 //eDRX 设置
    AT_CMD_LOWPOWER_CEDRXRDP,               //eDRX 动态参数读取
    AT_CMD_LOWPOWER_CPSMS,                  //节电模式（PSM）设置
    AT_CMD_LOWPOWER_ZSLR,                   //系统睡眠开关
    AT_CMD_LOWPOWER_SETWAKETIME,            //设置模组唤醒时间
    AT_CMD_LOWPOWER_MNBIOTEVENT,            //禁止/使能 PSM 状态主动上报
    AT_CMD_LOWPOWER_ESOWKUPDELAY,           //设置数据延时  

    //分组域命令
    AT_CMD_PDN_MCGDEFCONT,                  //设置默认的 PSD 连接设置
    AT_CMD_PDN_CGCONTRDP,                   //读取 PDP 上下文参数
    AT_CMD_PDN_IP,                          //自动拨号 IP 上报
    AT_CMD_PDN_EGACT,                       //激活/去激活 PDN 上下文

    //硬件相关及扩展AT命令
    AT_CMD_HARDWARE_ZADC,                   //读取 ADC 管脚值
    AT_CMD_HARDWARE_ZRST,                   //模组复位
    AT_CMD_HARDWARE_ZTURNOFF,               //关闭模组
    AT_CMD_HARDWARE_ZCONTLED,               //状态指示信号控制功能
    AT_CMD_HARDWARE_PWRKEYSTA,              //设置打开/关闭 POWERKEY 长拉低功能

    //域解释AT命令
    AT_CMD_DNS_EDNS,                        //通过域名获取 IP 地址

    //TCP/IP相关AT命令
    AT_CMD_TCPIP_ESOC,                      //创建一个 TCP/UDP
    AT_CMD_TCPIP_ESOCON,                    //套接字连接到远程地址和端口
    AT_CMD_TCPIP_ESOSEND,                   //发送数据
    AT_CMD_TCPIP_ESOCL,                     //关闭套接字
    AT_CMD_TCPIP_ESONMI,                    //套接字消息到达指示符
    AT_CMD_TCPIP_ESOERR,                    //套接字错误指示器
    AT_CMD_TCPIP_ESOSETRPT,                 //接收数据的显示形式
    AT_CMD_TCPIP_ESOREADEN,                 //设置数据上来主动上报
    AT_CMD_TCPIP_ESODATA,                   //数据到来主动上报
    AT_CMD_TCPIP_ESOREAD,                   //读取数据
    AT_CMD_TCPIP_ESOSENDRAW,                //发送原始数据
    AT_CMD_TCPIP_PING,                      //通过内置协议栈 ping 服务器

     //MQTT相关AT命令
    AT_CMD_MQTT_EMQNEW,                     //建立新的 MQTT
    AT_CMD_MQTT_EMQCON,                     //向 MQTT 服务器发送链接报文
    AT_CMD_MQTT_EMQDISCON,                  //断开与 MQTT 服务器的链接
    AT_CMD_MQTT_EMQSUB,                     //发送 MQTT 订阅报文
    AT_CMD_MQTT_EMQUNSUB,                   //发送 MQTT 取消订阅报文
    AT_CMD_MQTT_EMQPUB,                     //发送 MQTT 发布报文

    //CoAP相关AT命令
    AT_CMD_COAP_ECOAPSTA,                   //创建一个 COAP 服务器
    AT_CMD_COAP_ECOAPNEW,                   //创建一个 COAP 客户端
    AT_CMD_COAP_ECOAPSEND,                  //COAP 客户端发送数据
    AT_CMD_COAP_ECOAPDEL,                   //销毁 CoAP 客户端实例
    AT_CMD_COAP_ECOAPNMI,                   //返回服务器端响应    

    //HTTP/HTTPS相关命令
    AT_CMD_HTTP_EHTTPCREATE,                //创建客户端 HTTP/HTTPS 实例
    AT_CMD_HTTP_EHTTPCON,                   //建立 HTTP/HTTPS 链接
    AT_CMD_HTTP_EHTTPDISCON,                //关闭 HTTP/HTTPS 链接
    AT_CMD_HTTP_EHTTPDESTROY,               //释放创建的 HTTP/HTTPS 链接
    AT_CMD_HTTP_EHTTPSEND,                  //发送 HTTP/HTTPS 请求
    AT_CMD_HTTP_EHTTPNMIH,                  //从主机响应的头信息
    AT_CMD_HTTP_EHTTPNMIC,                  //从主机响应的内容信息
    AT_CMD_HTTP_EHTTPERR,                   //客户端连接的错误提示

    //电信 IOT 接入相关 AT 命令
    AT_CMD_LWM_M2MCLINEW,                   //LWM2M Client 注册电信 IOT 平台
    AT_CMD_LWM_M2MCLIDEL,                   //LWM2M Client 去注册电信 IOT 平台
    AT_CMD_LWM_M2MCLISEND,                  //LWM2M Client 数据发送
    AT_CMD_LWM_M2MCLIRECV,                  //LWM2M Client 数据上报
    AT_CMD_LWM_M2MCLICFG,                   //数据发送和上报模式设置

    //IPERF 带宽测试
    AT_CMD_IPERF_IPERF,                     //IPERF 带宽测试

    //FOTA 相关指令
    AT_CMD_FOTA_FOTATV,                     //设置 FOTA 升级参数
    AT_CMD_FOTA_FOTACTR,                    //启动 WeFOTA 升级
    AT_CMD_FOTA_FOTAIND,                    //WeFOTA 升级状态报告

    //FTP 相关 AT 指令
    AT_CMD_FTP_ZFTPOPEN,                    //启动文件服务
    AT_CMD_FTP_ZFTPCLOSE,                   //关闭文件服务
    AT_CMD_FTP_ZFTPSIZE,                    //获取 FTP 文件大小
    AT_CMD_FTP_ZFTPGET,                     //文件下载
    AT_CMD_FTP_ZFTPPUT,                      //文件上传命令

    //GPS 相关指令
    AT_CMD_GPS_ZGMODE,                      //设置定位模式
    AT_CMD_GPS_ZGURL,                       //设置 AGPS 服务器的 URL
    AT_CMD_GPS_ZGAUTO,                      //设置 AGNSS 数据自动下载功能
    AT_CMD_GPS_ZGDATA,                      //下载或查询 AGNSS 数据
    AT_CMD_GPS_ZGRUN,                       //开启/关闭 GPS 服务
    AT_CMD_GPS_ZGLOCK,                      //设置单次定位下使能锁定系统睡眠
    AT_CMD_GPS_ZGTMOUT,                     //设置单次定位超时时间
    AT_CMD_GPS_ZGRST,                       //重启 GPS
    AT_CMD_GPS_ZGPSR,                       //使能/禁止+ZGPSR 上报
    AT_CMD_GPS_ZGNMEA,                      //设置 GPS 数据 NMEA 上报格式    

    //中国移动 OneNET 平台接入相关 AT 命令
    AT_CMD_MIP_MIPLCREATE,                  //创建 OneNET instance
    AT_CMD_MIP_MIPLDELETE,                  //删除 OneNET instance
    AT_CMD_MIP_MIPLOPEN,                    //设备注册到 OneNET 平台
    AT_CMD_MIP_MIPLCLOSE,                   //去注册 OneNET 平台
    AT_CMD_MIP_MIPLADDOBJ,                  //创建一个 object（对象）
    AT_CMD_MIP_MIPLDELOBJ,                  //删除一个 object（对象）
    AT_CMD_MIP_MIPLUPDATE,                  //注册更新命令
    AT_CMD_MIP_MIPLREAD,                    //OneNET 平台向模组发起 read 请求
    AT_CMD_MIP_MIPLREADRSP,                 //模组响应平台的 READ 请求
    AT_CMD_MIP_MIPLWRITE,                   //OneNET 平台向模组发起 write 请求
    AT_CMD_MIP_MIPLWRITERSP,                //模组响应平台的 WRITE 请求
    AT_CMD_MIP_MIPLEXECUTE,                 //OneNET 平台向模组发起 execute 请求
    AT_CMD_MIP_MIPLEXEUTERSP,               //模组响应平台的 execute 请求
    AT_CMD_MIP_MIPLOBSERVE,                 //OneNET 平台向模组发起 observe 请求
    AT_CMD_MIP_MIPLOBSERVERSP,              //模组响应平台的 observe 请求
    AT_CMD_MIP_MIPLDISCOVER,                //OneNET 平台向模组发起 discover 请求
    AT_CMD_MIP_MIPLDISCOVERRSP,             //模组响应平台的 DISCOVER 请求
    AT_CMD_MIP_MIPLPARAMETER,               //OneNET 平台向模组发起设置 parameter 请求
    AT_CMD_MIP_MIPLPARAMETERRSP,            //模组响应平台的设置 paramete 请求
    AT_CMD_MIP_MIPLNOTIFY,                  //模组向平台请求同步数据
    AT_CMD_MIP_MIPLVER,                     //查询 OneNET SDK 版本号
    AT_CMD_MIP_MIPLEVENT,                   //模组状态上报

	AT_CMD_NONE,
    AT_CMD_IGNORE = 254
}AT_CMD_t;

typedef enum {
    AT_STATE_NONE = 0,                      //未发送过命令
    AT_STATE_SEND,                          //已发送
    AT_STATE_ATOK,                          //回复OK
    AT_STATE_TIMEOUT,                       //响应超时
    AT_STATE_ATERR,                         //回复错误
    AT_STATE_IGNORE = 254                   //忽略
}AT_State_t;

typedef enum {
	AT_BASE = 0,		//  
    AT_SET,				//  "="
    AT_READ,			//  "?"
    AT_TEST,			//  "=?"
    AT_ACTION_IGNORE = 254
}AT_Action_t;

typedef struct {
    AT_CMD_t            LastCMDBase;
    AT_Action_t         LastCMDAction; 
    AT_State_t          At_State;   
}AT_Cmd_Info_t;

typedef enum {
    SYS_STATE_POWERON =                 0x00000001,
    SYS_STATE_READY =                   0x00000002,
    SYS_STATE_BUSY =                    0x00000004,
    SYS_STATE_ERR =                     0x00000008,
    SYS_STATE_UNKNOW =                  0x00000010,

	SYS_STATE_MATREADY =                0x00000100,
	SYS_STATE_CPIN =                    0x00000200,
	SYS_STATE_CFUN =                    0x00000400,
	SYS_STATE_IPV4 =                    0x00000800,
	SYS_STATE_IPV6 =                    0x00001000,

	SYS_STATE_LWM_REGISTER_SUCCESS =	0x00010000,
	SYS_STATE_LWM_OBSERVE_SUCCESS =		0x00020000,
	SYS_STATE_LWM_NOTIFY_SUCCESS =		0x00040000,
	SYS_STATE_LWM_NEED_CMD_ACK =		0x00080000,
	
	//...add your Sys_State here

	
	SYS_STATE_INCOMMING_NEW_AT_STRING = 0x40000000
	
}SYS_State_t;

typedef struct __Me3616_DeviceType
{
	AT_Cmd_Info_t       AT_Info;                          		
    SYS_State_t        	Sys_State;

	UART_HandleTypeDef  * UartDevice;
    DMA_HandleTypeDef   * UartDMA_Tx;
    DMA_HandleTypeDef   * UartDMA_Rx;
    
	uint32_t	    	TxDataLastTime;							//SysTick time
	uint32_t 	    	RxDataLastTime;							//SysTick time

 	uint8_t				DBG_TxBuffer[ME3616_DBG_TX_BUFFER_SIZE +1];
 	uint8_t				DBG_RxBuffer[ME3616_DBG_RX_BUFFER_SIZE +1];

 	uint8_t 		    TxBuffer[ME3616_TX_BUFFER_SIZE +1];
 	uint16_t			TxStringLen;

 	uint16_t	    	RxStringBegin;
    uint16_t	    	RxStringEnd;
    uint16_t            RxStringLen;
    uint8_t             RxVaildString[ME3616_RX_BUFFER_SIZE +1];
 	uint8_t 	    	RxBuffer[ME3616_RX_BUFFER_SIZE +1];
       
	uint8_t		    	IPv4[ME3616_IPV4_SIZE];
	uint8_t		    	IPv6[ME3616_IPV6_SIZE];
    
}Me3616_DeviceType;


//ME3616 Instance Declaration
extern Me3616_DeviceType ME3616_Instance;


//Function Declaraion For me3616.h
void ME3616_ErrorHandler(char *file, int line, char * pch);

AT_State_t Get_AT_State(Me3616_DeviceType * Me3616);

AT_Action_t Get_Last_AT_Action(Me3616_DeviceType * Me3616);

AT_CMD_t Get_Last_AT_CMD(Me3616_DeviceType * Me3616);

void Set_Last_AT_CMD_None(Me3616_DeviceType * Me3616);

void Set_AT_Info(Me3616_DeviceType * Me3616, AT_CMD_t at_cmd, AT_Action_t at_action, AT_State_t at_state);

void Set_Sys_State(Me3616_DeviceType * Me3616, SYS_State_t mask);

void Clear_Sys_State(Me3616_DeviceType * Me3616, SYS_State_t mask);

bool Get_Sys_State(Me3616_DeviceType * Me3616, SYS_State_t mask);

void ME3616_PowerOn(Me3616_DeviceType * Me3616, uint32_t delay_ticks);

void ME3616_Reset(Me3616_DeviceType * Me3616, uint32_t	delay_ticks);

bool ME3616_Init(Me3616_DeviceType * Me3616, UART_HandleTypeDef * AT_huart, DMA_HandleTypeDef * DmaTx, DMA_HandleTypeDef * DmaRx);

bool ME3616_Send_AT_Command(Me3616_DeviceType * Me3616,  AT_CMD_t at_cmd, AT_Action_t at_action, bool override, char * pch);

void AT_ResultReport(Me3616_DeviceType * Me3616, bool result);

void Active_Report(Me3616_DeviceType * Me3616, char *pch, uint16_t len);

bool Wait_AT_SendReady(Me3616_DeviceType * Me3616);

bool Wait_AT_Response(Me3616_DeviceType * Me3616);

void RxHandler(Me3616_DeviceType * Me3616, char *p_Buff, uint16_t len);

void ME3616_String_Receive(Me3616_DeviceType * Me3616);

bool Check_Response(Me3616_DeviceType * Me3616, char *pch, uint16_t len);

void Hex2Str(char *sDest, const char *sSrc, int nSrcLen);

bool HexStrToByte(unsigned char* dest, const char* source, int sourceLen);

void Command_Response( Me3616_DeviceType * Me3616, char * pch, uint16_t len);
	
void MATREADY_Callback( Me3616_DeviceType * Me3616, char * pch, uint16_t len);

void CME_Callback( Me3616_DeviceType * Me3616, char * pch, uint16_t len);

void CFUN_Callback( Me3616_DeviceType * Me3616, char * pch, uint16_t len);

void CPIN_Callback( Me3616_DeviceType * Me3616, char * pch, uint16_t len);

void IP_Callback( Me3616_DeviceType * Me3616, char * pch, uint16_t len);

void ESONMI_Callback( Me3616_DeviceType * Me3616, char * pch, uint16_t len);

void ESODATA_Callback( Me3616_DeviceType * Me3616, char * pch, uint16_t len);

void EMQDISCON_Callback( Me3616_DeviceType * Me3616, char * pch, uint16_t len);

void EMQPUB_Callback( Me3616_DeviceType * Me3616, char * pch, uint16_t len);

void ECOAPNMI_Callback( Me3616_DeviceType * Me3616, char * pch, uint16_t len);

void M2MCLI_Callback( Me3616_DeviceType * Me3616, char * pch, uint16_t len);

void M2MCLIRECV_Callback( Me3616_DeviceType * Me3616, char * pch, uint16_t len);

void IPERF_Callback( Me3616_DeviceType * Me3616, char * pch, uint16_t len);

void ZGPSR_Callback( Me3616_DeviceType * Me3616, char * pch, uint16_t len);

void MIPLEVENT_Callback( Me3616_DeviceType * Me3616, char * pch, uint16_t len);

void MIPLREAD_Callback( Me3616_DeviceType * Me3616, char * pch, uint16_t len);

void MIPLWRITE_Callback( Me3616_DeviceType * Me3616, char * pch, uint16_t len);

void MIPLOBSERVE_Callback( Me3616_DeviceType * Me3616, char * pch, uint16_t len);

void MIPLDISCOVER_Callback( Me3616_DeviceType * Me3616, char * pch, uint16_t len);

void MIPLPARAMETER_Callback( Me3616_DeviceType * Me3616, char * pch, uint16_t len);

void UnknowActiveReport_Callback( Me3616_DeviceType * Me3616, char * pch, uint16_t len);


//Function Declaraion For me3616_if.h
void ME3616_IF_ErrorHandler(char *file, int line, char * pch);

void Init_UART_CM(UART_HandleTypeDef * huart);

bool UART_AT_Send(Me3616_DeviceType * Me3616);

void UART_AT_Receive(Me3616_DeviceType * Me3616);

void DBG_Forward(Me3616_DeviceType * Me3616);


//Function Declaraion For me3616_app.h
void ME3616_APP_ErrorHandler(char *file, int line, char * pch);
void ME3616_APP(Me3616_DeviceType * Me3616);


#ifdef DEBUG_ME3616

typedef enum {
    DBG_DIR_RX = 0,
    DBG_DIR_TX,
    DBG_DIR_AT,
    DBG_DIR_SDK,
    DBG_DIR_APP
}DBG_DIR_t;	
	
void DBG_Print(const char * ch, DBG_DIR_t direction);
void State_Hex2Str(char *sDest, uint32_t uSrc);

#else
#define DBG_Print(...)    UNUSED(0)
#endif


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ME3616_H__ */
