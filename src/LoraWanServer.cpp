#include "Arduino.h"
#include "LoraWanServer.h"

#define APP_PRINT_EN

#ifdef APP_PRINT_EN
#define APP_PRINTLN	Serial.println
#define APP_PRINT	Serial.print
#else
#define APP_PRINTLN(fmt,...)
#define APP_PRINT(fmt,...)
#endif

using namespace std;

LoraWanServer::LoraWanServer(Stream* loraStream, uint32_t streamBaudRate) {
	mLoraStream = loraStream;
	mLoraStreamBaudRate = streamBaudRate;
	mTransceiverSyncState = LoraWanServer::Async;
	mLoraStreamParserState = LoraWanServer::SOF;
	mParserDataIndex = 0;
	mIncommingMsgHander = (pf_handler_msg)0;
}

bool LoraWanServer::configRadio(uint32_t frequency, uint8_t datarate, uint8_t txPower) {
	if (setOperationMode(MODE_SERVER) && \
			setSendingConfigParams(frequency, datarate, txPower) && \
			setReceivingConfigParams(frequency, datarate)) {
		return true;
	}
	return false;
}

bool LoraWanServer::incommingMsgHandlerRegister(LoraWanServer::pf_handler_msg func) {
	if (func) {
		mIncommingMsgHander = func;
		return true;
	}
	return false;
}

void LoraWanServer::update() {
	while (mLoraStream->available()) {
		loraStreamParser((uint8_t)mLoraStream->read());
	}
}

uint8_t LoraWanServer::getMsgCommandId(uint8_t* msg) {
	return (uint8_t)(*(msg + 2));
}

uint8_t LoraWanServer::getMsgLength(uint8_t* msg) {
	return (uint8_t)(*(msg + 1));
}

uint8_t* LoraWanServer::getMsgData(uint8_t* msg) {
	if (getMsgLength(msg) > 1) {
		return (uint8_t*)(msg + 3);
	}
	return (uint8_t*)0;
}

uint8_t* LoraWanServer::getOTADeviceEui(uint8_t* msg) {
	if (getMsgCommandId(msg) == CMD_SERV_RESP_JOINT_PERMIT) {
		return (uint8_t*)(msg + 3);
	}
	return (uint8_t*)0;
}

uint8_t* LoraWanServer::getOTAApplicationEui(uint8_t* msg) {
	if (getMsgCommandId(msg) == CMD_SERV_RESP_JOINT_PERMIT) {
		return (uint8_t*)(msg + 11);
	}
	return (uint8_t*)0;
}

uint32_t LoraWanServer::getOTADeviceAddress(uint8_t* msg) {
	if (getMsgCommandId(msg) == CMD_SERV_RESP_JOINT_PERMIT) {
		return *((uint32_t*)(msg + 19));
	}
	return (uint32_t)0xFFFFFFFF;
}

uint32_t LoraWanServer::getDataDeviceAddress(uint8_t* msg) {
	uint8_t cmdId = getMsgCommandId(msg);
	if (cmdId == CMD_SERV_RECV_CONFIRM_MSG || cmdId == CMD_SERV_RECV_UNCONFIRM_MSG) {
		return *((uint32_t*)(msg + 3));
	}
	return (uint32_t)0xFFFFFFFF;
}

bool LoraWanServer::isReceivedAck(uint8_t* msg) {
	uint8_t cmdId = getMsgCommandId(msg);
	if (cmdId == CMD_SERV_RECV_CONFIRM_MSG || cmdId == CMD_SERV_RECV_UNCONFIRM_MSG) {
		return (bool)(*(msg + 7));
	}
	return false;
}

uint8_t LoraWanServer::getDataPort(uint8_t* msg) {
	uint8_t cmdId = getMsgCommandId(msg);
	if (cmdId == CMD_SERV_RECV_UNCONFIRM_MSG) {
		return *((uint8_t*)(msg + 8));
	}
	else if (cmdId == CMD_SERV_RECV_CONFIRM_MSG) {
		return *((uint8_t*)(msg + 9));
	}
	return (uint8_t)0xFF;
}

uint8_t LoraWanServer::getDataLength(uint8_t* msg) {
	uint8_t cmdId = getMsgCommandId(msg);
	if (cmdId == CMD_SERV_RECV_UNCONFIRM_MSG) {
		if (getMsgLength(msg) > 7) {
			return (getMsgLength(msg) - 7);
		}
	}
	else if (cmdId == CMD_SERV_RECV_CONFIRM_MSG) {
		if (getMsgLength(msg) > 8) {
			return (getMsgLength(msg) - 8);
		}
	}
	return (uint8_t)0xFF;
}

