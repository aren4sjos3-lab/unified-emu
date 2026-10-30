// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/GameDetector.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <iterator>
#include <system_error>

namespace fs = std::filesystem;

namespace UnifiedEmu
{
namespace
{
// Firmas conocidas (big endian salvo que se indique lo contrario).
constexpr std::uint32_t kGameCubeDiscMagic = 0xC2339F3D;  // offset 0x1C
constexpr std::uint32_t kWiiDiscMagic = 0x5D1C9EA3;       // offset 0x18
constexpr std::uint32_t kTgcMagic = 0xAE0F38A2;           // offset 0x00
constexpr std::uint32_t kGczMagicLE = 0xB10BC001;         // offset 0x00, little endian
constexpr std::uint32_t kWuxMagic1LE = 0x1099D02E;        // offset 0x04, little endian
constexpr std::uint8_t kElfOsAbiCafe = 0xCA;              // e_ident[EI_OSABI] de un RPX

// Un disco de GameCube/Wii o un contenedor no puede ser más pequeño que esto.
constexpr std::uint64_t kMinDiscHeaderSize = 0x440;

std::vector<std::uint8_t> ReadAt(const fs::path& file, std::uint64_t offset, std::size_t count)
{
  std::ifstream stream(file, std::ios::binary);
  if (!stream)
    return {};
  stream.seekg(static_cast<std::streamoff>(offset));
  if (!stream)
    return {};
  std::vector<std::uint8_t> data(count);
  stream.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(count));
  data.resize(static_cast<std::size_t>(stream.gcount()));
  return data;
}

std::uint32_t ReadBE32(const std::vector<std::uint8_t>& data, std::size_t offset)
{
  if (offset + 4 > data.size())
    return 0;
  return (std::uint32_t(data[offset]) << 24) | (std::uint32_t(data[offset + 1]) << 16) |
         (std::uint32_t(data[offset + 2]) << 8) | std::uint32_t(data[offset + 3]);
}

std::uint32_t ReadLE32(const std::vector<std::uint8_t>& data, std::size_t offset)
{
  if (offset + 4 > data.size())
    return 0;
  return std::uint32_t(data[offset]) | (std::uint32_t(data[offset + 1]) << 8) |
         (std::uint32_t(data[offset + 2]) << 16) | (std::uint32_t(data[offset + 3]) << 24);
}

// Cadena de longitud fija, cortada en el primer NUL y sin espacios a los lados.
// Los caracteres no ASCII se descartan (los títulos japoneses vienen en Shift-JIS).
std::string FixedString(const std::vector<std::uint8_t>& data, std::size_t offset,
                        std::size_t length)
{
  std::string result;
  for (std::size_t i = offset; i < offset + length && i < data.size(); ++i)
  {
    const std::uint8_t c = data[i];
    if (c == 0)
      break;
    if (c >= 0x20 && c < 0x7F)
      result.push_back(static_cast<char>(c));
  }
  const auto first = result.find_first_not_of(' ');
  if (first == std::string::npos)
    return {};
  const auto last = result.find_last_not_of(' ');
  return result.substr(first, last - first + 1);
}

std::string LowerExtension(const fs::path& file)
{
  std::string ext = file.extension().u8string();
  std::transform(ext.begin(), ext.end(), ext.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return ext;
}

std::string StemTitle(const fs::path& file)
{
  return file.stem().u8string();
}

std::uint64_t FileSize(const fs::path& file)
{
  std::error_code ec;
  const auto size = fs::file_size(file, ec);
  return ec ? 0 : static_cast<std::uint64_t>(size);
}

GameInfo MakeInfo(Platform platform, const fs::path& file, std::string format)
{
  GameInfo info;
  info.platform = platform;
  info.path = file;
  info.displayPath = file;
  info.format = std::move(format);
  info.size = FileSize(file);
  info.title = StemTitle(file);
  return info;
}

// Rellena ID y título a partir de una cabecera de disco GameCube/Wii situada en `base`.
void FillDiscHeader(GameInfo& info, const std::vector<std::uint8_t>& data, std::size_t base)
{
  const std::string id = FixedString(data, base, 6);
  const std::string title = FixedString(data, base + 0x20, 0x40);
  if (!id.empty())
    info.gameId = id;
  if (!title.empty())
    info.title = title;
}

// Plataforma según las firmas de una cabecera de disco situada en `base`.
Platform DiscPlatform(const std::vector<std::uint8_t>& data, std::size_t base)
{
  if (ReadBE32(data, base + 0x18) == kWiiDiscMagic)
    return Platform::Wii;
  if (ReadBE32(data, base + 0x1C) == kGameCubeDiscMagic)
    return Platform::GameCube;
  return Platform::Unknown;
}

std::optional<GameInfo> DetectN64(const fs::path& file)
{
  auto header = ReadAt(file, 0, 0x40);
  if (header.size() < 0x40)
    return std::nullopt;

  std::string format;
  if (header[0] == 0x80 && header[1] == 0x37 && header[2] == 0x12 && header[3] == 0x40)
  {
    format = "Z64 (big endian)";
  }
  else if (header[0] == 0x37 && header[1] == 0x80 && header[2] == 0x40 && header[3] == 0x12)
  {
    format = "V64 (byteswapped)";
    for (std::size_t i = 0; i + 1 < header.size(); i += 2)
      std::swap(header[i], header[i + 1]);
  }
  else if (header[0] == 0x40 && header[1] == 0x12 && header[2] == 0x37 && header[3] == 0x80)
  {
    format = "N64 (little endian)";
    for (std::size_t i = 0; i + 3 < header.size(); i += 4)
    {
      std::swap(header[i], header[i + 3]);
      std::swap(header[i + 1], header[i + 2]);
    }
  }
  else
  {
    return std::nullopt;
  }

  GameInfo info = MakeInfo(Platform::N64, file, format);
  const std::string title = FixedString(header, 0x20, 20);
  if (!title.empty())
    info.title = title;
  info.gameId = FixedString(header, 0x3B, 4);
  return info;
}

std::optional<GameInfo> DetectDolphinFile(const fs::path& file, const std::string& ext)
{
  if (ext == ".dol" || ext == ".elf")
    return MakeInfo(Platform::Wii, file, "Homebrew (" + ext.substr(1) + ")");
  if (ext == ".wad")
    return MakeInfo(Platform::Wii, file, "WAD (canal)");

  const auto head = ReadAt(file, 0, 0x100);
  if (head.size() < 8)
    return std::nullopt;

  // RVZ / WIA: disc_type en 0x48 y copia de la cabecera del disco en 0x58.
  if ((head[0] == 'R' && head[1] == 'V' && head[2] == 'Z' && head[3] == 0x01) ||
      (head[0] == 'W' && head[1] == 'I' && head[2] == 'A' && head[3] == 0x01))
  {
    const std::uint32_t discType = ReadBE32(head, 0x48);
    const Platform platform = discType == 2 ? Platform::Wii :
                              discType == 1 ? Platform::GameCube :
                                              DiscPlatform(head, 0x58);
    if (platform == Platform::Unknown)
      return std::nullopt;
    GameInfo info = MakeInfo(platform, file, head[0] == 'R' ? "RVZ" : "WIA");
    FillDiscHeader(info, head, 0x58);
    return info;
  }

  // GCZ: no guarda la cabecera sin comprimir; sub_type indica la plataforma.
  if (ReadLE32(head, 0) == kGczMagicLE)
  {
    const Platform platform = ReadLE32(head, 4) == 1 ? Platform::Wii : Platform::GameCube;
    return MakeInfo(platform, file, "GCZ");
  }

  // WBFS: la cabecera del disco está en 0x200.
  if (head[0] == 'W' && head[1] == 'B' && head[2] == 'F' && head[3] == 'S')
  {
    const auto disc = ReadAt(file, 0x200, 0x60);
    GameInfo info = MakeInfo(Platform::Wii, file, "WBFS");
    FillDiscHeader(info, disc, 0);
    return info;
  }

  // CISO: mapa de bloques de 0x8000 bytes y los datos empiezan en 0x8000.
  if (head[0] == 'C' && head[1] == 'I' && head[2] == 'S' && head[3] == 'O')
  {
    const auto disc = ReadAt(file, 0x8000, 0x60);
    const Platform platform = DiscPlatform(disc, 0);
    if (platform == Platform::Unknown)
      return std::nullopt;
    GameInfo info = MakeInfo(platform, file, "CISO");
    FillDiscHeader(info, disc, 0);
    return info;
  }

  // TGC: demo de GameCube; el disco interno empieza en el offset indicado en 0x10.
  if (ReadBE32(head, 0) == kTgcMagic)
  {
    GameInfo info = MakeInfo(Platform::GameCube, file, "TGC");
    const auto disc = ReadAt(file, ReadBE32(head, 0x10), 0x60);
    if (DiscPlatform(disc, 0) == Platform::GameCube)
      FillDiscHeader(info, disc, 0);
    return info;
  }

  // ISO / GCM sin comprimir.
  if (FileSize(file) < kMinDiscHeaderSize)
    return std::nullopt;
  const Platform platform = DiscPlatform(head, 0);
  if (platform == Platform::Unknown)
    return std::nullopt;
  GameInfo info = MakeInfo(platform, file, ext == ".gcm" ? "GCM" : "ISO");
  FillDiscHeader(info, head, 0);
  return info;
}

// Decodifica las entidades XML más habituales y normaliza los saltos de línea.
std::string XmlText(std::string text)
{
  const std::array<std::pair<const char*, const char*>, 7> entities = {{
      {"&#10;", " "},
      {"&#13;", ""},
      {"&lt;", "<"},
      {"&gt;", ">"},
      {"&quot;", "\""},
      {"&apos;", "'"},
      {"&amp;", "&"},
  }};
  for (const auto& [from, to] : entities)
  {
    const std::string needle = from;
    for (auto pos = text.find(needle); pos != std::string::npos; pos = text.find(needle, pos))
    {
      text.replace(pos, needle.size(), to);
      pos += std::char_traits<char>::length(to);
    }
  }
  std::replace(text.begin(), text.end(), '\n', ' ');
  text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
  const auto first = text.find_first_not_of(' ');
  if (first == std::string::npos)
    return {};
  return text.substr(first, text.find_last_not_of(' ') - first + 1);
}

std::string XmlElement(const std::string& xml, const std::string& name)
{
  const auto open = xml.find("<" + name);
  if (open == std::string::npos)
    return {};
  const auto start = xml.find('>', open);
  const auto end = xml.find("</" + name + ">", open);
  if (start == std::string::npos || end == std::string::npos || end < start)
    return {};
  return XmlText(xml.substr(start + 1, end - start - 1));
}

// Lee nombre e ID de un meta.xml de Wii U.
void FillWiiUMeta(GameInfo& info, const fs::path& metaXml)
{
  std::ifstream stream(metaXml, std::ios::binary);
  if (!stream)
    return;
  const std::string xml((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
  std::string title = XmlElement(xml, "longname_en");
  if (title.empty())
    title = XmlElement(xml, "shortname_en");
  if (!title.empty())
    info.title = title;
  const std::string titleId = XmlElement(xml, "title_id");
  if (!titleId.empty())
    info.gameId = titleId;
}

std::optional<GameInfo> DetectWiiUFile(const fs::path& file, const std::string& ext)
{
  if (ext == ".wua")
    return MakeInfo(Platform::WiiU, file, "WUA");

  const auto head = ReadAt(file, 0, 0x10);
  if (head.size() < 8)
    return std::nullopt;

  if (ext == ".wux")
  {
    if (!(head[0] == 'W' && head[1] == 'U' && head[2] == 'X' && head[3] == '0' &&
          ReadLE32(head, 4) == kWuxMagic1LE))
      return std::nullopt;
    return MakeInfo(Platform::WiiU, file, "WUX");
  }

  if (ext == ".wud")
  {
    GameInfo info = MakeInfo(Platform::WiiU, file, "WUD");
    // Los discos empiezan con el código de producto, p. ej. "WUP-P-ARDP".
    if (head[0] == 'W' && head[1] == 'U' && head[2] == 'P' && head[3] == '-')
      info.gameId = FixedString(head, 0, 10);
    return info;
  }

  if (ext == ".rpx")
  {
    if (!(head[0] == 0x7F && head[1] == 'E' && head[2] == 'L' && head[3] == 'F' &&
          head[7] == kElfOsAbiCafe))
      return std::nullopt;
    GameInfo info = MakeInfo(Platform::WiiU, file, "RPX");
    const fs::path metaXml = file.parent_path().parent_path() / "meta" / "meta.xml";
    std::error_code ec;
    if (file.parent_path().filename() == "code" && fs::is_regular_file(metaXml, ec))
      FillWiiUMeta(info, metaXml);
    return info;
  }

  return std::nullopt;
}

bool IsN64Extension(const std::string& ext)
{
  return ext == ".z64" || ext == ".n64" || ext == ".v64" || ext == ".u64";
}

bool IsDolphinExtension(const std::string& ext)
{
  return ext == ".iso" || ext == ".gcm" || ext == ".gcz" || ext == ".rvz" || ext == ".wia" ||
         ext == ".wbfs" || ext == ".ciso" || ext == ".tgc" || ext == ".dol" || ext == ".elf" ||
         ext == ".wad";
}

bool IsWiiUExtension(const std::string& ext)
{
  return ext == ".wud" || ext == ".wux" || ext == ".wua" || ext == ".rpx";
}
}  // namespace

const char* PlatformName(Platform platform)
{
  switch (platform)
  {
  case Platform::N64:
    return "Nintendo 64";
  case Platform::GameCube:
    return "GameCube";
  case Platform::Wii:
    return "Wii";
  case Platform::WiiU:
    return "Wii U";
  case Platform::Unknown:
    break;
  }
  return "Desconocida";
}

Backend BackendFor(Platform platform)
{
  switch (platform)
  {
  case Platform::N64:
    return Backend::Project64;
  case Platform::GameCube:
  case Platform::Wii:
    return Backend::Dolphin;
  case Platform::WiiU:
    return Backend::Cemu;
  case Platform::Unknown:
    break;
  }
  return Backend::None;
}

std::optional<GameInfo> DetectFile(const fs::path& file)
{
  std::error_code ec;
  if (!fs::is_regular_file(file, ec))
    return std::nullopt;

  const std::string ext = LowerExtension(file);
  if (IsN64Extension(ext))
    return DetectN64(file);
  if (IsDolphinExtension(ext))
    return DetectDolphinFile(file, ext);
  if (IsWiiUExtension(ext))
    return DetectWiiUFile(file, ext);
  return std::nullopt;
}

std::optional<GameInfo> DetectWiiUTitleDirectory(const fs::path& dir)
{
  std::error_code ec;
  const fs::path code = dir / "code";
  const fs::path metaXml = dir / "meta" / "meta.xml";
  if (!fs::is_directory(code, ec) || !fs::is_regular_file(metaXml, ec))
    return std::nullopt;

  for (fs::directory_iterator it(code, ec), end; !ec && it != end; it.increment(ec))
  {
    if (LowerExtension(it->path()) != ".rpx")
      continue;
    auto info = DetectWiiUFile(it->path(), ".rpx");
    if (!info)
      continue;
    info->format = "Carpeta (RPX)";
    info->displayPath = dir;
    if (info->title == StemTitle(it->path()))
      info->title = dir.filename().u8string();
    return info;
  }
  return std::nullopt;
}

std::vector<GameInfo> ScanDirectory(const fs::path& dir, bool recursive,
                                    const std::function<bool()>& cancelled)
{
  std::vector<GameInfo> games;
  std::error_code ec;
  if (!fs::is_directory(dir, ec))
    return games;

  if (auto title = DetectWiiUTitleDirectory(dir))
  {
    games.push_back(std::move(*title));
    return games;
  }

  const auto visit = [&](const fs::directory_entry& entry) -> bool {
    std::error_code entryEc;
    if (entry.is_directory(entryEc))
    {
      if (auto title = DetectWiiUTitleDirectory(entry.path()))
      {
        games.push_back(std::move(*title));
        return false;  // no hace falta entrar en la carpeta del título
      }
      return true;
    }
    if (auto game = DetectFile(entry.path()))
      games.push_back(std::move(*game));
    return true;
  };

  if (recursive)
  {
    fs::recursive_directory_iterator it(dir, fs::directory_options::skip_permission_denied, ec);
    for (fs::recursive_directory_iterator end; !ec && it != end; it.increment(ec))
    {
      if (cancelled && cancelled())
        break;
      if (!visit(*it))
        it.disable_recursion_pending();
    }
  }
  else
  {
    fs::directory_iterator it(dir, fs::directory_options::skip_permission_denied, ec);
    for (fs::directory_iterator end; !ec && it != end; it.increment(ec))
    {
      if (cancelled && cancelled())
        break;
      visit(*it);
    }
  }

  std::sort(games.begin(), games.end(), [](const GameInfo& a, const GameInfo& b) {
    return a.title < b.title;
  });
  return games;
}
}  // namespace UnifiedEmu
