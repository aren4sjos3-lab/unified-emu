// SPDX-License-Identifier: GPL-2.0-or-later
// Conversión entre QString y std::filesystem::path conservando Unicode.

#pragma once

#include <filesystem>

#include <QString>

namespace UnifiedEmu
{
inline QString ToQString(const std::filesystem::path& path)
{
  return QString::fromStdString(path.u8string());
}

inline std::filesystem::path ToPath(const QString& path)
{
  return std::filesystem::u8path(path.toUtf8().toStdString());
}
}  // namespace UnifiedEmu
