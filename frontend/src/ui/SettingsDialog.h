// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <map>

#include <QDialog>

#include "core/AppSettings.h"

class QCheckBox;
class QLineEdit;
class QListWidget;

namespace UnifiedEmu
{
class SettingsDialog final : public QDialog
{
  Q_OBJECT

public:
  SettingsDialog(AppSettings& settings, QWidget* parent = nullptr);

  void accept() override;

private:
  QWidget* createLibraryTab();
  QWidget* createEmulatorsTab();
  void addDirectory();
  void removeSelectedDirectories();
  void browseEmulator(Backend backend);

  AppSettings& m_settings;
  QListWidget* m_directories = nullptr;
  QCheckBox* m_recursive = nullptr;
  QCheckBox* m_fullscreen = nullptr;
  std::map<Backend, QLineEdit*> m_emulatorPaths;
};
}  // namespace UnifiedEmu
