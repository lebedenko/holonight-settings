// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 HoloNight contributors
#include "PortalFolderChooser.h"
#include "SettingsApplication.h"
#include "WallpaperController.h"
#include "WallpaperModel.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QImage>
#include <QMetaEnum>
#include <QProcess>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTextStream>
#include <QTimer>

#include <stdexcept>

namespace {
void require(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}
QQuickWindow* find(const QString& title) {
  for (auto* window : QGuiApplication::topLevelWindows()) {
    if (window->title() == title) {
      return qobject_cast<QQuickWindow*>(window);
    }
  }
  return nullptr;
}
void activate(const QString& action) {
  auto message = QDBusMessage::createMethodCall("org.holonight.Settings", "/org/holonight/Settings",
                                                "org.freedesktop.Application", "ActivateAction");
  message << action << QVariantList{} << QVariantMap{};
  const auto reply = QDBusConnection::sessionBus().call(message, QDBus::BlockWithGui, 3000);
  require(reply.type() == QDBusMessage::ReplyMessage, "Activation failed");
}
}  // namespace
int main(int argc, char** argv) {
  SettingsApplication app(argc, argv);
  if (!app.shouldRun()) {
    return app.startupExitCode();
  }
  QTimer::singleShot(300, &app, [&app] {
    try {
      const bool cold = SettingsApplication::arguments().contains("--wallpaper");
      auto* wallpaper = find("HoloNight Wallpaper");
      require(cold ? wallpaper != nullptr && find("HoloNight Settings") == nullptr : wallpaper == nullptr,
              "Startup created the wrong windows");
      QProcess secondary;
      QSignalSpy completed(&secondary, &QProcess::finished);
      secondary.start(QCoreApplication::applicationDirPath() + "/holonight-settings", {QStringLiteral("--wallpaper")});
      require(completed.wait(5000), "Secondary wallpaper startup timed out");
      require(secondary.exitStatus() == QProcess::NormalExit && secondary.exitCode() == 0, "Secondary startup failed");
      wallpaper = find("HoloNight Wallpaper");
      require(wallpaper != nullptr, "Missing wallpaper window");
      require(wallpaper->minimumWidth() == 960 && wallpaper->minimumHeight() == 640, "Incorrect minimum size");
      auto* controller = wallpaper->property("controller").value<WallpaperController*>();
      auto* browser = wallpaper->property("browser").value<WallpaperModel*>();
      require(controller && browser, "Missing picker models");
      QTemporaryDir images;
      QImage image(640, 360, QImage::Format_RGB32);
      image.fill(Qt::darkCyan);
      const auto path = images.filePath("wallpaper ?#.png");
      require(image.save(path), "Cannot save fixture");
      QSignalSpy scanned(browser, &WallpaperModel::stateChanged);
      auto* chooser = wallpaper->findChild<PortalFolderChooser*>(QStringLiteral("wallpaperFolderChooser"));
      require(chooser != nullptr, "Missing portal folder chooser");
      chooser->folderSelected(QUrl::fromLocalFile(images.path()));
      require(!browser->loading() || scanned.wait(5000), "Wallpaper scan timed out");
      auto* filter = wallpaper->property("wallpapers").value<WallpaperFilter*>();
      require(filter != nullptr, "Missing wallpaper filter");
      auto* navigation = wallpaper->findChild<QObject*>(QStringLiteral("wallpaperNavigation"));
      require(navigation != nullptr, "Missing shared navigation panel");
      require(QMetaObject::invokeMethod(navigation, "pageRequested", Q_ARG(QString, QStringLiteral("favorites"))),
              "Cannot select favorites through the navigation menu");
      require(filter->collection() == QStringLiteral("favorites"), "Navigation did not select favorites");
      require(QMetaObject::invokeMethod(navigation, "pageRequested", Q_ARG(QString, images.path())),
              "Cannot select folder through the navigation menu");
      require(filter->collection() == images.path(), "Navigation did not select the folder");
      auto* preview = wallpaper->findChild<QObject*>(QStringLiteral("wallpaperPreviewImage"));
      require(preview != nullptr, "Missing wallpaper preview");
      const auto fill_mode = preview->metaObject()->enumerator(preview->metaObject()->indexOfEnumerator("FillMode"));
      require(preview->property("fillMode").toInt() == fill_mode.keyToValue("PreserveAspectFit"),
              "Preview crops the image");
      controller->select(path);
      activate("wallpaper");
      require(controller->selectedPath() == path, "Repeated activation reset the pending image");
      activate("appearance");
      auto* settings = find("HoloNight Settings");
      require(settings && settings->isVisible() && wallpaper->isVisible(), "Both windows must remain visible");
      QTest::qWait(200);
      const auto capture = qEnvironmentVariable("WALLPAPER_CAPTURE_DIR");
      if (!capture.isEmpty()) {
        require(wallpaper->grabWindow().save(capture + "/wallpaper-default.png"), "Cannot capture default window");
        wallpaper->resize(960, 640);
        QTest::qWait(100);
        require(wallpaper->grabWindow().save(capture + "/wallpaper-minimum.png"), "Cannot capture minimum window");
      }
      wallpaper->close();
      require(settings->isVisible(), "Closing picker closed Settings");
      require(!controller->canApply(), "Closing picker kept unapplied changes");
      activate("wallpaper");
      require(wallpaper->isVisible(), "Picker did not reopen");
      settings->close();
      require(wallpaper->isVisible(), "Closing Settings closed picker");
      activate("audio");
      require(settings->isVisible(), "Settings did not reopen");
      QSignalSpy last(&app, &QGuiApplication::lastWindowClosed);
      settings->close();
      wallpaper->close();
      require(last.count() == 1, "Last window close did not request process exit");
      QTextStream(stdout) << "WALLPAPER_ACCEPTANCE_OK\n";
      SettingsApplication::exit(0);
    } catch (const std::exception& exception) {
      QTextStream(stderr) << exception.what() << '\n';
      SettingsApplication::exit(1);
    }
  });
  return QGuiApplication::exec();
}
