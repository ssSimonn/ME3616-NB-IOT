/*************************************************************************
    > File Name:    easyiot.c
    > Author:       Guangdong Research Institute of China Telecom Corporation Ltd.
	> See More:     https://www.easy-iot.cn/
	> Description:  本文件代码实现了 EasyIoT 终端侧接口协议，使用 Messages 抽象
                    完成数据的上报、下行指令处理；
    > Created Time: 2018/01/01
 ************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "easyiot.h"

static char gl_imei[20];
static char gl_imsi[20];

#define COMMAND_MAX_HANDLER 8
#define STANDARD_IMEI_LENGTH 15
#define STANDARD_IMSI_LENGTH 15
#define EASYIOT_COAP_VERSION 0x01


static TimestampCbFuncPtr gl_timestampcb;
static SignalCbFuncPtr gl_signalcb;
static BatteryCbFuncPtr gl_batterycb;
static CmdHandlerFuncPtr gl_ackhandler;
static OutputFuncPtr gl_nb_out;
static OutputFuncPtr gl_log_out;
static int cmd_handler_count;
static enum LoggingLevel gl_loglevel;


typedef struct {
	int CmdID;
	CmdHandlerFuncPtr ptr;
} cmd_handler_t;
static cmd_handler_t gl_cmd_handlers[COMMAND_MAX_HANDLER];

// 前置声明区

#define nb_htons(_n)  ((uint16_t)((((_n) & 0xff) << 8) | (((_n) >> 8) & 0xff)))
#define nb_ntohs(_n)  ((uint16_t)((((_n) & 0xff) << 8) | (((_n) >> 8) & 0xff)))
#define nb_htonl(_n)  ((uint32_t)( (((_n) & 0xff) << 24) | (((_n) & 0xff00) << 8) | (((_n) >> 8)  & 0xff00) | (((_n) >> 24) & 0xff) ))
#define nb_ntohl(_n)  ((uint32_t)( (((_n) & 0xff) << 24) | (((_n) & 0xff00) << 8) | (((_n) >> 8)  & 0xff00) | (((_n) >> 24) & 0xff) ))

float host2NetFloat(float value);
double host2NetDouble(double value);
int8_t host2NetInt8(int8_t value);
int16_t host2NetInt16(int16_t value);
int32_t host2NetInt32(int32_t value);
int64_t  host2NetInt64(int64_t value);

float net2HostFloat(float value);
double net2HostDouble(double value);
int8_t net2HostInt8(int8_t value);
int16_t net2HostInt16(int16_t value);
int32_t net2HostInt32(int32_t value);
int64_t   net2HostInt64(int64_t value);

int AddBuffer(struct Messages* msg, uint8_t type, uint8_t* v, uint16_t length, uint8_t vformat);

// 初始化，使用IMEI与IMSI初始化，并将全局变量置空
void EasyIotInit(const char* imei, const char* imsi)
{
	Logging(LOG_TRACE, "EasyIoT version %s\n", EASY_IOT_VERSION);
	
	memset(gl_imei, 0, sizeof(gl_imei));
	memset(gl_imsi, 0, sizeof(gl_imsi));
	
	gl_timestampcb = NULL;
	gl_signalcb = NULL;
	gl_batterycb = NULL;
	gl_ackhandler = NULL;
	memset(gl_cmd_handlers, 0, sizeof(gl_cmd_handlers));
	cmd_handler_count = 0;
	gl_loglevel = LOG_TRACE;
	
	if (strlen(imei) != STANDARD_IMEI_LENGTH) {
		Logging(LOG_WARNING, "IMEI %s length not equal %d\n", imei, STANDARD_IMEI_LENGTH);
	} else {
		strcpy(gl_imei, imei);
	}

	if (strlen(imsi) != STANDARD_IMSI_LENGTH) {
		Logging(LOG_WARNING, "IMSI %s length not equal %d\n", imsi, STANDARD_IMSI_LENGTH);
	} else {
		strcpy(gl_imsi, imsi);
	}

	Logging(LOG_TRACE, "EasyIoT Initialize finished.\n");
}


// 设置 TIMESTAMP 回调函数
void setsTimestampCb(TimestampCbFuncPtr func)
{
	Logging(LOG_TRACE, "set timestamp callback to 0x%p\n", func);
	gl_timestampcb = func;
}


// 设置 信号强度 回调函数
void setSignalCb(SignalCbFuncPtr func)
{
	Logging(LOG_TRACE, "set signal callback to 0x%p\n", func);
	gl_signalcb = func;
}


// 设置 电池电量 回调函数
void setBatteryCb(BatteryCbFuncPtr func)
{
	Logging(LOG_TRACE, "set battery callback to 0x%p\n", func);
	gl_batterycb = func;
}


// 设置下行指令处理回调函数，指令ID在EasyIoT平台中预定义
int setCmdHandler(int cmdid, CmdHandlerFuncPtr func)
{
	Logging(LOG_TRACE, "add cmd %d handler callback to 0x%p\n", cmdid, func);
	if (cmd_handler_count >= COMMAND_MAX_HANDLER) {
		Logging(LOG_ERROR, "COMMAND_MAX_HANDLER count %d\n", COMMAND_MAX_HANDLER);
		return -1;
	}
	
	gl_cmd_handlers[cmd_handler_count].CmdID = cmdid;
	gl_cmd_handlers[cmd_handler_count].ptr = func;
	cmd_handler_count += 1;
	Logging(LOG_TRACE, "add command %d processer 0x%p, current handler count %d.\r\n", cmdid, func, cmd_handler_count);

	return 0;
}


// 设置 下行ACK 处理回调函数，成功上行数据后会有ACK确认通知
void setAckHandler(CmdHandlerFuncPtr func)
{
	Logging(LOG_TRACE, "set ack handler to 0x%p\n", func);
	gl_ackhandler = func;
}


// 设置与NB模组通讯的回调函数
void setNbSerialOutputCb(OutputFuncPtr func)
{
	Logging(LOG_TRACE, "set nb serial output function to 0x%p\n", func);
	gl_nb_out = func;
}


// 设置日志输出的回调函数，不设置将不会输出任何日志，日志可使用setLogLevel调整输出等级
void setLogSerialOutputCb(OutputFuncPtr func)
{
	Logging(LOG_TRACE, "set logging serial output function to 0x%p\n", func);
	gl_log_out = func;
}


// Message 结构体初始化方法，使用 malloc 动态分配内存
struct Messages* NewMessage(void)
{
	struct Messages* msg = malloc(sizeof(struct Messages));
	if (!msg) {
		Logging(LOG_WARNING, "new messages, malloc failed.\n");
		return NULL;
	}
	memset(msg, 0, sizeof(struct Messages));
	Logging(LOG_TRACE, "new message from malloc at 0x%p.\r\n", msg);

	return msg;
}


// 设置Messages 基本信息，如 msgid，类型
void setMessages(struct Messages* msg, enum CoapMessageType type, uint8_t msgid)
{
	Logging(LOG_TRACE, "set msg 0x%p msgid: %d, type: %d.\r\n", msg, msgid, type);

	msg->msgType = type;
	msg->msgid = msgid;
}


// 静态初始化 Message 结构体，使用此函数构造的 Message 在随后的 addtlv中，亦不会动态分配内存
struct Messages* NewMessageStatic(uint8_t* buf, uint16_t inMaxLength)
{
	struct Messages* msg;

	if (!buf) {
		Logging(LOG_WARNING, "new message from static, but buffer is null.\n");
		return NULL;
	}
	if (inMaxLength < sizeof(struct Messages)) {
		Logging(LOG_WARNING, "new message from static, but buffer too small.\n");
		return NULL;
	}

	msg = (struct Messages*)buf;
	memset(msg, 0, sizeof(struct Messages));

	msg->sbuf_use = 1;
	msg->sbuf = buf;
	msg->sbuf_offset = sizeof(struct Messages);
	msg->sbuf_maxlength = inMaxLength;
	Logging(LOG_TRACE, "new message from static buffer at 0x%p.\r\n", msg);

	return msg;
}


// 在Message的内存空间内，分配一块指定大小 n 的内存空间
// 分配的空间为 4 字节已对齐
void* MessagesStaticMalloc(struct Messages* msg, uint16_t n)
{
	void* dst;
	uint16_t align_n;

	align_n = n;
	align_n += n % 4 ? 4 - n % 4 : 0;
	if (msg->sbuf_offset + align_n > msg->sbuf_maxlength) {
		Logging(LOG_WARNING, "message static malloc, buffer overloaded, malloc failed.\n");
		return NULL;
	}

	dst = msg->sbuf + msg->sbuf_offset;
	msg->sbuf_offset += align_n;
	return dst;
}


// 释放 Message 的内存空间，如果使用 NewMessageStatic 分配，则仅仅清空指定内存区域
void FreeMessage(struct Messages* msg)
{
	int i;

	// 如果使用静态内存分配版本，放弃free
	if (msg->sbuf_use) {
		memset(msg->sbuf, 0, msg->sbuf_maxlength);
		return;
	}

	if (msg == NULL) {
		Logging(LOG_FATAL, "free messages == NULL, aborted.\n");
		return;
	}

	for (i = 0; i != msg->tlv_count; ++i) {
		if (msg->tlvs[i]) {
			FreeTLV(msg->tlvs[i]);
		} else {
			Logging(LOG_WARNING, "tlv ptr == NULL.\n");
		}
	}
	free(msg);
}


// 对齐访问 Float
typedef union FLOAT_CONV
{
	float f;
	char c[4];
}floatConv;


// 对齐访问 Double
typedef union DOUBLE_CONV
{
	double d;
	char c[8];
}doubleConv;


// 本地字节序转网络字节序 float 版本
float host2NetFloat(float value)
{
	floatConv f1, f2;
	int size = sizeof(float);
	int i = 0;
	if (4 == size)
	{
		f1.f = value;
		f2.c[0] = f1.c[3];
		f2.c[1] = f1.c[2];
		f2.c[2] = f1.c[1];
		f2.c[3] = f1.c[0];
	}
	else {
		f1.f = value;
		for (; i < size; i++) {
			f2.c[size - 1 - i] = f1.c[i];
		}
	}
	return f2.f;
}


// 本地字节序转网络字节序 double 版本
double host2NetDouble(double value)
{
	doubleConv d1, d2;
	int size = sizeof(double);
	int i = 0;
	d1.d = value;
	for (; i < size; i++) {
		d2.c[size - 1 - i] = d1.c[i];
	}
	return d2.d;
}


// 本地字节序转网络字节序 int8 版本，会处理符号/负号
int8_t host2NetInt8(int8_t value)
{
	if (value >= 0 || 1)
	{
		return value;
	}
	else
	{
		value = ((~value + 1)) | 0x80;
		return value;
	}
}



// 本地字节序转网络字节序 int16 版本，会处理符号/负号
int16_t host2NetInt16(int16_t value)
{
	if (value >= 0 || 1)
	{
		return ((uint16_t)((((value) & 0xff) << 8) | (((value) >> 8) & 0xff)));
	}
	else
	{
		value = (((~value + 1)) | 0x8000);
		return ((uint16_t)((((value) & 0xff) << 8) | (((value) >> 8) & 0xff)));
	}
}


// 本地字节序转网络字节序 int32 版本，会处理符号/负号
int32_t host2NetInt32(int32_t value)
{
	if (value >= 0 || 1)
	{
		return ((uint32_t)((((value) & 0xff) << 24) | (((value) & 0xff00) << 8) | (((value) >> 8) & 0xff00) | (((value) >> 24) & 0xff)));
	}
	else
	{
		value = (((~value + 1)) | 0x80000000);
		return ((uint32_t)((((value) & 0xff) << 24) | (((value) & 0xff00) << 8) | (((value) >> 8) & 0xff00) | (((value) >> 24) & 0xff)));
	}
}


// 本地字节序转网络字节序 int64 版本，会处理符号/负号
int64_t  host2NetInt64(int64_t value)
{
	if (value >= 0 || 1)
	{
		return (value & 0x00000000000000FF) << 56 | (value & 0x000000000000FF00) << 40 |
			(value & 0x0000000000FF0000) << 24 | (value & 0x00000000FF000000) << 8 |
			(value & 0x000000FF00000000) >> 8 | (value & 0x0000FF0000000000) >> 24 |
			(value & 0x00FF000000000000) >> 40 | (value & 0xFF00000000000000) >> 56;
	}
	else
	{
		value = (((~value + 1)) | 0x8000000000000000);
		return  (value & 0x00000000000000FF) << 56 | (value & 0x000000000000FF00) << 40 |
			(value & 0x0000000000FF0000) << 24 | (value & 0x00000000FF000000) << 8 |
			(value & 0x000000FF00000000) >> 8 | (value & 0x0000FF0000000000) >> 24 |
			(value & 0x00FF000000000000) >> 40 | (value & 0xFF00000000000000) >> 56;
	}
}


// 网络字节序转本地字节序 float 版本
float net2HostFloat(float value)
{
	floatConv f1, f2;
	int size = sizeof(float);
	int i = 0;
	if (4 == size)
	{
		f1.f = value;
		f2.c[0] = f1.c[3];
		f2.c[1] = f1.c[2];
		f2.c[2] = f1.c[1];
		f2.c[3] = f1.c[0];
	}
	else {
		f1.f = value;
		for (; i < size; i++) {
			f2.c[size - 1 - i] = f1.c[i];
		}
	}
	return f2.f;
}


// 网络字节序转本地字节序 double 版本
double net2HostDouble(double value)
{
	doubleConv d1, d2;
	int size = sizeof(double);
	int i = 0;
	d1.d = value;
	for (; i < size; i++) {
		d2.c[size - 1 - i] = d1.c[i];
	}
	return d2.d;
}


// 网络字节序转本地字节序 int8 版本，符号有关
int8_t net2HostInt8(int8_t value)
{
	if (value >= 0 || 1)
	{
		return value;
	}
	else
	{
		value = ((~value + 1)) | 0x80;
		return value;
	}
}


// 网络字节序转本地字节序 Int6 版本，符号有关
int16_t net2HostInt16(int16_t value)
{
	if (value >= 0 || 1)
	{
		return ((uint16_t)((((value) & 0xff) << 8) | (((value) >> 8) & 0xff)));
	}
	else
	{
		value = (((~value + 1)) | 0x8000);
		return ((uint16_t)((((value) & 0xff) << 8) | (((value) >> 8) & 0xff)));
	}
}


// 网络字节序转本地字节序 int32 版本，符号有关
int32_t net2HostInt32(int32_t value)
{
	if (value >= 0 || 1)
	{
		return ((uint32_t)((((value) & 0xff) << 24) | (((value) & 0xff00) << 8) | (((value) >> 8) & 0xff00) | (((value) >> 24) & 0xff)));
	}
	else
	{
		value = (((~value + 1)) | 0x80000000);
		return ((uint32_t)((((value) & 0xff) << 24) | (((value) & 0xff00) << 8) | (((value) >> 8) & 0xff00) | (((value) >> 24) & 0xff)));
	}
}


// 网络字节序转本地字节序 int64 版本，符号有关
int64_t   net2HostInt64(int64_t value)
{
	if (value >= 0 || 1)
	{
		return (value & 0x00000000000000FF) << 56 | (value & 0x000000000000FF00) << 40 |
			(value & 0x0000000000FF0000) << 24 | (value & 0x00000000FF000000) << 8 |
			(value & 0x000000FF00000000) >> 8 | (value & 0x0000FF0000000000) >> 24 |
			(value & 0x00FF000000000000) >> 40 | (value & 0xFF00000000000000) >> 56;
	}
	else
	{
		value = (((~value + 1)) | 0x8000000000000000);
		return  (value & 0x00000000000000FF) << 56 | (value & 0x000000000000FF00) << 40 |
			(value & 0x0000000000FF0000) << 24 | (value & 0x00000000FF000000) << 8 |
			(value & 0x000000FF00000000) >> 8 | (value & 0x0000FF0000000000) >> 24 |
			(value & 0x00FF000000000000) >> 40 | (value & 0xFF00000000000000) >> 56;
	}
}


// 向一个 Message 对象中，增加一个 TLV，指定 type，length，以及值指针
// vformat 为 tlv 中 value 的格式，如 int 型、byte 型等
// 未知类型取 UNKNOWN，只有在反序列化时填此值
int AddBuffer(struct Messages* msg, uint8_t type, uint8_t* v, uint16_t length, uint8_t vformat)
{
	struct TLV* tlv = NULL;
	if (msg->tlv_count >= MESSAGE_MAX_TLV) {
		Logging(LOG_WARNING, "tlv count max %d\n", MESSAGE_MAX_TLV);
		return -1;
	}

	if (msg->sbuf_use) {
		tlv = NewTLVStatic(msg, type);
	} else {
		tlv = NewTLV(type);
	}
	if (!tlv) {
		Logging(LOG_WARNING, "new tlv, malloc failed.\n");
		return -1;
	}

	if (msg->sbuf_use) {
		tlv->value = (uint8_t*)MessagesStaticMalloc(msg, length);
	}
	else {
		tlv->value = (uint8_t*)malloc(length);
	}
	if (!tlv->value) {
		Logging(LOG_WARNING, "add buffer, new tlv, malloc failed.\n");
		return -1;
	}
	tlv->vformat = vformat;
	tlv->length = length;
	memcpy(tlv->value, v, length);

	msg->tlvs[msg->tlv_count] = tlv;
	++msg->tlv_count;
	return length + 3;
}


// 向Messages对象中，新增一个 int8 格式的 TLV 对象
int AddInt8(struct Messages* msg, uint8_t type, int8_t v)
{
	return AddBuffer(msg, type, (uint8_t*)&v, sizeof(int8_t), TLV_TYPE_BYTE);
}


// 向Messages对象中，新增一个 int16 格式的 TLV 对象
int AddInt16(struct Messages* msg, uint8_t type, int16_t v)
{
	return AddBuffer(msg, type, (uint8_t*)&v, sizeof(v), TLV_TYPE_SHORT);
}


// 向Messages对象中，新增一个 int32 格式的 TLV 对象
int AddInt32(struct Messages* msg, uint8_t type, int32_t v)
{
	return AddBuffer(msg, type, (uint8_t*)&v, sizeof(v), TLV_TYPE_INT32);
}


// 向Messages对象中，新增一个 int64 格式的 TLV 对象
int AddInt64(struct Messages* msg, uint8_t type, int32_t v)
{
	return AddBuffer(msg, type, (uint8_t*)&v, sizeof(v), TLV_TYPE_LONG64);
}


// 向Messages对象中，新增一个 float 格式的 TLV 对象
int AddFloat(struct Messages* msg, uint8_t type, float v)
{
	return AddBuffer(msg, type, (uint8_t*)&v, sizeof(v), TLV_TYPE_FLOAT);
}


// 向Messages对象中，新增一个 double 格式的 TLV 对象
int AddDouble(struct Messages* msg, uint8_t type, double v)
{
	return AddBuffer(msg, type, (uint8_t*)&v, sizeof(v), TLV_TYPE_DOUBLE);
}


// 向Messages对象中，新增一个 string 格式的 TLV 对象
int AddString(struct Messages* msg, uint8_t type, const char* v)
{
	return AddBuffer(msg, type, (uint8_t*)v, (uint16_t)strlen(v), TLV_TYPE_STRING_ISO_8859);
}


// 向Messages对象中，新增一个 二进制格式内存区的 TLV 对象
int AddBinary(struct Messages* msg, uint8_t type, const char* v, uint16_t length)
{
	return AddBuffer(msg, type, (uint8_t*)v, length, TLV_TYPE_STRING_HEX);
}


// 从一个Message对象中，获取指定type值的TLV结构体，并验证 vformat 是否正确
// 即如果获取一个 int8 的tlv，但实际上内容并不是 int8，则返回NULL
// 若msg对象中无指定type值的tlv，也返回NULL
const struct TLV* get_tlv_from_msg(const struct Messages* msg, uint8_t type, uint16_t length, uint8_t vformat)
{
	int i;
	const struct TLV* tlv;

	for (i = 0; i != msg->tlv_count; ++i) {
		if (msg->tlvs[i]->type != type) {
			continue;
		}
		
		tlv = msg->tlvs[i];
		if (vformat == TLV_TYPE_STRING_HEX || vformat == TLV_TYPE_STRING_ISO_8859) {
			// ignore length judge.
		} else {
			if (tlv->length != length) {
				Logging(LOG_WARNING, "Get tlv %d, length %d not match %d.\n", type, tlv->length, length);
				return NULL;
			}
		}

		if (tlv->vformat != TLV_TYPE_UNKNOWN && tlv->vformat != vformat) {
			Logging(LOG_WARNING, "Get tlv %d, vformat %d not match %d.\n", type, tlv->vformat, vformat);
			return NULL;
		}

		return tlv;
	}

	return NULL;
}


// 获取 Int8 的TLV，若 vformat 为 TLV_TYPE_UNKNOWN，则表示数据是未反序列化的，会进行端序转换
int GetInt8(const struct Messages* msg, uint8_t type, int8_t* v)
{
	const struct TLV* tlv;

	tlv = get_tlv_from_msg(msg, type, sizeof(uint8_t), TLV_TYPE_BYTE);
	if (!tlv) {
		Logging(LOG_WARNING, "cannot found valid tlv %d from msg 0x%p\n", type, msg);
		return -1;
	}

	if (tlv->vformat == TLV_TYPE_UNKNOWN) {
		*v = net2HostInt8(*(int8_t*)tlv->value);
	} else {
		*v = *(int8_t*)tlv->value;
	}

	return sizeof(int8_t);
}


// 获取 Int16 的TLV
int GetInt16(const struct Messages* msg, uint8_t type, int16_t* v)
{
	const struct TLV* tlv;

	tlv = get_tlv_from_msg(msg, type, sizeof(int16_t), TLV_TYPE_SHORT);
	if (!tlv) {
		Logging(LOG_WARNING, "cannot found valid tlv %d from msg 0x%p\n", type, msg);
		return -1;
	}

	if (tlv->vformat == TLV_TYPE_UNKNOWN) {
		*v = net2HostInt16(*(int16_t*)tlv->value);
	}
	else {
		*v = *(int16_t*)tlv->value;
	}

	return sizeof(int16_t);
}


// 获取 Int32 的TLV
int GetInt32(const struct Messages* msg, uint8_t type, int32_t* v)
{
	const struct TLV* tlv;

	tlv = get_tlv_from_msg(msg, type, sizeof(int32_t), TLV_TYPE_INT32);
	if (!tlv) {
		Logging(LOG_WARNING, "cannot found valid tlv %d from msg 0x%p\n", type, msg);
		return -1;
	}

	if (tlv->vformat == TLV_TYPE_UNKNOWN) {
		*v = net2HostInt32(*(int32_t*)tlv->value);
	}
	else {
		*v = *(int32_t*)tlv->value;
	}

	return sizeof(int32_t);
}


// 获取 Int64 的TLV
int GetLong64(const struct Messages* msg, uint8_t type, int64_t* v)
{
	const struct TLV* tlv;

	tlv = get_tlv_from_msg(msg, type, sizeof(int64_t), TLV_TYPE_LONG64);
	if (!tlv) {
		Logging(LOG_WARNING, "cannot found valid tlv %d from msg 0x%p\n", type, msg);
		return -1;
	}

	if (tlv->vformat == TLV_TYPE_UNKNOWN) {
		*v = net2HostInt64(*(int64_t*)tlv->value);
	}
	else {
		*v = *(int64_t*)tlv->value;
	}

	return sizeof(int64_t);
}


// 获取 float 的TLV
int GetFloat(const struct Messages* msg, uint8_t type, float* v)
{
	const struct TLV* tlv;

	tlv = get_tlv_from_msg(msg, type, sizeof(float), TLV_TYPE_FLOAT);
	if (!tlv) {
		Logging(LOG_WARNING, "cannot found valid tlv %d from msg 0x%p\n", type, msg);
		return -1;
	}

	if (tlv->vformat == TLV_TYPE_UNKNOWN) {
		*v = net2HostFloat(*(float*)tlv->value);
	}
	else {
		*v = *(float*)tlv->value;
	}

	return sizeof(float);
}


// 获取 double 的TLV
int GetDouble(const struct Messages* msg, uint8_t type, double* v)
{
	const struct TLV* tlv;

	tlv = get_tlv_from_msg(msg, type, sizeof(double), TLV_TYPE_DOUBLE);
	if (!tlv) {
		Logging(LOG_WARNING, "cannot found valid tlv %d from msg 0x%p\n", type, msg);
		return -1;
	}

	if (tlv->vformat == TLV_TYPE_UNKNOWN) {
		*v = net2HostDouble(*(double*)tlv->value);
	}
	else {
		*v = *(double*)tlv->value;
	}

	return sizeof(double);
}


// 获取 string 的TLV
int GetString(const struct Messages* msg, uint8_t type, char** v)
{
	const struct TLV* tlv;

	tlv = get_tlv_from_msg(msg, type, 0, TLV_TYPE_STRING_ISO_8859);
	if (!tlv) {
		Logging(LOG_WARNING, "cannot found valid tlv %d from msg 0x%p\n", type, msg);
		return -1;
	}

	*v = (char*)tlv->value;
	return tlv->length;
}


// 获取 binary 二进制内存区域 的TLV
int GetBinary(const struct Messages* msg, uint8_t type, uint8_t** v)
{
	const struct TLV* tlv;

	tlv = get_tlv_from_msg(msg, type, 0, TLV_TYPE_STRING_HEX);
	if (!tlv) {
		Logging(LOG_WARNING, "cannot found valid tlv %d from msg 0x%p\n", type, msg);
		return -1;
	}

	*v = tlv->value;
	return tlv->length;
}


// malloc 新的TLV空间，并设置TLV结构体的 type 值
struct TLV* NewTLV(uint8_t type)
{
	struct TLV* tlv = malloc(sizeof(struct TLV));
	if (!tlv) {
		Logging(LOG_WARNING, "new tlv, malloc failed.\n");
		return NULL;
	}
	tlv->length = 0;
	tlv->type = type;

	return tlv;
}


// 从 Message 的剩余空间中，为 TLV 分配内存空间，不使用malloc
struct TLV* NewTLVStatic(struct Messages* msg, uint8_t type)
{
	struct TLV* tlv = MessagesStaticMalloc(msg, sizeof(struct TLV));
	if (!tlv) {
		Logging(LOG_WARNING, "new tlv static, malloc failed.\n");
		return NULL;
	}
	tlv->type = type;
	tlv->length = 0;
	tlv->value = NULL;

	return tlv;
}


// 释放TLV内存空间
void FreeTLV(struct TLV* tlv)
{
	if (!tlv) {
		Logging(LOG_WARNING, "free null tlv?\n");
		return;
	} 

	if (tlv->value) {
		free(tlv->value);
	}
	free(tlv);
}


// 内存对齐写 uint16
union u16_setter {
	uint16_t val;
	char buf[2];
};


// 使用 union 方式，以内存对齐的方式写 uint16 数据到 buffer
void alignment_u16_w(char* buf, uint16_t val)
{
	union u16_setter setter;
	setter.val = val;
	memcpy(buf, setter.buf, sizeof(setter));
}


// 内存对齐写 uint32
union u32_setter {
	uint32_t val;
	char buf[4];
};


// 使用 union 方式，以内存对齐的方式写 uint32 数据到 buffer
void alignment_u32_w(char* buf, uint32_t val)
{
	union u32_setter setter;
	setter.val = val;
	memcpy(buf, setter.buf, sizeof(setter));
}


// 内存对齐写 uint64
union u64_setter {
	uint64_t val;
	char buf[8];
};


// 使用 union 方式，以内存对齐的方式写 uint64 数据到 buffer
void alignment_u64_w(char* buf, uint64_t val)
{
	union u64_setter setter;
	setter.val = val;
	memcpy(buf, setter.buf, sizeof(setter));
}


// 使用 union 方式，以内存对齐的方式读取 uint16 并返回
uint16_t alignment_u16_r(const char* buf)
{
	union u16_setter reader;
	memcpy(reader.buf, buf, sizeof(reader));
	return reader.val;
}


// 值序列化 int8
void int8_serialize(int8_t v, char* inBuf)
{
	memcpy(inBuf, &v, sizeof(v));
}


// 值序列化 int16
void int16_serialize(int16_t v, char* inBuf)
{
	memcpy(inBuf, &v, sizeof(v));
}


// 值序列化 int32
void int32_serialize(int32_t v, char* inBuf)
{
	memcpy(inBuf, &v, sizeof(v));
}


// 值序列化 int64
void int64_serialize(int64_t v, char* inBuf)
{
	memcpy(inBuf, &v, sizeof(v));
}


// 值序列化 float
void float_serialize(float v, char* inBuf)
{
	memcpy(inBuf, &v, sizeof(v));
}


// 值序列化 double
void double_serialize(double v, char* inBuf)
{
	memcpy(inBuf, &v, sizeof(v));
}


// 值序列化，根据 tlv->vformat 选择不同的序列化函数
void value_serialize(struct TLV* tlv, char* inBuf, uint16_t inMaxLength)
{
	switch (tlv->vformat) {
	case TLV_TYPE_BYTE:
	case TLV_TYPE_ENUM:
	case TLV_TYPE_BOOL:
		int8_serialize(host2NetInt8(*(int8_t*)tlv->value), inBuf);
		break;
	case TLV_TYPE_SHORT:
		int16_serialize(host2NetInt16(*(int16_t*)tlv->value), inBuf);
		break;
	case TLV_TYPE_INT32:
		int32_serialize(host2NetInt32(*(int32_t*)tlv->value), inBuf);
		break;
	case TLV_TYPE_LONG64:
		int64_serialize(host2NetInt64(*(int64_t*)tlv->value), inBuf);
		break;
	case TLV_TYPE_FLOAT:
		float_serialize(host2NetFloat(*(float*)tlv->value), inBuf);
		break;
	case TLV_TYPE_DOUBLE:
		double_serialize(host2NetDouble(*(double*)tlv->value), inBuf);
		break;
	default:
		memcpy(inBuf, tlv->value, tlv->length);
		break;
	}
}


// 成功返回序列化后的长度，失败返回-1
int TLVSerialize(struct TLV* tlv, char* inBuf, uint16_t inMaxLength)
{
	if (tlv == NULL) {
		Logging(LOG_WARNING, "tlv serialize failed, tlv == NULL.\n");
		return -1;
	}

	if (inBuf == NULL) {
		Logging(LOG_WARNING, "tlv serialize failed, inBuf == NULL.\n");
		return -1;
	}
	if (inMaxLength < tlv->length + 3) {
		Logging(LOG_TRACE, "tlv serialize buffer is too small, aborted.\n");
		return -1;
	}
	inBuf[0] = tlv->type;
	alignment_u16_w(inBuf + 1, nb_htons(tlv->length));

	value_serialize(tlv, inBuf + 3, inMaxLength - 3);

	return tlv->length + 3;
}


// 序列化 Message 对象的 data 部分
int SerializeBody(const struct Messages* msg, char* inBuf, uint16_t inMaxLength)
{
	int i, ret, pos, length;
	inBuf[0] = msg->msgid;

	length = 0;
	pos = 3;
	for (i = 0; i != msg->tlv_count; ++i) {
		length += msg->tlvs[i]->length + 3;
		ret = TLVSerialize(msg->tlvs[i], inBuf + pos, inMaxLength - pos);
		if (ret < 0) {
			Logging(LOG_WARNING, "serialize tlv failed. \n");
			return -1;
		}
		pos += ret;
	}

	alignment_u16_w(inBuf + 1, nb_htons(length));
	return 3 + length;
}


// checksum 计算，累加和
uint8_t CalcCheckSum(const char* buf, uint16_t length)
{
	uint16_t i;
	uint32_t sum = 0;

	for (i = 0; i != length; ++i) {
		sum += (uint8_t)(buf[i]);
	}
	return sum % 256;
}


/*
用户指令响应报文序列化
1，计算包体总长度
2，逐个序列化tlv，并填充
*/
int UserCmdRspMsgSerialize(const struct Messages* msg, char* inBuf, uint16_t inMaxLength)
{
	int i, pos, packet_length;

	// 入参判断
	if (msg == NULL) {
		Logging(LOG_WARNING, "tlv messages msg == NULL.\n");
		return -1;
	}
	if (inBuf == NULL) {
		Logging(LOG_WARNING, "serialize buffer == NULL.\n");
		return -1;
	}

	// 除去data后的长度
	// 1	1	2	2	1 == 7
	packet_length = 7;
	for (i = 0; i != msg->tlv_count; ++i) {
		// 每个 TLV 的头，有3字节
		packet_length += msg->tlvs[i]->length + 3;
	}
	// data 区域也是一个很大的tlv，其t + l 长度为3
	packet_length += 3;
	if (inMaxLength < packet_length) {
		Logging(LOG_WARNING, "Messages serialize buffer too small.\n");
		return -1;
	}
	// 开始序列化，首先是公共部分
	inBuf[0] = EASYIOT_COAP_VERSION;
	inBuf[1] = CMT_USER_CMD_RSP;
	pos = 2;

	// 长度位，字段 4 ~ 11，为总长度 - 4
	alignment_u16_w(inBuf + pos, nb_htons(packet_length - 4));
	pos += 2;

	// mid
	alignment_u16_w(inBuf + pos, msg->dtag_mid);
	pos += 2;

	// 然后序列化 data 部分
	i = SerializeBody(msg, inBuf + pos, inMaxLength - pos);
	if (i < 0) {
		Logging(LOG_WARNING, "messages body serialize failed.\n");
		return -1;
	}
	pos += i;

	// check sum.
	inBuf[pos] = CalcCheckSum(inBuf, pos);

	return pos + 1;
}


