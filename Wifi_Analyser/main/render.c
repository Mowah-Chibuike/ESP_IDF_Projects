#include "wifi_analyser.h"
#include "display_driver.h"
#include "font.h"
#include "stdio.h"

void render_scanning_mode(output_message_t msg)
{
    int num_ap = 4; // number of access points to display
    dd_render_text(0, 2, SIZE8, "SCANNING...");
    dd_draw_hline(11);

    char line[24] = {0};
    if (msg.networkCount > 0)
        for (int i = 0; i < num_ap; i++)
        {
            memset(line, 0, sizeof(line));
            snprintf(line, sizeof(line), "> %-8.8s %4ddBm", msg.foundSSIDs[i], (int)msg.foundRSSIs[i]);
            dd_render_text(0, 13 + i * FONT_HEIGHT, SIZE8, line);
        }
    
    memset(line, 0, sizeof(line));
    snprintf(line, sizeof(line), "Found: %3d", (int)msg.networkCount);
    dd_render_text(0, 47, SIZE8, line);

    memset(line, 0, sizeof(line));
    // char truncatedSSID[14] = {0};
    // snprintf(truncatedSSID, sizeof(truncatedSSID), "%s", TARGET_SSID);
    snprintf(line, sizeof(line), "Target %-9s", msg.targetFound ? "Seen" : "Not Seen");
    dd_render_text(0, 56, SIZE8, line);
}

void render_locked_mode(output_message_t msg)
{
    char line[24] = {0};
    char truncatedSSID[14] = {0};
    snprintf(truncatedSSID, sizeof(truncatedSSID), "%s", TARGET_SSID);
    dd_render_text(0, 0, SIZE8, truncatedSSID);

    memset(line, 0, sizeof(line));
    // get_bssid_string(line, targetBSSID);
    snprintf(line, sizeof(line), "Channel: %4d", targetChannel);
    dd_render_text(0, 11, SIZE8, line);

    dd_draw_hline(22);

    memset(line, 0, sizeof(line));
    snprintf(line, sizeof(line), "RSSI: %4ddBm", msg.RSSI);
    dd_render_text(0, 32, SIZE8, line);

    memset(line, 0, sizeof(line));
    snprintf(line, sizeof(line), "SNR: %4d", msg.SNR);
    dd_render_text(0, 42, SIZE8, line);

    memset(line, 0, sizeof(line));
    snprintf(line, sizeof(line), "Quality: %-10s", quality_to_string(msg.quality));
    dd_render_text(0, 52, SIZE8, line);
}

void render_degraded(output_message_t msg)
{
    char line[24] = {0};
    char truncatedSSID[14] = {0};
    snprintf(truncatedSSID, sizeof(truncatedSSID), "%s", TARGET_SSID);
    dd_render_text(0, 0, SIZE8, truncatedSSID);

    dd_render_text(0, 10, SIZE8, "!!WARNING!!");

    memset(line, 0, sizeof(line));
    snprintf(line, sizeof(line), "RSSI: %4ddBm", msg.RSSI);
    dd_render_text(0, 22, SIZE8, line);

    memset(line, 0, sizeof(line));
    snprintf(line, sizeof(line), "SNR: %4d", msg.SNR);
    dd_render_text(0, 33, SIZE8, line);

    memset(line, 0, sizeof(line));
    snprintf(line, sizeof(line), "Quality: %-10s", quality_to_string(msg.quality));
    dd_render_text(0, 43, SIZE8, line);
}

void render_lost(output_message_t msg)
{

    dd_render_text(32, 24, SIZE8, "!! Signal Lost !!");
    dd_render_text(32, 35, SIZE8, "Rescanning...");

}