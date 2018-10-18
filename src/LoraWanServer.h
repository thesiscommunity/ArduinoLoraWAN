#ifndef __LORAWAN_SERVER_H__
#define __LORAWAN_SERVER_H__

#include <string.h>

#include "Stream.h"
#include <SimpleList.h>

class LoraWanServer {
public:
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

	LoraWanServer(Stream* loraStream,  uint32_t streamBaudRate);

	typedef void (*pf_handler_msg)(uint8_t*);

	bool configRadio(uint32_t frequency, uint8_t datarate, uint8_t txPower);
	void update();

	bool setOperationMode(uint8_t operationMode);
	bool setSendingConfigParams(uint32_t frequency, uint8_t dataRate, uint8_t power);
	bool getSendingConfigParams(uint32_t* frequency, uint8_t* dataRate, uint8_t* power);
	bool setReceivingConfigParams(uint32_t frequency, uint8_t dataRate);
	bool getReceivingConfigParams(uint32_t* frequency, uint8_t* dataRate);
	bool setNetworkingConfigParams(uint32_t networkId, uint8_t* appicationKey);
	bool getNetworkingConfigParams(uint32_t* networkId, uint8_t* appicationKey);
	bool setOTADeviceParams(uint8_t* deviceEui, uint8_t* applicationEui, uint32_t timeout);
	bool getOTADeviceParams(uint8_t* deviceEui, uint8_t* applicationEui, uint32_t* timeout);
	bool addABPDevice(uint8_t* networkSkey, uint8_t* applicationSkey, uint32_t deviceAddress);
	int getDevices();
	bool getDeviceByIndex(int idex);
	bool removeDevice(uint32_t deviceAddress);
	bool removeAllDevice(void);
	bool sendUnconfirmedData(uint32_t deviceAddress, uint8_t port, uint8_t* pdata, uint8_t length);
	bool sendConfirmedData(uint32_t deviceAddress, uint8_t retries, uint8_t port, uint8_t* pdata, uint8_t length);

	bool incommingMsgHandlerRegister(pf_handler_msg func);

	static uint8_t getMsgCommandId(uint8_t* msg);
	static uint8_t getMsgLength(uint8_t* msg);
	static uint8_t* getMsgData(uint8_t* msg);

	static uint8_t* getOTADeviceEui(uint8_t* msg);
	static uint8_t* getOTAApplicationEui(uint8_t* msg);
	static uint32_t getOTADeviceAddress(uint8_t* msg);

	static uint32_t getDataDeviceAddress(uint8_t* msg);
	static bool isReceivedAck(uint8_t* msg);
	static uint8_t getDataPort(uint8_t* msg);
	static uint8_t getDataLength(uint8_t* msg);
	static uint8_t* getData(uint8_t* msg);

	static uint32_t getAckDeviceAddress(uint8_t* msg);
private:

#define MODE_ENDDEVICE					0x00
#define MODE_FORWARDER					0x01
#define MODE_SERVER						0x02

#define LORAWAN_BUFFER_SIZE			255
#define STREAM_PARSER_BUFFER_SIZE	255

#define COMMAND_FRAME_SOF		0xEF
#define COMMAND_TO_OFFSET		2000 /*ms*/

	typedef enum { Async, Sync } eTransceiverSyncState;
	typedef enum {
		SOF,    // Start of frame state
		LENGTH, // Lenght state
		CMD_ID, // Command Id state
		DATA,   // Data state
		FCS     // Frame checksum state
	} eLoraStreamParserState;
	typedef struct {
		struct {
			uint32_t tmts;
			uint32_t tx_frequency;
			uint8_t tx_datarate;
			uint8_t tx_power;
			uint8_t data_len;
		} header;
		uint8_t payload[LORAWAN_BUFFER_SIZE];
	} downLinkQueue_t;

	Stream* mLoraStream;
	uint16_t mLoraStreamBaudRate;
	uint8_t mLoraStreamParserBuffer[STREAM_PARSER_BUFFER_SIZE];
	uint8_t mParserDataIndex;
	eTransceiverSyncState mTransceiverSyncState;
	eLoraStreamParserState mLoraStreamParserState;

	pf_handler_msg mIncommingMsgHander;

	void loraStreamFlush();
	uint8_t cmdFrameCalcFcs(uint8_t* pdata, uint8_t length);
	int loraStreamPushReqCmdFrame(uint8_t cmdID, uint8_t* pdataReq, uint8_t lengthReq);
	int loraStreamCheckTimeOut(uint8_t length);
	int loraStreamPushReqResCmdFrame(uint8_t cmdID, uint8_t* pdataReq, uint8_t lengthReq \
									 , uint8_t* pdataRes, uint8_t lengthRes);
	int loraStreamParser(uint8_t byteData);
};

#endif //__LORAWAN_SERVER_H__