/*
用户数据上报的数据报文序列化
1，计算包体总长度
2，逐个序列化tlv，并填充
*/
int UserUpMsgSerialize(const struct Messages* msg, char* inBuf, uint16_t inMaxLength)
{
	int i, pos, packet_length;
	uint8_t battery;
	int32_t signal;
	uint64_t timestamp;

	// 入参判断
	if (msg == NULL) {
		Logging(LOG_WARNING, "tlv messages msg == NULL.\n");
		return -1;
	}
	if (inBuf == NULL) {
		Logging(LOG_WARNING, "serialize buffer == NULL.\n");
		return -1;
	}

	// 除去data后的长度
	// 1	1	2	2	1	4	15	15	8	1 == 50
	packet_length = 50;
	for (i = 0; i != msg->tlv_count; ++i) {
		// 每个 TLV 的头，有3字节
		packet_length += msg->tlvs[i]->length + 3;
	}
	// data 区域也是一个很大的tlv，其t + l 长度为3
	packet_length += 3;
	if (inMaxLength < packet_length) {
		Logging(LOG_WARNING, "Messages serialize buffer too small.\n");
		return -1;
	}

	// 获得 电池电量 与信号强度，时间戳
	if (gl_timestampcb) {
		timestamp = gl_timestampcb();
	} else {
		Logging(LOG_WARNING, "timestamp callback empty, ignore and set to 0.\n");
		timestamp = 0;
	}
	if (gl_signalcb) {
		signal = gl_signalcb();
	} else {
		Logging(LOG_WARNING, "signal strength callback empty, ignore and set to 0.\n");
		signal = 0;
	}
	if (gl_batterycb) {
		battery = gl_batterycb();
	} else {
		Logging(LOG_WARNING, "battery status callback empty, ignore and set to 0.\n");
		battery = 0;
	}
	if (battery > 100) {
		Logging(LOG_WARNING, "battery level lager than 100, set to 100.\n");
		battery = 100;
	}

	// 开始序列化，首先是公共部分
	inBuf[0] = EASYIOT_COAP_VERSION;
	inBuf[1] = CMT_USER_UP;
	pos = 2;

	// 长度位，字段 4 ~ 11，为总长度 - 4
	alignment_u16_w(inBuf + pos, nb_htons(packet_length - 4));
	pos += sizeof(uint16_t);

	// dtag.
	alignment_u16_w(inBuf + pos, msg->dtag_mid);
	pos += sizeof(uint16_t);

	// battery.
	inBuf[pos] = battery;
	pos += sizeof(uint8_t);

	// signal strength
	alignment_u32_w(inBuf + pos, host2NetInt32(signal));
	pos += sizeof(uint32_t);

	// imei && imsi;
	memcpy(inBuf + pos, gl_imei, 15);
	pos += 15;
	memcpy(inBuf + pos, gl_imsi, 15);
	pos += 15;

	// timestamp
	alignment_u64_w(inBuf + pos, host2NetInt64(timestamp));
	pos += sizeof(uint64_t);

	// 然后序列化 data 部分
	i = SerializeBody(msg, inBuf + pos, inMaxLength - pos);
	if (i < 0) {
		Logging(LOG_WARNING, "messages body serialize failed.\n");
		return -1;
	}
	pos += i;

	// check sum.
	inBuf[pos] = CalcCheckSum(inBuf, pos);

	//确保字符串以0结尾。
	inBuf[pos + 1] = '\0';
	
	return pos + 1;
}


