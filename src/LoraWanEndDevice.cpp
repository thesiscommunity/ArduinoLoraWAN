#include "LoraWanEndDevice.h"

LoraWanEndDevice::LoraWanEndDevice(Stream* loraStream, uint32_t streamBaudRate) {
	mLoraStream = loraStream;
	mLoraStreamBaudRate = streamBaudRate;
	mTransceiverSyncState = LoraWanEndDevice::Async;
	mLoraStreamParserState = LoraWanEndDevice::SOF;
	mParserDataIndex = 0;
	mIncommingMsgHander = (pf_handler_msg)0;
}

void LoraWanEndDevice::update() {
	while (mLoraStream->available()) {
		loraStreamParser((uint8_t)mLoraStream->read());
	}
}

bool LoraWanEndDevice::incommingMsgHandlerRegister(LoraWanEndDevice::pf_handler_msg func) {
	if (func) {
		mIncommingMsgHander = func;
		return true;
	}
	return false;
}

uint8_t LoraWanEndDevice::getMsgCommandId(uint8_t* msg) {
	return *((uint8_t*)(msg + 2));
}

uint8_t LoraWanEndDevice::getMsgLength(uint8_t* msg) {
	return *((uint8_t*)(msg + 1));
}

uint8_t* LoraWanEndDevice::getMsgData(uint8_t* msg) {
	if (getMsgLength(msg) > 1) {
		return (uint8_t*)(msg + 3);
	}
	return (uint8_t*)0;
}

uint8_t LoraWanEndDevice::getDataPort(uint8_t* msg) {
	return *((uint8_t*)(msg + 3));
}

uint8_t LoraWanEndDevice::getDataRxSlot(uint8_t* msg) {
	return *((uint8_t*)(msg + 4));
}

int16_t LoraWanEndDevice::getDataRSSI(uint8_t* msg) {
	return *((int16_t*)(msg + 5));
}

uint8_t LoraWanEndDevice::getDataSNR(uint8_t* msg) {
	return *((uint8_t*)(msg + 7));
}

uint8_t LoraWanEndDevice::getDataLength(uint8_t* msg) {
	return (uint8_t)(getMsgLength(msg) - 6);
}

uint8_t* LoraWanEndDevice::getData(uint8_t* msg) {
	if (getMsgLength(msg) > 7) {
		return (uint8_t*)(msg + 8);
	}
	return (uint8_t*)0;
}

bool LoraWanEndDevice::setOperationMode(uint8_t operationMode) {
	uint8_t sReturn;
	if ( loraStreamPushReqResCmdFrame(CMD_SET_MODE, &operationMode, sizeof(uint8_t), &sReturn, sizeof(uint8_t)) == 0) {
		return (bool)sReturn;
	}
	return false;
}

bool LoraWanEndDevice::setActiveMode(uint8_t activeMode) {
	uint8_t sReturn;
	if (loraStreamPushReqResCmdFrame(CMD_ENDD_SET_ACTIVE_MODE, &activeMode, sizeof(uint8_t), &sReturn, sizeof(uint8_t)) == 0) {
		return (bool)sReturn;
	}
	return false;
}

bool LoraWanEndDevice::setDeviceClass(uint8_t deviceClass) {
	uint8_t sReturn;
	if (loraStreamPushReqResCmdFrame(CMD_ENDD_SET_CLASS, &deviceClass, sizeof(uint8_t), &sReturn, sizeof(uint8_t)) == 0) {
		return (bool)sReturn;
	}
	return false;
}

bool LoraWanEndDevice::getDeviceClass(uint8_t* deviceClass) {
	if (loraStreamPushReqResCmdFrame(CMD_ENDD_GET_CLASS, NULL, 0, deviceClass, sizeof(uint8_t)) == 0) {
		return true;
	}
	return false;
}

bool LoraWanEndDevice::setChannelMask(uint16_t channelMark) {
	uint8_t sReturn;
	if (loraStreamPushReqResCmdFrame(CMD_ENDD_SET_CHAN_MASK, (uint8_t*)&channelMark, sizeof(uint16_t), &sReturn, sizeof(uint8_t)) == 0) {
		return (bool)sReturn;
	}
	return false;
}

bool LoraWanEndDevice::getChannelMask(uint16_t *channelMark) {
	if (loraStreamPushReqResCmdFrame(CMD_ENDD_GET_CHAN_MASK, NULL, 0, (uint8_t*)channelMark, sizeof(uint16_t)) == 0) {
		return true;
	}
	return false;
}

bool LoraWanEndDevice::setDataRate(uint8_t dataRate) {
	uint8_t sReturn;
	if (loraStreamPushReqResCmdFrame(CMD_ENDD_SET_DATARATE, &dataRate, sizeof(uint8_t), &sReturn, sizeof(uint8_t)) == 0) {
		return (bool)sReturn;
	}
	return false;
}

bool LoraWanEndDevice::getDataRate(uint8_t *dataRate) {
	if (loraStreamPushReqResCmdFrame(CMD_ENDD_GET_DATARATE, NULL, 0, dataRate, sizeof(uint8_t)) == 0) {
		return true;
	}
	return false;
}

