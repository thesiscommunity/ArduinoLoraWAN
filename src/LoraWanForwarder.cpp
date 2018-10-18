#include "Arduino.h"
#include "base64.hpp"
#include <TimeLib.h>
#include "LoraWanForwarder.h"

//#define APP_PRINT_EN

#ifdef APP_PRINT_EN
#define APP_PRINTLN	Serial.println
#define APP_PRINT	Serial.print
#else
#define APP_PRINTLN(fmt,...)
#define APP_PRINT(fmt,...)
#endif

using namespace std;

LoraWanForwarder::LoraWanForwarder(Stream* loraStream, uint32_t streamBaudRate) {
	mLoraStream = loraStream;
	mLoraStreamBaudRate = streamBaudRate;
	mTransceiverSyncState = LoraWanForwarder::Async;
	mLoraStreamParserState = LoraWanForwarder::SOF;
	mParserDataIndex = 0;
	mIncommingMsgHander = (pf_handler_msg)0;

	mlastStat = 0;
	cp_rxok = 0, cp_rxfw = 0, cp_dwnb = 0, cp_txnb = 0;

	mServerAddr[0] = '\0';
	mServerPortUp = 1700;
	mServerPortDown = 1700;
}

void LoraWanForwarder::configMAC(const unsigned char* mac) {
	mGatewayId[0] = mac[0];
	mGatewayId[1] = mac[1];
	mGatewayId[2] = mac[2];
	mGatewayId[3] = 0xFF;
	mGatewayId[4] = 0xFF;
	mGatewayId[5] = mac[3];
	mGatewayId[6] = mac[4];
	mGatewayId[7] = mac[5];
}

void LoraWanForwarder::configServer(UDP* udp, const char* serverAddr, int upPort, int downPort) {
	mUdp = udp;
	strcpy(mServerAddr, serverAddr);
	mServerPortUp = upPort;
	mServerPortDown = downPort;
}

bool LoraWanForwarder::configRadio(uint32_t frequency, uint8_t datarate) {
	if (setOperationMode(MODE_FORWARDER) && setReceivingConfigParams(frequency, datarate)) {
		mGwFrequency = frequency;
		mGwDatarate = datarate;
		return true;
	}
	return false;
}

bool LoraWanForwarder::incommingMsgHandlerRegister(LoraWanForwarder::pf_handler_msg func) {

	if (func) {
		mIncommingMsgHander = func;
		return true;
	}
	return false;

}

void LoraWanForwarder::update() {
	if ( mlastStat == 0 || \
		 ((uint32_t)(millis() - mlastStat) >= TIMER_STAT)) {
		mlastStat = millis();
		updateStat();
	}

	updateUp();
	updateDown();
	sendDownQueue();
}