/*
数据反序列化，反序列化Message的data部分
1，逐个抽出TLV结构体
条件： inLength >= 3
length + 3 >= inLength
*/
int MessageDeserializeBodyData(const char* inBuf, uint16_t inLength, struct Messages* out)
{
	uint8_t type;
	uint16_t length;

	if (inLength < 3) {
		Logging(LOG_WARNING, "message deserialize body data failed, too small.\n");
		return -1;
	}

	type = inBuf[0];
	length = nb_htons(alignment_u16_r(inBuf + 1));
	if (length + 3 > inLength) {
		Logging(LOG_WARNING, "message deserialize body data failed, length not match.\n");
		return -1;
	}

	AddBuffer(out, type, (uint8_t*)(inBuf + 3), length, TLV_TYPE_UNKNOWN);

	// 返回用掉了多少字节
	return length + 3;
}


// ACK的反序列化，未实现
int UserUpAckMsgDeserialize(const char* inBuf, uint16_t inLength, struct Messages* out)
{
	return 0;
}


// 用户指令的反序列化
int UserCmdReqMsgDeserialize(const char* inBuf, uint16_t inLength, struct Messages* out)
{
	int rsp, pos, left;
	uint16_t length;
	// dtag / mid 不需要进行大小端转换
	out->dtag_mid = alignment_u16_r(inBuf + 4);

	/*
	2，检查 data 里的tlv 里的 length 是否正确
	3，逐个反序列化 tlv
	*/
	out->msgid = inBuf[6];
	// 检查length是否正确, 7 是 指令body区length的偏移，详见文档
	length = nb_htons(alignment_u16_r(inBuf + 7));
	// 10 是 整个长度中，除去指令tlv区域外，无关字节数
	if (length != inLength - 10) {
		Logging(LOG_WARNING, "tlv body deserialize failed, length not match.\n");
		return -1;
	}

	// 逐个反序列化
	pos = 9;
	do {
		// left == inLength - pos - 1, -1 是因为要去除 checksum
		rsp = MessageDeserializeBodyData(inBuf + pos, inLength - pos - 1, out);
		if (rsp < 0) {
			Logging(LOG_WARNING, "message deserialize body data failed, left %d\n", rsp);
			return -1;
		}
		pos += rsp;
		left = inLength - pos - 1;
	} while (left > 0);

	return rsp;
}


