#include "FileRevision.h"

#include <QCryptographicHash>
#include <QFile>

FileRevision readFileRevision(const QString& path) {
  QFile file(path);
  if (!file.exists() || !file.open(QIODevice::ReadOnly)) return {};
  return {.exists = true, .sha256 = QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha256)};
}