bool LoraWanEndDevice::setRx2Frequency(uint32_t frequency) {
	uint8_t sReturn;
	if (loraStreamPushReqResCmdFrame(CMD_ENDD_SET_RX2_CHAN, (uint8_t*)&frequency, sizeof(uint32_t), &sReturn, sizeof(uint8_t)) == 0) {
		return (bool)sReturn;
	}
	return false;
}

bool LoraWanEndDevice::getRx2Frequency(uint32_t *frequency) {
	if (loraStreamPushReqResCmdFrame(CMD_ENDD_GET_RX2_CHAN, NULL, 0, (uint8_t*)frequency, sizeof(uint32_t)) == 0) {
		return true;
	}
	return false;
}

bool LoraWanEndDevice::setRx2DataRate(uint8_t dataRate) {
	uint8_t sReturn;
	if (loraStreamPushReqResCmdFrame(CMD_ENDD_SET_RX2_DR, &dataRate, sizeof(uint8_t), &sReturn, sizeof(uint8_t)) == 0) {
		return (bool)sReturn;
	}
	return false;
}

bool LoraWanEndDevice::getRx2DataRate(uint8_t *dataRate) {
	if (loraStreamPushReqResCmdFrame(CMD_ENDD_GET_RX2_DR, NULL, 0, dataRate, sizeof(uint8_t)) == 0) {
		return true;
	}
	return false;
}

bool LoraWanEndDevice::setTxPower(uint8_t power) {
	uint8_t sReturn;
	if (loraStreamPushReqResCmdFrame(CMD_ENDD_SET_TX_PWR, &power, sizeof(uint8_t), &sReturn, sizeof(uint8_t)) == 0) {
		return (bool)sReturn;
	}
	return false;
}

bool LoraWanEndDevice::getTxPower(uint8_t *power) {
	if (loraStreamPushReqResCmdFrame(CMD_ENDD_GET_TX_PWR, NULL, 0, power, sizeof(uint8_t)) == 0) {
		return true;
	}
	return false;
}

bool LoraWanEndDevice::setAntGain(uint8_t antGain) {
	uint8_t sReturn;
	if (loraStreamPushReqResCmdFrame(CMD_ENDD_SET_ANT_GAIN, &antGain, sizeof(uint8_t), &sReturn, sizeof(uint8_t)) == 0) {
		return (bool)sReturn;
	}
	return false;
}

bool LoraWanEndDevice::getAntGain(uint8_t *antGain) {
	if (loraStreamPushReqResCmdFrame(CMD_ENDD_GET_ANT_GAIN, NULL, 0, antGain, sizeof(uint8_t)) == 0) {
		return true;
	}
	return false;
}

bool LoraWanEndDevice::setDeviceEUI(uint8_t *pdata, uint8_t length) {
	uint8_t sReturn;
	if (loraStreamPushReqResCmdFrame(CMD_ENDD_SET_DEV_EUI, pdata, length, &sReturn, sizeof(uint8_t)) == 0) {
		return (bool)sReturn;
	}
	return false;
}

bool LoraWanEndDevice::getDeviceEUI(uint8_t *pdata, uint8_t length) {
	if (loraStreamPushReqResCmdFrame(CMD_ENDD_GET_DEV_EUI, NULL, 0, pdata, length) == 0) {
		return true;
	}
	return false;
}

bool LoraWanEndDevice::setDeviceAddr(uint32_t deviceAddress) {
	uint8_t sReturn;
	if (loraStreamPushReqResCmdFrame(CMD_ENDD_SET_DEV_ADDR, (uint8_t*)&deviceAddress, sizeof(uint32_t), &sReturn, sizeof(uint8_t)) == 0) {
		return (bool)sReturn;
	}
	return false;
}

bool LoraWanEndDevice::getDeviceAddr(uint32_t* deviceAddress) {
	if (loraStreamPushReqResCmdFrame(CMD_ENDD_GET_DEV_ADDR, NULL, 0, (uint8_t*)deviceAddress, sizeof(uint32_t)) == 0) {
		return true;
	}
	return false;
}

bool LoraWanEndDevice::setAppEUI(uint8_t *pdata, uint8_t length) {
	uint8_t sReturn;
	if (loraStreamPushReqResCmdFrame(CMD_ENDD_SET_APP_EUI, pdata, length, &sReturn, sizeof(uint8_t)) == 0) {
		return (bool)sReturn;
	}
	return false;
}

bool LoraWanEndDevice::getAppEUI(uint8_t *pdata, uint8_t length) {
	if (loraStreamPushReqResCmdFrame(CMD_ENDD_GET_APP_EUI, NULL, 0, pdata, length) == 0) {
		return true;
	}
	return false;
}

bool LoraWanEndDevice::setAppKey(uint8_t *pdata, uint8_t length) {
	uint8_t sReturn;
	if (loraStreamPushReqResCmdFrame(CMD_ENDD_SET_APP_KEY, pdata, length, &sReturn, sizeof(uint8_t)) == 0) {
		return (bool)sReturn;
	}
	return false;
}