/*
   数据反序列化
1，版本号是否匹配
2，校验和是否正确
3，长度是否匹配
4，按类型进行反序列化
*/
int MessagesDeserialize(const char* inBuf, uint16_t inLength, struct Messages* out)
{
	uint8_t version, checksum, rchecksum;
	uint16_t packet_length;
	int rsp;

	rsp = -1;
	Logging(LOG_TRACE, "prepare deserialize buffer 0x%p, length: %d\n", inBuf, inLength);

	version = inBuf[0];
	if (version != EASYIOT_COAP_VERSION) {
		Logging(LOG_WARNING, "message deserialize failed, version not match.");
		return -1;
	}

	checksum = CalcCheckSum(inBuf, inLength - 1);
	rchecksum = inBuf[inLength - 1];
	if (checksum != rchecksum) {
		Logging(LOG_WARNING, "message checksum failed, expected [%d], but get [%d].\n", checksum, inBuf[inLength - 1]);
		return -1;
	}

	packet_length = nb_htons(alignment_u16_r(inBuf + 2));
	if (packet_length + 4 != inLength) {
		Logging(LOG_WARNING, "packet length not match.\n");
		return -1;
	}

	out->msgType = ((unsigned char*)inBuf)[1];
	switch (out->msgType) {
	case CMT_USER_CMD_REQ:
		rsp = UserCmdReqMsgDeserialize(inBuf, inLength, out);
		break;
	case CMT_SYS_CONF_REQ:
		break;
	case CMT_USER_UP_ACK:
		rsp = UserUpAckMsgDeserialize(inBuf, inLength, out);
		break;
	case CMT_SYS_QUERY_REQ:
		break;
	default:
		break;
	}

	return rsp;
}


