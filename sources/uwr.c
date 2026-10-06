// Ultra Warp Ride overlay: exact light-years of every warp hole in view,
// with the odds for its rings and distance.
// Missao Living Dex (OTBella). GPL-3.0, see LICENSE.
#include <3ds.h>
#include <string.h>
#include <stdio.h>
#include "uwr.h"
#include "font.h"
#include "title_info.h"

// ---------------------------------------------------------------------------
// The warp holes
//
// The game keeps the holes of the ride in a pool of 10 slots (0xE4 bytes each),
// preceded by the slot count. Each hole carries its own distance (light-years
// x 100), set when it spawns; entering it shows ceil(distance / 100). Found in
// Ultra Sun save states and confirmed with the debugger (802 and 900 al).
//   +0x00 vtable 0x006FB9F0   +0x04 float distance x100
//   +0x64 u8 color (5 = empty slot)   +0x65 u8 rings (0-2, 3 = flower)
// ---------------------------------------------------------------------------

#define POOL_GUESS   0x305651B8u
#define HOLE_VT      0x006FB9F0u
#define HOLE_SIZE    0xE4u
#define HOLE_SLOTS   10
#define SCAN_START   0x30000000u
#define SCAN_END     0x38000000u
#define SCAN_CHUNK   0x10000u
#define HOLD_FRAMES  90            // ~3 s without holes: hide

typedef struct { int ly; int color; int rings; } Hole;

static u32 g_pool = 0;
static u32 g_scan = SCAN_START;
static u32 g_frame = 0;
static u32 g_last_seen = 0;
static Hole g_holes[HOLE_SLOTS];
static int g_n = 0;
static u32 g_sig = 0;           // changes when the set of holes changes
static int g_version = 0;       // 0 = Ultra Sun, 1 = Ultra Moon

static bool readable(u32 addr, u32 len, u32 *block_end)
{
    MemInfo info;
    PageInfo page;
    if (svcQueryMemory(&info, &page, addr) != 0) return false;
    if (block_end) *block_end = info.base_addr + info.size;
    if (info.state == MEMSTATE_FREE || !(info.perm & MEMPERM_READ)) return false;
    return addr + len <= info.base_addr + info.size;
}

static inline u32 w(u32 p, s32 off) { return *(vu32 *)(p + off); }

static bool is_pool(u32 p)
{
    if (!readable(p - 4, HOLE_SIZE * HOLE_SLOTS + 4, NULL)) return false;
    if (w(p, -4) != HOLE_SLOTS) return false;
    for (int i = 0; i < 3; i++)
        if (w(p + i * HOLE_SIZE, 0) != HOLE_VT) return false;
    return true;
}

static void scan_step(void)
{
    if (is_pool(POOL_GUESS)) { g_pool = POOL_GUESS; return; }
    u32 end = 0;
    if (!readable(g_scan, 4, &end))
    {
        g_scan = end > g_scan ? end : g_scan + SCAN_CHUNK;
    }
    else
    {
        u32 stop = g_scan + SCAN_CHUNK;
        if (stop > end) stop = end;
        for (u32 q = g_scan & ~3u; q + 8 <= stop; q += 4)
            if (*(vu32 *)q == HOLE_SLOTS && *(vu32 *)(q + 4) == HOLE_VT && is_pool(q + 4))
            {
                g_pool = q + 4;
                break;
            }
        g_scan = stop;
    }
    if (g_scan >= SCAN_END) g_scan = SCAN_START;
}

static int ceil_ly(float f)
{
    int v = (int)(f / 100.0f);
    if ((float)v * 100.0f < f) v++;
    return v;
}

static void update(void)
{
    g_frame++;
    if (!g_pool) { scan_step(); if (!g_pool) return; }
    if ((g_frame % 60) == 0 && !is_pool(g_pool)) { g_pool = 0; g_n = 0; return; }

    Hole tmp[HOLE_SLOTS];
    int n = 0;
    u32 sig = 0;
    for (int i = 0; i < HOLE_SLOTS; i++)
    {
        u32 h = g_pool + i * HOLE_SIZE;
        if (w(h, 0) != HOLE_VT) continue;
        int color = *(vu8 *)(h + 0x64), rings = *(vu8 *)(h + 0x65);
        float f = *(volatile float *)(h + 4);
        if (color > 4 || rings > 3 || !(f > 0.0f && f < 10000000.0f)) continue;
        Hole x = {ceil_ly(f), color, rings};
        int k = n++;
        while (k > 0 && tmp[k - 1].ly > x.ly) { tmp[k] = tmp[k - 1]; k--; }  // nearest first
        tmp[k] = x;
    }
    for (int i = 0; i < n; i++) sig = sig * 31u + (u32)(tmp[i].ly * 41 + tmp[i].color * 7 + tmp[i].rings);
    if (n > 0) g_last_seen = g_frame;
    if (n > 0 || g_frame - g_last_seen > HOLD_FRAMES)
    {
        memcpy(g_holes, tmp, sizeof(tmp));
        g_n = n;
        g_sig = sig + (u32)n;
    }
}