void LoraWanForwarder::updateStat() {
	int len = 0, buffLength = 0;
	time_t t;

	t = now();
	mstatBuff = (uint8_t*)malloc(PUSH_PACKET_MAX_SIZE);

	mtoken_h = 0xFF & random(0x00, 0xFF);
	mtoken_l = 0xFF & random(0x00, 0xFF);
	mstatBuff[0] = (uint8_t)PROTOCOL_VERSION;
	mstatBuff[1] = mtoken_h;
	mstatBuff[2] = mtoken_l;
	mstatBuff[3] = (uint8_t)PKT_PUSH_DATA;
	mstatBuff[4] = mGatewayId[0];
	mstatBuff[5] = mGatewayId[1];
	mstatBuff[6] = mGatewayId[2];
	mstatBuff[7] = 0xFF;
	mstatBuff[8] = 0xFF;
	mstatBuff[9] = mGatewayId[3];
	mstatBuff[10] = mGatewayId[4];
	mstatBuff[11] = mGatewayId[5];

	len += 12;
	buffLength = snprintf((char *)(mstatBuff + len), \
						  PUSH_PACKET_MAX_SIZE - len, \
						  "{\"stat\":{\"time\":\"%04d-%02d-%02d %02d:%02d:%02d GMT\",\"lati\":0.00000,\"long\":0.00000,\"alti\":0,\"rxok\":%u,\"rxfw\":%u,\"dwnb\":%u,\"txnb\":%u}}", \
						  year(t), month(t), day(t), hour(t), minute(t), second(t), cp_rxok, cp_rxfw, cp_dwnb, cp_txnb);
	len += buffLength;
	mstatBuff[len] = 0; /* add string terminator, for safety */

	/* debug */
	APP_PRINTLN((char *)(mstatBuff + 12));

	while (mUdp->parsePacket() > 0) ;
	mUdp->beginPacket(mServerAddr, mServerPortUp);
	mUdp->write((const unsigned char *)mstatBuff, len);
	mUdp->endPacket();

	/* wait for acknowledge (in 2 times, to catch extra packets) */
	memset(mstatBuff, 0, PUSH_ACK_PACKET_MAX_SIZE + 1);
	for (int i = 0; i < 2; i++) {
		delay(PUSH_TIMEOUT_MS / 2);
		if ( mUdp->parsePacket() > 0 ) {
			int got = mUdp->read((char *)mstatBuff, PUSH_ACK_PACKET_MAX_SIZE);
			if ((got < 4) || ((mstatBuff[0] & 0xFF) != PROTOCOL_VERSION) || ((mstatBuff[3] & 0xFF) != PKT_PUSH_ACK)) {
				APP_PRINTLN("ERR: [stt] ignored invalid non-ACL packet");
				continue;
			} else if (((mstatBuff[1] & 0xFF) != mtoken_h) || ((mstatBuff[2] & 0xFF) != mtoken_l)) {
				APP_PRINTLN("ERR: [stt] ignored out-of sync ACK packet");
				continue;
			} else {
				APP_PRINTLN("INFO: [stt] PUSH_ACK received");
				break;
			}
		}
	}

	free(mstatBuff);

	cp_rxok = 0;
	cp_rxfw = 0;
	cp_dwnb = 0;
	cp_txnb = 0;
}

void LoraWanForwarder::updateDown() {
	mdownBuff = (uint8_t*)malloc(PULL_PACKET_MAX_SIZE);
	int len = 0;

	mtoken_h = 0xFF & random(0x00, 0xFF);
	mtoken_l = 0xFF & random(0x00, 0xFF);
	mdownBuff[0] = (uint8_t)PROTOCOL_VERSION;
	mdownBuff[1] = mtoken_h;
	mdownBuff[2] = mtoken_l;
	mdownBuff[3] = (uint8_t)PKT_PULL_DATA;
	mdownBuff[4] = mGatewayId[0];
	mdownBuff[5] = mGatewayId[1];
	mdownBuff[6] = mGatewayId[2];
	mdownBuff[7] = 0xFF;
	mdownBuff[8] = 0xFF;
	mdownBuff[9] = mGatewayId[3];
	mdownBuff[10] = mGatewayId[4];
	mdownBuff[11] = mGatewayId[5];

	len = 12;

	mUdp->beginPacket(mServerAddr, mServerPortDown);
	mUdp->write((const unsigned char *)mdownBuff, len);
	mUdp->endPacket();
	free(mdownBuff);

	for (int i = 0; i < 2; i++) {
		delay(PULL_TIMEOUT_MS / 2);
		int avai, got;
		if ( (avai = mUdp->parsePacket()) ) {
			mdownBuff = (uint8_t*)malloc(avai + 1);
			got = mUdp->read((char *)mdownBuff, avai);

			if ((got < 4) || ((0XFF & mdownBuff[0]) != PROTOCOL_VERSION) || (((0XFF & mdownBuff[3]) != PKT_PULL_RESP) && ((0XFF & mdownBuff[3]) != PKT_PULL_ACK))) {
				APP_PRINTLN("WARNING: [down] ignoring invalid packet");
				free(mdownBuff);
				break;
			}

			if ((0XFF & mdownBuff[3]) == PKT_PULL_ACK) {
				if (((0XFF & mdownBuff[1]) == mtoken_h) && ((0XFF & mdownBuff[2]) == mtoken_l)) {
					APP_PRINTLN("INFO: [down] PULL_ACK received");
				} else { /* out-of-sync token */
					APP_PRINTLN("INFO: [down] received out-of-sync ACK");
				}
				free(mdownBuff);
				break;
			}

			cp_dwnb++;
			mdownBuff[got] = 0;
			APP_PRINTLN("INFO: [down] PULL_RESP received");
			APP_PRINTLN((char *)(mdownBuff + 4));

			parserJson((char *)(mdownBuff + 4), got);
			free(mdownBuff);
		}
	}
}