// Message 对象序列化
int MessagesSerialize(const struct Messages* msg, char* inBuf, uint16_t inMaxLength)
{
	int rsp;

	rsp = -1;
	Logging(LOG_TRACE, "prepare serialize message 0x%p, sensor count: %d\n", msg, msg->tlv_count);
	switch (msg->msgType) {
	case CMT_USER_UP:
		Logging(LOG_TRACE, "message type is CMT_USER_UP.\r\n");
		rsp = UserUpMsgSerialize(msg, inBuf, inMaxLength);
		break;
	case CMT_USER_CMD_RSP:
		Logging(LOG_TRACE, "message type is CMT_USER_CMD_RSP.\r\n");
		rsp = UserCmdRspMsgSerialize(msg, inBuf, inMaxLength);
		break;
	default:
		Logging(LOG_WARNING, "unknown message type.\n");
		break;
	}

	return rsp;
}


// 直接预声明一个较大的缓冲区，用于数据序列化与Coap数据发送
int pushMessageStackedBuffer(struct Messages* msg)
{
	// 序列化
	int rsp, length;
	char buf[512];

	length = MessagesSerialize(msg, buf, sizeof(buf));
	if (length < 0) {
		Logging(LOG_WARNING, "message serialize failed.\n");
		return -1;
	}

	// 发送出去
	rsp = CoapOutput((uint8_t*)buf, length);
	if (rsp < 0) {
		Logging(LOG_WARNING, "coap output failed.\n");
		return -1;
	}
	return -1;
}


