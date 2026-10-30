// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/EmulatorLauncher.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcess>

#include "core/Paths.h"

namespace UnifiedEmu
{
EmulatorLauncher::EmulatorLauncher(const AppSettings& settings) : m_settings(settings)
{
}

LaunchCommand EmulatorLauncher::commandForGame(const GameInfo& game) const
{
  const Backend backend = BackendFor(game.platform);
  const QString file = QDir::toNativeSeparators(ToQString(game.path));
  const bool fullscreen = m_settings.fullscreen();

  LaunchCommand command;
  command.program = m_settings.resolvedEmulatorPath(backend);
  switch (backend)
  {
  case Backend::Project64:
    command.arguments << file;
    break;
  case Backend::Dolphin:
    // -b: sin la ventana principal de Dolphin; se cierra al parar el juego.
    command.arguments << QStringLiteral("-b") << QStringLiteral("-e") << file;
    if (fullscreen)
      command.arguments << QStringLiteral("-C") << QStringLiteral("Dolphin.Display.Fullscreen=True");
    break;
  case Backend::Cemu:
    command.arguments << QStringLiteral("-g") << file;
    if (fullscreen)
      command.arguments << QStringLiteral("-f");
    break;
  case Backend::None:
    break;
  }
  return command;
}

QString EmulatorLauncher::launchGame(const GameInfo& game) const
{
  const Backend backend = BackendFor(game.platform);
  if (backend == Backend::None)
    return QCoreApplication::translate("EmulatorLauncher", "Plataforma no soportada.");
  return start(backend, commandForGame(game));
}

QString EmulatorLauncher::openEmulator(Backend backend) const
{
  return start(backend, {m_settings.resolvedEmulatorPath(backend), {}});
}

QString EmulatorLauncher::start(Backend backend, const LaunchCommand& command) const
{
  const QFileInfo program(command.program);
  if (!program.isFile())
  {
    return QCoreApplication::translate("EmulatorLauncher",
                                       "No se encuentra %1 en:\n%2\n\n"
                                       "Revisa la ruta en Configuración.")
        .arg(AppSettings::backendName(backend), QDir::toNativeSeparators(command.program));
  }

  // Cada emulador busca sus archivos (plugins, Sys, etc.) junto a su ejecutable.
  if (!QProcess::startDetached(program.absoluteFilePath(), command.arguments,
                               program.absolutePath()))
  {
    return QCoreApplication::translate("EmulatorLauncher", "No se pudo iniciar %1.")
        .arg(AppSettings::backendName(backend));
  }
  return {};
}
}  // namespace UnifiedEmu
