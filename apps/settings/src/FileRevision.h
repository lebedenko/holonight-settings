#pragma once

#include <QByteArray>
#include <QString>

struct FileRevision {
  bool exists{false};
  QByteArray sha256;
  bool operator==(const FileRevision&) const = default;
};

[[nodiscard]] FileRevision readFileRevision(const QString& path);
