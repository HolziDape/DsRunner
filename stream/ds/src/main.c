// SPDX-License-Identifier: Zlib
//
// DsRunner Stream: shows a web page that a Raspberry Pi or PC streams over WiFi
// (stream/host) and sends the buttons and the touch screen back.
//
// Flow: settings -> WiFi (WFC settings of the firmware, or an open network) ->
// find the host (broadcast, or host= in /_dsrunner/stream.ini) -> stream.
// While streaming, SELECT + START held for one second opens the pause menu.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nds.h>
#include <dswifi9.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "bench.h"
#include "config.h"
#include "decode.h"
#include "video.h"

#ifndef DSSTREAM_VERSION
#define DSSTREAM_VERSION "0.1"
#endif

#define MAX_APS 12

static Config cfg;
static volatile uint32_t vbl;               // VBlank counter, 60 per second

static uint8_t framebuf[MAX_FRAME_SIZE + 64] ALIGN(32);
static uint8_t scratch[MAX_RAW_SIZE + 64] ALIGN(32);
static uint8_t pktbuf[2048] ALIGN(32);

static int sock = -1;
static struct sockaddr_in host_addr;
static char host_name[64];
static uint8_t host_mode = MODE_256x192;

static void on_vblank(void)
{
    vbl++;
}

// Waits for the next VBlank and lets the WiFi thread (lwIP runs in a cothread)
// do its work meanwhile.
static void wait_frame(void)
{
    cothread_yield_irq(IRQ_VBLANK);
}

static void status_clear(void)
{
    consoleSelect(&con_status);
    consoleClear();
}

static void show_info(void)
{
    consoleSelect(&con_info);
    consoleClear();
    printf("\x1b[0;0HDsRunner Stream " DSSTREAM_VERSION "\n");
    printf("Webseiten-Spiele vom Pi/PC\n");
    printf("--------------------------------");
    printf("Einstellungen:\n /_dsrunner/stream.ini\n");
    if (!cfg.fat)
        printf(" (SD-Karte nicht lesbar)\n");
    else if (!cfg.found)
        printf(" (fehlt, Standardwerte)\n");
    else
        printf(" (gelesen)\n");
    printf("Host: %s\n", cfg.host[0] ? cfg.host : "automatisch suchen");
    printf("Port: %d\n", cfg.port);
    printf("WLAN: %s\n", cfg.wifi == WIFI_WFC ? "nur WFC-Einstellungen" :
                         cfg.wifi == WIFI_LIST ? "Liste" : "automatisch");
    if (cfg.ssid[0])
        printf("Netz: %s\n", cfg.ssid);
    printf("\nIm Spiel: SELECT+START 1 s\nhalten = Pause-Menue\n");
    printf("\nY: Geschwindigkeitstest\nSTART: beenden\n");
    consoleSelect(&con_status);
}

// ---------------------------------------------------------------- WiFi

static const char *assoc_text(int s)
{
    switch (s)
    {
        case ASSOCSTATUS_DISCONNECTED: return "getrennt";
        case ASSOCSTATUS_SEARCHING: return "suche Netz";
        case ASSOCSTATUS_AUTHENTICATING: return "anmelden";
        case ASSOCSTATUS_ASSOCIATING: return "verbinden";
        case ASSOCSTATUS_ACQUIRINGDHCP: return "hole IP-Adresse (DHCP)";
        case ASSOCSTATUS_ASSOCIATED: return "verbunden";
        case ASSOCSTATUS_CANNOTCONNECT: return "fehlgeschlagen";
    }
    return "?";
}

static const char *ap_kind(const Wifi_AccessPoint *ap)
{
    if (ap->flags & WFLAG_APDATA_CONFIG_IN_WFC)
        return "WFC";
    if (ap->security_type == AP_SECURITY_OPEN)
        return "offen";
    if (ap->security_type == AP_SECURITY_WEP)
        return "WEP";
    return "WPA-";   // DS Lite can't do WPA
}

static int scan_aps(Wifi_AccessPoint *aps, int max)
{
    int n = Wifi_GetNumAP();
    int k = 0;
    for (int i = 0; i < n && k < max; i++)
    {
        if (Wifi_GetAPData(i, &aps[k]) != WIFI_RETURN_OK || aps[k].ssid_len == 0)
            continue;
        k++;
    }
    // strongest first (simple insertion sort)
    for (int i = 1; i < k; i++)
    {
        Wifi_AccessPoint t = aps[i];
        int j = i - 1;
        while (j >= 0 && aps[j].rssi < t.rssi)
        {
            aps[j + 1] = aps[j];
            j--;
        }
        aps[j + 1] = t;
    }
    return k;
}

