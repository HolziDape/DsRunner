// Host test of /_pico/igr-log.txt (arm7/source/fat/igrLog.cpp from patch 0015): renders the log
// for a game start followed by a return to the launcher, and for the usual failure cases, and
// checks the text. Run with loader/test/run.sh; it also prints the logs as examples.
#include <stdlib.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include "common.h"
#include "fat/igrLog.h"
#include "inGameReset.h"
#include "DsRunnerInGameResetKeys.h"

std::string gIgrTestCardDir;
static int sFailures = 0;

static std::string readLog()
{
    std::ifstream file(gIgrTestCardDir + "/_pico/igr-log.txt", std::ios::binary);
    std::stringstream text;
    text << file.rdbuf();
    return text.str();
}

static void expect(const std::string& log, const char* text)
{
    bool found = log.find(text) != std::string::npos;
    printf("%s  log contains \"%s\"\n", found ? "ok  " : "FAIL", text);
    if (!found)
        sFailures++;
}

static void writeCardFile(const char* path, const char* text)
{
    FILE* file = fopen((gIgrTestCardDir + path).c_str(), "wb");
    fputs(text, file);
    fclose(file);
}

static void checkValue(bool condition, const char* text)
{
    printf("%s  %s\n", condition ? "ok  " : "FAIL", text);
    if (!condition)
        sFailures++;
}

static nds_header_ntr_t makeHeader(const char* title, const char* gameCode)
{
    nds_header_ntr_t header;
    memset(&header, 0, sizeof(header));
    memcpy(header.gameTitle, title, strnlen(title, sizeof(header.gameTitle)));
    memcpy(&header.gameCode, gameCode, 4);
    return header;
}

static dsrunner_igr_diag_t makeDiag(u32 status)
{
    dsrunner_igr_diag_t diag;
    memset(&diag, 0, sizeof(diag));
    diag.sdkVersion = 0x04017530;
    diag.status = status;
    memcpy(diag.platform, "M3DS", 4);
    return diag;
}

