// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QString>
#include <QStringList>

#include "core/AppSettings.h"
#include "core/GameDetector.h"

namespace UnifiedEmu
{
struct LaunchCommand
{
  QString program;
  QStringList arguments;
};

class EmulatorLauncher
{
public:
  explicit EmulatorLauncher(const AppSettings& settings);

  // Línea de comandos con la que se arranca un juego en su emulador.
  LaunchCommand commandForGame(const GameInfo& game) const;

  // Arranca el juego. Devuelve un mensaje de error vacío si todo fue bien.
  QString launchGame(const GameInfo& game) const;

  // Abre la interfaz del emulador sin juego (para configurar mandos, gráficos, etc.).
  QString openEmulator(Backend backend) const;

private:
  QString start(Backend backend, const LaunchCommand& command) const;

  const AppSettings& m_settings;
};
}  // namespace UnifiedEmu
