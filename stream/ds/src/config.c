// SPDX-License-Identifier: Zlib
//
// DsRunner Stream: settings from /_dsrunner/stream.ini
//
//   ; DsRunner Stream
//   host=192.168.4.1     ; empty: search the host automatically
//   port=7770
//   wifi=auto            ; auto, wfc or list
//   ssid=DsRunner-Stream ; open network to use when no WFC setting matches
//   key=                 ; WEP key of that network, empty for open networks
//   ip=auto              ; auto: DHCP, on the DsRunner hotspot a fixed address when DHCP
//                        ; doesn't answer; dhcp: only DHCP; or a fixed address like 192.168.4.5
//   gateway=             ; with a fixed address: the Pi/router (default x.y.z.1)
//   mask=255.255.255.0
//   screen=bottom        ; bottom (touch works on the picture) or top

#include <ctype.h>
#include <fat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "config.h"

static char *trim(char *s)
{
    while (*s && isspace((unsigned char)*s))
        s++;
    char *e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1]))
        *--e = 0;
    return s;
}

static void copy(char *dst, size_t size, const char *src)
{
    strncpy(dst, src, size - 1);
    dst[size - 1] = 0;
}

void config_load(Config *c)
{
    memset(c, 0, sizeof(*c));
    c->port = 7770;
    c->wifi = WIFI_AUTO;
    // the name the hotspot of stream/host/tools/hotspot.sh gets
    copy(c->ssid, sizeof(c->ssid), "DsRunner-Stream");
    // fixed address in the hotspot of hotspot.sh (its DHCP range starts at .10)
    copy(c->ip, sizeof(c->ip), "192.168.4.5");
    copy(c->mask, sizeof(c->mask), "255.255.255.0");

    c->fat = fatInitDefault();
    if (!c->fat)
        return;
    FILE *f = fopen(CONFIG_PATH, "r");
    if (!f)
        return;
    c->found = true;
    char line[160];
    while (fgets(line, sizeof(line), f))
    {
        char *hash = strpbrk(line, ";#");
        if (hash)
            *hash = 0;
        char *eq = strchr(line, '=');
        if (!eq)
            continue;
        *eq = 0;
        char *k = trim(line);
        char *v = trim(eq + 1);
        if (!strcasecmp(k, "host"))
            copy(c->host, sizeof(c->host), v);
        else if (!strcasecmp(k, "port") && atoi(v) > 0)
            c->port = atoi(v);
        else if (!strcasecmp(k, "wifi"))
            c->wifi = !strcasecmp(v, "wfc") ? WIFI_WFC : !strcasecmp(v, "list") ? WIFI_LIST : WIFI_AUTO;
        else if (!strcasecmp(k, "ssid"))
            copy(c->ssid, sizeof(c->ssid), v);
        else if (!strcasecmp(k, "key"))
            copy(c->key, sizeof(c->key), v);
        else if (!strcasecmp(k, "ip"))
        {
            if (!*v || !strcasecmp(v, "auto"))
                c->ip_mode = IP_AUTO;
            else if (!strcasecmp(v, "dhcp"))
                c->ip_mode = IP_DHCP;
            else
            {
                c->ip_mode = IP_STATIC;
                copy(c->ip, sizeof(c->ip), v);
            }
        }
        else if (!strcasecmp(k, "gateway"))
            copy(c->gateway, sizeof(c->gateway), v);
        else if (!strcasecmp(k, "mask") && *v)
            copy(c->mask, sizeof(c->mask), v);
        else if (!strcasecmp(k, "screen"))
            c->screen_top = !strcasecmp(v, "top") || !strcasecmp(v, "oben");
    }
    fclose(f);
}
