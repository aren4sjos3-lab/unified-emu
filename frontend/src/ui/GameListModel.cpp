// SPDX-License-Identifier: GPL-2.0-or-later

#include "ui/GameListModel.h"

#include <algorithm>

#include <QDir>
#include <QLocale>

#include "core/Paths.h"

namespace UnifiedEmu
{
GameListModel::GameListModel(QObject* parent) : QAbstractTableModel(parent)
{
}

int GameListModel::rowCount(const QModelIndex& parent) const
{
  return parent.isValid() ? 0 : static_cast<int>(m_games.size());
}

int GameListModel::columnCount(const QModelIndex& parent) const
{
  return parent.isValid() ? 0 : ColumnCount;
}

QVariant GameListModel::data(const QModelIndex& index, int role) const
{
  const GameInfo* info = game(index.row());
  if (!info)
    return {};

  if (role == PlatformRole)
    return static_cast<int>(info->platform);

  if (role == Qt::ToolTipRole)
    return QDir::toNativeSeparators(ToQString(info->displayPath));

  if (role != Qt::DisplayRole && role != SortRole)
    return {};

  switch (index.column())
  {
  case TitleColumn:
    return QString::fromStdString(info->title);
  case PlatformColumn:
    return QString::fromUtf8(PlatformName(info->platform));
  case IdColumn:
    return QString::fromStdString(info->gameId);
  case FormatColumn:
    return QString::fromStdString(info->format);
  case SizeColumn:
    if (role == SortRole)
      return static_cast<qulonglong>(info->size);
    return info->size ? QLocale().formattedDataSize(static_cast<qint64>(info->size)) :
                        QStringLiteral("—");
  case PathColumn:
    return QDir::toNativeSeparators(ToQString(info->displayPath));
  default:
    return {};
  }
}

QVariant GameListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
    return {};
  switch (section)
  {
  case TitleColumn:
    return tr("Título");
  case PlatformColumn:
    return tr("Consola");
  case IdColumn:
    return tr("ID");
  case FormatColumn:
    return tr("Formato");
  case SizeColumn:
    return tr("Tamaño");
  case PathColumn:
    return tr("Ruta");
  default:
    return {};
  }
}

void GameListModel::setGames(std::vector<GameInfo> games)
{
  beginResetModel();
  m_games = std::move(games);
  endResetModel();
}

const GameInfo* GameListModel::game(int row) const
{
  if (row < 0 || row >= static_cast<int>(m_games.size()))
    return nullptr;
  return &m_games[static_cast<std::size_t>(row)];
}

int GameListModel::countFor(Platform platform) const
{
  return static_cast<int>(std::count_if(m_games.begin(), m_games.end(), [platform](const GameInfo& g) {
    return g.platform == platform;
  }));
}

GameFilterProxyModel::GameFilterProxyModel(QObject* parent) : QSortFilterProxyModel(parent)
{
  setSortRole(GameListModel::SortRole);
  setSortCaseSensitivity(Qt::CaseInsensitive);
}

void GameFilterProxyModel::setPlatform(Platform platform)
{
  m_platform = platform;
  invalidateFilter();
}

void GameFilterProxyModel::setSearchText(const QString& text)
{
  m_searchText = text.trimmed();
  invalidateFilter();
}

bool GameFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
  const QAbstractItemModel* model = sourceModel();
  const QModelIndex titleIndex = model->index(sourceRow, GameListModel::TitleColumn, sourceParent);

  if (m_platform != Platform::Unknown &&
      model->data(titleIndex, GameListModel::PlatformRole).toInt() != static_cast<int>(m_platform))
    return false;

  if (m_searchText.isEmpty())
    return true;

  const QModelIndex idIndex = model->index(sourceRow, GameListModel::IdColumn, sourceParent);
  return model->data(titleIndex).toString().contains(m_searchText, Qt::CaseInsensitive) ||
         model->data(idIndex).toString().contains(m_searchText, Qt::CaseInsensitive);
}
}  // namespace UnifiedEmu
