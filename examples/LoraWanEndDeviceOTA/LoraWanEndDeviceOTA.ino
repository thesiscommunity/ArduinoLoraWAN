#include "LoraWanEndDevice.h"
#include "SoftwareSerial.h"

bool actived_flag;
uint32_t now, timer = 0;
int sensorPin = A0;
uint16_t sensorValue = 0;

SoftwareSerial loraStream(2, 3);
LoraWanEndDevice exLoraWan(&loraStream, 9600);

uint8_t exDevEUI[8] = { 0x00, 0xD9, 0xC3, 0x8F, 0x87, 0x01, 0x69, 0x94 };
uint8_t exAppEUI[8] = { 0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x00, 0xBF, 0x50 };
uint8_t exAppKey[16] = { 0xD0, 0xF5, 0x47, 0x76, 0x4C, 0xEE, 0x28, 0xE5, 0x03, 0x96, 0x32, 0x5E, 0xED, 0x08, 0x0A, 0x5C };

void exLoraWanHandleIncommingMsg(uint8_t* msg) {
	uint8_t cmdID = exLoraWan.getMsgCommandId(msg);

	switch (cmdID) {
	case LoraWanEndDevice::CMD_ENDD_OTA_STATUS: {
		uint8_t OTAStatus = *exLoraWan.getData(msg);
		if (OTAStatus) {
			actived_flag = true;
			Serial.println("OTA succeeded");

			exLoraWan.setRx2Frequency((uint32_t)433375000); // 43337A5000 Mhz
			exLoraWan.setRx2DataRate(DR_0); // SF12, BW125
		}
		else {
			Serial.println("OTA timeout");
		}
	}
		break;

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

	if (!exLoraWan.setDeviceEUI(exDevEUI, 8))
	{
		Serial.println("<-setDeviceEUI()");
		while (1);
	}

	if (!exLoraWan.setAppEUI(exAppEUI, 8))
	{
		Serial.println("<-setAppEUI()");
		while (1);
	}

	if (!exLoraWan.setAppKey(exAppKey, 16))
	{
		Serial.println("<-setAppKey()");
		while (1);
	}

	exLoraWan.setChannelMask((uint16_t)0x0002); // 433375000 Mhz
	exLoraWan.setDataRate(DR_0); // SF12, BW125	
	exLoraWan.setRx2Frequency((uint32_t)433375000); // 43337A5000 Mhz
	exLoraWan.setRx2DataRate(DR_0); // SF12, BW125
	exLoraWan.setTxPower(TX_PWR_20dB); //20dBm

	exLoraWan.setActiveMode(ACTIVE_OTA);
	exLoraWan.setAntGain(0);	

	actived_flag = false;
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
