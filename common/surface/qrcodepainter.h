#pragma once
#include <QPainter>
#include <QRectF>
#include <QString>
#include <vector>
#include <array>

namespace QRv4
{

// GF(256) táblák Reed–Solomonhoz (QR polinom: x^8 + x^4 + x^3 + x^2 + 1)
static const std::array<uint8_t, 256> GF_EXP = []{
    std::array<uint8_t,256> e{};
    uint8_t x = 1;
    for (int i = 0; i < 256; ++i) {
        e[i] = x;
        x ^= (x << 1);
        if (x & 0x100) x ^= 0x11D;
    }
    return e;
}();

static const std::array<uint8_t, 256> GF_LOG = []{
    std::array<uint8_t,256> l{};
    for (int i = 0; i < 255; ++i)
        l[GF_EXP[i]] = i;
    l[0] = 0;
    return l;
}();

// GF(256) szorzás
inline uint8_t gfMul(uint8_t a, uint8_t b)
{
    if (!a || !b) return 0;
    int la = GF_LOG[a];
    int lb = GF_LOG[b];
    return GF_EXP[(la + lb) % 255];
}

// Version 4-L: 50 adatbyte, 24 ECC byte
// Generator polinom 24 ECC byte-hoz (előre számított, QR szabvány szerint)
static const std::array<uint8_t, 25> RS_GEN = {
    0x1D,0xC6,0x96,0x5D,0xC1,0x47,0xAE,0x3D,0x19,0xC5,0x5A,0xB3,
    0x8F,0xF8,0x4E,0xE3,0xA9,0xCB,0x57,0x9F,0xC0,0xB2,0x7A,0xD5,0x00
};

// Reed–Solomon ECC generálás (24 byte)
inline std::vector<uint8_t> rsCompute(const std::vector<uint8_t>& data)
{
    std::vector<uint8_t> ecc(24, 0);
    for (uint8_t d : data) {
        uint8_t factor = d ^ ecc[0];
        // shift
        for (int i = 0; i < 23; ++i)
            ecc[i] = ecc[i+1] ^ gfMul(RS_GEN[i], factor);
        ecc[23] = gfMul(RS_GEN[23], factor);
    }
    return ecc;
}

// Bitstream builder (byte mode, Version 4-L)
inline std::vector<bool> buildBitStream(const QString& text)
{
    std::vector<uint8_t> bytes;
    QByteArray ba = text.toUtf8();
    for (int i = 0; i < ba.size() && i < 50; ++i)
        bytes.push_back(static_cast<uint8_t>(ba[i]));

    std::vector<bool> bits;

    // Mode: 0100 (byte)
    bits.push_back(0); bits.push_back(1); bits.push_back(0); bits.push_back(0);

    // Length: 8 bit
    uint8_t len = static_cast<uint8_t>(bytes.size());
    for (int i = 7; i >= 0; --i)
        bits.push_back((len >> i) & 1);

    // Data bytes
    for (uint8_t b : bytes) {
        for (int i = 7; i >= 0; --i)
            bits.push_back((b >> i) & 1);
    }

    // Terminator (max 4 bit)
    int maxBits = 50 * 8; // Version 4-L data capacity (simplified)
    int remaining = maxBits - static_cast<int>(bits.size());
    for (int i = 0; i < remaining && i < 4; ++i)
        bits.push_back(0);

    // Pad to byte boundary
    while (bits.size() % 8 != 0)
        bits.push_back(0);

    // Pad bytes (0xEC, 0x11)
    bool toggle = true;
    while (bits.size() < maxBits) {
        uint8_t pad = toggle ? 0xEC : 0x11;
        toggle = !toggle;
        for (int i = 7; i >= 0; --i)
            bits.push_back((pad >> i) & 1);
    }

    // Data bytes visszaállítása ECC-hez
    std::vector<uint8_t> dataBytes;
    for (size_t i = 0; i < bits.size(); i += 8) {
        uint8_t v = 0;
        for (int j = 0; j < 8; ++j)
            v = (v << 1) | (bits[i + j] ? 1 : 0);
        dataBytes.push_back(v);
    }

    // ECC
    std::vector<uint8_t> ecc = rsCompute(dataBytes);

    // ECC bitek hozzáfűzése
    for (uint8_t e : ecc) {
        for (int i = 7; i >= 0; --i)
            bits.push_back((e >> i) & 1);
    }

    return bits;
}

// 33x33 mátrix inicializálása
inline std::vector<std::vector<bool>> initMatrix()
{
    int size = 33;
    std::vector<std::vector<bool>> m(size, std::vector<bool>(size, false));

    auto drawFinder = [&](int ox, int oy) {
        for (int y = 0; y < 7; ++y)
            for (int x = 0; x < 7; ++x) {
                bool v = (x == 0 || x == 6 || y == 0 || y == 6 ||
                          (x >= 2 && x <= 4 && y >= 2 && y <= 4));
                m[oy + y][ox + x] = v;
            }
    };

    // Finder patterns
    drawFinder(0, 0);
    drawFinder(33 - 7, 0);
    drawFinder(0, 33 - 7);

    // Timing patterns
    for (int i = 8; i < 33 - 8; ++i) {
        m[6][i] = (i % 2 == 0);
        m[i][6] = (i % 2 == 0);
    }

    // Alignment pattern (Version 4: center at (26,26))
    auto drawAlign = [&](int cx, int cy) {
        for (int y = -2; y <= 2; ++y)
            for (int x = -2; x <= 2; ++x) {
                bool v = (x == -2 || x == 2 || y == -2 || y == 2 ||
                          (x == 0 && y == 0));
                m[cy + y][cx + x] = v;
            }
    };
    drawAlign(26, 26);

    return m;
}

// Bitstream beillesztése (zig-zag, mask 0: (r+c)%2==0 invert)
inline void placeData(std::vector<std::vector<bool>>& m,
                      const std::vector<bool>& bits)
{
    int size = 33;
    int bitIdx = 0;
    int dir = -1; // up
    int col = size - 1;

    auto isReserved = [&](int r, int c) -> bool {
        // Finder + timing + alignment + format info környéke
        if (r < 9 && c < 9) return true;
        if (r < 9 && c >= size - 8) return true;
        if (r >= size - 8 && c < 9) return true;
        if (r == 6 || c == 6) return true;
        if (r >= 24 && r <= 28 && c >= 24 && c <= 28) return true;
        return false;
    };

    while (col > 0 && bitIdx < static_cast<int>(bits.size())) {
        if (col == 6) col--; // skip timing column

        for (int i = 0; i < size; ++i) {
            int row = (dir == -1) ? (size - 1 - i) : i;

            for (int j = 0; j < 2; ++j) {
                int c = col - j;
                int r = row;
                if (isReserved(r, c)) continue;
                if (bitIdx >= static_cast<int>(bits.size())) break;

                bool v = bits[bitIdx++];
                // mask 0: invert, ha (r+c)%2==0
                if ((r + c) % 2 == 0)
                    v = !v;
                m[r][c] = v;
            }
        }
        col -= 2;
        dir = -dir;
    }
}

// Format info (Low ECC, mask 0) – fix bitminta Version 4-L, mask 0
inline void placeFormatInfo(std::vector<std::vector<bool>>& m)
{
    // ECC: L (01), mask: 000 → format bits: 01 000
    // QR szabvány szerint generált 15 bit (példa): 0b111011111000100
    const uint16_t fmt = 0b111011111000100;

    int size = 33;

    // Felső bal
    for (int i = 0; i < 6; ++i)
        m[8][i] = (fmt >> i) & 1;
    m[8][7] = (fmt >> 6) & 1;
    m[8][8] = (fmt >> 7) & 1;
    m[7][8] = (fmt >> 8) & 1;
    for (int i = 9; i < 15; ++i)
        m[14 - i][8] = (fmt >> i) & 1;

    // Felső jobb
    for (int i = 0; i < 8; ++i)
        m[i][size - 1 - 8] = (fmt >> i) & 1;

    // Alsó bal
    for (int i = 8; i < 15; ++i)
        m[size - 1 - (14 - i)][8] = (fmt >> i) & 1;
}

// Teljes mátrix generálása Version 4-L, byte mode
inline std::vector<std::vector<bool>> makeMatrix(const QString& text)
{
    auto bits = buildBitStream(text);
    auto m    = initMatrix();
    placeData(m, bits);
    placeFormatInfo(m);
    return m;
}

// Rajzolás QPainter-rel
inline void drawQR(QPainter& p, const QString& text, const QRectF& rect)
{
    auto m = makeMatrix(text);
    int size = static_cast<int>(m.size());
    qreal scale = rect.width() / size;

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            if (m[y][x]) {
                p.fillRect(rect.left() + x * scale,
                           rect.top()  + y * scale,
                           scale,
                           scale,
                           Qt::black);
            }
        }
    }
}

} // namespace QRv4