// 使用Message对象内的剩余内存空间，完成数据序列化与CoAP 数据发送
// 完成后返还这部分内存空间，即 msg->sbuf_offset 变量并不新增；
int pushMessageStatic(struct Messages* msg)
{
	int rsp, length, left;
	char* buf;

	buf = (char*)msg->sbuf + msg->sbuf_offset;
	left = msg->sbuf_maxlength - msg->sbuf_offset;
	length = MessagesSerialize(msg, buf, left);
	if (length < 0) {
		Logging(LOG_WARNING, "message serialize failed.\n");
		return -1;
	}

	// 发送出去
	rsp = CoapOutput((uint8_t*)buf, length);
	if (rsp < 0) {
		Logging(LOG_WARNING, "coap output failed.\n");
		return -1;
	}

	memset(buf, 0, left);
	return -1;
}


// 将Message对象上送到EasyIoT平台
int pushMessages(struct Messages *msg)
{
	if (msg->sbuf_use) {
		Logging(LOG_TRACE, "message using static buffer, using static push messages.\r\n");
		return pushMessageStatic(msg);
	} else {
		return pushMessageStackedBuffer(msg);
	}
}


/*
   CoAP数据输入后的流程
1，反序列化
2，根据input的类型，对应调用回调；
*/
int CoapInput(struct Messages* msg, uint8_t *data, uint16_t inLength)
{
	int ret, i, found;
	CmdHandlerFuncPtr ptr;
	
	ret = MessagesDeserialize((const char*)data, inLength, msg);
	if (ret < 0) {
		Logging(LOG_WARNING, "message deserialize failed.\n");
		return -1;
	}
	Logging(LOG_INFO, "message deserialize succ, tlv count: %d\n", msg->tlv_count);

	switch (msg->msgType) {
	case CMT_USER_UP_ACK: {
		Logging(LOG_INFO, "recv ack.\n");
		if (!gl_ackhandler) {
			Logging(LOG_INFO, "ack handler == null, ignore ack.\r\n");
		} else {
			Logging(LOG_INFO, "found ack handler at 0x%p, process it.\r\n", gl_ackhandler);
			gl_ackhandler(msg);
		}
		break;
	}
	case CMT_USER_CMD_REQ: {
		Logging(LOG_INFO, "recv cmd.\n");
		// 根据cmdid进行分发handler
		found = 0;
		for (i = 0; i != cmd_handler_count; ++i){
			if (msg->msgid == gl_cmd_handlers[i].CmdID) {
				found = 1;
				ptr = gl_cmd_handlers[i].ptr;
				if (!ptr) {
					Logging(LOG_WARNING, "msg %d cmd handler is null.\n", msg->msgid);
				} else {
					Logging(LOG_INFO, "found cmdid %d handler at 0x%p, process it.\r\n", msg->msgid, ptr);
					ptr(msg);
					break;
				}
			}
		}
		if (!found) {
			Logging(LOG_WARNING, "cannot found cmdhandler for %d \n", msg->msgid);
		}
		break;
	}
	default:
		break;
	}

	FreeMessage(msg);
	return ret;
}


