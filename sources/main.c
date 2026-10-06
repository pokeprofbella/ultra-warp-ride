// Ultra Warp Ride overlay for Pokemon Ultra Sun / Ultra Moon.
// Based on PokeReader by zaksabeast (GPL-3.0): https://github.com/zaksabeast/PokeReader
// The framebuffer hook below comes from PokeReader; everything about the Warp Ride is ours.
#include <3ds.h>
#include <string.h>
#include "plgldr.h"
#include "csvc.h"
#include "common.h"
#include "memmem.h"
#include "uwr.h"

static void flush(u8 *fb, u32 bytes)
{
    if (fb) svcFlushProcessDataCache(CUR_PROCESS_HANDLE, (u32)fb, bytes);
}

void run_hook(u32 _1, u32 _2, u32 _3, u32 _4, u32 screenId, u32 swap, u8 *fb_a, u8 *fb_b, u32 stride, u32 format)
{
    u32 width = screenId == 0 ? 400 : 320;
    if (uwr_draw(screenId, fb_a, stride, format))
    {
        flush(fb_a, stride * width);
        if (fb_b != fb_a) flush(fb_b, stride * width);
    }
}

u8 DRAW_PATCH[0x94] = {
    0xf0, 0x5f, 0x2d, 0xe9, // stmdb      sp!,{r4 r5 r6 r7 r8 r9 r10 r11 r12 lr}
    0x0f, 0x00, 0x2d, 0xe9, // stmdb      sp!,{r0 r1 r2 r3}
    // Injected reader call
    0x64, 0xc0, 0x9f, 0xe5, // ldr        r12,[run_hook_addr]
    0x3c, 0xff, 0x2f, 0xe1, // blx        r12
    // End reader call
    0xf0, 0x00, 0xbd, 0xe8, // ldmia      sp!,{r4 r5 r6 r7}
    0x28, 0x00, 0x8d, 0xe2, // add        r0,sp,#0x28
    0x00, 0x0e, 0x90, 0xe8, // ldmia      r0,{r9 r10 r11}
    0xd3, 0x03, 0x00, 0xeb, // bl         get_screen
    0x5c, 0x10, 0x80, 0xe2, // add        r1,r0,#0x5c
    0x04, 0x21, 0x91, 0xe7, // ldr        r2,[r1,r4,lsl #0x2]
    0x04, 0x30, 0xa0, 0xe3, // mov        r3,#0x4
    0x00, 0x00, 0xd2, 0xe5, // ldrb       r0,[r2,#0x0]
    0x01, 0x00, 0x60, 0xe2, // rsb        r0,r0,#0x1
    0xff, 0x00, 0x00, 0xe2, // and        r0,r0,#0xff
    0x80, 0xe1, 0x60, 0xe0, // rsb        lr,r0,r0, lsl #0x3
    0x0e, 0x31, 0x83, 0xe0, // add        r3,r3,lr, lsl #0x2
    0x03, 0x30, 0x82, 0xe0, // add        r3,r2,r3
    0xe0, 0x0e, 0x83, 0xe8, // stmia      r3,{ r5 r6 r7 r9 r10 r11 }
    0x9a, 0x8f, 0x07, 0xee, // mcr        p15,0x0,r8,cr7,cr10,0x4
    0x04, 0x21, 0x91, 0xe7, // ldr        r2,[r1,r4,lsl #0x2]
    0x9f, 0x3f, 0x92, 0xe1, // ldrex      r3,[r2]
    0xff, 0x30, 0xc3, 0xe3, // bic        r3,r3,#0xff
    0x00, 0x30, 0x83, 0xe1, // orr        r3,r3,r0
    0xff, 0x3c, 0xc3, 0xe3, // bic        r3,r3,#0xff00
    0x01, 0x3c, 0x83, 0xe3, // orr        r3,r3,#0x100
    0x93, 0x6f, 0x82, 0xe1, // strex      r6,r3,[r2]
    0x00, 0x00, 0x56, 0xe3, // cmp        r6,#0x0
    0xf6, 0xff, 0xff, 0x1a, // bne        loop
    0xf0, 0x9f, 0xbd, 0xe8, // ldmia      sp!,{r4 r5 r6 r7 r8 r9 r10 r11 r12 pc}
    0x00, 0x00, 0x00, 0x00, // run_hook_addr
    // Hook trampoline
    0xff, 0xdf, 0x2d, 0xe9, // stmdb      sp!,{r0-r12, lr, pc}
    0x0d, 0x00, 0xa0, 0xe1, // cpy        r0,sp
    0x08, 0xc0, 0x9f, 0xe5, // ldr        r12,[trampoline_addr]
    0x3c, 0xff, 0x2f, 0xe1, // blx        r12
    0x00, 0x40, 0xbd, 0xe8, // ldmia      sp!,{lr}
    0xff, 0x9f, 0xbd, 0xe8, // ldmia      sp!,{r0-r12, pc}    
    0x00, 0x00, 0x00, 0x00, // trampoline_addr
};

u8 PRESENT_FRAMEBUFFER_BYTES[0X10] = {
    0x28, 0x00, 0x8d, 0xe2, 0x00, 0x80, 0xa0, 0xe3, 0x01, 0x70, 0xa0, 0xe1, 0x00, 0x0e, 0x90, 0xe8,
};


extern char *fake_heap_start;
extern char *fake_heap_end;
extern u32 __ctru_heap;
extern u32 __ctru_linear_heap;

u32 __ctru_heap_size = 0;
u32 __ctru_linear_heap_size = 0;

void __system_allocateHeaps(PluginHeader *header)
{
    __ctru_heap_size = header->heapSize;
    __ctru_heap = header->heapVA;
    fake_heap_start = (char *)__ctru_heap;
    fake_heap_end = fake_heap_start + __ctru_heap_size;
}

// Entrypoint: the game starts when this returns.
void main(void)
{
    PluginHeader *header = (PluginHeader *)0x07000000;
    __system_allocateHeaps(header);
    srvInit();

    uwr_init();

    MemInfo info;
    PageInfo out;
    svcQueryMemory(&info, &out, 0x100000);

    u8 *found = memmem((u8 *)info.base_addr, info.size, PRESENT_FRAMEBUFFER_BYTES, sizeof(PRESENT_FRAMEBUFFER_BYTES));
    if (!found) return; // unknown game build: do nothing rather than crash
    u32 present_buffer_ptr = (u32)found - 8;

    u32 get_screen_branch = *(u32 *)(present_buffer_ptr + 0x20) + 1;
    u32 *present_buffer_pa = (u32 *)PA_FROM_VA_PTR(present_buffer_ptr);
    memcpy(present_buffer_pa, DRAW_PATCH, 0x94);
    present_buffer_pa[7] = get_screen_branch; // fix get_screen branch instruction
    present_buffer_pa[29] = (u32)run_hook;

    svcInvalidateEntireInstructionCache();
}
