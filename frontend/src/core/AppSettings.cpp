// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/AppSettings.h"

#include <QCoreApplication>
#include <QDir>

namespace UnifiedEmu
{
namespace
{
QString SettingsFile()
{
  return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("UnifiedEmu.ini"));
}

QString BackendKey(Backend backend)
{
  return QStringLiteral("Emulators/") + AppSettings::backendName(backend);
}
}  // namespace

AppSettings::AppSettings() : m_settings(SettingsFile(), QSettings::IniFormat)
{
}

QStringList AppSettings::gameDirectories() const
{
  return m_settings.value(QStringLiteral("Library/Directories")).toStringList();
}

void AppSettings::setGameDirectories(const QStringList& directories)
{
  m_settings.setValue(QStringLiteral("Library/Directories"), directories);
}

bool AppSettings::recursiveScan() const
{
  return m_settings.value(QStringLiteral("Library/Recursive"), true).toBool();
}

void AppSettings::setRecursiveScan(bool recursive)
{
  m_settings.setValue(QStringLiteral("Library/Recursive"), recursive);
}

bool AppSettings::fullscreen() const
{
  return m_settings.value(QStringLiteral("Launch/Fullscreen"), false).toBool();
}

void AppSettings::setFullscreen(bool fullscreen)
{
  m_settings.setValue(QStringLiteral("Launch/Fullscreen"), fullscreen);
}

QString AppSettings::emulatorPath(Backend backend) const
{
  return m_settings.value(BackendKey(backend), defaultEmulatorPath(backend)).toString();
}

void AppSettings::setEmulatorPath(Backend backend, const QString& path)
{
  if (path.isEmpty() || path == defaultEmulatorPath(backend))
    m_settings.remove(BackendKey(backend));
  else
    m_settings.setValue(BackendKey(backend), path);
}

QString AppSettings::resolvedEmulatorPath(Backend backend) const
{
  const QString path = emulatorPath(backend);
  if (path.isEmpty())
    return {};
  return QDir::cleanPath(QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(path));
}

QString AppSettings::defaultEmulatorPath(Backend backend)
{
  switch (backend)
  {
  case Backend::Project64:
    return QStringLiteral("emulators/Project64/Project64.exe");
  case Backend::Dolphin:
    return QStringLiteral("emulators/Dolphin/Dolphin.exe");
  case Backend::Cemu:
    return QStringLiteral("emulators/Cemu/Cemu.exe");
  case Backend::None:
    break;
  }
  return {};
}

QString AppSettings::backendName(Backend backend)
{
  switch (backend)
  {
  case Backend::Project64:
    return QStringLiteral("Project64");
  case Backend::Dolphin:
    return QStringLiteral("Dolphin");
  case Backend::Cemu:
    return QStringLiteral("Cemu");
  case Backend::None:
    break;
  }
  return {};
}

void AppSettings::sync()
{
  m_settings.sync();
}
}  // namespace UnifiedEmu
