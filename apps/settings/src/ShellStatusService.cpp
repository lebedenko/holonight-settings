#include "ShellStatusService.h"

#include <QDir>
#include <QFile>

ShellStatusService::ShellStatusService(QObject* parent) : QObject(parent) { refresh(); }

QString ShellStatusService::statusText() const {
  return shell_running_ ? tr("Shell is running") : tr("Shell is not running");
}

void ShellStatusService::refresh() {
  const bool running = detectShellProcess();
  if (running == shell_running_) {
    return;
  }
  shell_running_ = running;
  emit shellRunningChanged();
}

bool ShellStatusService::detectShellProcess() {
  const QDir proc_dir(QStringLiteral("/proc"));
  const QStringList entries = proc_dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
  for (const QString& entry : entries) {
    bool is_pid = false;
    entry.toInt(&is_pid);
    if (!is_pid) {
      continue;
    }

    QFile comm(proc_dir.filePath(entry + QStringLiteral("/comm")));
    if (!comm.open(QIODevice::ReadOnly | QIODevice::Text)) {
      continue;
    }
    if (QString::fromUtf8(comm.readAll()).trimmed() == QLatin1String("holonight-shell")) {
      return true;
    }
  }
  return false;
}