// CoAP 数据输出，此处的实现为 BC95 ，其他模组需要根据AT指令集做对应修改
int CoapOutput(uint8_t *inBuf, uint16_t inLength)
{
//	int i, all_length;
//	char headbuf[16];

	if (!gl_nb_out) {
		Logging(LOG_WARNING, "nb serial output cb is null, pls use setNbSerialOutputCb set it.\r\n");
		return -1;
	}

//	memset(headbuf, 0, sizeof(headbuf));
//	sprintf(headbuf, "AT+NMGS=%d,", inLength);
//	all_length = strlen(headbuf);

	gl_nb_out((uint8_t*)inBuf, inLength);

//	for (i = 0; i != inLength; ++i) {
//		sprintf(headbuf, "%02X", inBuf[i]);
//		gl_nb_out((uint8_t*)headbuf, 2);
//		all_length += 2;
//	}
//
//	gl_nb_out((uint8_t*)"\r\n", 2);
//	return all_length + 2;
    return inLength;
}


// 设置日志输出等级，低于此等级的将会忽略
void SetLogLevel(enum LoggingLevel level)
{
	gl_loglevel = level;
}


// 日志输出，需要使用 stdarg 中的函数，若不需要，可去除
int Logging(enum LoggingLevel level, const char* fmt, ...)
{
	int ret;
	char buf[128];

	va_list arg_ptr;
	va_start(arg_ptr, fmt);
	if (level >= gl_loglevel) {
		ret = vsprintf(buf, fmt, arg_ptr);
	}
	va_end(arg_ptr);

	if (ret > 0 && gl_log_out) {
		gl_log_out((uint8_t*)buf, ret);
	}
	return -1;
}


