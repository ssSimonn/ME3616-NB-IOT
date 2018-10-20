/*
 * Copyright (C) 2017-, ChinaTelecom. Ltd.
 * FileName:     TestDevice_example.h
 * Description:  EasyIoT 产品定义头文件
 * Created:      2018.10.16
 * Notice:       注意, 请不要随意编辑此文件，且在 EasyIoT 产品中心更新了产品定义后，
 * 此文件需要重新导出，旧有的设备消息定义极有可能导致您无法收到新的设备消息或指令
 */

#ifndef _GUARD_H_TestDevice
#define _GUARD_H_TestDevice

//#include "tlv.h"   // tlv.h 中包含了所有可用数据类型的字节序转换函数
#include <stdint.h>

/*
 * 传感器 Sensor_1
 * 如下为所有您在 EasyIoT产品中心定义的传感器，每个传感器为一个 struct类型
 * 本struct为 TLV (Type-Length-Value) 结构，具体定义请参考 EasyIoT文档中心
 * 《开发板套件 => 终端接口协议》 有详细介绍
 * SENSOR_1_TLV_PARAMID为此传感器的 Type值，请勿修改
 */
#define SENSOR_1_TLV_PARAMID 1
typedef __packed struct {
  int8_t Sensor_1_t;   // Sensor_1 type
  int16_t Sensor_1_l;   // Sensor_1 length
  int8_t Sensor_1_v;   // Sensor_1 value
} Sensor_1_tlv;      // 传感器 传感器_1

/*
 * 传感器 Sensor_2
 * 如下为所有您在 EasyIoT产品中心定义的传感器，每个传感器为一个 struct类型
 * 本struct为 TLV (Type-Length-Value) 结构，具体定义请参考 EasyIoT文档中心
 * 《开发板套件 => 终端接口协议》 有详细介绍
 * SENSOR_2_TLV_PARAMID为此传感器的 Type值，请勿修改
 */
#define SENSOR_2_TLV_PARAMID 2
typedef __packed struct {
  int8_t Sensor_2_t;   // Sensor_2 type
  int16_t Sensor_2_l;   // Sensor_2 length
  int32_t Sensor_2_v;   // Sensor_2 value
} Sensor_2_tlv;      // 传感器 传感器_2

/*
 * 传感器 Led_Green
 * 如下为所有您在 EasyIoT产品中心定义的传感器，每个传感器为一个 struct类型
 * 本struct为 TLV (Type-Length-Value) 结构，具体定义请参考 EasyIoT文档中心
 * 《开发板套件 => 终端接口协议》 有详细介绍
 * LED_GREEN_TLV_PARAMID为此传感器的 Type值，请勿修改
 */
#define LED_GREEN_TLV_PARAMID 3
typedef __packed struct {
  int8_t Led_Green_t;   // Led_Green type
  int16_t Led_Green_l;   // Led_Green length
  int8_t Led_Green_v;   // Led_Green value
} Led_Green_tlv;      // 传感器 指示灯

/*
 * 传感器 result
 * 如下为所有您在 EasyIoT产品中心定义的传感器，每个传感器为一个 struct类型
 * 本struct为 TLV (Type-Length-Value) 结构，具体定义请参考 EasyIoT文档中心
 * 《开发板套件 => 终端接口协议》 有详细介绍
 * RESULT_TLV_PARAMID为此传感器的 Type值，请勿修改
 */
#define RESULT_TLV_PARAMID 0
typedef __packed struct {
  int8_t result_t;   // result type
  int16_t result_l;   // result length
  int8_t result_v;   // result value
} result_tlv;      // 传感器 执行结果

/*
 * 消息：msg_1
 * 在 EasyIoT产品中心定义的设备消息
 * 其中每个字段为您选择的待上报的传感器值列表
 * 首字段 _msgid为消息唯一标识符
 * MSG_1_MSGID为消息的标识符值，请勿修改
 */
#define MSG_1_MSGID 1
typedef __packed struct {
  int8_t _msgid;
  int16_t _length;
  char* value;
} msg_1_Message;   // This is a test message.

/*
 * Function:     msg_1_collector
 * Description:  上报数据采集函数msg_1_collector，在每个上报周期时，固件调用此函数，
 *               采集数据完成后，数据将自动被上送到平台端。
 *               在此函数中采集需要的数据，数据格式需要与EasyIoT产品msg_1的定义符合。
 * Input:        pDeviceMsgBuf，输入缓冲区，所有采集的数据置于此缓冲区中并返回，pDeviceMsgBuf可强转为msg_1_Message类型并使用
 *               inMaxLength，输入缓冲区最大可写长度
 * Output:       pDeviceMsgBuf，传感器数据输出
 * Return:       int，返回0或-1，0成功，-1将放弃本次数据上报
 * Notice:       一般地，我们返回 sizeof(msg_1_Message)，表示完成本次采集。
 */
int msg_1_collector(char* pDeviceMsgBuf, uint16_t inMaxLength);

/* 
 * 命令请求：CMD_1
 * 在 EasyIoT产品中心定义的设备指令，CMD_1_Command是指令的请求 Request类型
 * 其中每个字段为您选择的待配置的传感器列表，设备需受理此消息，并读取传感器值
 * 首字段 _cmdid为消息唯一标识符
 */
#define CMD_1_CMDID 2
typedef __packed struct {
  int8_t _cmdid;
  int16_t _length;
  char* cmd;
} CMD_1_Command;   // This is a test CMD

/* 
 * 命令响应：CMD_1
 * 在 EasyIoT产品中心定义的设备指令，CMD_1_Response是指令的响应 Response类型
 * 首字段 _cmdid为响应消息唯一标识符
 */
typedef __packed struct {
  int8_t _cmdid;
  int16_t _length;
  int8_t result_;
  char* rsp;
} CMD_1_Response;   // This is a test CMD

/* 
 * Function:     onCMD_CMD_1_HANDLER
 * Description:  指令响应处理函数，用于响应平台定义的指令CMD_1，并返回
 * Input:        pCmdRequestBuf，输入缓冲区，所有请求命令的传感器值都置于此处，pCmdRequestBuf可强转为CMD_1_Command类型并使用
 *               inMaxLength，输入缓冲区长度
 *               outMaxLength，输出缓冲区最大可用长度
 * Output:       pCmdResponseBuf，输出缓冲区，所有响应的字段或值均置于此处，pCmdResponseBuf可强转为 CMD_1_Response类型并使用
 * Return:       int，返回0或-1，0成功，-1将放弃本次指令处理过程
 * Notice:       一般地，我们返回 sizeof(CMD_1_Response)，表示完成本次采集。
 */
int onCMD_CMD_1_HANDLER(const char* pCmdRequestBuf, uint16_t inMaxLength, char* pCmdResponseBuf, uint16_t outMaxLength);

#endif