static int parse_key(const char *s, uint8_t *out)
{
    size_t n = strlen(s);
    if (n == 5 || n == 13)
    {
        memcpy(out, s, n);
        return n;
    }
    if (n == 10 || n == 26)
    {
        for (size_t i = 0; i < n / 2; i++)
        {
            unsigned v;
            if (sscanf(s + 2 * i, "%2x", &v) != 1)
                return -1;
            out[i] = v;
        }
        return n / 2;
    }
    return -1;
}

// Starts connecting to ap. Returns false (with a message) if that can't work.
static bool start_connect(Wifi_AccessPoint *ap)
{
    printf("Verbinde mit \"%s\" (%s)\n", ap->ssid, ap_kind(ap));
    if (ap->flags & WFLAG_APDATA_CONFIG_IN_WFC)
        return Wifi_ConnectWfcAP(ap) == 0;
    if (ap->security_type == AP_SECURITY_OPEN)
        return Wifi_ConnectOpenAP(ap) == 0;
    if (ap->security_type == AP_SECURITY_WEP)
    {
        uint8_t key[16];
        int n = (cfg.key[0] && !strcmp(cfg.ssid, ap->ssid)) ? parse_key(cfg.key, key) : -1;
        if (n < 0)
        {
            printf("WEP-Schluessel fehlt: in\nstream.ini ssid= und key=\neintragen.\n");
            return false;
        }
        return Wifi_ConnectSecureAP(ap, key, n) == 0;
    }
    printf("WPA/WPA2 kann der DS Lite\nnicht. Offenes Netz oder WEP\nnehmen (siehe Anleitung).\n");
    return false;
}

// Waits until connected. Returns true when connected.
static bool wait_assoc(void)
{
    int last = -1;
    uint32_t start = vbl;
    while (vbl - start < 60 * 25)
    {
        int s = Wifi_AssocStatus();
        if (s != last)
        {
            printf(" %s\n", assoc_text(s));
            last = s;
        }
        if (s == ASSOCSTATUS_ASSOCIATED)
            return true;
        if (s == ASSOCSTATUS_CANNOTCONNECT)
            return false;
        wait_frame();
    }
    printf(" Zeitueberschreitung\n");
    return false;
}

// Picks an access point from a list. Returns its index, -1 = rescan, -2 = exit.
static int choose_ap(Wifi_AccessPoint *aps, int *n)
{
    int sel = 0;
    uint32_t last_scan = vbl;
    for (;;)
    {
        if (vbl - last_scan > 60)
        {
            *n = scan_aps(aps, MAX_APS);
            last_scan = vbl;
        }
        if (sel >= *n)
            sel = *n ? *n - 1 : 0;
        consoleClear();
        printf("WLAN waehlen (%d gefunden)\n", *n);
        printf("A: verbinden B: neu suchen\nSTART: beenden\n\n");
        for (int i = 0; i < *n; i++)
            printf("%c%-20.20s %-5s\n", i == sel ? '>' : ' ', aps[i].ssid, ap_kind(&aps[i]));
        if (*n == 0)
            printf("Kein Netz gefunden.\nHotspot des Pi an? (Anleitung)\n");
        for (int f = 0; f < 6; f++)
        {
            wait_frame();
            scanKeys();
            int k = keysDown();
            if (k & KEY_UP && sel > 0)
                sel--;
            else if (k & KEY_DOWN && sel + 1 < *n)
                sel++;
            else if (k & KEY_A && *n)
                return sel;
            else if (k & KEY_B)
                return -1;
            else if (k & KEY_START)
                return -2;
            else
                continue;
            break;
        }
    }
}