void LoraWanForwarder::updateUp() {
	while (mLoraStream->available()) {
		loraStreamParser((uint8_t)mLoraStream->read());
	}
}

uint8_t LoraWanForwarder::getMsgCommandId(uint8_t* msg) {
	return (uint8_t)(*(msg + 2));
}

uint8_t LoraWanForwarder::getMsgLength(uint8_t* msg) {
	return (uint8_t)(*(msg + 1));
}

uint8_t* LoraWanForwarder::getMsgData(uint8_t* msg) {
	if (getMsgLength(msg) > 1) {
		return (uint8_t*)(msg + 3);
	}
	return (uint8_t*)0;
}

bool LoraWanForwarder::setOperationMode(uint8_t operationMode) {
	uint8_t sReturn;
	if ( loraStreamPushReqResCmdFrame(CMD_SET_MODE, &operationMode, sizeof(uint8_t), &sReturn, sizeof(uint8_t)) == 0) {
		return (bool)sReturn;
	}
	return false;
}

bool LoraWanForwarder::setSendingConfigParams(uint32_t frequency, uint8_t dataRate, uint8_t power) {
	uint8_t sReturn;
	uint8_t sendBuffer[6] = {0, 0, 0, 0, 0, 0};
	*((uint8_t*)&sendBuffer[4]) = dataRate;
	*((uint8_t*)&sendBuffer[5]) = power;
	*((uint32_t*)&sendBuffer[0]) = frequency;
	if (loraStreamPushReqResCmdFrame(CMD_FW_SET_TX_CONFIG, (uint8_t*)sendBuffer, 6, &sReturn, sizeof(uint8_t)) == 0) {
		return (bool)sReturn;
	}
	return false;
}

