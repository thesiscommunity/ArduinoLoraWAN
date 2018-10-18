#ifndef __LORAWAN_ENDDEVICE_H__
#define __LORAWAN_ENDDEVICE_H__

#include "Arduino.h"
#include "Stream.h"

class LoraWanEndDevice {
public:
#define MODE_ENDDEVICE    0x00
#define MODE_FORWARDER    0x01
#define MODE_SERVER     0x02

#define ACTIVE_OTA     0x00
#define ACTIVE_ABP     0x01

#define CLASS_A    0x00
#define CLASS_B    0x01
#define CLASS_C    0x02

#define DR_0    0x00
#define DR_1    0x01
#define DR_2    0x02
#define DR_3    0x03
#define DR_4    0x04
#define DR_5    0x05
#define DR_6    0x06

#define TX_PWR_20dB    0x00
#define TX_PWR_18dB    0x01
#define TX_PWR_16dB    0x02
#define TX_PWR_14dB    0x03
#define TX_PWR_12dB    0x04
#define TX_PWR_10dB    0x05

	typedef enum {
		CMD_SET_MODE,

		CMD_ENDD_SET_ACTIVE_MODE,
		CMD_ENDD_OTA_STATUS,
		CMD_ENDD_SET_DEV_EUI ,
		CMD_ENDD_SET_APP_EUI,
		CMD_ENDD_SET_APP_KEY,
		CMD_ENDD_SET_NET_ID,
		CMD_ENDD_SET_DEV_ADDR,
		CMD_ENDD_SET_NKW_SKEY,
		CMD_ENDD_SET_APP_SKEY,
		CMD_ENDD_SET_CLASS,
		CMD_ENDD_SET_CHAN_MASK,
		CMD_ENDD_SET_DATARATE,
		CMD_ENDD_SET_RX2_CHAN,
		CMD_ENDD_SET_RX2_DR,
		CMD_ENDD_SET_TX_PWR,
		CMD_ENDD_SET_ANT_GAIN,
		CMD_ENDD_GET_DEV_EUI,
		CMD_ENDD_GET_APP_EUI,
		CMD_ENDD_GET_APP_KEY,
		CMD_ENDD_GET_NET_ID,
		CMD_ENDD_GET_DEV_ADDR,
		CMD_ENDD_GET_NKW_SKEY,
		CMD_ENDD_GET_APP_SKEY,
		CMD_ENDD_GET_CLASS,
		CMD_ENDD_GET_CHAN_MASK,
		CMD_ENDD_GET_DATARATE,
		CMD_ENDD_GET_RX2_CHAN,
		CMD_ENDD_GET_RX2_DR,
		CMD_ENDD_GET_TX_PWR,
		CMD_ENDD_GET_ANT_GAIN,
		CMD_ENDD_SEND_UNCONFIRM_MSG,
		CMD_ENDD_SEND_CONFIRM_MSG,
		CMD_ENDD_RECV_CONFIRM_MSG,
		CMD_ENDD_RECV_UNCONFIRM_MSG,

		CMD_FW_SET_TX_CONFIG,
		CMD_FW_GET_TX_CONFIG,
		CMD_FW_SET_RX_CONFIG,
		CMD_FW_GET_RX_CONFIG,
		CMD_FW_SEND,
		CMD_FW_RECV,

		CMD_SERV_SET_TX_CONFIG,
		CMD_SERV_GET_TX_CONFIG,
		CMD_SERV_SET_RX_CONFIG,
		CMD_SERV_GET_RX_CONFIG,
		CMD_SERV_SET_NET_PARAM,
		CMD_SERV_GET_NET_PARAM,
		CMD_SERV_SET_JOINT_PERMIT,
		CMD_SERV_GET_JOINT_PERMIT,
		CMD_SERV_RESP_JOINT_PERMIT,
		CMD_SERV_REMOVE_DEVICE,
		CMD_SERV_REMOVE_DEVICE_ALL,
		CMD_SERV_SEND_UNCONFIRM_MSG,
		CMD_SERV_SEND_CONFIRM_MSG,
		CMD_SERV_RECV_UNCONFIRM_MSG,
		CMD_SERV_RECV_CONFIRM_MSG,
		CMD_SERV_RECV_CONFIRM,
	} eCmdId;

	typedef void (*pf_handler_msg)(uint8_t*);

	LoraWanEndDevice(Stream* loraStream, uint32_t streamBaudRate);

