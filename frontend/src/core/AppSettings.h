// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QSettings>
#include <QString>
#include <QStringList>

#include "core/GameDetector.h"

namespace UnifiedEmu
{
// Configuración portable: se guarda en UnifiedEmu.ini junto al ejecutable.
class AppSettings
{
public:
  AppSettings();

  QStringList gameDirectories() const;
  void setGameDirectories(const QStringList& directories);

  bool recursiveScan() const;
  void setRecursiveScan(bool recursive);

  bool fullscreen() const;
  void setFullscreen(bool fullscreen);

  // Ruta tal como la ha escrito el usuario (puede ser relativa a la carpeta de la app).
  QString emulatorPath(Backend backend) const;
  void setEmulatorPath(Backend backend, const QString& path);
  // Ruta absoluta lista para lanzar.
  QString resolvedEmulatorPath(Backend backend) const;

  static QString defaultEmulatorPath(Backend backend);
  static QString backendName(Backend backend);

  void sync();

private:
  QSettings m_settings;
};
}  // namespace UnifiedEmu
