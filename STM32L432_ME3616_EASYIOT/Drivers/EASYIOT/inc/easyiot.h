/*************************************************************************
    > File Name:    easyiot.h
    > Author:       Guangdong Research Institute of China Telecom Corporation Ltd.
	> See More:     https://www.easy-iot.cn/
	> Description:  EasyIoT SDK 
    > Created Time: 2018/01/01
 ************************************************************************/

#ifndef _GUARD_H_EASYIOT_H_
#define _GUARD_H_EASYIOT_H_

#include <stdint.h>

#define EASY_IOT_VERSION "0.0.1"


enum TlvValueType {
	TLV_TYPE_BYTE=0x01,
	TLV_TYPE_SHORT,
	TLV_TYPE_INT32,
	TLV_TYPE_LONG64,
	TLV_TYPE_FLOAT,
	TLV_TYPE_DOUBLE,
	TLV_TYPE_BOOL,
	TLV_TYPE_ENUM,
	TLV_TYPE_STRING_ISO_8859,
	TLV_TYPE_STRING_HEX,
	TLV_TYPE_UNKNOWN,
};

enum CoapMessageType {
	CMT_USER_UP = 0xF0,
	CMT_USER_UP_ACK = 0xF1,
	CMT_USER_CMD_REQ = 0xF2,
	CMT_USER_CMD_RSP = 0xF3,
	CMT_SYS_CONF_REQ = 0xF4,
	CMT_SYS_CONF_RSP = 0xF5,
	CMT_SYS_QUERY_REQ = 0xF6,
	CMT_SYS_QUERY_RSP = 0xF7
};

/*
TLV 结构体
*/
struct TLV {
	uint16_t length;
	uint8_t type;
	uint8_t vformat;
	uint8_t *value;
};

/*
* Messages，当下最多允许有 5个 TLV
*/
#define MESSAGE_MAX_TLV 5
struct Messages {
    
    /* 静态内存分配区，在 MessageMalloc函数中，实现了一个简单的静态内存分配机制，即只分配，不释放。 */
	uint8_t * sbuf;
	// dtag_mid，在上行数据中使用 dtag 语义，用于在一个较短的时间内，去除重复上行的数据
	// 在下行指令中，取其mid语义，上行的指令响应必须使用同样的mid值，用以关联指令的执行结果。
	uint16_t dtag_mid;

	// msgid，消息ID，即为整个消息的Type，在EasyIoT平台定义消息时，此值会被定义
	// 在导出的头文件中，亦有此值的宏定义，代码中建议使用宏定义。
	uint8_t msgid;

	// 当前Message对象中的TLV个数总和
	uint8_t tlv_count;

	
    
    // 内存buffer起始地址，一旦赋值，将不再变动，具体内存使用，由 sbuf_offset 值确定
	uint16_t sbuf_offset;
	// 原始 sbuf 的最大长度
	uint16_t sbuf_maxlength;
	// 是否使用静态内存分配机制
	uint8_t sbuf_use;
	/* 静态内存分配区 */

	// enum TlvValueType 值，用于区分是何种类型数据
	uint8_t msgType;

	// TLV对象指针数组
	struct TLV* tlvs[MESSAGE_MAX_TLV];
};

enum LoggingLevel {
	LOG_TRACE,
	LOG_DEBUG,
	LOG_INFO,
	LOG_WARNING,
	LOG_ERROR,
	LOG_FATAL
};

struct TLV* NewTLV(uint8_t type);
void FreeTLV(struct TLV* tlv);

int TLVSerialize(struct TLV* tlv, char* inBuf, uint16_t inMaxLength);

struct Messages* NewMessage(void);
/* static version */
struct Messages* NewMessageStatic(uint8_t* buf, uint16_t inMaxLength);
struct TLV* NewTLVStatic(struct Messages* msg, uint8_t type);

void FreeMessage(struct Messages* msg);
void setMessages(struct Messages* msg, enum CoapMessageType type, uint8_t msgid);

// 序列化与反序列化
int MessagesSerialize(const struct Messages* msg, char* inBuf, uint16_t inMaxLength);
int MessagesDeserialize(const char* inBuf, uint16_t inLength, struct Messages* out);

/* Coap 输入操作函数 */
int CoapInput(struct Messages* msg, uint8_t *data, uint16_t inLength);
int CoapHexInput(const char* data);
int CoapHexInputStatic(const char* data, uint8_t* inBuf, uint16_t inMaxLength);
/*
* 数据上送函数，上送EasyIoT 平台定义的 Messages
* 最终内部会调用 CoapOutput 输出指令到硬件
*/
int pushMessages(struct Messages *msg);

/* 将数据加入message结构体 */
int AddInt8(struct Messages* msg, uint8_t type, int8_t v);
int AddInt16(struct Messages* msg, uint8_t type, int16_t v);
int AddInt32(struct Messages* msg, uint8_t type, int32_t v);
int AddFloat(struct Messages* msg, uint8_t type, float v);
int AddDouble(struct Messages* msg, uint8_t type, double v);
int AddString(struct Messages* msg, uint8_t type, const char* v);
int AddBinary(struct Messages* msg, uint8_t type, const char* v, uint16_t length);

/*
 * 从已反序列化的结构体中获取值
 * 但是 TLV 结构体中，vformat 应为 UNKNOWN，即value字段是尚未被反序列化的
 * 如果不是UNKNOWN的，那 vformat 应该匹配；
 */
int GetInt8(const struct Messages* msg, uint8_t type, int8_t* v);
int GetInt16(const struct Messages* msg, uint8_t type, int16_t* v);
int GetInt32(const struct Messages* msg, uint8_t type, int32_t* v);
int GetLong64(const struct Messages* msg, uint8_t type, int64_t* v);
int GetFloat(const struct Messages* msg, uint8_t type, float* v);
int GetDouble(const struct Messages* msg, uint8_t type, double* v);
int GetString(const struct Messages* msg, uint8_t type, char** v);
int GetBinary(const struct Messages* msg, uint8_t type, uint8_t** v);

/* EasyIoT Initialize */
void EasyIotInit(const char* imei, const char* imsi);

typedef uint64_t(*TimestampCbFuncPtr)(void);
typedef uint8_t(*BatteryCbFuncPtr)(void);
typedef int32_t(*SignalCbFuncPtr)(void);
typedef void(*OutputFuncPtr)(const uint8_t* buf, uint16_t length);
typedef void(*CmdHandlerFuncPtr)(struct Messages* req);

void setsTimestampCb(TimestampCbFuncPtr func);
void setSignalCb(SignalCbFuncPtr func);
void setBatteryCb(BatteryCbFuncPtr func);
void setNbSerialOutputCb(OutputFuncPtr func);
void setLogSerialOutputCb(OutputFuncPtr func);
void setAckHandler(CmdHandlerFuncPtr func);
int setCmdHandler(int cmdid, CmdHandlerFuncPtr func);


/* 底层接口 输出CoAP 数据流，不同的模组，需要使用不同的PORTING */
int CoapOutput(uint8_t *inBuf, uint16_t inLength);

void SetLogLevel(enum LoggingLevel level);
int Logging(enum LoggingLevel level, const char* fmt, ...);

#endif /* _GUARD_H_EASYIOT_H_ */
