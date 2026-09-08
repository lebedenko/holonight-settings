// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Andrii L <lebeden@gmail.com>

#include "SettingsApplication.h"

#include <QDir>
#include <QFile>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSet>
#include <QTest>
#include <QTextStream>

#include <stdexcept>

namespace {
void require(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

QObject* named(QObject* root, const char* name) {
  QList<QObject*> pending{root};
  QSet<QObject*> visited;
  while (!pending.isEmpty()) {
    auto* object = pending.takeLast();
    if (visited.contains(object)) {
      continue;
    }
    visited.insert(object);
    if (object->objectName() == QString::fromLatin1(name)) {
      return object;
    }
    pending.append(object->children());
    if (auto* item = qobject_cast<QQuickItem*>(object)) {
      for (auto* child : item->childItems()) {
        pending.append(child);
      }
    }
  }
  throw std::runtime_error(name);
}

QObject* objectProperty(QObject* object, const char* property) {
  auto* result = object->property(property).value<QObject*>();
  require(result != nullptr, property);
  return result;
}

void invoke(QObject* object, const char* method) { require(QMetaObject::invokeMethod(object, method), method); }

void set(QObject* object, const char* property, const QVariant& value) {
  require(object->setProperty(property, value), property);
}

void origin(QObject* object, const QString& suffix) {
  // Inline instances have the application's creation context. The control's
  // implementation children retain the defining style/composite QML context.
  auto objects = object->findChildren<QObject*>();
  objects.prepend(object);
  for (auto* child : objects) {
    auto* context = qmlContext(child);
    if (context != nullptr && context->baseUrl().toString().endsWith(suffix)) {
      QTextStream(stdout) << "ORIGIN " << object->objectName() << ' ' << context->baseUrl().toString() << '\n';
      return;
    }
  }
  require(false,
          qPrintable(QStringLiteral("No implementation context for ") + object->objectName() + " expected " + suffix));
}

void verify(QQuickWindow* window) {
  const bool holonight = qEnvironmentVariable("QT_QUICK_CONTROLS_STYLE") != QStringLiteral("Fusion");
  const QString style = holonight ? QStringLiteral("/Holonight/") : QStringLiteral("/QtQuick/Controls/Fusion/");
  auto* stack = named(window, "settingsContentStack");
  auto* appearance = objectProperty(window, "appearanceModel");
  auto* shell = objectProperty(window, "shellModel");
  auto* coordinator = objectProperty(window, "saveCoordinator");
  const auto page = [&](const char* key) {
    set(window, "currentPage", QString::fromLatin1(key));
    require(QTest::qWaitFor(
                [&] {
                  auto* current = stack->property("currentItem").value<QObject*>();
                  return current != nullptr &&
                         current->objectName() == QStringLiteral("contentPage-") + QString::fromLatin1(key) &&
                         !stack->property("busy").toBool();
                },
                3000),
            "Page transition did not complete");
    QTextStream(stdout) << "PAGE " << key << '\n';
    return objectProperty(stack, "currentItem");
  };

  page("appearance");
  origin(named(window, "saveButton"), style + "Button.qml");
  origin(named(window, "settingsNavFrame"), QStringLiteral("/Holonight/Controls/HnSurfaceFrame.qml"));
  origin(named(window, "settingsContentHeaderTitle"), QStringLiteral("/Holonight/Controls/HnPanelHeader.qml"));
  auto* mode_switch = named(window, "darkModeSwitch");
  origin(mode_switch, style + "Switch.qml");
  require(mode_switch->property("sizeRole").isValid() == holonight, "Switch size-role compatibility");
  if (holonight) {
    require(mode_switch->property("sizeRole").toInt() == 2, "Switch must retain Large size role");
    require(mode_switch->property("implicitHeight").toReal() > 0, "Switch geometry");
  }
  set(appearance, "themeMode", QStringLiteral("light"));
  require(!mode_switch->property("checked").toBool(), "Switch model binding");
  set(mode_switch, "checked", true);
  invoke(mode_switch, "toggled");
  require(appearance->property("themeMode").toString() == QStringLiteral("dark"), "Switch editor binding");
  set(appearance, "uiFontSize", 17);
  auto* font_slider = named(window, "uiFontSizeSlider");
  origin(font_slider, style + "Slider.qml");
  require(font_slider->property("value").toInt() == 17, "Font slider model binding");
  set(font_slider, "value", 19);
  invoke(font_slider, "moved");
  require(appearance->property("uiFontSize").toInt() == 19, "Font slider editor binding");
  auto* font_combo = named(window, "uiFontCombo");
  require(font_combo->property("count").toInt() > 0, "Font ComboBox model");
  origin(font_combo, QStringLiteral("/Holonight/Controls/HnIconComboBox.qml"));
  auto* swatch = named(window, "themeFamilyCard");
  require(swatch->property("surfaceColor").isValid() && swatch->property("accentColor").isValid(),
          "Custom swatch painting tokens");
  // Keep popup overflow deterministic even on CI systems with very few fonts.
  QVariantList font_rows;
  for (int index = 0; index < 40; ++index) {
    font_rows.append(QVariantMap{{QStringLiteral("display"), QStringLiteral("Test Font %1").arg(index)}});
  }
  set(font_combo, "model", font_rows);
  set(font_combo, "currentIndex", 0);
  auto* popup = objectProperty(font_combo, "popup");
  invoke(popup, "open");
  require(QTest::qWaitFor([&] { return popup->property("opened").toBool(); }), "Font popup did not open");
  auto* list = objectProperty(popup, "contentItem");
  require(list->property("contentHeight").toReal() > list->property("height").toReal(), "Font popup overflow");
  set(list, "contentY", 30.0);
  require(list->property("contentY").toReal() > 0, "Font popup scrolling");
  set(font_combo, "currentIndex", 12);
  require(QMetaObject::invokeMethod(font_combo, "activated", Q_ARG(int, 12)), "Font selection signal");
  require(appearance->property("uiFont").toString() == QStringLiteral("Test Font 12"), "Font selection binding");
  invoke(popup, "close");

  page("bar");
  auto* workspace_slider = named(window, "workspaceCountSlider");
  origin(workspace_slider, style + "Slider.qml");
  set(shell, "workspaceCount", 8);
  require(workspace_slider->property("value").toInt() == 8, "Workspace slider model binding");
  set(workspace_slider, "value", 7);
  invoke(workspace_slider, "moved");
  require(shell->property("workspaceCount").toInt() == 7, "Workspace editor binding");

  auto* weather = page("weather");
  auto* city = named(window, "weatherCityTextField");
  origin(city, style + "TextField.qml");
  set(shell, "weatherCity", QStringLiteral("Test City"));
  require(city->property("text").toString() == QStringLiteral("Test City"), "City model binding");
  set(city, "text", QStringLiteral("Disposable City"));
  require(shell->property("weatherCity").toString() == QStringLiteral("Disposable City"), "City editor binding");
  origin(named(window, "weatherProviderComboBox"), style + "ComboBox.qml");
  require(weather->property("contentHeight").toReal() > weather->property("height").toReal(), "Weather overflow");
  set(weather, "contentY", 100.0);
  require(weather->property("contentY").toReal() == 100.0, "Weather scrolling");

  page("integrations");
  origin(named(window, "refreshIntegrationsButton"), style + "Button.qml");
  page("audio");
  require(!objectProperty(window, "audioController")->property("available").toBool(),
          "Audio endpoint must be unavailable");
  page("about");

  // Force a revision conflict before validation or adapter invocation. Only the
  // disposable appearance file is touched; no save reaches the native adapter.
  QFile appearance_file(qEnvironmentVariable("HOLONIGHT_APPEARANCE_FILE"));
  require(appearance_file.open(QIODevice::WriteOnly), "Cannot create disposable external revision");
  appearance_file.write("# external test revision\n");
  appearance_file.close();
  invoke(coordinator, "save");
  require(coordinator->property("conflictDomain").toString() == QStringLiteral("Appearance"),
          "Missing appearance conflict");
  QObject* dialog = nullptr;
  for (auto* child : window->findChildren<QObject*>()) {
    if (child->property("modal").toBool() && child->property("visible").toBool()) {
      dialog = child;
      break;
    }
  }
  require(dialog != nullptr, "Conflict dialog did not open");
  require(QTest::qWaitFor([&] { return dialog->property("opened").toBool(); }), "Conflict dialog transition");
  origin(dialog, holonight ? QStringLiteral("/QtQuick/Controls/Basic/Dialog.qml")
                           : QStringLiteral("/QtQuick/Controls/Fusion/Dialog.qml"));
  require(objectProperty(dialog, "background")->property("surfaceRole").isValid(),
          "Conflict frame lost Core appearance");
  invoke(coordinator, "cancelConflict");
  invoke(dialog, "close");
  require(coordinator->property("conflictDomain").toString().isEmpty(), "Conflict cancel");
  require(coordinator->property("isDirty").toBool(), "Conflict cancel discarded edits");
  QFile mappings(QStringLiteral("/proc/self/maps"));
  require(mappings.open(QIODevice::ReadOnly), "Cannot inspect plugin mappings");
  QTextStream(stdout) << "MAPS_BEGIN\n" << mappings.readAll() << "MAPS_END\nACCEPTANCE_OK\n";
}
}  // namespace

int main(int argc, char* argv[]) {
  SettingsApplication app(argc, argv);
  try {
    require(app.shouldRun(), "Settings did not become primary");
    QQuickWindow* window = nullptr;
    require(QTest::qWaitFor([&] {
              for (auto* candidate : QGuiApplication::allWindows()) {
                if (candidate->title() == QStringLiteral("HoloNight Settings")) {
                  window = qobject_cast<QQuickWindow*>(candidate);
                  return window != nullptr;
                }
              }
              return false;
            }),
            "Settings window missing");
    verify(window);
  } catch (const std::exception& error) {
    QTextStream(stderr) << "ACCEPTANCE_FAILED: " << error.what() << '\n';
    return 1;
  }
  return 0;
}