uint8_t* LoraWanServer::getData(uint8_t* msg) {
	uint8_t cmdId = getMsgCommandId(msg);
	if (cmdId == CMD_SERV_RECV_UNCONFIRM_MSG) {
		return (uint8_t*)(msg + 9);
	}
	else if (cmdId == CMD_SERV_RECV_CONFIRM_MSG) {
		return (uint8_t*)(msg + 10);
	}
	return (uint8_t*)0;
}

uint32_t LoraWanServer::getAckDeviceAddress(uint8_t* msg) {
	if (getMsgCommandId(msg) == CMD_SERV_RECV_CONFIRM) {
		return *((uint32_t*)(msg + 3));
	}
	return (uint32_t)0xFFFFFFFF;
}

bool LoraWanServer::setOperationMode(uint8_t operationMode) {
	uint8_t sReturn;
	if ( loraStreamPushReqResCmdFrame(CMD_SET_MODE, &operationMode, sizeof(uint8_t), &sReturn, sizeof(uint8_t)) == 0) {
		return (bool)sReturn;
	}
	return false;
}

bool LoraWanServer::setSendingConfigParams(uint32_t frequency, uint8_t dataRate, uint8_t power) {
	uint8_t sReturn;
	uint8_t sendBuffer[6] = {0, 0, 0, 0, 0, 0};
	*((uint8_t*)&sendBuffer[4]) = dataRate;
	*((uint8_t*)&sendBuffer[5]) = power;
	*((uint32_t*)&sendBuffer[0]) = frequency;
	if (loraStreamPushReqResCmdFrame(CMD_SERV_SET_TX_CONFIG, (uint8_t*)sendBuffer, 6, &sReturn, sizeof(uint8_t)) == 0) {
		return (bool)sReturn;
	}
	return false;
}

bool LoraWanServer::getSendingConfigParams(uint32_t *frequency, uint8_t *dataRate, uint8_t *power) {
	uint8_t sendBuffer[6];
	if (loraStreamPushReqResCmdFrame(CMD_SERV_GET_TX_CONFIG, NULL, 0, sendBuffer, 6) == 0) {
		*frequency = *((uint32_t*)&sendBuffer[0]);
		*dataRate = *((uint8_t*)&sendBuffer[4]);
		*power = *((uint8_t*)&sendBuffer[5]);
		return true;
	}
	*frequency = (uint32_t)0xFFFFFFFF;
	*dataRate = (uint8_t)0xFF;
	*power = (uint8_t)0xFF;
	return false;
}

bool LoraWanServer::setReceivingConfigParams(uint32_t frequency, uint8_t dataRate) {
	uint8_t sReturn;
	uint8_t sendBuffer[5] = {0, 0, 0, 0, 0};

	*((uint32_t*)&sendBuffer[0]) = frequency;
	*((uint8_t*)&sendBuffer[4]) = dataRate;

	if (loraStreamPushReqResCmdFrame(CMD_SERV_SET_RX_CONFIG, sendBuffer, 5, &sReturn, sizeof(uint8_t)) == 0) {
		return (bool)sReturn;
	}
	return false;
}

bool LoraWanServer::getReceivingConfigParams(uint32_t *frequency, uint8_t *dataRate) {
	uint8_t sendBuffer[5];
	if (loraStreamPushReqResCmdFrame(CMD_SERV_GET_RX_CONFIG, NULL, 0, sendBuffer, 5) == 0) {
		*frequency = *((uint32_t*)&sendBuffer[0]);
		*dataRate = *((uint8_t*)&sendBuffer[4]);
		return true;
	}
	*frequency = (uint32_t)0xFFFFFFFF;
	*dataRate = (uint8_t)0xFF;
	return false;
}

bool LoraWanServer::setNetworkingConfigParams(uint32_t networkId, uint8_t* appicationKey) {
	uint8_t sReturn;
	uint8_t sendBuffer[20];

	memcpy((uint8_t*)sendBuffer, (uint8_t*)&networkId, 4);
	memcpy((uint8_t*)(sendBuffer + 4), (uint8_t*)appicationKey, 16);

	if (loraStreamPushReqResCmdFrame(CMD_SERV_SET_NET_PARAM, sendBuffer, 20, &sReturn, sizeof(uint8_t)) == 0) {
		return (bool)sReturn;
	}
	return false;
}

bool LoraWanServer::getNetworkingConfigParams(uint32_t* networkId, uint8_t* appicationKey) {
	uint8_t sendBuffer[20];
	if (loraStreamPushReqResCmdFrame(CMD_SERV_GET_NET_PARAM, NULL, 0, sendBuffer, 20) == 0) {
		memcpy((uint8_t*)networkId, (uint8_t*)sendBuffer, 4);
		memcpy((uint8_t*)appicationKey, (uint8_t*)(sendBuffer + 4), 16);
		return true;
	}
	return false;
}

