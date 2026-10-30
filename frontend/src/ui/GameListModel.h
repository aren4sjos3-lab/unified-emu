// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <vector>

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>

#include "core/GameDetector.h"

namespace UnifiedEmu
{
class GameListModel final : public QAbstractTableModel
{
  Q_OBJECT

public:
  enum Column
  {
    TitleColumn,
    PlatformColumn,
    IdColumn,
    FormatColumn,
    SizeColumn,
    PathColumn,
    ColumnCount,
  };

  enum Role
  {
    SortRole = Qt::UserRole + 1,
    PlatformRole,
  };

  explicit GameListModel(QObject* parent = nullptr);

  int rowCount(const QModelIndex& parent = {}) const override;
  int columnCount(const QModelIndex& parent = {}) const override;
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;

  void setGames(std::vector<GameInfo> games);
  const GameInfo* game(int row) const;
  int countFor(Platform platform) const;

private:
  std::vector<GameInfo> m_games;
};

// Filtra por plataforma y por texto (título o ID).
class GameFilterProxyModel final : public QSortFilterProxyModel
{
  Q_OBJECT

public:
  explicit GameFilterProxyModel(QObject* parent = nullptr);

  void setPlatform(Platform platform);
  void setSearchText(const QString& text);

protected:
  bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:
  Platform m_platform = Platform::Unknown;  // Unknown = todas
  QString m_searchText;
};
}  // namespace UnifiedEmu
