// SPDX-License-Identifier: GPL-2.0-or-later

#include "ui/SettingsDialog.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTabWidget>
#include <QToolButton>
#include <QVBoxLayout>

namespace UnifiedEmu
{
namespace
{
constexpr Backend kBackends[] = {Backend::Project64, Backend::Dolphin, Backend::Cemu};

QString BackendDescription(Backend backend)
{
  switch (backend)
  {
  case Backend::Project64:
    return QCoreApplication::translate("SettingsDialog", "Nintendo 64");
  case Backend::Dolphin:
    return QCoreApplication::translate("SettingsDialog", "GameCube y Wii");
  case Backend::Cemu:
    return QCoreApplication::translate("SettingsDialog", "Wii U");
  case Backend::None:
    break;
  }
  return {};
}
}  // namespace

SettingsDialog::SettingsDialog(AppSettings& settings, QWidget* parent)
    : QDialog(parent), m_settings(settings)
{
  setWindowTitle(tr("Configuración"));
  resize(640, 420);

  auto* tabs = new QTabWidget(this);
  tabs->addTab(createLibraryTab(), tr("Biblioteca"));
  tabs->addTab(createEmulatorsTab(), tr("Emuladores"));

  auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);

  auto* layout = new QVBoxLayout(this);
  layout->addWidget(tabs);
  layout->addWidget(buttons);
}

QWidget* SettingsDialog::createLibraryTab()
{
  auto* page = new QWidget(this);

  m_directories = new QListWidget(page);
  m_directories->setSelectionMode(QAbstractItemView::ExtendedSelection);
  for (const QString& dir : m_settings.gameDirectories())
    m_directories->addItem(QDir::toNativeSeparators(dir));

  auto* add = new QPushButton(tr("Añadir carpeta…"), page);
  auto* remove = new QPushButton(tr("Quitar"), page);
  connect(add, &QPushButton::clicked, this, &SettingsDialog::addDirectory);
  connect(remove, &QPushButton::clicked, this, &SettingsDialog::removeSelectedDirectories);

  auto* dirButtons = new QHBoxLayout;
  dirButtons->addWidget(add);
  dirButtons->addWidget(remove);
  dirButtons->addStretch();

  m_recursive = new QCheckBox(tr("Buscar también en subcarpetas"), page);
  m_recursive->setChecked(m_settings.recursiveScan());

  auto* layout = new QVBoxLayout(page);
  layout->addWidget(new QLabel(tr("Carpetas donde están tus juegos de N64, GameCube, Wii y Wii U:"),
                               page));
  layout->addWidget(m_directories);
  layout->addLayout(dirButtons);
  layout->addWidget(m_recursive);
  return page;
}

QWidget* SettingsDialog::createEmulatorsTab()
{
  auto* page = new QWidget(this);
  auto* form = new QFormLayout;

  for (const Backend backend : kBackends)
  {
    auto* edit = new QLineEdit(QDir::toNativeSeparators(m_settings.emulatorPath(backend)), page);
    edit->setPlaceholderText(QDir::toNativeSeparators(AppSettings::defaultEmulatorPath(backend)));
    auto* browse = new QToolButton(page);
    browse->setText(QStringLiteral("…"));
    connect(browse, &QToolButton::clicked, this, [this, backend] { browseEmulator(backend); });

    auto* row = new QHBoxLayout;
    row->addWidget(edit);
    row->addWidget(browse);
    form->addRow(QStringLiteral("%1 (%2):").arg(AppSettings::backendName(backend),
                                               BackendDescription(backend)),
                 row);
    m_emulatorPaths[backend] = edit;
  }

  m_fullscreen = new QCheckBox(tr("Iniciar los juegos en pantalla completa"), page);
  m_fullscreen->setChecked(m_settings.fullscreen());

  auto* hint = new QLabel(tr("Las rutas relativas se resuelven desde la carpeta de UnifiedEmu. "
                             "Los mandos, gráficos y audio de cada consola se configuran desde "
                             "el menú «Emuladores» de la ventana principal."),
                          page);
  hint->setWordWrap(true);

  auto* layout = new QVBoxLayout(page);
  layout->addLayout(form);
  layout->addWidget(m_fullscreen);
  layout->addWidget(hint);
  layout->addStretch();
  return page;
}

void SettingsDialog::addDirectory()
{
  const QString dir = QFileDialog::getExistingDirectory(this, tr("Elegir carpeta de juegos"));
  if (dir.isEmpty())
    return;
  const QString native = QDir::toNativeSeparators(dir);
  if (m_directories->findItems(native, Qt::MatchFixedString).isEmpty())
    m_directories->addItem(native);
}

void SettingsDialog::removeSelectedDirectories()
{
  qDeleteAll(m_directories->selectedItems());
}

void SettingsDialog::browseEmulator(Backend backend)
{
  QLineEdit* edit = m_emulatorPaths.at(backend);
  const QString file = QFileDialog::getOpenFileName(
      this, tr("Elegir ejecutable de %1").arg(AppSettings::backendName(backend)),
      QFileInfo(m_settings.resolvedEmulatorPath(backend)).absolutePath(),
      tr("Ejecutables (*.exe);;Todos los archivos (*)"));
  if (!file.isEmpty())
    edit->setText(QDir::toNativeSeparators(file));
}

void SettingsDialog::accept()
{
  QStringList dirs;
  for (int i = 0; i < m_directories->count(); ++i)
    dirs << QDir::fromNativeSeparators(m_directories->item(i)->text());
  m_settings.setGameDirectories(dirs);
  m_settings.setRecursiveScan(m_recursive->isChecked());
  m_settings.setFullscreen(m_fullscreen->isChecked());
  for (const auto& [backend, edit] : m_emulatorPaths)
    m_settings.setEmulatorPath(backend, QDir::fromNativeSeparators(edit->text().trimmed()));
  m_settings.sync();
  QDialog::accept();
}
}  // namespace UnifiedEmu
