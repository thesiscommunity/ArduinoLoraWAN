#include "LoraWanEndDevice.h"
#include "SoftwareSerial.h"

bool actived_flag;
uint32_t now, timer = 0;
int sensorPin = A0;
uint16_t sensorValue = 0;

SoftwareSerial loraStream(2, 3);
LoraWanEndDevice exLoraWan(&loraStream, 9600);

uint8_t exDevEUI[8] = { 0x00, 0xE0, 0x5B, 0x76, 0x10, 0xED, 0x42, 0x1C };
uint8_t exAppEUI[8] = { 0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x00, 0xBF, 0x50 };

uint32_t exDeviceAddress = 0x260415B5;
uint8_t exAppSKey[16]	= { 0x7E, 0x81, 0xE4, 0x86, 0xEA, 0x57, 0x60, 0xA9, 0x34, 0xBF, 0xD8, 0xE7, 0xF1, 0xE8, 0x15, 0x92 };
uint8_t exNwkSKey[16]	= { 0x0D, 0x14, 0xF2, 0xD1, 0xCE, 0xD3, 0xE8, 0x54, 0x33, 0x37, 0x43, 0xBF, 0xD5, 0xF1, 0xEA, 0x65 };

void exLoraWanHandleIncommingMsg(uint8_t* msg) {
	uint8_t cmdID = exLoraWan.getMsgCommandId(msg);

	switch (cmdID) {
	case LoraWanEndDevice::CMD_ENDD_RECV_UNCONFIRM_MSG:
	case LoraWanEndDevice::CMD_ENDD_RECV_CONFIRM_MSG: {

		Serial.print("->getDataRxSlot: "); Serial.println(exLoraWan.getDataRxSlot(msg));
		Serial.print("->getDataRSSI: "); Serial.println(exLoraWan.getDataRSSI(msg));
		Serial.print("->getDataSNR: "); Serial.println(exLoraWan.getDataSNR(msg));
		Serial.print("->getDataPort: "); Serial.println(exLoraWan.getDataPort(msg));
		Serial.print("->DataLen: "); Serial.println(exLoraWan.getDataLength(msg));
		Serial.print("->Data[HEX]: ");
		uint8_t * sData = exLoraWan.getData(msg);
		for (int i = 0; i < exLoraWan.getDataLength(msg); i++)
			Serial.print(*(sData + i), HEX);
		Serial.println();
	}
		break;

	default:
		break;
	}

}

void setup() {
	Serial.begin(9600);
	loraStream.begin(9600);

	Serial.println();
	Serial.println("Setting...");

	exLoraWan.incommingMsgHandlerRegister(exLoraWanHandleIncommingMsg);
	exLoraWan.setOperationMode(MODE_ENDDEVICE);
	exLoraWan.setDeviceClass(CLASS_C);

	exLoraWan.setDeviceEUI(exDevEUI, 8);
	exLoraWan.setAppEUI(exAppEUI, 8);

	if (!exLoraWan.setDeviceAddr(exDeviceAddress))
	{
		Serial.println("<-setDeviceAddr()");
		while (1);
	}

	if (!exLoraWan.setNetworkSessionKey(exNwkSKey, 16)) {
		Serial.println("<-setNetworkSessionKey()");
	}

	if (!exLoraWan.setAppSessionKey(exAppSKey, 16)) {
		Serial.println("<-setAppSessionKey()");
	}

	exLoraWan.setChannelMask((uint16_t)0x0002); // 433375000 Mhz
	exLoraWan.setDataRate(DR_0); // SF12, BW125
	exLoraWan.setRx2Frequency((uint32_t)433375000); // 433375000 Mhz
	exLoraWan.setRx2DataRate(DR_0); // SF12, BW125
	exLoraWan.setTxPower(TX_PWR_20dB); //20dBm

	exLoraWan.setActiveMode(ACTIVE_ABP);
	exLoraWan.setAntGain(0);

	actived_flag = true;
	Serial.println("Done!");
}

void loop() {
	exLoraWan.update();

	now = millis();
	if ( actived_flag && \
		 ((timer == 0 || ((uint32_t)(now - timer)) > 15000))) { /* 15 second */
		timer = now;

		sensorValue = analogRead(sensorPin);
		if ( exLoraWan.sendUnconfirmedData(1, DR_0, (uint8_t*)&sensorValue, sizeof(sensorValue)) ) {
			//if ( exLoraWan.sendConfirmedData(1, DR_0, 1, (uint8_t*)&sensorValue, sizeof(sensorValue)) ) {
			Serial.print("<-Sent[HEX]: ");
			Serial.println(sensorValue, HEX);
		}
		else {
			Serial.println("<-Can't Sent");
		}
	}
}