// ascii2binary，即将每两字节ASCII表示的HEX数据，转换到1字节的二进制内存byte数据，如2字节的字符串"AA"，将转换到1字节的byte数据 0xAA；
int a2b_hex(const char* s, char* out, int inMaxLength)
{
	int i, pos;
	int length = strlen(s);

	if (length % 2) {
		Logging(LOG_WARNING, "input hex data must be even\n");
		return -1;
	}
	if (inMaxLength < length / 2) {
		Logging(LOG_WARNING, "input buffer too small.\n");
		return -1;
	}

	pos = 0;
	for (i = 0; i < length - 1; i += 2) {
		out[pos++] = (s[i] >= 'A' ? s[i] - 'A' + 10 : s[i] - '0') * 16 + (s[i + 1] >= 'A' ? s[i + 1] - 'A' + 10 : s[i + 1] - '0');
	}

	return length / 2;
}


// ASCII HEX格式的CoAP数据输入处理，首先调用 a2b_hex ，然后直接 CoapInput
int CoapHexInput(const char* data)
{
	int ret;
	char cmdbuf[512];
	struct Messages* msg;

	ret = a2b_hex(data, cmdbuf, sizeof(cmdbuf));
	if (ret < 0) {
		Logging(LOG_WARNING, "ascii to binary hex failed.\n");
		return -1;
	}
	Logging(LOG_TRACE, "coap hex static input %d, to binary %d.\r\n", strlen(data), ret);

	msg = NewMessage();
	ret = CoapInput(msg, (uint8_t*)cmdbuf, ret);
	if (ret < 0) {
		Logging(LOG_WARNING, "coap input process failed.\n");
		return -1;
	}
	Logging(LOG_TRACE, "coap input static process finished, ret %d.\r\n", ret);

	return ret;
}


// ASCII HEX格式的CoAP数据输入处理，首先调用 a2b_hex ，然后直接 CoapInput
// 类同 CoapHexInput，只是并没有在函数里直接使用一块较大的内存空间，使用了指定的buffer处理
int CoapHexInputStatic(const char* data, uint8_t* inBuf, uint16_t inMaxLength)
{
	int ret;
	struct Messages* msg;

	ret = a2b_hex(data, (char*)inBuf, inMaxLength);
	if (ret < 0) {
		Logging(LOG_WARNING, "ascii to binary hex failed.\n");
		return -1;
	}
	Logging(LOG_TRACE, "coap hex input %d, to binary %d.\r\n", strlen(data), ret);

	msg = NewMessageStatic(inBuf + ret, inMaxLength - ret);
	ret = CoapInput(msg, inBuf, ret);
	if (ret < 0) {
		Logging(LOG_WARNING, "coap input process failed.\n");
		return -1;
	}
	Logging(LOG_TRACE, "coap input process finished, ret %d.\r\n", ret);

	return ret;
}