// Returns true when connected, false when the user wants to quit.
static bool wifi_connect(void)
{
    static bool inited;
    static Wifi_AccessPoint aps[MAX_APS];
    status_clear();
    if (!inited)
    {
        printf("WLAN wird gestartet...\n");
        if (!Wifi_InitDefault(INIT_ONLY))
        {
            printf("WLAN-Start fehlgeschlagen.\nSTART: beenden\n");
            while (1)
            {
                wait_frame();
                scanKeys();
                if (keysDown() & KEY_START)
                    return false;
            }
        }
        inited = true;
    }

    for (;;)
    {
        if (cfg.wifi == WIFI_WFC)
        {
            printf("Verbinde mit den WFC-\nEinstellungen des DS...\n");
            Wifi_AutoConnect();
            if (wait_assoc())
                return true;
            printf("\nKein WFC-Netz erreichbar.\nIm WFC-Menue eines DS-Spiels\neinrichten oder wifi=auto.\n"
                   "A: nochmal  START: beenden\n");
        }
        else
        {
            printf("Suche WLAN-Netze...\n");
            Wifi_ScanMode();
            int n = 0;
            for (int f = 0; f < 60 * 3; f++)
            {
                wait_frame();
                if (f % 30 == 29)
                    n = scan_aps(aps, MAX_APS);
            }
            int pick = -1;
            if (cfg.wifi == WIFI_AUTO)
            {
                // 1. the network from stream.ini (default "DsRunner-Stream"),
                //    with its WFC settings if it has some
                for (int i = 0; i < n && pick < 0; i++)
                    if (cfg.ssid[0] && !strcmp(aps[i].ssid, cfg.ssid))
                        pick = i;
                // 2. any network from the WFC settings of the DS
                for (int i = 0; i < n && pick < 0; i++)
                    if (aps[i].flags & WFLAG_APDATA_CONFIG_IN_WFC)
                        pick = i;
            }
            if (pick < 0)
            {
                pick = choose_ap(aps, &n);
                status_clear();
                if (pick == -2)
                    return false;
                if (pick == -1)
                    continue;
            }
            if (start_connect(&aps[pick]) && wait_assoc())
                return true;
            printf("\nA: nochmal  START: beenden\n");
        }
        for (;;)
        {
            wait_frame();
            scanKeys();
            int k = keysDown();
            if (k & KEY_START)
                return false;
            if (k & (KEY_A | KEY_B))
                break;
        }
        status_clear();
        if (cfg.wifi == WIFI_AUTO)
            cfg.wifi = WIFI_LIST;   // retry: let the user pick
    }
}

// ---------------------------------------------------------------- network

static bool open_socket(void)
{
    if (sock >= 0)
        return true;
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
        return false;
    int one = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &one, sizeof(one));
    int rcvbuf = 64 * 1024;
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf)); // may be unsupported
    struct sockaddr_in a;
    memset(&a, 0, sizeof(a));
    a.sin_len = sizeof(a);
    a.sin_family = AF_INET;
    a.sin_port = htons(0);
    a.sin_addr.s_addr = INADDR_ANY;
    if (bind(sock, (struct sockaddr *)&a, sizeof(a)) < 0)
        return false;
    unsigned long nb = 1;
    ioctl(sock, FIONBIO, &nb);
    return true;
}

static void send_to(const struct sockaddr_in *to, const void *data, int len)
{
    sendto(sock, data, len, 0, (const struct sockaddr *)to, sizeof(*to));
}

static void set_addr(struct sockaddr_in *a, uint32_t ip, int port)
{
    memset(a, 0, sizeof(*a));
    a->sin_len = sizeof(*a);
    a->sin_family = AF_INET;
    a->sin_port = htons(port);
    a->sin_addr.s_addr = ip;
}