bool LoraWanEndDevice::getAppKey(uint8_t *pdata, uint8_t length) {
	if (loraStreamPushReqResCmdFrame(CMD_ENDD_GET_APP_KEY, NULL, 0, pdata, length) == 0) {
		return true;
	}
	return false;
}

bool LoraWanEndDevice::setNetworkID(uint8_t *pdata, uint8_t length) {
	uint8_t sReturn;
	if (loraStreamPushReqResCmdFrame(CMD_ENDD_SET_NET_ID, pdata, length, &sReturn, sizeof(uint8_t)) == 0) {
		return (bool)sReturn;
	}
	return false;
}

bool LoraWanEndDevice::getNetworkID(uint8_t *pdata, uint8_t length) {
	if (loraStreamPushReqResCmdFrame(CMD_ENDD_GET_NET_ID, NULL, 0, pdata, length) == 0) {
		return true;
	}
	return false;
}

bool LoraWanEndDevice::setNetworkSessionKey(uint8_t *pdata, uint8_t length) {
	uint8_t sReturn;
	if (loraStreamPushReqResCmdFrame(CMD_ENDD_SET_NKW_SKEY, pdata, length, &sReturn, sizeof(uint8_t)) == 0) {
		return (bool)sReturn;
	}
	return false;
}

bool LoraWanEndDevice::getNetworkSessionKey(uint8_t *pdata, uint8_t length) {
	if (loraStreamPushReqResCmdFrame(CMD_ENDD_GET_NKW_SKEY, NULL, 0, pdata, length) == 0) {
		return true;
	}
	return false;
}

bool LoraWanEndDevice::setAppSessionKey(uint8_t *pdata, uint8_t length) {
	uint8_t sReturn;
	if (loraStreamPushReqResCmdFrame(CMD_ENDD_SET_APP_SKEY, pdata, length, &sReturn, sizeof(uint8_t)) == 0) {
		return (bool)sReturn;
	}
	return false;
}

bool LoraWanEndDevice::getAppSessionKey(uint8_t *pdata, uint8_t length) {
	if (loraStreamPushReqResCmdFrame(CMD_ENDD_GET_APP_SKEY, NULL, 0, pdata, length) == 0) {
		return true;
	}
	return false;
}

bool LoraWanEndDevice::sendUnconfirmedData(uint8_t port, uint8_t datarate, uint8_t* pdata, uint8_t length) {
	uint8_t sReturn;

	uint8_t* sPframe = (uint8_t*)malloc(1 + length);
	*((uint8_t*)&sPframe[0]) = port;

	memcpy(&sPframe[1], pdata, length);
	if (loraStreamPushReqResCmdFrame(CMD_ENDD_SEND_UNCONFIRM_MSG, sPframe, 1 +length, &sReturn, sizeof(uint8_t))>=0) {
		free(sPframe);
		return true;
	}
	free(sPframe);
	return false;
}

bool LoraWanEndDevice::sendConfirmedData(uint8_t port, uint8_t datarate,uint8_t retries ,uint8_t* pdata, uint8_t length) {
	uint8_t sReturn;

	uint8_t* sPframe = (uint8_t*)malloc(2 + length);
	*((uint8_t*)&sPframe[0]) = port;
	*((uint8_t*)&sPframe[1]) = retries;

	memcpy(&sPframe[2], pdata, length);
	if (loraStreamPushReqResCmdFrame(CMD_ENDD_SEND_CONFIRM_MSG, sPframe, 2 +length, &sReturn, sizeof(uint8_t))>=0) {
		free(sPframe);
		return true;
	}
	free(sPframe);
	return false;
}

uint8_t LoraWanEndDevice::cmdFrameCalcFcs(uint8_t *pdata, uint8_t length) {
	uint8_t xorResult = length;
	for (int i = 0; i < length; i++, pdata++) {
		xorResult = xorResult ^ *pdata;
	}
	return xorResult;
}

int LoraWanEndDevice::loraStreamPushReqCmdFrame(uint8_t cmdID, uint8_t *pdataReq, uint8_t lengthReq) {
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

int LoraWanEndDevice::loraStreamCheckTimeOut(uint8_t length) {


	uint32_t sAvailableTime =( length * ( 1 / ( mLoraStreamBaudRate / 10 /* StartBit + 8DataBit + StopBit */ ) ) ) \
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

int LoraWanEndDevice::loraStreamPushReqResCmdFrame(uint8_t cmdID, uint8_t *pdataReq, uint8_t lengthReq, uint8_t *pdataRes, uint8_t lengthRes) {
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

int LoraWanEndDevice::loraStreamParser(uint8_t byteData) {
	int sReturn = 0;
	//Serial.print(byteData, HEX);
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

		if (mLoraStreamParserBuffer[1]==1) {//error
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

void LoraWanEndDevice::loraStreamFlush() {
	uint8_t c;
	while (mLoraStream->available()) {
		mLoraStream->read();
		(void)c;
	}
}
