// SPDX-License-Identifier: GPL-2.0-or-later

#include "ui/MainWindow.h"

#include <set>

#include <QAction>
#include <QComboBox>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QStyle>
#include <QTableView>
#include <QToolBar>
#include <QUrl>
#include <QtConcurrent/QtConcurrentRun>

#include "core/Paths.h"
#include "ui/GameListModel.h"
#include "ui/SettingsDialog.h"

namespace UnifiedEmu
{
namespace
{
std::vector<GameInfo> ScanAll(const QStringList& directories, bool recursive,
                              const std::shared_ptr<std::atomic<bool>>& cancelled)
{
  std::vector<GameInfo> games;
  // Evita duplicados si dos carpetas configuradas se solapan.
  std::set<std::filesystem::path> seen;
  const auto isCancelled = [&cancelled] { return cancelled->load(); };

  for (const QString& dir : directories)
  {
    if (isCancelled())
      break;
    for (GameInfo& game : ScanDirectory(ToPath(dir), recursive, isCancelled))
    {
      if (seen.insert(game.path.lexically_normal()).second)
        games.push_back(std::move(game));
    }
  }
  return games;
}
}  // namespace

MainWindow::MainWindow() : m_launcher(m_settings)
{
  setWindowTitle(QStringLiteral("UnifiedEmu"));
  resize(1100, 650);

  createLibraryView();
  createActions();

  m_statusLabel = new QLabel(this);
  statusBar()->addWidget(m_statusLabel, 1);

  connect(&m_scanWatcher, &QFutureWatcher<std::vector<GameInfo>>::finished, this,
          &MainWindow::onScanFinished);

  rescan();
}

MainWindow::~MainWindow()
{
  if (m_scanCancelled)
    m_scanCancelled->store(true);
  m_scanWatcher.waitForFinished();
}

void MainWindow::createLibraryView()
{
  m_model = new GameListModel(this);
  m_proxy = new GameFilterProxyModel(this);
  m_proxy->setSourceModel(m_model);

  m_view = new QTableView(this);
  m_view->setModel(m_proxy);
  m_view->setSortingEnabled(true);
  m_view->sortByColumn(GameListModel::TitleColumn, Qt::AscendingOrder);
  m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_view->setSelectionMode(QAbstractItemView::SingleSelection);
  m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_view->setAlternatingRowColors(true);
  m_view->setWordWrap(false);
  m_view->setContextMenuPolicy(Qt::CustomContextMenu);
  m_view->verticalHeader()->hide();
  m_view->horizontalHeader()->setStretchLastSection(true);
  m_view->horizontalHeader()->setSectionResizeMode(GameListModel::TitleColumn,
                                                   QHeaderView::Interactive);
  m_view->setColumnWidth(GameListModel::TitleColumn, 380);
  m_view->setColumnWidth(GameListModel::PlatformColumn, 110);
  m_view->setColumnWidth(GameListModel::IdColumn, 140);
  m_view->setColumnWidth(GameListModel::FormatColumn, 140);
  m_view->setColumnWidth(GameListModel::SizeColumn, 90);

  connect(m_view, &QTableView::activated, this, &MainWindow::launchRow);
  connect(m_view, &QTableView::customContextMenuRequested, this, &MainWindow::showContextMenu);

  setCentralWidget(m_view);
}

void MainWindow::createActions()
{
  m_playAction = new QAction(style()->standardIcon(QStyle::SP_MediaPlay), tr("&Jugar"), this);
  m_playAction->setShortcut(Qt::Key_Return);
  m_playAction->setShortcutContext(Qt::WidgetShortcut);
  connect(m_playAction, &QAction::triggered, this, &MainWindow::launchSelected);

  m_rescanAction =
      new QAction(style()->standardIcon(QStyle::SP_BrowserReload), tr("&Actualizar lista"), this);
  m_rescanAction->setShortcut(QKeySequence::Refresh);
  connect(m_rescanAction, &QAction::triggered, this, &MainWindow::rescan);

  auto* settingsAction = new QAction(style()->standardIcon(QStyle::SP_FileDialogDetailedView),
                                     tr("&Configuración…"), this);
  settingsAction->setShortcut(QKeySequence::Preferences);
  connect(settingsAction, &QAction::triggered, this, &MainWindow::openSettings);

  auto* quitAction = new QAction(tr("&Salir"), this);
  quitAction->setShortcut(QKeySequence::Quit);
  connect(quitAction, &QAction::triggered, this, &QWidget::close);

  QMenu* fileMenu = menuBar()->addMenu(tr("&Archivo"));
  fileMenu->addAction(m_playAction);
  fileMenu->addAction(m_rescanAction);
  fileMenu->addSeparator();
  fileMenu->addAction(settingsAction);
  fileMenu->addSeparator();
  fileMenu->addAction(quitAction);

  // Abre cada emulador sin juego para configurar mandos, gráficos, audio, claves de Wii U, etc.
  QMenu* emulatorsMenu = menuBar()->addMenu(tr("&Emuladores"));
  const std::pair<Backend, QString> emulators[] = {
      {Backend::Project64, tr("Abrir Project64 (Nintendo 64)")},
      {Backend::Dolphin, tr("Abrir Dolphin (GameCube / Wii)")},
      {Backend::Cemu, tr("Abrir Cemu (Wii U)")},
  };
  for (const auto& [backend, label] : emulators)
  {
    const Backend b = backend;
    emulatorsMenu->addAction(label, this, [this, b] { openEmulator(b); });
  }

  QMenu* helpMenu = menuBar()->addMenu(tr("A&yuda"));
  helpMenu->addAction(tr("Acerca de UnifiedEmu"), this, [this] {
    QMessageBox::about(
        this, tr("Acerca de UnifiedEmu"),
        tr("<b>UnifiedEmu %1</b><br>Frontend unificado para Nintendo 64, GameCube, Wii y Wii U."
           "<br><br>Usa Project64 (GPLv2), Dolphin (GPLv2+) y Cemu (MPL-2.0).<br>"
           "UnifiedEmu se distribuye bajo la licencia GPLv2.")
            .arg(QCoreApplication::applicationVersion()));
  });

  m_platformFilter = new QComboBox(this);
  m_platformFilter->addItem(tr("Todas las consolas"), static_cast<int>(Platform::Unknown));
  for (const Platform p : {Platform::N64, Platform::GameCube, Platform::Wii, Platform::WiiU})
    m_platformFilter->addItem(QString::fromUtf8(PlatformName(p)), static_cast<int>(p));
  connect(m_platformFilter, qOverload<int>(&QComboBox::currentIndexChanged), this, [this] {
    m_proxy->setPlatform(static_cast<Platform>(m_platformFilter->currentData().toInt()));
    updateStatus();
  });

  m_search = new QLineEdit(this);
  m_search->setPlaceholderText(tr("Buscar por título o ID…"));
  m_search->setClearButtonEnabled(true);
  m_search->setMaximumWidth(320);
  connect(m_search, &QLineEdit::textChanged, this, [this](const QString& text) {
    m_proxy->setSearchText(text);
    updateStatus();
  });

  QToolBar* toolbar = addToolBar(tr("Principal"));
  toolbar->setMovable(false);
  toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  toolbar->addAction(m_playAction);
  toolbar->addAction(m_rescanAction);
  toolbar->addAction(settingsAction);
  toolbar->addSeparator();
  toolbar->addWidget(m_platformFilter);
  toolbar->addWidget(m_search);

  m_view->addAction(m_playAction);
}

void MainWindow::showEvent(QShowEvent* event)
{
  QMainWindow::showEvent(event);
  if (!m_firstShow)
    return;
  m_firstShow = false;

  if (m_settings.gameDirectories().isEmpty())
  {
    QMessageBox::information(this, tr("Bienvenido a UnifiedEmu"),
                             tr("Para empezar, añade las carpetas donde guardas tus juegos."));
    openSettings();
  }
}

void MainWindow::rescan()
{
  if (m_scanWatcher.isRunning())
  {
    // Ya hay un escaneo en marcha: se cancela y se repite al terminar.
    m_scanCancelled->store(true);
    m_rescanPending = true;
    return;
  }

  m_scanCancelled = std::make_shared<std::atomic<bool>>(false);
  m_rescanAction->setEnabled(false);
  m_statusLabel->setText(tr("Buscando juegos…"));
  m_scanWatcher.setFuture(QtConcurrent::run(ScanAll, m_settings.gameDirectories(),
                                            m_settings.recursiveScan(), m_scanCancelled));
}

void MainWindow::onScanFinished()
{
  m_rescanAction->setEnabled(true);
  if (m_rescanPending)
  {
    m_rescanPending = false;
    rescan();
    return;
  }
  m_model->setGames(m_scanWatcher.result());
  updateStatus();
}

void MainWindow::updateStatus()
{
  if (!m_model || m_scanWatcher.isRunning())
    return;
  m_statusLabel->setText(tr("%1 juegos mostrados  ·  N64: %2  ·  GameCube: %3  ·  Wii: %4  ·  Wii U: %5")
                             .arg(m_proxy->rowCount())
                             .arg(m_model->countFor(Platform::N64))
                             .arg(m_model->countFor(Platform::GameCube))
                             .arg(m_model->countFor(Platform::Wii))
                             .arg(m_model->countFor(Platform::WiiU)));
}

const GameInfo* MainWindow::selectedGame() const
{
  const QModelIndex current = m_view->currentIndex();
  if (!current.isValid())
    return nullptr;
  return m_model->game(m_proxy->mapToSource(current).row());
}

void MainWindow::launchSelected()
{
  launchRow(m_view->currentIndex());
}

void MainWindow::launchRow(const QModelIndex& proxyIndex)
{
  if (!proxyIndex.isValid())
    return;
  const GameInfo* game = m_model->game(m_proxy->mapToSource(proxyIndex).row());
  if (!game)
    return;
  const QString error = m_launcher.launchGame(*game);
  if (!error.isEmpty())
    QMessageBox::warning(this, tr("No se pudo iniciar el juego"), error);
  else
    statusBar()->showMessage(tr("Iniciando %1…").arg(QString::fromStdString(game->title)), 5000);
}

void MainWindow::openEmulator(Backend backend)
{
  const QString error = m_launcher.openEmulator(backend);
  if (!error.isEmpty())
    QMessageBox::warning(this, tr("No se pudo abrir el emulador"), error);
}

void MainWindow::openContainingFolder()
{
  const GameInfo* game = selectedGame();
  if (!game)
    return;
  const QFileInfo info(ToQString(game->displayPath));
  const QString folder = info.isDir() ? info.absoluteFilePath() : info.absolutePath();
  QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
}

void MainWindow::openSettings()
{
  SettingsDialog dialog(m_settings, this);
  if (dialog.exec() == QDialog::Accepted)
    rescan();
}

void MainWindow::showContextMenu(const QPoint& pos)
{
  const QModelIndex index = m_view->indexAt(pos);
  if (!index.isValid())
    return;
  m_view->setCurrentIndex(index);

  QMenu menu(this);
  menu.addAction(m_playAction);
  menu.addAction(tr("Abrir carpeta del juego"), this, &MainWindow::openContainingFolder);
  menu.exec(m_view->viewport()->mapToGlobal(pos));
}
}  // namespace UnifiedEmu
