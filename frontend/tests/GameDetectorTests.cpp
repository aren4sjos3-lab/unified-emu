// SPDX-License-Identifier: GPL-2.0-or-later
// Pruebas del detector con cabeceras sintéticas (no contienen datos de juegos reales).

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <vector>

#include "core/GameDetector.h"

namespace fs = std::filesystem;
using namespace UnifiedEmu;

namespace
{
int g_failures = 0;

#define CHECK(cond)                                                       \
  do                                                                      \
  {                                                                       \
    if (!(cond))                                                          \
    {                                                                     \
      std::fprintf(stderr, "%s:%d: falló CHECK(%s)\n", __FILE__, __LINE__, #cond); \
      ++g_failures;                                                       \
    }                                                                     \
  } while (0)

#define CHECK_EQ(a, b)                                                                      \
  do                                                                                        \
  {                                                                                         \
    if (!((a) == (b)))                                                                      \
    {                                                                                       \
      std::fprintf(stderr, "%s:%d: falló CHECK_EQ(%s, %s)\n", __FILE__, __LINE__, #a, #b); \
      ++g_failures;                                                                         \
    }                                                                                       \
  } while (0)

using Bytes = std::vector<std::uint8_t>;

void PutBE32(Bytes& data, std::size_t offset, std::uint32_t value)
{
  data[offset] = std::uint8_t(value >> 24);
  data[offset + 1] = std::uint8_t(value >> 16);
  data[offset + 2] = std::uint8_t(value >> 8);
  data[offset + 3] = std::uint8_t(value);
}

void PutLE32(Bytes& data, std::size_t offset, std::uint32_t value)
{
  data[offset] = std::uint8_t(value);
  data[offset + 1] = std::uint8_t(value >> 8);
  data[offset + 2] = std::uint8_t(value >> 16);
  data[offset + 3] = std::uint8_t(value >> 24);
}

void PutString(Bytes& data, std::size_t offset, const std::string& text)
{
  std::memcpy(data.data() + offset, text.data(), text.size());
}

void WriteFile(const fs::path& path, const Bytes& data)
{
  fs::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::binary);
  out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
}

void WriteText(const fs::path& path, const std::string& text)
{
  fs::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::binary);
  out << text;
}

Bytes N64Header()
{
  Bytes rom(0x1000, 0);
  rom[0] = 0x80;
  rom[1] = 0x37;
  rom[2] = 0x12;
  rom[3] = 0x40;
  PutString(rom, 0x20, "SUPER TEST 64       ");
  PutString(rom, 0x3B, "NTEE");
  return rom;
}

Bytes DiscHeader(bool wii, const std::string& id, const std::string& title)
{
  Bytes disc(0x440, 0);
  PutString(disc, 0, id);
  PutString(disc, 0x20, title);
  if (wii)
    PutBE32(disc, 0x18, 0x5D1C9EA3);
  else
    PutBE32(disc, 0x1C, 0xC2339F3D);
  return disc;
}

void TestN64ByteOrders(const fs::path& root)
{
  const Bytes z64 = N64Header();

  Bytes v64 = z64;
  for (std::size_t i = 0; i + 1 < v64.size(); i += 2)
    std::swap(v64[i], v64[i + 1]);

  Bytes n64 = z64;
  for (std::size_t i = 0; i + 3 < n64.size(); i += 4)
  {
    std::swap(n64[i], n64[i + 3]);
    std::swap(n64[i + 1], n64[i + 2]);
  }

  for (const auto& [name, data] : {std::pair{"a.z64", z64}, {"b.v64", v64}, {"c.N64", n64}})
  {
    WriteFile(root / name, data);
    const auto info = DetectFile(root / name);
    CHECK(info.has_value());
    if (!info)
      continue;
    CHECK(info->platform == Platform::N64);
    CHECK_EQ(info->title, std::string("SUPER TEST 64"));
    CHECK_EQ(info->gameId, std::string("NTEE"));
    CHECK(BackendFor(info->platform) == Backend::Project64);
  }

  Bytes garbage(0x1000, 0xAB);
  WriteFile(root / "bad.z64", garbage);
  CHECK(!DetectFile(root / "bad.z64").has_value());
}

void TestDiscImages(const fs::path& root)
{
  WriteFile(root / "gc.iso", DiscHeader(false, "GTEE01", "Test GameCube Game"));
  auto gc = DetectFile(root / "gc.iso");
  CHECK(gc && gc->platform == Platform::GameCube);
  CHECK(gc && gc->gameId == "GTEE01");
  CHECK(gc && gc->title == "Test GameCube Game");
  CHECK(gc && BackendFor(gc->platform) == Backend::Dolphin);

  WriteFile(root / "wii.iso", DiscHeader(true, "RTEE01", "Test Wii Game"));
  auto wii = DetectFile(root / "wii.iso");
  CHECK(wii && wii->platform == Platform::Wii);
  CHECK(wii && wii->title == "Test Wii Game");

  // ISO sin firmas de Nintendo (p. ej. una ISO de PC): no se reconoce.
  WriteFile(root / "pc.iso", Bytes(0x1000, 0));
  CHECK(!DetectFile(root / "pc.iso").has_value());

  // RVZ: disc_type = 2 (Wii) y cabecera del disco en 0x58.
  Bytes rvz(0x100, 0);
  PutString(rvz, 0, std::string("RVZ\x01", 4));
  PutBE32(rvz, 0x48, 2);
  const Bytes wiiHeader = DiscHeader(true, "RTEF01", "Compressed Wii");
  std::copy(wiiHeader.begin(), wiiHeader.begin() + 0x80, rvz.begin() + 0x58);
  WriteFile(root / "game.rvz", rvz);
  auto rvzInfo = DetectFile(root / "game.rvz");
  CHECK(rvzInfo && rvzInfo->platform == Platform::Wii);
  CHECK(rvzInfo && rvzInfo->gameId == "RTEF01");
  CHECK(rvzInfo && rvzInfo->format == "RVZ");

  // GCZ de GameCube (sub_type = 0).
  Bytes gcz(0x40, 0);
  PutLE32(gcz, 0, 0xB10BC001);
  WriteFile(root / "game.gcz", gcz);
  auto gczInfo = DetectFile(root / "game.gcz");
  CHECK(gczInfo && gczInfo->platform == Platform::GameCube);

  // WBFS con la cabecera del disco en 0x200.
  Bytes wbfs(0x300, 0);
  PutString(wbfs, 0, "WBFS");
  const Bytes wbfsHeader = DiscHeader(true, "RTEG01", "WBFS Game");
  std::copy(wbfsHeader.begin(), wbfsHeader.begin() + 0x60, wbfs.begin() + 0x200);
  WriteFile(root / "game.wbfs", wbfs);
  auto wbfsInfo = DetectFile(root / "game.wbfs");
  CHECK(wbfsInfo && wbfsInfo->platform == Platform::Wii);
  CHECK(wbfsInfo && wbfsInfo->title == "WBFS Game");
}

fs::path MakeWiiUTitle(const fs::path& dir)
{
  Bytes rpx(0x40, 0);
  PutString(rpx, 0, "\x7F" "ELF");
  rpx[4] = 1;
  rpx[5] = 2;
  rpx[6] = 1;
  rpx[7] = 0xCA;
  WriteFile(dir / "code" / "game.rpx", rpx);
  WriteText(dir / "meta" / "meta.xml",
            "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<menu type=\"complex\">\n"
            "  <title_id type=\"hexBinary\" length=\"8\">0005000010ABCD00</title_id>\n"
            "  <longname_en type=\"string\" length=\"512\">Test &amp; Game\nU</longname_en>\n"
            "</menu>\n");
  return dir / "code" / "game.rpx";
}

void TestWiiU(const fs::path& root)
{
  const fs::path titleDir = root / "WiiU" / "My Title";
  const fs::path rpx = MakeWiiUTitle(titleDir);

  auto title = DetectWiiUTitleDirectory(titleDir);
  CHECK(title && title->platform == Platform::WiiU);
  CHECK(title && title->title == "Test & Game U");
  CHECK(title && title->gameId == "0005000010ABCD00");
  CHECK(title && title->path == rpx);
  CHECK(title && title->displayPath == titleDir);
  CHECK(title && BackendFor(title->platform) == Backend::Cemu);

  Bytes wux(0x20, 0);
  PutString(wux, 0, "WUX0");
  PutLE32(wux, 4, 0x1099D02E);
  WriteFile(root / "WiiU" / "disc.wux", wux);
  auto wuxInfo = DetectFile(root / "WiiU" / "disc.wux");
  CHECK(wuxInfo && wuxInfo->platform == Platform::WiiU);

  // Un ELF normal con extensión .rpx no es un ejecutable de Wii U.
  Bytes elf(0x40, 0);
  PutString(elf, 0, "\x7F" "ELF");
  WriteFile(root / "fake.rpx", elf);
  CHECK(!DetectFile(root / "fake.rpx").has_value());
}

void TestScan(const fs::path& root)
{
  // root contiene: 3 ROMs N64 válidas, gc.iso, wii.iso, game.rvz, game.gcz, game.wbfs,
  // WiiU/My Title (carpeta), WiiU/disc.wux. Más archivos no válidos.
  WriteText(root / "readme.txt", "no es un juego");
  const auto recursive = ScanDirectory(root, true);
  CHECK_EQ(recursive.size(), std::size_t(10));

  std::size_t wiiU = 0;
  for (const auto& game : recursive)
    wiiU += game.platform == Platform::WiiU;
  // La carpeta del título cuenta una vez; su .rpx no se añade por separado.
  CHECK_EQ(wiiU, std::size_t(2));

  // Sin recursión: los juegos de WiiU/ no se ven, pero sí una carpeta de título directa.
  const auto flat = ScanDirectory(root, false);
  CHECK_EQ(flat.size(), std::size_t(8));
  const auto wiiUFlat = ScanDirectory(root / "WiiU", false);
  CHECK_EQ(wiiUFlat.size(), std::size_t(2));

  // Apuntar directamente a la carpeta del título.
  CHECK_EQ(ScanDirectory(root / "WiiU" / "My Title", true).size(), std::size_t(1));

  // Cancelación inmediata.
  CHECK(ScanDirectory(root, true, [] { return true; }).empty());
}
}  // namespace

int main()
{
  std::random_device rd;
  const fs::path root =
      fs::temp_directory_path() / ("unifiedemu-tests-" + std::to_string(rd()));
  fs::create_directories(root);

  TestN64ByteOrders(root);
  TestDiscImages(root);
  TestWiiU(root);
  fs::remove(root / "fake.rpx");
  fs::remove(root / "bad.z64");
  fs::remove(root / "pc.iso");
  TestScan(root);

  std::error_code ec;
  fs::remove_all(root, ec);

  if (g_failures)
  {
    std::fprintf(stderr, "%d comprobaciones fallidas\n", g_failures);
    return 1;
  }
  std::puts("Todas las pruebas del detector pasaron");
  return 0;
}