// Returns true when a host was found, false when the user wants to quit.
static bool find_host(void)
{
    struct in_addr gw, mask, dns1, dns2;
    struct in_addr ip = Wifi_GetIPInfo(&gw, &mask, &dns1, &dns2);
    status_clear();
    uint8_t mac[6] = { 0 };
    Wifi_GetData(WIFIGETDATA_MACADDRESS, 6, mac);
    printf("WLAN verbunden.\nIP des DS:  %s\n", inet_ntoa(ip));
    printf("MAC des DS: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    if (!open_socket())
    {
        printf("Netzwerkfehler (socket)\n");
        return false;
    }
    if (cfg.host[0])
    {
        set_addr(&host_addr, inet_addr(cfg.host), cfg.port);
        snprintf(host_name, sizeof(host_name), "%s", cfg.host);
        printf("Host aus stream.ini: %s\n", cfg.host);
        return true;
    }
    printf("Suche Host (Port %d)...\nSTART: beenden\n", cfg.port);
    struct sockaddr_in bc_all, bc_net;
    set_addr(&bc_all, INADDR_BROADCAST, cfg.port);
    set_addr(&bc_net, ip.s_addr | ~mask.s_addr, cfg.port);
    uint8_t hello[4 + 16];
    hello[0] = PKT_HELLO;
    hello[1] = PROTO_VERSION;
    wr16(hello + 2, vbl);
    memcpy(hello + 4, "DsRunnerStream", 14);
    uint32_t start = vbl;
    bool hinted = false;
    for (uint32_t f = 0;; f++)
    {
        if (f % 30 == 0)
        {
            send_to(&bc_all, hello, 18);
            send_to(&bc_net, hello, 18);
        }
        struct sockaddr_in from;
        socklen_t fl = sizeof(from);
        int len;
        while ((len = recvfrom(sock, pktbuf, sizeof(pktbuf), 0, (struct sockaddr *)&from, &fl)) > 0)
        {
            if (len >= 5 && pktbuf[0] == PKT_HERE && pktbuf[1] == PROTO_VERSION)
            {
                int port = rd16(pktbuf + 2);
                host_mode = pktbuf[4];
                int nl = len - 5 < 31 ? len - 5 : 31;
                memcpy(host_name, pktbuf + 5, nl);
                host_name[nl] = 0;
                set_addr(&host_addr, from.sin_addr.s_addr, port);
                printf("Gefunden: %s\n %s\n", host_name, inet_ntoa(from.sin_addr));
                return true;
            }
            fl = sizeof(from);
        }
        if (!hinted && vbl - start > 60 * 10)
        {
            printf("\nNoch kein Host. Laeuft\ndsstream auf dem Pi/PC? Im\ngleichen WLAN? Sonst host=\n"
                   "in stream.ini eintragen.\n");
            hinted = true;
        }
        wait_frame();
        scanKeys();
        if (keysDown() & KEY_START)
            return false;
    }
}

// ---------------------------------------------------------------- streaming

enum { NEXT_EXIT, NEXT_RECONNECT, NEXT_FIND_HOST };

static const char *mode_text(uint8_t m)
{
    return m == MODE_128x96 ? "128x96" : m == MODE_256x384 ? "256x384" : "256x192";
}

// Pause menu. Returns -1 = continue, else NEXT_*
static int pause_menu(void)
{
    video_menu();
    show_info();
    status_clear();
    printf("Pause\n\nA: weiter\nB: Host neu suchen\nSTART: beenden\n");
    // SELECT + START are still held: wait until they are released
    for (;;)
    {
        wait_frame();
        scanKeys();
        if (!(keysHeld() & (KEY_START | KEY_SELECT)))
            break;
    }
    for (;;)
    {
        wait_frame();
        scanKeys();
        int k = keysDown();
        if (k & KEY_A)
            return -1;
        if (k & KEY_B)
            return NEXT_FIND_HOST;
        if (k & KEY_START)
            return NEXT_EXIT;
    }
}

static int stream(void)
{
    static Reassembler rea;
    DecodeState ds = { 0, MODE_NONE };
    uint8_t layout = MODE_NONE;
    bool need = true;
    uint32_t frames_ok = 0, frames_bad = 0, bytes_rx = 0;
    uint16_t seq = 0;
    uint32_t last_vbl = vbl, last_frame_vbl = vbl, combo = 0;
    uint32_t stat_vbl = vbl, stat_frames = 0, stat_bytes = 0;
    uint8_t in[INPUT_SIZE];

    reasm_init(&rea, framebuf);
    status_clear();
    printf("Verbunden mit %s\nWarte auf Bilder...\n", host_name);

    for (;;)
    {
        // 1. take all packets that arrived
        struct sockaddr_in from;
        socklen_t fl = sizeof(from);
        int len;
        while ((len = recvfrom(sock, pktbuf, sizeof(pktbuf), 0, (struct sockaddr *)&from, &fl)) > 0)
        {
            fl = sizeof(from);
            if (from.sin_addr.s_addr != host_addr.sin_addr.s_addr)
                continue;
            bytes_rx += len;
            uint32_t flen = reasm_add(&rea, pktbuf, len);
            if (!flen)
                continue;
            FrameHeader h;
            if (!frame_header_read(framebuf, flen, &h))
                continue;
            if ((h.flags & FLAG_KEY) && h.mode != layout)
            {
                // new picture size: switch the layout before the palette is written
                video_stream(h.mode, cfg.screen_top);
                layout = h.mode;
            }
            int r = frame_decode(framebuf, flen, &ds, video_target(), scratch);
            if (r == DEC_KEY || r == DEC_DELTA)
            {
                frames_ok++;
                stat_frames++;
                need = false;
                last_frame_vbl = vbl;
            }
            else
            {
                frames_bad++;
                need = true;
                if (r == DEC_ERROR)
                    ds.cur_frame = 0;   // picture may be half updated: ask for a keyframe
            }
        }

        // 2. once per VBlank: buttons to the host
        if (vbl != last_vbl)
        {
            last_vbl = vbl;
            scanKeys();
            uint32_t held = keysHeld();
            touchPosition tp = { 0 };
            if (held & KEY_TOUCH)
                touchRead(&tp);
            if (vbl - last_frame_vbl > 60)
                need = true;    // nothing for a second: maybe a frame got lost
            in[0] = PKT_INPUT;
            in[1] = PROTO_VERSION;
            wr16(in + 2, ++seq);
            wr16(in + 4, held & 0x1FFF);
            in[6] = tp.px > 255 ? 255 : tp.px;
            in[7] = tp.py > 191 ? 191 : tp.py;
            wr16(in + 8, ds.cur_frame);
            in[10] = (need ? IN_NEED_REF : 0) | (frames_ok == 0 ? IN_HELLO : 0);
            in[11] = 0;
            wr16(in + 12, frames_ok);
            wr16(in + 14, frames_bad + rea.dropped);
            wr32(in + 16, bytes_rx);
            send_to(&host_addr, in, INPUT_SIZE);

            // SELECT + START for one second: pause menu
            if ((held & (KEY_SELECT | KEY_START)) == (KEY_SELECT | KEY_START))
            {
                if (++combo >= 60)
                {
                    int next = pause_menu();
                    if (next >= 0)
                        return next;
                    combo = 0;
                    layout = MODE_NONE;   // the menu took the screens and palettes
                    ds.cur_frame = 0;     // -> the host sends a keyframe
                    need = true;
                    status_clear();
                    continue;
                }
            }
            else
            {
                combo = 0;
            }

            if (Wifi_AssocStatus() != ASSOCSTATUS_ASSOCIATED)
            {
                video_menu();
                show_info();
                status_clear();
                printf("WLAN-Verbindung verloren.\n");
                return NEXT_RECONNECT;
            }

            // status text (on the screen without picture), twice per second
            if (vbl - stat_vbl >= 30)
            {
                uint32_t dt = vbl - stat_vbl;
                uint32_t rx = bytes_rx - stat_bytes;
                if (layout != MODE_256x384)
                {
                    consoleSelect(&con_status);
                    printf("\x1b[0;0HDsRunner Stream\n%-31.31s\n", host_name);
                    printf("Bild %-7s %2lu.%lu fps     \n", layout == MODE_NONE ? "-" : mode_text(layout),
                           stat_frames * 60 / dt, (stat_frames * 600 / dt) % 10);
                    printf("Empfang %3lu KB/s           \n", rx * 60 / dt / 1000);
                    printf("Bilder %lu, verloren %lu      \n", frames_ok, frames_bad + rea.dropped);
                    if (vbl - last_frame_vbl > 120)
                        printf("Keine Bilder vom Host!      \n");
                    else
                        printf("                            \n");
                    printf("\nSELECT+START 1 s: Pause\n");
                }
                stat_vbl = vbl;
                stat_frames = 0;
                stat_bytes = bytes_rx;
            }
        }

        // 3. let lwIP work; wake up at the next VBlank at the latest
        cothread_yield_irq(IRQ_VBLANK);
    }
}

static void say_bye(void)
{
    if (sock >= 0 && host_addr.sin_port)
    {
        uint8_t bye[2] = { PKT_BYE, PROTO_VERSION };
        for (int i = 0; i < 3; i++)
        {
            send_to(&host_addr, bye, 2);
            wait_frame();
        }
    }
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    irqSet(IRQ_VBLANK, on_vblank);
    irqEnable(IRQ_VBLANK);
    video_init();
    consoleSelect(&con_status);
    printf("Lade Einstellungen...\n");
    config_load(&cfg);
    show_info();

    // a moment to start the benchmark or to quit before WiFi starts
    status_clear();
    printf("A: verbinden  Y: Test\nSTART: beenden\n\n(verbinde automatisch in 2 s)\n");
    for (int f = 0; f < 120; f++)
    {
        wait_frame();
        scanKeys();
        int k = keysDown();
        if (k & KEY_START)
            return 0;
        if (k & KEY_A)
            break;
        if (k & KEY_Y)
        {
            status_clear();
            bench_run();
            printf("\nA: verbinden  START: beenden\n");
            for (;;)
            {
                wait_frame();
                scanKeys();
                k = keysDown();
                if (k & KEY_START)
                    return 0;
                if (k & KEY_A)
                    break;
            }
            video_menu();
            show_info();
            break;
        }
    }

    int next = NEXT_RECONNECT;
    while (next != NEXT_EXIT)
    {
        if (next == NEXT_RECONNECT && !wifi_connect())
            break;
        if (!find_host())
            break;
        next = stream();
        if (next != NEXT_EXIT)
        {
            video_menu();
            show_info();
        }
    }

    say_bye();
    if (sock >= 0)
        closesocket(sock);
    if (Wifi_CheckInit())
    {
        Wifi_DisconnectAP();
        Wifi_DisableWifi();
        for (int i = 0; i < 3; i++)
            wait_frame();
    }
    return 0;   // back to DsRunner (or the loader that started us)
}
