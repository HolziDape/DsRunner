// Host test of the search for the stock SDK arm9 irq dispatcher (OS_IrqHandler) that the
// in-game reset hooks: compiles the real IrqDispatcher.cpp and IrqDispatcherSdk.cpp from the
// patched Pico Loader against small stand-ins (loader/test/host) and runs them on synthetic
// arm9 images. Run with loader/test/run.sh.
//
// The dispatcher below follows the NitroSDK OS_IrqHandler; its start and end are the same
// signatures nds-bootstrap uses to hook it (retail/bootloader/source/arm7/hook_arm9.c). What
// this can't show is whether a particular game contains exactly this code: that is what
// /_pico/igr-log.txt on the console tells.
#include <sys/mman.h>
#include <vector>
#include "common.h"
#include "patches/PatchContext.h"
#include "dsrunnerIgrDiag.h"
#include "IrqDispatcherSdk.h"

dsrunner_igr_diag_t gDsRunnerIgrDiag;
u32 gDsRunnerIgrLastPatternResult;

static const u32 IMAGE = 0x02000000;      // arm9 load address
static const u32 IMAGE_SIZE = 0x10000;
static const u32 ITCM_INITIAL = 0x02008000; // autoload block that goes to ITCM
static const u32 ITCM_SIZE = 0x2000;
static const u32 ITCM_FINAL = 0x01FF8000;
static const u32 IRQ_TABLE = 0x027E0000;

static int sFailures = 0;

static void check(bool condition, const char* text)
{
    printf("%s  %s\n", condition ? "ok  " : "FAIL", text);
    if (!condition)
        sFailures++;
}

/// @brief Writes an OS_IrqHandler at address, with extraBodyWords extra instructions between
///        its start and the dispatch. Returns the address of the dispatch (the hook).
static u32 writeDispatcher(u32 address, u32 extraBodyWords, bool correctReturn = true, u32 table = IRQ_TABLE)
{
    std::vector<u32> code =
    {
        0xE92D4000, // push {lr}
        0xE3A0C301, // mov r12, #0x04000000
        0xE28CCE21, // add r12, r12, #0x210
        0xE51C1008, // ldr r1, [r12, #-8]   (IME)
        0xE3510000, // cmp r1, #0
        0x08BD8000, // ldmeqfd sp!, {pc}
        0xE89C0006, // ldmia r12, {r1, r2}  (IE, IF)
        0xE0111002, // ands r1, r1, r2
        0x08BD8000, // ldmeqfd sp!, {pc}
        0xE3A03102, // mov r3, #0x80000000
        0xE16F0F11, // clz r0, r1
        0xE1D11033, // bics r1, r1, r3, lsr r0
        0x1AFFFFFC, // bne (clz)
        0xE1A01033, // mov r1, r3, lsr r0
        0xE58C1004, // str r1, [r12, #4]    (IF)
        0xE260001F, // rsb r0, r0, #31
    };
    for (u32 i = 0; i < extraBodyWords; i++)
        code.push_back(0xE1A00000); // nop
    u32 hook = address + code.size() * 4;
    code.push_back(0xE59F1008); // ldr r1, =irqTable
    code.push_back(0xE7910100); // ldr r0, [r1, r0, lsl #2]
    code.push_back(0xE59FE004); // ldr lr, =irqReturn
    code.push_back(0xE12FFF10); // bx r0
    u32 hookFinal = hook - ITCM_INITIAL + ITCM_FINAL;
    code.push_back(table);
    code.push_back(correctReturn ? hookFinal + 0x18 : hookFinal + 0x1C);
    code.push_back(0xE8BD4000); // irqReturn: pop {lr} ... (OS_IrqHandler_ThreadSwitch)
    memcpy((void*)(uintptr_t)address, code.data(), code.size() * 4);
    return hook;
}

static void clearImage()
{
    memset((void*)(uintptr_t)IMAGE, 0, IMAGE_SIZE);
}

static bool search(IrqDispatcherMatch& match)
{
    TestAutoloadAdjuster adjuster(ITCM_INITIAL, ITCM_SIZE, ITCM_FINAL);
    PatchContext context((void*)(uintptr_t)IMAGE, IMAGE_SIZE, &adjuster);
    match = { };
    gDsRunnerIgrDiag.sdkPatternCount = 0;
    gDsRunnerIgrLastPatternResult = DSRUNNER_IGR_MATCH_NOT_TRIED;
    return FindSdkIrqDispatcher(context, match);
}

int main()
{
    // The code casts pointers to u32 like on the DS, so the image lives at its DS address.
    void* image = mmap((void*)(uintptr_t)IMAGE, IMAGE_SIZE, PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
    if (image != (void*)(uintptr_t)IMAGE)
    {
        printf("can't map the test image at 0x%08X\n", IMAGE);
        return 2;
    }
    IrqDispatcherMatch match;

    clearImage();
    u32 hook = writeDispatcher(ITCM_INITIAL + 0x100, 0);
    bool found = search(match);
    check(found && (u32)(uintptr_t)match.hook == hook, "stock dispatcher in the ITCM block is found, hook on the dispatch");
    check(found && match.irqTable == IRQ_TABLE, "irq table taken from the literal pool");
    check(found && match.continueAddress == hook - ITCM_INITIAL + ITCM_FINAL + 0x18, "return address is the final (ITCM) one");
    check(gDsRunnerIgrDiag.sdkPatternCount == 1, "pattern counted once");

    clearImage();
    writeDispatcher(ITCM_INITIAL + 0x100, 0);
    writeDispatcher(ITCM_INITIAL + 0x400, 0);
    check(!search(match) && gDsRunnerIgrLastPatternResult == DSRUNNER_IGR_MATCH_AMBIGUOUS, "two copies: ambiguous, not patched");

    clearImage();
    check(!search(match) && gDsRunnerIgrLastPatternResult == DSRUNNER_IGR_MATCH_NO_MATCH, "no dispatcher: no match");

    clearImage();
    writeDispatcher(ITCM_INITIAL + 0x100, 0, false);
    check(!search(match) && gDsRunnerIgrLastPatternResult == DSRUNNER_IGR_MATCH_RETURN_MISMATCH, "wrong return literal: rejected");

    clearImage();
    writeDispatcher(ITCM_INITIAL + 0x100, 0, true, IRQ_TABLE + 2);
    check(!search(match) && gDsRunnerIgrLastPatternResult == DSRUNNER_IGR_MATCH_BAD_TABLE, "unaligned irq table: rejected");

    // The start is looked for up to 32 words before the dispatch (HANDLER_MAX_LENGTH_WORDS);
    // the dispatcher above is 16 words to the dispatch, nds-bootstrap allows 50.
    clearImage();
    writeDispatcher(ITCM_INITIAL + 0x100, 16);
    check(search(match), "dispatcher with 16 extra instructions (32 words) is still found");
    clearImage();
    writeDispatcher(ITCM_INITIAL + 0x100, 17);
    check(!search(match) && gDsRunnerIgrLastPatternResult == DSRUNNER_IGR_MATCH_START_NOT_FOUND,
        "dispatcher with 17 extra instructions: start not found (limit of the search)");

    // A copy outside the ITCM block (not moved by autoload) does not get a matching return literal.
    clearImage();
    writeDispatcher(IMAGE + 0x100, 0);
    check(!search(match), "dispatcher outside the autoload block: rejected (return literal points to ITCM)");

    printf("\n%s\n", sFailures ? "checks failed" : "all checks passed");
    return sFailures ? 1 : 0;
}
