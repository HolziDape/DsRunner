// SPDX-License-Identifier: Zlib
//
// DsRunner Stream: settings from /_dsrunner/stream.ini

#ifndef DSSTREAM_CONFIG_H
#define DSSTREAM_CONFIG_H

#include <stdbool.h>

#define CONFIG_PATH "/_dsrunner/stream.ini"

typedef enum {
    WIFI_AUTO,  // WFC access point if one is in range, else ssid=, else a list
    WIFI_WFC,   // only the WFC settings of the firmware
    WIFI_LIST,  // always show the list
} WifiChoice;

typedef enum {
    IP_AUTO,    // DHCP; on the DsRunner hotspot a fixed address when DHCP doesn't answer
    IP_DHCP,    // only DHCP
    IP_STATIC,  // the address from ip= (gateway= / mask=)
} IpChoice;

typedef struct {
    char host[64];          // IP of the host, empty = search
    int port;
    WifiChoice wifi;
    char ssid[33];          // network for WIFI_AUTO when no WFC AP is found
                            // (default "DsRunner-Stream", empty in the file = none)
    char key[33];           // WEP key for ssid (5/13 characters or 10/26 hex digits)
    IpChoice ip_mode;
    char ip[16];            // fixed address of the DS (IP_STATIC, and the IP_AUTO fallback)
    char gateway[16];       // address of the Pi/router, empty = x.y.z.1 of ip
    char mask[16];
    bool screen_top;        // picture on the top screen (one-screen modes)
    bool found;             // the file was read
    bool fat;               // the SD card could be read at all
} Config;

void config_load(Config *c);

#endif