bool LoraWanServer::setOTADeviceParams(uint8_t* deviceEui, uint8_t* applicationEui, uint32_t timeout) {
	uint8_t sReturn;
	uint8_t sendBuffer[20];

	memcpy((uint8_t*)sendBuffer, (uint8_t*)deviceEui, 8);
	memcpy((uint8_t*)(sendBuffer + 8), (uint8_t*)applicationEui, 8);
	memcpy((uint8_t*)(sendBuffer + 16), (uint8_t*)&timeout, 4);

	if (loraStreamPushReqResCmdFrame(CMD_SERV_SET_JOINT_PERMIT, sendBuffer, 20, &sReturn, sizeof(uint8_t)) == 0) {
		return (bool)sReturn;
	}
	return false;
}

bool LoraWanServer::getOTADeviceParams(uint8_t* deviceEui, uint8_t* applicationEui, uint32_t* timeout) {
	uint8_t sendBuffer[20];
	if (loraStreamPushReqResCmdFrame(CMD_SERV_GET_JOINT_PERMIT, NULL, 0, sendBuffer, 20) == 0) {
		memcpy((uint8_t*)deviceEui, (uint8_t*)sendBuffer, 8);
		memcpy((uint8_t*)applicationEui, (uint8_t*)(sendBuffer + 8), 8);
		memcpy((uint8_t*)timeout, (uint8_t*)(sendBuffer + 16), 4);
		return true;
	}
	return false;
}

bool LoraWanServer::addABPDevice(uint8_t* networkSkey, uint8_t* applicationSkey, uint32_t deviceAddress) {

}

int LoraWanServer::getDevices() {

}

bool LoraWanServer::getDeviceByIndex(int idex) {

}

bool LoraWanServer::removeDevice(uint32_t deviceAddress) {
	uint8_t sReturn;

	if (loraStreamPushReqResCmdFrame(CMD_SERV_REMOVE_DEVICE, (uint8_t*)&deviceAddress, 4, &sReturn, sizeof(uint8_t)) == 0) {
		return (bool)sReturn;
	}
	return false;
}

bool LoraWanServer::removeAllDevice(void) {
	uint8_t sReturn;

	if (loraStreamPushReqResCmdFrame(CMD_SERV_REMOVE_DEVICE_ALL, NULL, 0, &sReturn, sizeof(uint8_t)) == 0) {
		return (bool)sReturn;
	}
	return false;
}

bool LoraWanServer::sendUnconfirmedData(uint32_t deviceAddress, uint8_t port, uint8_t* pdata, uint8_t length) {
	uint8_t sReturn;

	uint8_t* sPframe = (uint8_t*)malloc(6 + length);
	memcpy((uint8_t*)sPframe, (uint8_t*)&deviceAddress, 4);
	memcpy((uint8_t*)(sPframe + 5), (uint8_t*)&port, 1);
	memcpy((uint8_t*)(sPframe + 6), (uint8_t*)pdata, length);

	if (loraStreamPushReqResCmdFrame(CMD_SERV_SEND_UNCONFIRM_MSG, sPframe, 6 +length, &sReturn, sizeof(uint8_t))>=0) {
		free(sPframe);
		return true;
	}
	free(sPframe);
	return false;
}

bool LoraWanServer::sendConfirmedData(uint32_t deviceAddress, uint8_t retries, uint8_t port, uint8_t* pdata, uint8_t length) {
	uint8_t sReturn;

	uint8_t* sPframe = (uint8_t*)malloc(7 + length);
	memcpy((uint8_t*)sPframe, (uint8_t*)&deviceAddress, 4);
	memcpy((uint8_t*)(sPframe + 5), (uint8_t*)&retries, 1);
	memcpy((uint8_t*)(sPframe + 6), (uint8_t*)&port, 1);
	memcpy((uint8_t*)(sPframe + 7), (uint8_t*)pdata, length);

	if (loraStreamPushReqResCmdFrame(CMD_SERV_SEND_CONFIRM_MSG, sPframe, 7 +length, &sReturn, sizeof(uint8_t))>=0) {
		free(sPframe);
		return true;
	}
	free(sPframe);
	return false;
}

uint8_t LoraWanServer::cmdFrameCalcFcs(uint8_t *pdata, uint8_t length) {
	uint8_t xorResult = length;
	for (int i = 0; i < length; i++, pdata++) {
		xorResult = xorResult ^ *pdata;
	}
	return xorResult;
}

