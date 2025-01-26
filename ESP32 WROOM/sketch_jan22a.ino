#include <esp_bt.h>
#include <esp_gap_ble_api.h>
#include <esp_bt_main.h>

// Constants
static const uint8_t HWID[5] = {0x01, 0x83, 0xee, 0x80, 0xe7};
static const uint8_t ADC_PIN = 35;
static const float R1 = 4000.0;
static const float R2 = 2000.0;
static const uint16_t UUID_FOR_LINECORP = 0xFE6F;
static const uint8_t MAX_SIMPLEBEACON_DEVICEMESSAGE_SIZE = 13;
static uint8_t deviceMessageSize = 1;
static uint8_t deviceMessage[MAX_SIMPLEBEACON_DEVICEMESSAGE_SIZE];
static const uint8_t MAX_BLE_ADVERTISING_DATA_SIZE = 31;
static const uint16_t HCI_LE_Set_Advertising_Data = (0x08 << 10) | 0x0008;
static const uint16_t HCI_LE_Set_Advertising_Enable = (0x08 << 10) | 0x000A;
static const uint16_t HCI_LE_Set_Advertising_Parameters = (0x08 << 10) | 0x0006;

// Advertising Parameters
static const uint16_t MIN_ADVERTISING_INTERVAL = 0x0020; // 20ms
static const uint16_t MAX_ADVERTISING_INTERVAL = 0x0020;
static const uint8_t ADVERTISING_TYPE = 0x03; // NON_CONNECT_NON_SCAN
static const uint8_t OWN_ADDRESS_TYPE = 0x00;
static const uint8_t PEER_ADDRESS_TYPE = 0x00;
static const uint8_t PEER_ADDRESS[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t ADVERTISING_CHANNEL_MAP = 0x07; // All channels
static const uint8_t ADVERTISING_FILTER = 0x00;

static void _dump(const char *title, uint8_t *data, size_t dataSize) {
    Serial.printf("%s [%d]:", title, dataSize);
    for (size_t i = 0; i < dataSize; i++) {
        Serial.printf(" %02x", data[i]);
    }
    Serial.println();
}

static void putUint8(uint8_t** bufferPtr, uint8_t data) {
    *(*bufferPtr)++ = data;
}

static void putUint16LE(uint8_t** bufferPtr, uint16_t data) {
    *(*bufferPtr)++ = lowByte(data);
    *(*bufferPtr)++ = highByte(data);
}

static void putArray(uint8_t** bufferPtr, const void* data, size_t dataSize) {
    memcpy(*bufferPtr, data, dataSize);
    (*bufferPtr) += dataSize;
}

static void executeBluetoothHCICommand(uint16_t opCode, const uint8_t *hciData, uint8_t hciDataSize) {
    uint8_t buf[5 + MAX_BLE_ADVERTISING_DATA_SIZE];
    uint8_t* bufPtr = buf;

    putUint8(&bufPtr, 1);
    putUint16LE(&bufPtr, opCode);
    putUint8(&bufPtr, hciDataSize);
    putArray(&bufPtr, hciData, hciDataSize);

    uint8_t bufSize = bufPtr - buf;
    while (!esp_vhci_host_check_send_available());
    esp_vhci_host_send_packet(buf, bufSize);
}

static void updateSimpleBeaconDeviceMessage(char dm[], int dmsize) {
    memset(deviceMessage, 0x00, MAX_SIMPLEBEACON_DEVICEMESSAGE_SIZE);
    uint8_t* deviceMessagePtr = deviceMessage;
    const uint8_t voltageStringSize = dmsize - 1;
    putArray(&deviceMessagePtr, dm, voltageStringSize);
    deviceMessageSize = 13;
}

static void setAdvertisingParameters() {
    uint8_t params[15];
    uint8_t* ptr = params;

    putUint16LE(&ptr, MIN_ADVERTISING_INTERVAL);
    putUint16LE(&ptr, MAX_ADVERTISING_INTERVAL);
    putUint8(&ptr, ADVERTISING_TYPE);
    putUint8(&ptr, OWN_ADDRESS_TYPE);
    putUint8(&ptr, PEER_ADDRESS_TYPE);
    putArray(&ptr, PEER_ADDRESS, 6);
    putUint8(&ptr, ADVERTISING_CHANNEL_MAP);
    putUint8(&ptr, ADVERTISING_FILTER);

    executeBluetoothHCICommand(HCI_LE_Set_Advertising_Parameters, params, sizeof(params));
}

static void updateAdvertisingData() {
    uint8_t data[MAX_BLE_ADVERTISING_DATA_SIZE];
    uint8_t* dataPtr = data;

    // Add BLE flags
    putUint8(&dataPtr, 1 + 1);
    putUint8(&dataPtr, ESP_BLE_AD_TYPE_FLAG);
    putUint8(&dataPtr, ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT);

    // Add service UUID
    putUint8(&dataPtr, 1 + 2);
    putUint8(&dataPtr, ESP_BLE_AD_TYPE_16SRV_CMPL);
    putUint16LE(&dataPtr, UUID_FOR_LINECORP);

    // Add beacon data with increased TX power
    putUint8(&dataPtr, 1 + 9 + deviceMessageSize);
    putUint8(&dataPtr, ESP_BLE_AD_TYPE_SERVICE_DATA);
    putUint16LE(&dataPtr, UUID_FOR_LINECORP);
    putUint8(&dataPtr, 0x02);
    putArray(&dataPtr, HWID, 5);
    putUint8(&dataPtr, 0x01); // Maximum TX power
    putArray(&dataPtr, deviceMessage, deviceMessageSize);

    uint8_t dataSize = dataPtr - data;
    _dump("simple beacon", data, dataSize);

    uint8_t hciDataSize = 1 + MAX_BLE_ADVERTISING_DATA_SIZE;
    uint8_t hciData[hciDataSize];
    hciData[0] = dataSize;
    memcpy(hciData + 1, data, dataSize);

    executeBluetoothHCICommand(HCI_LE_Set_Advertising_Data, hciData, hciDataSize);
}

static void enableBluetoothAdvertising() {
    uint8_t enable = 1;
    executeBluetoothHCICommand(HCI_LE_Set_Advertising_Enable, &enable, 1);
}

static void notifyHostSendAvailableHandler() {
    // Empty callback
}

static int notifyHostRecvHandler(uint8_t *data, uint16_t len) {
    return 0;
}

static esp_vhci_host_callback_t vhciHostCallback = {
    notifyHostSendAvailableHandler,
    notifyHostRecvHandler
};

void setup() {
    Serial.begin(115200, SERIAL_8N1);
    
    // Set ADC parameters
    analogSetAttenuation(ADC_11db);
    analogReadResolution(12);

    // Initialize Bluetooth with maximum power
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    bt_cfg.ble_max_conn = 0;
    bt_cfg.mode = ESP_BT_MODE_BLE;
    bt_cfg.bt_max_acl_conn = 0;
    bt_cfg.bt_max_sync_conn = 0;

    esp_bt_controller_init(&bt_cfg);
    esp_bt_controller_enable(ESP_BT_MODE_BLE);
    
    // Set maximum TX power
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_P9);

    btStart();
    esp_vhci_host_register_callback(&vhciHostCallback);
    
    setAdvertisingParameters();
    enableBluetoothAdvertising();
}

void loop() {
    char dm[] = "Hello World";
    int dm_size = sizeof(dm) / sizeof(dm[0]);
    
    updateSimpleBeaconDeviceMessage(dm, dm_size);
    updateAdvertisingData();
    
    delay(100); // Minimal delay
}