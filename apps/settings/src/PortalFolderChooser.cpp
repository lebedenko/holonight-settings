// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 HoloNight contributors
#include "PortalFolderChooser.h"

#include <QCoreApplication>
#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QFileInfo>
#include <QPointer>
#include <QUuid>

namespace {
constexpr auto kPortalService = "org.freedesktop.portal.Desktop";
constexpr auto kPortalPath = "/org/freedesktop/portal/desktop";
constexpr auto kChooserInterface = "org.freedesktop.portal.FileChooser";
constexpr auto kRequestInterface = "org.freedesktop.portal.Request";
void closeRequest(const QDBusConnection& connection, const QString& path) {
  const auto call = QDBusMessage::createMethodCall(kPortalService, path, kRequestInterface, "Close");
  static_cast<void>(connection.asyncCall(call, 500));
}
}  // namespace
PortalFolderChooser::PortalFolderChooser(QObject* parent)
    : PortalFolderChooser(QDBusConnection::sessionBus(), parent) {}
PortalFolderChooser::PortalFolderChooser(QDBusConnection connection, QObject* parent)
    : QObject(parent), connection_(std::move(connection)) {}
PortalFolderChooser::~PortalFolderChooser() { cancel(); }
void PortalFolderChooser::open() {
  if (busy()) {
    return;
  }
  error_.clear();
  if (!connection_.isConnected()) {
    fail(tr("The desktop portal is unavailable: the session bus is not connected."));
    return;
  }
  const auto token =
      QStringLiteral("wallpaper_") + QUuid::createUuid().toString(QUuid::WithoutBraces).replace('-', '_');
  const auto sender = connection_.baseService().mid(1).replace('.', '_');
  const auto path = QStringLiteral("/org/freedesktop/portal/desktop/request/%1/%2").arg(sender, token);
  if (!subscribe(path)) {
    fail(tr("Could not listen for the desktop portal folder selection."));
    return;
  }
  const auto generation = ++generation_;
  emit stateChanged();
  const QVariantMap options{
      {"handle_token", token},
      {"directory", true},
      {"multiple", false},
      {"modal", true},
      {"accept_label", tr("Add Folder")},
  };
  auto call = QDBusMessage::createMethodCall(kPortalService, kPortalPath, kChooserInterface, "OpenFile");
  // An empty parent identifier lets the portal work without native window-export dependencies.
  call << QString{} << tr("Add wallpaper folder") << options;
  auto* watcher = new QDBusPendingCallWatcher(connection_.asyncCall(call, 5000), QCoreApplication::instance());
  connect(watcher, &QDBusPendingCallWatcher::finished, watcher,
          [guard = QPointer<PortalFolderChooser>(this), connection = connection_,
           generation](QDBusPendingCallWatcher* completed) {
            const QDBusPendingReply<QDBusObjectPath> reply(*completed);
            completed->deleteLater();
            if (guard.isNull() || guard->generation_ != generation || !guard->busy()) {
              if (!reply.isError()) {
                closeRequest(connection, reply.value().path());
              }
              return;
            }
            if (reply.isError()) {
              guard->fail(tr("Could not open the desktop portal folder chooser: %1").arg(reply.error().message()));
            } else if (reply.value().path() != guard->request_path_) {
              guard->finish();
              if (!guard->subscribe(reply.value().path())) {
                closeRequest(connection, reply.value().path());
                guard->fail(tr("Could not listen for the desktop portal folder selection."));
              }
            }
          });
}
bool PortalFolderChooser::subscribe(const QString& path) {
  if (!connection_.connect(kPortalService, path, kRequestInterface, "Response", this,
                           SLOT(response(uint, QVariantMap, QDBusMessage)))) {
    return false;
  }
  request_path_ = path;
  return true;
}
void PortalFolderChooser::finish() {
  if (!request_path_.isEmpty()) {
    connection_.disconnect(kPortalService, request_path_, kRequestInterface, "Response", this,
                           SLOT(response(uint, QVariantMap, QDBusMessage)));
  }
  request_path_.clear();
}
void PortalFolderChooser::fail(const QString& message) {
  finish();
  error_ = message;
  emit stateChanged();
}
void PortalFolderChooser::cancel() {
  if (busy()) {
    closeRequest(connection_, request_path_);
    ++generation_;
    finish();
    emit stateChanged();
  }
}
void PortalFolderChooser::response(uint code, const QVariantMap& results, const QDBusMessage& message) {
  if (!busy() || message.path() != request_path_) {
    return;
  }
  finish();
  if (code == 0) {
    const auto uris = results.value("uris").toStringList();
    const QUrl folder(uris.value(0), QUrl::StrictMode);
    if (uris.size() != 1 || !folder.isLocalFile() || !QFileInfo(folder.toLocalFile()).isDir()) {
      error_ = tr("The desktop portal did not return a local folder.");
    } else {
      emit folderSelected(folder);
    }
  }
  emit stateChanged();
}