int main()
{
    char dir[] = "/tmp/igrlogtestXXXXXX";
    gIgrTestCardDir = mkdtemp(dir);
    system(("mkdir -p " + gIgrTestCardDir + "/_pico").c_str());

    // 1. a game where the hook is installed, then the return to the launcher
    gIgrLog = { };
    gIgrLog.bootMode = IGR_LOG_BOOT_NORMAL;
    gIgrLog.dldiState = IGR_LOG_DLDI_SAVED;
    gIgrLog.launcherState = IGR_LOG_LAUNCHER_UP_TO_DATE;
    gIgrLog.launcherPath = "fat:/_nds/TWiLightMenu/main.srldr";
    auto game = makeHeader("MARIOKARTDS", "AMCP");
    auto diag = makeDiag(DSRUNNER_IGR_STATUS_APPLIED);
    diag.matchResults = DSRUNNER_IGR_MATCH_MATCHED << (4 * DSRUNNER_IGR_VARIANT_SDK);
    diag.sdkPatternCount = 1;
    diag.hookAddress = 0x0200A1C0;
    diag.continueAddress = 0x01FF81D8;
    diag.dispatchAddress = 0x02000F01;
    diag.rebootCreated = 1;
    igrLog_write("fat:/Games/NDS/Mario Kart DS.nds", &game, false, &diag, false);

    gIgrLog = { };
    gIgrLog.bootMode = IGR_LOG_BOOT_IN_GAME_RESET;
    gIgrLog.dldiState = IGR_LOG_DLDI_LOADED;
    gIgrLog.launcherState = IGR_LOG_LAUNCHER_FROM_FILE;
    gIgrLog.launcherPath = "fat:/_nds/TWiLightMenu/main.srldr";
    auto launcher = makeHeader("DSRUNNER", "####");
    igrLog_write("fat:/_nds/TWiLightMenu/main.srldr", &launcher, true, nullptr, true);

    std::string log = readLog();
    printf("---- example 1 ----\n%s-------------------\n", log.c_str());
    expect(log, "build: unknown");
    expect(log, "[normal start]");
    expect(log, "game: AMCP \"MARIOKARTDS\" (retail)");
    expect(log, "platform: M3DS");
    expect(log, "in-game reset: applied");
    expect(log, "dispatcher sdk: MATCH (pattern found 1 times)");
    expect(log, "hook at 0x200A1C0");
    expect(log, "reboot created for the hotkey");
    expect(log, "[start after the in-game reset (PIGR): booting the launcher]");
    expect(log, "none handed down, loaded /_pico/dldi.bin");
    expect(log, "DLDI for homebrew: available");
    expect(log, "[from /_pico/launcher.txt]");

    // 2. a normal start rewrites the log; no dispatcher, not enough space, switched off
    struct { u32 status; const char* text; } cases[] =
    {
        { DSRUNNER_IGR_STATUS_NO_DISPATCHER, "NOT applied: no known irq dispatcher found" },
        { DSRUNNER_IGR_STATUS_NO_PATCH_SPACE, "NOT applied: not enough patch space" },
        { DSRUNNER_IGR_STATUS_DISABLED, "NOT applied: switched off by /_pico/igr.off" },
    };
    for (auto& c : cases)
    {
        gIgrLog = { };
        gIgrLog.bootMode = IGR_LOG_BOOT_NORMAL;
        gIgrLog.dldiState = IGR_LOG_DLDI_NOT_CACHEABLE;
        gIgrLog.launcherState = IGR_LOG_LAUNCHER_EMPTY;
        gIgrLog.igrOffSeen = c.status == DSRUNNER_IGR_STATUS_DISABLED;
        diag = makeDiag(c.status);
        diag.matchResults = (DSRUNNER_IGR_MATCH_START_NOT_FOUND << (4 * DSRUNNER_IGR_VARIANT_SDK))
            | (DSRUNNER_IGR_MATCH_NO_MATCH << (4 * DSRUNNER_IGR_VARIANT_BLX));
        diag.spaceNeeded = 0x1F8;
        auto pokemon = makeHeader("POKEMON D", "ADAD");
        igrLog_write("fat:/Games/NDS/Pokemon Diamant.nds", &pokemon, false, &diag, false);
        log = readLog();
        expect(log, c.text);
        if (log.find("MARIOKARTDS") != std::string::npos)
        {
            printf("FAIL  a normal start must rewrite the log\n");
            sFailures++;
        }
    }
    printf("---- example 2 ----\n%s-------------------\n", log.c_str());
    expect(log, "/_pico/igr.off: present (hotkey off)");
    expect(log, "dispatcher sdk: dispatcher start not found");
    expect(log, "dispatcher blx: no match");
    expect(log, "the launcher handed down no path");
    expect(log, "but not cacheable (NOT saved)");

    // 3. a return that ends without DLDI driver, and one the arm7 took for the game's own reset
    gIgrLog = { };
    gIgrLog.bootMode = IGR_LOG_BOOT_IN_GAME_RESET;
    gIgrLog.dldiState = IGR_LOG_DLDI_NO_CACHE;
    gIgrLog.launcherState = IGR_LOG_LAUNCHER_FALLBACK;
    gIgrLog.launcherPath = "/_nds/TWiLightMenu/main.srldr";
    igrLog_write("/_nds/TWiLightMenu/main.srldr", &launcher, true, nullptr, false);
    log = readLog();
    expect(log, "DLDI for homebrew: MISSING (error screen)");
    expect(log, "[fallback, /_pico/launcher.txt missing");

    // 4. keys: DsRunner's parser (settings.json "inGameResetKeys") ...
    using namespace DsRunnerInGameResetKeys;
    uint32_t mask = 0;
    checkValue(Parse("L+R+DOWN+B", mask) == ParseResult::Ok && mask == IN_GAME_RESET_DEFAULT_KEY_MASK,
        "DsRunner: L+R+DOWN+B is the loader's default mask");
    checkValue(Parse("select+start+down", mask) == ParseResult::Ok && mask == 0x08C, "DsRunner: select+start+down = 0x08C (case-insensitive)");
    checkValue(Parse(" SELECT , DOWN + B ", mask) == ParseResult::Ok && mask == 0x086, "DsRunner: separators '+', ',' and spaces");
    checkValue(Parse("Select+Unten+B", mask) == ParseResult::Ok && mask == 0x086, "DsRunner: German direction names");
    checkValue(Parse("L+R+X", mask) == ParseResult::UnknownKey, "DsRunner: X is rejected (the arm9 can't read it)");
    checkValue(Parse("L+R+FOO", mask) == ParseResult::UnknownKey, "DsRunner: unknown names are rejected");
    checkValue(Parse("SELECT+START", mask) == ParseResult::TooFewKeys, "DsRunner: two keys are too few");
    checkValue(Parse("B+B+B", mask) == ParseResult::TooFewKeys, "DsRunner: the same key three times counts once");
    checkValue(Parse("A+B+SELECT+START+RIGHT+LEFT+UP+DOWN+R+L", mask) == ParseResult::Ok && mask == 0x3FF, "DsRunner: all ten keys");
    char names[64];
    Format(0x08C, names, sizeof(names));
    checkValue(strcmp(names, "SELECT+START+DOWN") == 0, "DsRunner: names of 0x08C");
    Format(IN_GAME_RESET_DEFAULT_KEY_MASK, names, sizeof(names));
    checkValue(strcmp(names, "L+R+DOWN+B") == 0, "DsRunner: names of the default are L+R+DOWN+B");

    // ... and the loader reading /_pico/igr.keys as DsRunner writes it
    std::string keysFile = gIgrTestCardDir + IN_GAME_RESET_KEYS_PATH;
    remove(keysFile.c_str());
    checkValue(igrKeys_read() == 0, "loader: no igr.keys = default");
    writeCardFile(IN_GAME_RESET_KEYS_PATH, "0x08C SELECT+START+DOWN\r\n");
    checkValue(igrKeys_read() == 0x08C, "loader: reads 0x08C from \"0x08C SELECT+START+DOWN\"");
    writeCardFile(IN_GAME_RESET_KEYS_PATH, "86\n");
    checkValue(igrKeys_read() == 0x086, "loader: reads a mask without 0x");
    writeCardFile(IN_GAME_RESET_KEYS_PATH, "0x1000\r\n");
    checkValue(igrKeys_read() == 0, "loader: bits above the 10 keys = default");
    writeCardFile(IN_GAME_RESET_KEYS_PATH, "SELECT+START\r\n");
    checkValue(igrKeys_read() == 0, "loader: text without a number = default");
    writeCardFile(IN_GAME_RESET_KEYS_PATH, "0x0\r\n");
    checkValue(igrKeys_read() == 0, "loader: 0 = default");

    gIgrLog = { };
    gIgrLog.bootMode = IGR_LOG_BOOT_NORMAL;
    gIgrLog.keyMask = 0x08C;
    diag = makeDiag(DSRUNNER_IGR_STATUS_APPLIED);
    diag.keyMask = 0x08C;
    auto zelda = makeHeader("ZELDA DS", "AZEP");
    igrLog_write("fat:/Games/NDS/Zelda.nds", &zelda, false, &diag, false);
    log = readLog();
    printf("---- example 3 ----\n%s-------------------\n", log.c_str());
    expect(log, "keys: SELECT+START+DOWN (0x8C, from /_pico/igr.keys), hold 2 s");
    expect(log, "key mask patched in: 0x8C");

    gIgrLog.keyMask = 0;
    igrLog_write("fat:/Games/NDS/Zelda.nds", &zelda, false, &diag, false);
    expect(readLog(), "keys: L+R+DOWN+B (0x382, default), hold 2 s");

    system(("rm -rf " + gIgrTestCardDir).c_str());
    printf("\n%s\n", sFailures ? "checks failed" : "all checks passed");
    return sFailures ? 1 : 0;
}