int LoraWanServer::loraStreamPushReqCmdFrame(uint8_t cmdID, uint8_t *pdataReq, uint8_t lengthReq) {
	uint8_t* pframe = (uint8_t*)malloc(4 + lengthReq);
	if (pframe) {
		pframe[0] = COMMAND_FRAME_SOF;
		pframe[1] = sizeof(uint8_t) + lengthReq;
		pframe[2] = cmdID;
		memcpy(&pframe[3], pdataReq, lengthReq);
		pframe[pframe[1] + 2] = cmdFrameCalcFcs(&pframe[2], pframe[1]);
		mLoraStream->write(pframe, 4 + lengthReq);
		free(pframe);
		return 0;
	}
	return -1;
}

int LoraWanServer::loraStreamCheckTimeOut(uint8_t length) {
	uint32_t sAvailableTime = ( length * ( 1 / ( mLoraStreamBaudRate / 10 /* StartBit + 8DataBit + StopBit */ ) ) ) \
			+ COMMAND_TO_OFFSET \
			+ millis();
	while (millis() - sAvailableTime) {
		if (mLoraStream->available() == (int)length) {
			return 0;

		}

	}
	loraStreamFlush();
	return -1;
}

int LoraWanServer::loraStreamPushReqResCmdFrame(uint8_t cmdID, uint8_t *pdataReq, uint8_t lengthReq, uint8_t *pdataRes, uint8_t lengthRes) {
	uint8_t* pframe = (uint8_t*)malloc(4 + lengthReq);

	if (pframe) {
		pframe[0] = COMMAND_FRAME_SOF;
		pframe[1] = sizeof(uint8_t) + lengthReq;
		pframe[2] = cmdID;

		if (lengthReq) {
			memcpy(&pframe[3], pdataReq, lengthReq);
		}
		pframe[pframe[1] + 2] = cmdFrameCalcFcs(&pframe[2], pframe[1]);


		mLoraStream->write(pframe, 4 + lengthReq);
		free(pframe);

		if (loraStreamCheckTimeOut(4 + lengthRes) == 0) {

			mTransceiverSyncState = Sync;

			for (int i = 0; i < 4 + lengthRes; i++) {
				if (loraStreamParser((uint8_t)mLoraStream->read()) == 1) {
					if (getMsgCommandId(mLoraStreamParserBuffer) != cmdID) {
						return -5;
					}
					else if ((lengthRes + 1) == getMsgLength(mLoraStreamParserBuffer)) {
						memcpy(pdataRes, getMsgData(mLoraStreamParserBuffer), lengthRes);
					}
					else {
						return -6;
					}
				}
			}
			mTransceiverSyncState = Async;

			return 0;
		}

		return -2;
	}

	return -1;
}

int LoraWanServer::loraStreamParser(uint8_t byteData) {
	int sReturn = 0;
	//APP_PRINT(byteData, HEX);
	switch (mLoraStreamParserState) {
	case SOF: { // Start of frame state
		if (COMMAND_FRAME_SOF == byteData) {
			mLoraStreamParserState = LENGTH;
			mLoraStreamParserBuffer[0] = byteData;
		}
		else {
			sReturn = -1; //(-1) Mean parser ignore data.
		}
	}
		break;

	case LENGTH: { // Lenght state
		mLoraStreamParserBuffer[1] = byteData;
		mLoraStreamParserState = CMD_ID;
	}
		break;

	case CMD_ID: { // Command Id state
		mLoraStreamParserBuffer[2] = byteData;

		if (mLoraStreamParserBuffer[1] == 1) { //error
			mLoraStreamParserState = FCS;
		}
		else {
			mParserDataIndex = 0;
			mLoraStreamParserState = DATA;
		}
	}
		break;

	case DATA: { // Data state
		mLoraStreamParserBuffer[3 + mParserDataIndex++] = byteData;
		if (mParserDataIndex == (mLoraStreamParserBuffer[1] - 1)) {
			mLoraStreamParserState = FCS;
		}
	}
		break;

	case FCS: { // Frame checksum state
		sReturn = 1;
		mLoraStreamParserState = SOF;
		if (cmdFrameCalcFcs(&mLoraStreamParserBuffer[2], mLoraStreamParserBuffer[1]) == byteData) {
			// handle new message
			if (mTransceiverSyncState == Async) {
				if (mIncommingMsgHander) {
					mIncommingMsgHander(mLoraStreamParserBuffer);
				}
			}
			else {
				sReturn = 1;
			}
		}
		else {
			//TODO: handle checksum incorrect !
		}
	}
		break;

	default:
		break;
	}

	return sReturn;
}

void LoraWanServer::loraStreamFlush() {
	uint8_t c;
	while (mLoraStream->available()) {
		mLoraStream->read();
	}
}