static bool visible(void) { return g_n > 0; }

// ---------------------------------------------------------------------------
// Odds tables (Serebii + Bulbapedia, same data as the guide's Ultra Space tab)
// Bands: 0-999, 1000-1499, ..., 4500-4999, 5000+. -1 = not possible.
// Rings: 0 = no ring, 1, 2, 3 = flower.
// ---------------------------------------------------------------------------

static const s8 APPEAR[10][4] = {
    {100, 0, 0, 0}, {78, 20, 2, 0}, {66, 30, 4, 0}, {52, 40, 8, 0}, {33, 50, 16, 1},
    {32, 50, 16, 2}, {31, 50, 16, 3}, {30, 50, 16, 4}, {29, 50, 16, 5}, {29, 50, 16, 5}};

static int legend(int ring, int b)
{
    switch (ring)
    {
    case 0: return 3;
    case 1: return b ? 3 + b : -1;
    case 2: return b ? 7 + 3 * b : -1;
    default: return b >= 4 ? 100 : -1;
    }
}

static int shiny(int ring, int b)
{
    switch (ring)
    {
    case 0: return 1;
    case 1: return b ? 1 + b : -1;
    case 2: return b ? 1 + 2 * b : -1;
    default: return b >= 4 ? 4 * b : -1;
    }
}

// Ultra Beasts in white holes: Nihilego, Buzzwole/Pheromosa, Xurkitree, Kartana/Celesteela, Guzzlord
static int ub(int which, int ring, int b)
{
    switch (which)
    {
    case 0: return ring == 0 ? 60 - 6 * b : ring == 1 ? (b ? 2 : -1) : -1;
    case 1: return ring == 0 ? 35 + 5 * b : ring == 1 ? (b ? 60 - 6 * b : -1) : ring == 2 ? (b ? 2 : -1) : -1;
    case 2: return ring == 0 ? 2 + b : ring == 1 ? (b ? 35 + 4 * b : -1) : ring == 2 ? (b ? 60 - 6 * b : -1) : -1;
    case 3: return ring == 0 ? 2 : ring == 1 ? (b ? 2 + 2 * b : -1) : ring == 2 ? (b ? 35 + 4 * b : -1) : (b >= 4 ? 64 - 6 * b : -1);
    default: return ring == 0 ? 1 : ring == 1 ? (b ? 1 : -1) : ring == 2 ? (b ? 3 + 2 * b : -1) : (b >= 4 ? 36 + 6 * b : -1);
    }
}

static int band_of(int d)
{
    if (d < 1000) return 0;
    int b = 1 + (d - 1000) / 500;
    return b > 9 ? 9 : b;
}



// ---------------------------------------------------------------------------
// Drawing. Framebuffers are rotated: column-major, x = 0..width-1, y = 0..239.
// ---------------------------------------------------------------------------

#define BG      0x1E171B
#define PANEL   0x2A2026
#define LINE    0x46363F
#define FG      0xF6E9EF
#define MUTED   0xBBA3AF
#define DIMC    0x6B5A63
#define ACCENT  0xE48DAB

typedef struct { u8 *fb; u32 stride; u32 fmt; u32 width; } Surf;

static u32 bpp(u32 fmt)
{
    switch (fmt & 7) { case 0: return 4; case 1: return 3; default: return 2; }
}

static inline void px(const Surf *s, int x, int y, u32 rgb)
{
    if (x < 0 || y < 0 || x >= (int)s->width || y >= 240) return;
    u32 r = (rgb >> 16) & 0xFF, g = (rgb >> 8) & 0xFF, b = rgb & 0xFF;
    u32 f = s->fmt & 7, n = bpp(f);
    u8 *p = s->fb + x * s->stride + (239 - y) * n;
    switch (f)
    {
    case 0: p[0] = 0xFF; p[1] = b; p[2] = g; p[3] = r; break;
    case 1: p[0] = b; p[1] = g; p[2] = r; break;
    case 2: *(u16 *)p = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3); break;
    case 3: *(u16 *)p = ((r >> 3) << 11) | ((g >> 3) << 6) | ((b >> 3) << 1) | 1; break;
    default: *(u16 *)p = ((r >> 4) << 12) | ((g >> 4) << 8) | ((b >> 4) << 4) | 0xF; break;
    }
}