	void update();

	// Handle app messages.
	bool incommingMsgHandlerRegister(pf_handler_msg func);

	static uint8_t getMsgCommandId(uint8_t* msg); // Return command id of message
	static uint8_t getMsgLength(uint8_t* msg); // Return ength of message
	static uint8_t* getMsgData(uint8_t* msg); // Return data pointer

	static uint8_t getDataPort(uint8_t* msg);
	static uint8_t getDataRxSlot(uint8_t* msg);
	static int16_t getDataRSSI(uint8_t* msg);
	static uint8_t getDataSNR(uint8_t* msg);
	static uint8_t getDataLength(uint8_t* msg);
	static uint8_t* getData(uint8_t* msg);

	// Lora method
	bool setOperationMode(uint8_t operationMode);
	bool setActiveMode(uint8_t activeMode);

	bool setDeviceClass(uint8_t deviceClass);
	bool getDeviceClass(uint8_t* deviceClass);

	bool setChannelMask(uint16_t channelMark);
	bool getChannelMask(uint16_t* channelMark);

	bool setDataRate(uint8_t dataRate);
	bool getDataRate(uint8_t* dataRate);

	bool setRx2Frequency(uint32_t frequency);
	bool getRx2Frequency(uint32_t* frequency);

	bool setRx2DataRate(uint8_t dataRate);
	bool getRx2DataRate(uint8_t* dataRate);

	bool setTxPower(uint8_t power);
	bool getTxPower(uint8_t* power);

	bool setAntGain(uint8_t antGain);
	bool getAntGain(uint8_t* antGain);

	bool setDeviceEUI(uint8_t* pdata, uint8_t length);
	bool getDeviceEUI(uint8_t* pdata, uint8_t length);

	bool setDeviceAddr(uint32_t deviceAddress);
	bool getDeviceAddr(uint32_t* deviceAddress);

	bool setAppEUI(uint8_t* pdata, uint8_t length);
	bool getAppEUI(uint8_t* pdata, uint8_t length);

	bool setAppKey(uint8_t* pdata, uint8_t length);
	bool getAppKey(uint8_t* pdata, uint8_t length);

	bool setNetworkID(uint8_t* pdata, uint8_t length);
	bool getNetworkID(uint8_t* pdata, uint8_t length);

	bool setNetworkSessionKey(uint8_t* pdata, uint8_t length);
	bool getNetworkSessionKey(uint8_t* pdata, uint8_t length);

	bool setAppSessionKey(uint8_t* pdata, uint8_t length);
	bool getAppSessionKey(uint8_t* pdata, uint8_t length);

	bool sendUnconfirmedData(uint8_t port, uint8_t datarate, uint8_t* pdata, uint8_t length);
	bool sendConfirmedData(uint8_t port, uint8_t datarate, uint8_t retries, uint8_t* pdata, uint8_t length);

private:
#define STREAM_PARSER_BUFFER_SIZE  160
#define COMMAND_FRAME_SOF  0xEF
#define COMMAND_TO_OFFSET 2000  // (ms)

	typedef enum { Async, Sync } eTransceiverSyncState;
	typedef enum {
		SOF,    // Start of frame state
		LENGTH, // Lenght state
		CMD_ID, // Command Id state
		DATA,   // Data state
		FCS     // Frame checksum state
	} eLoraStreamParserState;

	uint8_t mLoraStreamParserBuffer[STREAM_PARSER_BUFFER_SIZE];
	uint8_t mParserDataIndex;

	eTransceiverSyncState mTransceiverSyncState;
	eLoraStreamParserState mLoraStreamParserState;
	Stream* mLoraStream;
	uint16_t mLoraStreamBaudRate;

	pf_handler_msg mIncommingMsgHander;

	void loraStreamFlush();
	uint8_t cmdFrameCalcFcs(uint8_t* pdata, uint8_t length);
	int loraStreamPushReqCmdFrame(uint8_t cmdID, uint8_t* pdataReq, uint8_t lengthReq);
	int loraStreamCheckTimeOut(uint8_t length);
	int loraStreamPushReqResCmdFrame(uint8_t cmdID, uint8_t* pdataReq, uint8_t lengthReq \
									 , uint8_t* pdataRes, uint8_t lengthRes);
	int loraStreamParser(uint8_t byteData);
};

#endif //__LORAWAN_ENDDEVICE_H__