bool LoraWanForwarder::getSendingConfigParams(uint32_t *frequency, uint8_t *dataRate, uint8_t *power) {
	uint8_t sendBuffer[6];
	if (loraStreamPushReqResCmdFrame(CMD_FW_GET_TX_CONFIG, NULL, 0, sendBuffer, 6) == 0) {
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

bool LoraWanForwarder::setReceivingConfigParams(uint32_t frequency, uint8_t dataRate) {
	uint8_t sReturn;
	uint8_t sendBuffer[5] = {0, 0, 0, 0, 0};

	*((uint32_t*)&sendBuffer[0]) = frequency;
	*((uint8_t*)&sendBuffer[4]) = dataRate;

	if (loraStreamPushReqResCmdFrame(CMD_FW_SET_RX_CONFIG, sendBuffer, 5, &sReturn, sizeof(uint8_t)) == 0) {
		return (bool)sReturn;
	}
	return false;
}

bool LoraWanForwarder::getReceivingConfigParams(uint32_t *frequency, uint8_t *dataRate) {
	uint8_t sendBuffer[5];
	if (loraStreamPushReqResCmdFrame(CMD_FW_GET_RX_CONFIG, NULL, 0, sendBuffer, 5) == 0) {
		*frequency = *((uint32_t*)&sendBuffer[0]);
		*dataRate = *((uint8_t*)&sendBuffer[4]);
		return true;
	}
	*frequency = (uint32_t)0xFFFFFFFF;
	*dataRate = (uint8_t)0xFF;
	return false;
}

bool LoraWanForwarder::sendData(uint8_t *pdata, uint8_t length) {
	uint8_t sReturn;

	uint8_t* sPframe = (uint8_t*)malloc(length);
	memcpy(&sPframe[0], pdata, length);
	if (loraStreamPushReqResCmdFrame(CMD_FW_SEND, sPframe, length, &sReturn, sizeof(uint8_t)) == 0) {
		return (bool)sReturn;
	}
	free(sPframe);
	return false;
}

void LoraWanForwarder::receivData(uint8_t *pdata, uint8_t length) {
	uint32_t tmst;
	int len, j;
	char freqStr[10];

	dtostrf((double)mGwFrequency / 1000000.0f, 7, 3, freqStr);
	cp_rxok++;
	mupBuff = (uint8_t*)malloc(PUSH_PACKET_MAX_SIZE);

	mtoken_h = 0xFF & random(0x00, 0xFF);
	mtoken_l = 0xFF & random(0x00, 0xFF);

	mupBuff[0] = (uint8_t)PROTOCOL_VERSION;
	mupBuff[1] = mtoken_h;
	mupBuff[2] = mtoken_l;
	mupBuff[3] = (uint8_t)PKT_PUSH_DATA;
	mupBuff[4] = mGatewayId[0];
	mupBuff[5] = mGatewayId[1];
	mupBuff[6] = mGatewayId[2];
	mupBuff[7] = 0xFF;
	mupBuff[8] = 0xFF;
	mupBuff[9] = mGatewayId[3];
	mupBuff[10] = mGatewayId[4];
	mupBuff[11] = mGatewayId[5];

	len += 12;
	tmst = micros();

	memcpy((void *)(mupBuff + len), (void *)"{\"rxpk\":[", 9);
	len += 9;
	mupBuff[len] = '{';
	++len;
	j = snprintf((char *)(mupBuff + len), PUSH_PACKET_MAX_SIZE - len, "\"tmst\":%lu", tmst);
	len += j;
	j = snprintf((char *)(mupBuff + len), PUSH_PACKET_MAX_SIZE - len, ",\"chan\":0,\"rfch\":0,\"freq\":%s", freqStr);
	len += j;
	memcpy((void *)(mupBuff + len), (void *)",\"stat\":1", 9);
	len += 9;
	memcpy((void *)(mupBuff + len), (void *)",\"modu\":\"LORA\"", 14);
	len += 14;
	/* Lora datarate & bandwidth, 16-19 useful chars */
	switch (mGwDatarate) {
	case 5:
		memcpy((void *)(mupBuff + len), (void *)",\"datr\":\"SF7", 12);
		len += 12;
		break;
	case 4:
		memcpy((void *)(mupBuff + len), (void *)",\"datr\":\"SF8", 12);
		len += 12;
		break;
	case 3:
		memcpy((void *)(mupBuff + len), (void *)",\"datr\":\"SF9", 12);
		len += 12;
		break;
	case 2:
		memcpy((void *)(mupBuff + len), (void *)",\"datr\":\"SF10", 13);
		len += 13;
		break;
	case 1:
		memcpy((void *)(mupBuff + len), (void *)",\"datr\":\"SF11", 13);
		len += 13;
		break;
	case 0:
		memcpy((void *)(mupBuff + len), (void *)",\"datr\":\"SF12", 13);
		len += 13;
		break;
	default:
		memcpy((void *)(mupBuff + len), (void *)",\"datr\":\"SF?", 12);
		len += 12;
	}

	memcpy((void *)(mupBuff + len), (void *)"BW125\"", 6);
	len += 6;
	memcpy((void *)(mupBuff + len), (void *)",\"codr\":\"4/5\"", 13);
	len += 13;
	j = snprintf((char *)(mupBuff + len), PUSH_PACKET_MAX_SIZE - len, ",\"lsnr\":%d", 0);
	len += j;
	j = snprintf((char *)(mupBuff + len), PUSH_PACKET_MAX_SIZE - len, ",\"rssi\":%d,\"size\":%d", 0, length);
	len += j;
	memcpy((void *)(mupBuff + len), (void *)",\"data\":\"", 9);
	len += 9;
	j = encode_base64(pdata, length, (unsigned char *)(mupBuff + len));
	len += j;
	mupBuff[len] = '"';
	++len;

	mupBuff[len] = '}';
	++len;
	mupBuff[len] = ']';
	++len;
	mupBuff[len] = '}';
	++len;
	mupBuff[len] = 0;

	/*Debug*/
	APP_PRINTLN((char*)(mupBuff + 12));

	mUdp->beginPacket(mServerAddr, mServerPortUp);
	mUdp->write((const unsigned char *)mupBuff, len);
	mUdp->endPacket();

	memset(mupBuff, 0, PUSH_ACK_PACKET_MAX_SIZE);
	for (int i = 0; i < 2; i++) {
		delay(PUSH_TIMEOUT_MS / 2);
		if ( mUdp->parsePacket() > 0 ) {
			int got = mUdp->read((char *)mupBuff, PUSH_ACK_PACKET_MAX_SIZE);
			if ((got < 4) || ((mupBuff[0] & 0xFF) != PROTOCOL_VERSION) || ((mupBuff[3] & 0xFF) != PKT_PUSH_ACK)) {
				APP_PRINTLN("ERR: [up] ignored invalid non-ACL packet");
				continue;
			} else if (((mupBuff[1] & 0xFF) != mtoken_h) || ((mupBuff[2] & 0xFF) != mtoken_l)) {
				APP_PRINTLN("ERR: [up] ignored out-of sync ACK packet");
				continue;
			} else {
				cp_rxfw++;
				APP_PRINTLN("INFO: [up] PUSH_ACK received");
				break;
			}
		}
	}
	free(mupBuff);
}

void LoraWanForwarder::parserJson(char* buffer, int len) {
	int size, bin_size, i, p1, p0;
	bool imme;
	uint32_t now;
	downLinkQueue_t* downlink;

	DeserializationError error = deserializeJson(jsdoc, buffer);
	if (error) {
		APP_PRINTLN("ERR: [down] deserializeJson()");
		return;
	}

	JsonObject root = jsdoc.as<JsonObject>();
	String modu = root["txpk"]["modu"].as<String>();
	if (!modu.equals("LORA")) {
		APP_PRINT("ERR: [down] modu:");
    APP_PRINTLN(modu);
		return;
	}

	now = micros();
	size = root["txpk"]["size"].as<int>();
	String data = root["txpk"]["data"].as<String>();
	if (data.length() <= 0 || size <= 0) return;

	bin_size = decode_base64_length((unsigned char*)data.c_str());
	downlink = (downLinkQueue_t*)malloc(sizeof(downlink->header) + bin_size);
	decode_base64((unsigned char*)data.c_str(), downlink->payload);
	downlink->header.data_len = bin_size;

	imme = root["txpk"]["imme"].as<bool>();
	if (imme) {
		downlink->header.tmts = micros();
	}
	else {
		downlink->header.tmts = root["txpk"]["tmst"].as<unsigned long>();
	}

	downlink->header.tx_frequency = (uint32_t)(root["txpk"]["freq"].as<double>() * 1000000);
	downlink->header.tx_power = (uint8_t)(root["txpk"]["powe"].as<int>());

	String datr = root["txpk"]["datr"].as<String>();  
	//i = sscanf(datr.c_str(), "SF%2hdBW%3hd", &p0, &p1);
	//if (i != 2) {
	//	APP_PRINT("ERR: [down] datr:");
  //  APP_PRINTLN(datr);
	//	free(downlink);
	//	return;
	//}
  String sf = datr.substring(datr.indexOf('F') + 1, datr.indexOf('B'));
	switch (sf.toInt()) {
	case  7: downlink->header.tx_datarate = 5;  break;
	case  8: downlink->header.tx_datarate = 4;  break;
	case  9: downlink->header.tx_datarate = 3;  break;
	case 10: downlink->header.tx_datarate = 2; break;
	case 11: downlink->header.tx_datarate = 1; break;
	case 12: downlink->header.tx_datarate = 0; break;
	default: {
		APP_PRINT("ERR: [down] SF:");
    APP_PRINTLN(sf.toInt());
		free(downlink);
		return;
	}
	}

	downLinkQueue.push_back(*downlink);

	APP_PRINT("tmts: ");
	APP_PRINTLN(downlink->header.tmts);
	APP_PRINT("tx_frequency: ");
	APP_PRINTLN(downlink->header.tx_frequency);
	APP_PRINT("tx_datarate: ");
	APP_PRINTLN(downlink->header.tx_datarate);
	APP_PRINT("tx_power: ");
	APP_PRINTLN(downlink->header.tx_power);
	APP_PRINT("data_len: ");
	APP_PRINTLN(downlink->header.data_len);
	APP_PRINT("payload: ");
	for (int i = 0; i < downlink->header.data_len; i++)
		APP_PRINT(downlink->payload[i], HEX);
	APP_PRINTLN();

	free(downlink);
}

void LoraWanForwarder::sendDownQueue(void) {
	downLinkQueue_t* downlink;

	for (SimpleList<downLinkQueue_t>::iterator itr = downLinkQueue.begin(); itr != downLinkQueue.end(); ) {
		downlink = &(*itr);
		if (micros() > downlink->header.tmts) {

			/* workaround because the thingnetwork not support EU433*/
			downlink->header.tx_frequency = mGwFrequency;
			downlink->header.tx_datarate = mGwDatarate;

			APP_PRINT("now: ");
			APP_PRINTLN(micros());
			APP_PRINT("tx_frequency: ");
			APP_PRINTLN(downlink->header.tx_frequency);
			APP_PRINT("tx_datarate: ");
			APP_PRINTLN(downlink->header.tx_datarate);
			APP_PRINT("tx_power: ");
			APP_PRINTLN(downlink->header.tx_power);
			APP_PRINT("data_len: ");
			APP_PRINTLN(downlink->header.data_len);
			APP_PRINT("payload: ");
			for (int i = 0; i < downlink->header.data_len; i++)
				APP_PRINT(downlink->payload[i], HEX);
			APP_PRINTLN();

			if ( setSendingConfigParams(downlink->header.tx_frequency, downlink->header.tx_datarate, downlink->header.tx_power)) {
				if ( sendData(downlink->payload, downlink->header.data_len)) {
					APP_PRINTLN("<-Sent");
				}
				else {
					APP_PRINTLN("<-Can't Sent");
				}
			}
			else {
				APP_PRINTLN("<-setSendingConfigParams()");
			}

			cp_txnb++;
			itr = downLinkQueue.erase(itr);
		}
		else {
			itr++;
		}
	}

}

uint8_t LoraWanForwarder::cmdFrameCalcFcs(uint8_t *pdata, uint8_t length) {
	uint8_t xorResult = length;
	for (int i = 0; i < length; i++, pdata++) {
		xorResult = xorResult ^ *pdata;
	}
	return xorResult;
}

int LoraWanForwarder::loraStreamPushReqCmdFrame(uint8_t cmdID, uint8_t *pdataReq, uint8_t lengthReq) {
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

int LoraWanForwarder::loraStreamCheckTimeOut(uint8_t length) {
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

int LoraWanForwarder::loraStreamPushReqResCmdFrame(uint8_t cmdID, uint8_t *pdataReq, uint8_t lengthReq, uint8_t *pdataRes, uint8_t lengthRes) {
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

int LoraWanForwarder::loraStreamParser(uint8_t byteData) {
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

void LoraWanForwarder::loraStreamFlush() {
	uint8_t c;
	while (mLoraStream->available()) {
		mLoraStream->read();
	}
}