static void rect(const Surf *s, int x, int y, int ww, int hh, u32 rgb)
{
    if (x < 0) { ww += x; x = 0; }
    if (y < 0) { hh += y; y = 0; }
    if (x + ww > (int)s->width) ww = (int)s->width - x;
    if (y + hh > 240) hh = 240 - y;
    if (ww <= 0 || hh <= 0) return;
    u32 f = s->fmt & 7, n = bpp(f);
    u32 r = (rgb >> 16) & 0xFF, g = (rgb >> 8) & 0xFF, b = rgb & 0xFF;
    if (n == 2)
    {
        u16 v = f == 2 ? (((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3))
              : f == 3 ? (((r >> 3) << 11) | ((g >> 3) << 6) | ((b >> 3) << 1) | 1)
                       : (((r >> 4) << 12) | ((g >> 4) << 8) | ((b >> 4) << 4) | 0xF);
        for (int i = x; i < x + ww; i++)
        {
            u16 *p = (u16 *)(s->fb + i * s->stride) + (240 - y - hh);
            for (int j = 0; j < hh; j++) p[j] = v;
        }
        return;
    }
    for (int i = x; i < x + ww; i++)
        for (int j = y; j < y + hh; j++) px(s, i, j, rgb);
}

static void chr(const Surf *s, int x, int y, char c, u32 rgb, int k)
{
    unsigned char ch = (unsigned char)c;
    if (ch < 32 || ch > 127) ch = '?';
    const unsigned char *g = &font[(ch - 32) * 8];
    for (int row = 0; row < 8; row++)
        for (int col = 0; col < 8; col++)
            if (g[row] & (0x80 >> col))
            {
                if (k == 1) px(s, x + col, y + row, rgb);
                else rect(s, x + col * k, y + row * k, k, k, rgb);
            }
}

static int text(const Surf *s, int x, int y, const char *t, u32 rgb, int k)
{
    for (; *t; t++, x += 8 * k) chr(s, x, y, *t, rgb, k);
    return x;
}

static void text_r(const Surf *s, int right, int y, const char *t, u32 rgb)
{
    text(s, right - 8 * (int)strlen(t), y, t, rgb, 1);
}

static void thousands(char *out, int v)
{
    if (v >= 1000) sprintf(out, "%d.%03d", v / 1000, v % 1000);
    else sprintf(out, "%d", v);
}

static void pct(const Surf *s, int right, int y, int v, bool strong)
{
    char b[8];
    if (v < 0) { text_r(s, right, y, "--", DIMC); return; }
    sprintf(b, "%d%%", v);
    text_r(s, right, y, b, strong ? ACCENT : FG);
}

// Hole colors as stored by the game (order confirmed in game by Bella).
static const char *COLOR_NAME[5] = {"Branco", "Vermelho", "Azul", "Amarelo", "Verde"};
static const u32 COLOR_RGB[5] = {0xEDEDED, 0xD4483F, 0x3E7BD6, 0xE0AE1F, 0x3FA35A};
#define WHITE 0
static const char *RING_NAME[4] = {"sem anel", "1 anel", "2 aneis", "flor"};

static const char *ub_name(int u)
{
    static const char *us[5] = {"Nihilego", "Buzzwole", "Xurkitree", "Kartana", "Guzzlord"};
    static const char *um[5] = {"Nihilego", "Pheromosa", "Xurkitree", "Celesteela", "Guzzlord"};
    return g_version == 1 ? um[u] : us[u];
}

// Top screen: the nearest hole, cached and copied every frame.
#define TOP_W 248
#define TOP_Y 4
#define TOP_H 30
static u8 g_topbuf[TOP_W * 240 * 4];
static int g_top_w = 0;
static u32 g_top_sig = 1, g_top_fmt = 99;

static void render_top(u32 fmt)
{
    Surf s = {g_topbuf, 240 * bpp(fmt), fmt, TOP_W};
    char num[16], line[24];
    const Hole *h = &g_holes[0];
    char stars[8] = "";
    thousands(num, h->ly);
    sprintf(line, "%s al", num);
    if (h->rings > 0) sprintf(stars, " %d*", h->rings);
    int wpx = 8 + 14 + (int)(strlen(line) + strlen(stars)) * 16 + 10;
    if (wpx > TOP_W) wpx = TOP_W;
    rect(&s, 0, TOP_Y, wpx, TOP_H, PANEL);
    rect(&s, 0, TOP_Y, 3, TOP_H, ACCENT);
    rect(&s, 8, TOP_Y + 8, 12, 14, COLOR_RGB[h->color]);
    int x = text(&s, 26, TOP_Y + 7, line, FG, 2);
    text(&s, x, TOP_Y + 7, stars, ACCENT, 2);
    g_top_w = wpx;
}

static void blit_top(u8 *fb, u32 stride, u32 fmt)
{
    u32 n = bpp(fmt), cs = 240 * n;
    u32 off = (240 - TOP_Y - TOP_H) * n, len = TOP_H * n;
    int x0 = 400 - 6 - g_top_w;
    for (int i = 0; i < g_top_w; i++)
        memcpy(fb + (x0 + i) * stride + off, g_topbuf + i * cs + off, len);
}

// Bottom screen: one row per hole in view, redrawn only when the holes change.
static u8 g_botbuf[320 * 240 * 4];
static u32 g_bot_sig = 1, g_bot_fmt = 99;

static void render_bottom(const Surf *s)
{
    char b[64], num[16];
    rect(s, 0, 0, 320, 240, BG);
    text(s, 8, 6, "ULTRA WARP RIDE", ACCENT, 1);
    text_r(s, 312, 6, "MISSAO LIVING DEX", MUTED);
    text(s, 8, 22, "Buracos a vista, do mais perto", MUTED, 1);

    int y = 38;
    int rows = g_n > 6 ? 6 : g_n;
    for (int i = 0; i < rows; i++, y += 31)
    {
        const Hole *h = &g_holes[i];
        int band = band_of(h->ly);
        rect(s, 8, y, 304, 28, i == 0 ? PANEL : BG);
        rect(s, 12, y + 4, 10, 20, COLOR_RGB[h->color]);
        thousands(num, h->ly);
        sprintf(b, "%s al", num);
        text(s, 28, y + 4, b, FG, 1);
        sprintf(b, "%s %s", COLOR_NAME[h->color], RING_NAME[h->rings]);
        text(s, 28, y + 16, b, h->rings == 3 ? ACCENT : MUTED, 1);
        if (h->color == WHITE)
        {
            int best = 0, bv = -1;
            for (int u = 0; u < 5; u++) if (ub(u, h->rings, band) > bv) { bv = ub(u, h->rings, band); best = u; }
            sprintf(b, "%s %d%%", ub_name(best), bv < 0 ? 0 : bv);
            text_r(s, 308, y + 4, b, FG);
            text_r(s, 308, y + 16, "mais provavel", DIMC);
        }
        else
        {
            sprintf(b, "Lend. %d%%", legend(h->rings, band) < 0 ? 0 : legend(h->rings, band));
            text_r(s, 308, y + 4, b, h->rings == 3 ? ACCENT : FG);
            sprintf(b, "Shiny %d%%", shiny(h->rings, band) < 0 ? 0 : shiny(h->rings, band));
            text_r(s, 308, y + 16, b, MUTED);
        }
    }
    if (g_holes[0].ly >= 5000) text(s, 8, 228, "Passou de 5000 al: tudo no maximo!", ACCENT, 1);
    else text(s, 8, 228, "Valores exatos (iguais aos do pouso)", DIMC, 1);
}

void uwr_init(void) {}

bool uwr_draw(u32 screenId, u8 *fb, u32 stride, u32 format)
{
    static bool version_checked = false;
    if (screenId == 0)
    {
        if (!version_checked)
        {
            version_checked = true;
            g_version = ((get_title_id() >> 8) & 0xFFFF) == 0x1B51 ? 1 : 0;
        }
        update();
    }
    if (!fb || !visible()) return false;
    u32 n = bpp(format);
    if (stride != 240 * n) return false;

    if (screenId == 0)
    {
        if (format != g_top_fmt || g_sig != g_top_sig)
        {
            render_top(format);
            g_top_fmt = format;
            g_top_sig = g_sig;
        }
        blit_top(fb, stride, format);
        return true;
    }

    Surf s = {g_botbuf, stride, format, 320};
    if (format != g_bot_fmt || g_sig != g_bot_sig)
    {
        render_bottom(&s);
        g_bot_fmt = format;
        g_bot_sig = g_sig;
    }
    memcpy(fb, g_botbuf, 320 * stride);
    return true;
}
