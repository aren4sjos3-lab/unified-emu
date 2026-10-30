// SPDX-License-Identifier: GPL-2.0-or-later

#include <QApplication>

#include "ui/MainWindow.h"

int main(int argc, char* argv[])
{
  QApplication app(argc, argv);
  QApplication::setApplicationName(QStringLiteral("UnifiedEmu"));
  QApplication::setApplicationVersion(QStringLiteral(UNIFIEDEMU_VERSION));

  UnifiedEmu::MainWindow window;
  window.show();
  return QApplication::exec();
}
