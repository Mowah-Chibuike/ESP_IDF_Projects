#include "wifi_analyser.h"

esp_err_t initialize_nvs() {
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    
    return(ret);
}

esp_err_t initialize_wifi() {
    esp_err_t ret;

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    
    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WIFI init failed: %s", esp_err_to_name(ret));
    }
    return (ret);
}

void scan_timer_callback(void *arg)
{
    analyser_event_t message = {
        .type = STATE_SCANNING
    };
    xQueueSendToBackFromISR(stateQueue, &message, NULL);
}

void perform_scan(void)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "Scanning...");
    ret = esp_wifi_scan_start(NULL, true);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "WiFi scan failed: %s", esp_err_to_name(ret));
        analyser_event_t message;
        message.type = EVENT_SCAN_FAILED;
        xQueueSendToBack(stateQueue, &message, portMAX_DELAY);
        return;
    }

    uint16_t num_of_ap = 20;
    wifi_ap_record_t records[20];
    ret = esp_wifi_scan_get_ap_num(&num_of_ap);
    memset(records, 0, sizeof(records));
    ret = esp_wifi_scan_get_ap_records(&num_of_ap, records); 
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "WiFi scan failed: %s", esp_err_to_name(ret));
        analyser_event_t message;
        message.type = EVENT_SCAN_FAILED;
        xQueueSendToBack(stateQueue, &message, portMAX_DELAY);
    } else {
        bool found = false;

        memset(&out, 0, sizeof(out));
        out.state = STATE_SCANNING;
        out.networkCount = num_of_ap;
        for (uint16_t i = 0; i < num_of_ap; i++)
        {
            char *ssid = (char *)records[i].ssid;
            uint8_t channel = records[i].primary;
            int8_t rssi = records[i].rssi;

            if (i < 5)
            {
                memset(out.foundSSIDs[i], 0, sizeof(out.foundSSIDs[i]));
                memcpy(out.foundSSIDs[i], records[i].ssid, 33);
                out.foundRSSIs[i] = rssi;
            }

            
            ESP_LOGI(TAG, "Found: SSID: %s; Channel: %d; rssi: %d", ssid, channel, rssi);
            if (strcmp(ssid, TARGET_SSID) == 0)
            {
                found = true;
                out.targetFound = true;
                ESP_LOGI(TAG, "Found %s successfully", ssid);
                xQueueReset(stateQueue);
                xQueueReset(outputQueue);
                analyser_event_t message;
                message.type = EVENT_SCAN_SUCCESS;
                memcpy(message.data.scanResult.bssid, records[i].bssid, 6);
                message.data.scanResult.channel = channel;
                message.data.scanResult.rssi = rssi;
                xQueueSendToBack(stateQueue, &message, portMAX_DELAY);
            }

            if (found && i > 5)
                break;
        }
        if (!found)
        {
            analyser_event_t message;
            message.type = EVENT_SCAN_FAILED;
            xQueueSendToBack(stateQueue, &message, portMAX_DELAY);
        }
    }
}

bool parse_beacon(const uint8_t *payload, uint16_t length, int8_t rssi, beacon_info_t *out)
{
    if (length < 38) return false;

    uint8_t frameType    = (payload[0] & 0x0C) >> 2;
    uint8_t frameSubtype = (payload[0] & 0xF0) >> 4;
    if(frameType != 0 || frameSubtype != 8) return false;

    memcpy(out->bssid, payload + 10, 6);
    out->beaconInterval = payload[32] | (payload[33] << 8);
    out->rssi = rssi;

    memset(out->ssid, 0, sizeof(out->ssid));

    uint16_t pos = 36;
    bool foundSSID = false;
    while(pos + 2 <= length)
    {
        uint8_t tagID  = payload[pos];
        uint8_t tagLen = payload[pos + 1];

        if(pos + 2 + tagLen > length) break;

        const uint8_t *tagData = payload + pos + 2;

        switch(tagID)
        {
            case 0:  // SSID
                if(tagLen > 0 && tagLen <= 32)
                {
                    memcpy(out->ssid, tagData, tagLen);
                    out->ssid[tagLen] = '\0';
                }
                foundSSID = true;
                break;
        }

        if (foundSSID)
            break;
        pos += 2 + tagLen;
    }
    return (true);
}

void packet_rx_cb(void *buf, wifi_promiscuous_pkt_type_t type) {

    if (type != WIFI_PKT_MGMT) return;

    wifi_promiscuous_pkt_t *packet = (wifi_promiscuous_pkt_t *)buf;
    uint16_t frameLen = packet->rx_ctrl.sig_len;

    beacon_info_t info;
    if (!parse_beacon(packet->payload, frameLen, packet->rx_ctrl.rssi, &info))
        return;
    
    if (memcmp(info.bssid, targetBSSID, 6) == 0)
    {
        int8_t raw_snr = packet->rx_ctrl.rssi - packet->rx_ctrl.noise_floor;
        analyser_val_t raw = {
            .rssi = packet->rx_ctrl.rssi,
            .snr = raw_snr
        };
        lastSeen = esp_timer_get_time();
        BaseType_t xHigherPriorityTaskWoken;
        xHigherPriorityTaskWoken = pdFALSE;
        xQueueSendFromISR(rawValQueue, &raw, &xHigherPriorityTaskWoken);
        
        if(xHigherPriorityTaskWoken) portYIELD_FROM_ISR();
    }
        
}

signal_quality_t classify_snr(int8_t snr)
{
    if(snr > 25) return SIGNAL_EXCELLENT;
    if(snr > 15) return SIGNAL_GOOD;
    if(snr > 10) return SIGNAL_FAIR;
    if(snr >  5) return SIGNAL_POOR;
    return SIGNAL_UNUSABLE;
}

const char* quality_to_string(signal_quality_t quality)
{
    switch(quality)
    {
        case SIGNAL_EXCELLENT: return "Excellent";
        case SIGNAL_GOOD:      return "Good";
        case SIGNAL_FAIR:      return "Fair";
        case SIGNAL_POOR:      return "Poor";
        case SIGNAL_UNUSABLE:  return "Unusable";
        default:               return "Unknown";
    }
}

void get_bssid_string(char *output, uint8_t bssid[6])
{
    char template[18] = {0};

    snprintf(template, sizeof(template), "%02X:%.0X:%02X:%02X:%02X:%02X", bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
    memset(output, 0, 19);
    memcpy(output, template, sizeof(template));
}