// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <memory>
#include <vector>

#include <QFutureWatcher>
#include <QMainWindow>

#include "core/AppSettings.h"
#include "core/EmulatorLauncher.h"
#include "core/GameDetector.h"

class QComboBox;
class QLabel;
class QLineEdit;
class QTableView;

namespace UnifiedEmu
{
class GameFilterProxyModel;
class GameListModel;

class MainWindow final : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow();
  ~MainWindow() override;

protected:
  void showEvent(QShowEvent* event) override;

private:
  void createActions();
  void createLibraryView();
  void rescan();
  void onScanFinished();
  void launchSelected();
  void launchRow(const QModelIndex& proxyIndex);
  void openContainingFolder();
  void openSettings();
  void openEmulator(Backend backend);
  void showContextMenu(const QPoint& pos);
  void updateStatus();
  const GameInfo* selectedGame() const;

  AppSettings m_settings;
  EmulatorLauncher m_launcher;

  GameListModel* m_model = nullptr;
  GameFilterProxyModel* m_proxy = nullptr;
  QTableView* m_view = nullptr;
  QComboBox* m_platformFilter = nullptr;
  QLineEdit* m_search = nullptr;
  QLabel* m_statusLabel = nullptr;
  QAction* m_playAction = nullptr;
  QAction* m_rescanAction = nullptr;

  QFutureWatcher<std::vector<GameInfo>> m_scanWatcher;
  std::shared_ptr<std::atomic<bool>> m_scanCancelled;
  bool m_rescanPending = false;
  bool m_firstShow = true;
};
}  // namespace UnifiedEmu
