// SPDX-License-Identifier: GPL-2.0-or-later
// Detección de juegos por cabecera de archivo. Sin dependencias de Qt para
// poder probarlo de forma aislada.

#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace UnifiedEmu
{
enum class Platform
{
  Unknown,
  N64,
  GameCube,
  Wii,
  WiiU,
};

// Emulador que se encarga de cada plataforma.
enum class Backend
{
  None,
  Project64,
  Dolphin,
  Cemu,
};

struct GameInfo
{
  Platform platform = Platform::Unknown;
  // Ruta que se le pasa al emulador (para un título de Wii U extraído es el .rpx).
  std::filesystem::path path;
  // Ruta que se muestra al usuario (para un título de Wii U extraído es la carpeta).
  std::filesystem::path displayPath;
  std::string title;
  std::string gameId;
  std::string format;
  std::uint64_t size = 0;
};

const char* PlatformName(Platform platform);
Backend BackendFor(Platform platform);

// Identifica un archivo suelto. Devuelve nullopt si no es un juego reconocido.
std::optional<GameInfo> DetectFile(const std::filesystem::path& file);

// Identifica una carpeta con un título de Wii U extraído (code/*.rpx + meta/meta.xml).
std::optional<GameInfo> DetectWiiUTitleDirectory(const std::filesystem::path& dir);

// Recorre una carpeta y devuelve todos los juegos reconocidos.
// `cancelled` se consulta periódicamente; si devuelve true se detiene el escaneo.
std::vector<GameInfo> ScanDirectory(const std::filesystem::path& dir, bool recursive,
                                    const std::function<bool()>& cancelled = {});
}  // namespace UnifiedEmu
