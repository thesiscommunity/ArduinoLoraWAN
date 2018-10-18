#include "SoftwareSerial.h"
#include "LoraWanServer.h"

/* Local config */
#define loraStream Serial1
//SoftwareSerial loraStream(2, 3);
LoraWanServer exLoraWan(&loraStream, 9600);

/* LoraWan config */
#define SERVER_FREQUENCY		433375000
#define SERVER_DATARATE		0
#define SERVER_TX_POWER		20

#define SERVER_NETWORK_ID	0x00112233
uint8_t exAppKey[16]	= { 0xD0, 0xF5, 0x47, 0x76, 0x4C, 0xEE, 0x28, 0xE5, 0x03, 0x96, 0x32, 0x5E, 0xED, 0x08, 0x0A, 0x5C };

//00D9C38F87016994
//70B3D57ED000BF50

void exLoraWanHandleIncommingMsg(unsigned char* msg) {
	int cmdId;
	//int dataLength;
	//unsigned char* data;

	cmdId = exLoraWan.getMsgCommandId(msg);
	//dataLength = exLoraWan.getMsgLength(msg);
	//data = exLoraWan.getMsgData(msg);

	Serial.print("->cmdId: "); Serial.println(cmdId);
	//Serial.print("->dataLength: "); Serial.println(dataLength);
	//Serial.print("->data[HEX]: ");
	//for (int i = 0; i < dataLength; i++)
	//	Serial.print(*(data + i), HEX);
	//Serial.println();

	switch (cmdId) {
	case LoraWanServer::CMD_SERV_RESP_JOINT_PERMIT: {
		uint8_t *devEui = exLoraWan.getOTADeviceEui(msg);
		uint8_t *appEui = exLoraWan.getOTAApplicationEui(msg);

		Serial.print("->DevEui[HEX]: ");
		for (int i = 0; i < 8; i++)
			Serial.print(*(devEui + i), HEX);
		Serial.println();

		Serial.print("->AppEui[HEX]: ");
		for (int i = 0; i < 8; i++)
			Serial.print(*(appEui + i), HEX);
		Serial.println();

		Serial.print("->DeviceAddr[HEX]: ");
		Serial.println(exLoraWan.getOTADeviceAddress(msg), HEX);
	}
		break;

	case LoraWanServer::CMD_SERV_RECV_UNCONFIRM_MSG:
	case LoraWanServer::CMD_SERV_RECV_CONFIRM_MSG: {
		uint8_t length = exLoraWan.getDataLength(msg);
		uint8_t *payload = exLoraWan.getData(msg);

		Serial.print("->DeviceAddr[HEX]: ");
		Serial.println(exLoraWan.getDataDeviceAddress(msg), HEX);

		Serial.print("->Ack: ");
		Serial.println(exLoraWan.isReceivedAck(msg));

		Serial.print("->Port: ");
		Serial.println(exLoraWan.getDataPort(msg));

		Serial.print("->Length: ");
		Serial.println(length);

		Serial.print("->Payload[HEX]: ");
		for (int i = 0; i < length; i++)
			Serial.print(*(payload + i), HEX);
		Serial.println();

		//Serial.print("->");
		//Serial.println(*((uint16_t*)payload));
	}
		break;

	case LoraWanServer::CMD_SERV_RECV_CONFIRM: {
		Serial.print("->DeviceAddr: ");
		Serial.println(exLoraWan.getAckDeviceAddress(msg));
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

	if (!exLoraWan.configRadio(SERVER_FREQUENCY, SERVER_DATARATE, SERVER_TX_POWER)) {
		Serial.println("<-configRadio()");
		while(1);
	}

	if (!exLoraWan.setNetworkingConfigParams(SERVER_NETWORK_ID, exAppKey)) {
		Serial.println("<-setNetworkingConfigParams()");
		while(1);
	}

	exLoraWan.incommingMsgHandlerRegister(exLoraWanHandleIncommingMsg);
	Serial.println("Done!");

	printPanel();
}

void loop() {
	exLoraWan.update();

	if (Serial.available() > 0) {
		switch (Serial.read()) {
		case '1': {
			uint32_t devAddr;
			uint8_t devPort;
			char *devString, *devData;

			Serial.print("DeviceAddr[HEX]: ");
			devAddr = htol(readInput());
			Serial.println(devAddr, HEX);
			Serial.print("Port: ");
			devPort = atoi(readInput());
			Serial.println(devPort);
			Serial.print("Data: ");
			devString = readInput();
			Serial.println(devString);
			devData = htoa(devString);
			if (devData == NULL ) break;

			if (exLoraWan.sendUnconfirmedData(devAddr, devPort, devData, strlen(devString) / 2)) {
				Serial.println("OK");
			}
			else {
				Serial.println("Error");
			}
		}
			break;

		case '2': {
			uint32_t devAddr;
			uint8_t devPort, devRetries;
			char *devString, *devData;

			Serial.print("DeviceAddr[HEX]: ");
			devAddr = htol(readInput());
			Serial.println(devAddr, HEX);
			Serial.print("Retries: ");
			devRetries = htol(readInput());
			Serial.println(devRetries);
			Serial.print("Port: ");
			devPort = atoi(readInput());
			Serial.println(devPort);
			Serial.print("Data: ");
			devString = readInput();
			Serial.println(devString);
			devData = htoa(devString);
			if (devData == NULL ) break;

			if (exLoraWan.sendConfirmedData(devAddr, devRetries, devPort, devData, strlen(devString) / 2)) {
				Serial.println("OK");
			}
			else {
				Serial.println("Error");
			}
		}
			break;

		case '3': {
			char *serString;
			uint8_t *tmp;
			uint8_t devEUI[8], appEUI[8];

			Serial.println("DeviceEUI: ");
			serString = readInput();
			Serial.println(serString);
			tmp = htoa(serString);
			if (tmp == NULL) break;
			memcpy(devEUI, tmp, 8);
			Serial.println("AppicationEUI: ");
			serString = readInput();
			Serial.println(serString);
			tmp = htoa(serString);
			if (tmp == NULL) break;
			memcpy(appEUI, tmp, 8);

			if (exLoraWan.setOTADeviceParams(devEUI, appEUI, 90000)) {
				Serial.println("OK");
			}
			else {
				Serial.println("Error");
			}
		}
			break;

		case '4':
			if (exLoraWan.removeAllDevice()) {
				Serial.println("OK");
			}
			else {
				Serial.println("Error");
			}
			break;

		case '5': {
			uint32_t devAddr;

			Serial.print("DeviceAddr[HEX]: ");
			devAddr = htol(readInput());
			Serial.println(devAddr, HEX);
			if (exLoraWan.removeDevice(devAddr)) {
				Serial.println("OK");
			}
			else {
				Serial.println("Error");
			}
		}
			break;

		default:
			break;
		}
		printPanel();
		serialClear();
	}
}

void printPanel() {
	Serial.println();
	Serial.println("1: Send Unconfirmed Msg");
	Serial.println("2: Send Confirmed Msg");
	Serial.println("3: Set OTA Device");
	Serial.println("4: Remove all devices");
	Serial.println("5: Remove device");
	Serial.println();
}

void serialClear() {
	delay(100);
	while (Serial.available() > 0) Serial.read();
}

uint32_t htol(const char *s) {
	uint32_t al=0,l=0; char c;
	if(s) {
		do  {
			c=*s++; l<<=4;
			if(c>58)    c&=223;
			al=c-'0';
			if(al>10)    al-=7;
			l+=al;
		} while(*s);
	}
	return l;
}

char* htoa(const char* source) {
	static char dest[64];
	char sbuff[3];
	int  dbuff[1];
	int source_size = strlen(source);
	if (!source_size || (source_size % 2 != 0)) {
		return NULL;
	}
	for (int i = 0, k = 0; i < source_size; i=i+2, k++) {
		sbuff[0] = tolower(source[i]);
		sbuff[1] = tolower(source[i+1]);
		sbuff[2] = '\0';
		if (sscanf(sbuff, "%02x", dbuff) == 1){
			dest[k] = (char) dbuff[0];
		}
		else {
			return NULL;
		}
	}
	return dest;
}

char* readInput() {
#define MAX_BUFFER_SIZE	256
	static char buffer[MAX_BUFFER_SIZE];
	serialClear();
	Serial.setTimeout(5000);
	int got = Serial.readBytesUntil('\r', buffer, MAX_BUFFER_SIZE);
	if (!got) return NULL;
	buffer[got] = '\0';
	Serial.setTimeout(1000);
	return buffer;
}
