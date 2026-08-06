#include "ConfigFileService.h"
#include "FontListModel.h"
#include "SettingsEditModel.h"
#include "ShellStatusService.h"
#include "ThemeConfigFile.h"
#include "theme_swatch_tokens.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <algorithm>
#include <appearanceconfig.h>
#include <cmath>
#include <gtest/gtest.h>
#include <holonight/palette.h>
#include <holonight/theme_catalog.h>
#include <holonight_config/config_parsers.h>
#include <memory>

namespace {

[[maybe_unused]] const int kSettingsEditModelRegistration = qmlRegisterUncreatableType<SettingsEditModel>(
    "HolonightSettings", 1, 0, "SettingsEditModel", "SettingsEditModel is supplied by the application");
[[maybe_unused]] const int kConfigFileServiceRegistration = qmlRegisterUncreatableType<ConfigFileService>(
    "HolonightSettings", 1, 0, "ConfigFileService", "ConfigFileService is supplied by the application");
[[maybe_unused]] const int kShellStatusServiceRegistration = qmlRegisterUncreatableType<ShellStatusService>(
    "HolonightSettings", 1, 0, "ShellStatusService", "ShellStatusService is supplied by the application");

class ScopedXdgConfigHome {
 public:
  explicit ScopedXdgConfigHome(const QString& path) : previous_(qgetenv("XDG_CONFIG_HOME")) {
    qputenv("XDG_CONFIG_HOME", path.toUtf8());
  }

  ~ScopedXdgConfigHome() {
    if (previous_.isNull()) {
      qunsetenv("XDG_CONFIG_HOME");
    } else {
      qputenv("XDG_CONFIG_HOME", previous_);
    }
  }

  ScopedXdgConfigHome(const ScopedXdgConfigHome&) = delete;
  ScopedXdgConfigHome& operator=(const ScopedXdgConfigHome&) = delete;
  ScopedXdgConfigHome(ScopedXdgConfigHome&&) = delete;
  ScopedXdgConfigHome& operator=(ScopedXdgConfigHome&&) = delete;

 private:
  QByteArray previous_;
};

class ScopedAppearanceFile {
 public:
  explicit ScopedAppearanceFile(const QString& path) : previous_(qgetenv("HOLONIGHT_APPEARANCE_FILE")) {
    qputenv("HOLONIGHT_APPEARANCE_FILE", path.toUtf8());
  }

  ~ScopedAppearanceFile() {
    if (previous_.isNull()) {
      qunsetenv("HOLONIGHT_APPEARANCE_FILE");
    } else {
      qputenv("HOLONIGHT_APPEARANCE_FILE", previous_);
    }
  }

  ScopedAppearanceFile(const ScopedAppearanceFile&) = delete;
  ScopedAppearanceFile& operator=(const ScopedAppearanceFile&) = delete;
  ScopedAppearanceFile(ScopedAppearanceFile&&) = delete;
  ScopedAppearanceFile& operator=(ScopedAppearanceFile&&) = delete;

 private:
  QByteArray previous_;
};

ParsedConfig readConfig(const QString& path) {
  MissingDefaults missing;
  return parseConfigTable(toml::parse_file(path.toStdString()), missing);
}

QQuickItem* findVisualChild(QQuickItem* parent, const QString& object_name) {
  for (QQuickItem* child : parent->childItems()) {
    if (child->objectName() == object_name) {
      return child;
    }
    if (QQuickItem* match = findVisualChild(child, object_name)) {
      return match;
    }
  }
  return nullptr;
}

QList<QQuickItem*> findVisualChildren(QQuickItem* parent, const QString& object_name) {
  QList<QQuickItem*> matches;
  for (QQuickItem* child : parent->childItems()) {
    if (child->objectName() == object_name) {
      matches.append(child);
    }
    matches.append(findVisualChildren(child, object_name));
  }
  return matches;
}

QList<QQuickItem*> findAllVisualChildren(QQuickItem* parent) {
  QList<QQuickItem*> matches;
  for (QQuickItem* child : parent->childItems()) {
    matches.append(child);
    matches.append(findAllVisualChildren(child));
  }
  return matches;
}

bool isVisualDescendantOf(QQuickItem* item, QQuickItem* ancestor) {
  for (QQuickItem* current = item; current != nullptr; current = current->parentItem()) {
    if (current == ancestor) {
      return true;
    }
  }
  return false;
}

}  // namespace

TEST(ThemeSwatchTokensTest, ReturnsExactlyTheSemanticSwatchColors) {
  const QVariantMap swatch_tokens = ThemeSwatchTokens::getTokensForScheme(QStringLiteral("holonight-storm"));
  const Holonight::ColorTokens expected =
      Holonight::tokensForScheme(Holonight::schemeKindForSchemeId(QStringLiteral("holonight-storm")));

  EXPECT_EQ(swatch_tokens.size(), 10);
  EXPECT_EQ(swatch_tokens.value(QStringLiteral("surface")).value<QColor>(), expected.surface);
  EXPECT_EQ(swatch_tokens.value(QStringLiteral("surfaceHover")).value<QColor>(), expected.surfaceHover);
  EXPECT_EQ(swatch_tokens.value(QStringLiteral("surfacePressed")).value<QColor>(), expected.surfaceElevated);
  EXPECT_EQ(swatch_tokens.value(QStringLiteral("borderPassive")).value<QColor>(), expected.borderPassive);
  EXPECT_EQ(swatch_tokens.value(QStringLiteral("primary")).value<QColor>(), expected.primary);
  EXPECT_EQ(swatch_tokens.value(QStringLiteral("onPrimary")).value<QColor>(), expected.onPrimary);
  EXPECT_EQ(swatch_tokens.value(QStringLiteral("selectionIndicator")).value<QColor>(), expected.selectionIndicator);
  EXPECT_EQ(swatch_tokens.value(QStringLiteral("disabledOverlay")).value<QColor>(), expected.disabledOverlay);
  EXPECT_EQ(swatch_tokens.value(QStringLiteral("accent")).value<QColor>(), expected.accentBlue);
  EXPECT_EQ(swatch_tokens.value(QStringLiteral("secondaryAccent")).value<QColor>(), expected.accentViolet);
}

TEST(SettingsEditModelTest, MarkSavedClearsDirtyStateAfterApply) {
  SettingsEditModel model;
  ParsedConfig config;
  model.setFromParsedConfig(config);

  QSignalSpy dirty_spy(&model, &SettingsEditModel::isDirtyChanged);
  model.setThemeMode(QStringLiteral("light"));

  ASSERT_TRUE(model.isDirty());
  EXPECT_EQ(dirty_spy.count(), 1);

  model.markSaved(model.toParsedConfig(), model.toThemeConfig(), model.toShapeAppearanceConfig());

  EXPECT_FALSE(model.isDirty());
  EXPECT_EQ(dirty_spy.count(), 2);
}

TEST(SettingsEditModelTest, LoadingConfigEmitsOnlyChangedPropertiesAndRecomputesDirtyState) {
  SettingsEditModel model;
  ParsedConfig config;
  model.setFromParsedConfig(config);

  QSignalSpy theme_spy(&model, &SettingsEditModel::themeSchemeChanged);
  QSignalSpy font_spy(&model, &SettingsEditModel::uiFontChanged);
  QSignalSpy workspace_spy(&model, &SettingsEditModel::workspaceCountChanged);
  QSignalSpy dirty_spy(&model, &SettingsEditModel::isDirtyChanged);

  model.setFromParsedConfig(config);

  EXPECT_EQ(theme_spy.count(), 0);
  EXPECT_EQ(font_spy.count(), 0);
  EXPECT_EQ(workspace_spy.count(), 0);
  EXPECT_EQ(dirty_spy.count(), 0);

  model.setWorkspaceCount(8);
  ASSERT_TRUE(model.isDirty());
  ASSERT_EQ(workspace_spy.count(), 1);
  ASSERT_EQ(dirty_spy.count(), 1);

  model.setFromParsedConfig(config);

  EXPECT_FALSE(model.isDirty());
  EXPECT_EQ(theme_spy.count(), 0);
  EXPECT_EQ(font_spy.count(), 0);
  EXPECT_EQ(workspace_spy.count(), 2);
  EXPECT_EQ(dirty_spy.count(), 2);
}

TEST(SettingsEditModelTest, InitialAccentUsesThemeCatalogDefault) {
  SettingsEditModel model;

  EXPECT_EQ(model.themeAccent(), ThemeConfigFile::defaultAccent());
  EXPECT_FALSE(model.isDirty());
}

TEST(SettingsEditModelTest, ClampsNumericPropertiesToUiRanges) {
  SettingsEditModel model;

  model.setUiFontSize(-1);
  model.setFixedFontSize(100);
  model.setWorkspaceCount(-1);
  model.setTrayMaxItems(100);

  EXPECT_EQ(model.uiFontSize(), 8);
  EXPECT_EQ(model.fixedFontSize(), 18);
  EXPECT_EQ(model.workspaceCount(), 3);
  EXPECT_EQ(model.trayMaxItems(), 5);

  model.setUiFontSize(16);
  model.setFixedFontSize(18);
  model.setWorkspaceCount(7);
  model.setTrayMaxItems(4);

  const Holonight::ThemeConfig theme_config = model.toThemeConfig();
  EXPECT_EQ(theme_config.base_font_size, 16);
  EXPECT_EQ(theme_config.fixed_font_size, 18);
  const ParsedConfig config = model.toParsedConfig();
  EXPECT_EQ(config.bar_workspaces.count, 7);
  EXPECT_EQ(config.bar_system_tray.max_items, 4);
}

TEST(SettingsEditModelTest, ThemeFontSizesNormalizeWhenLoaded) {
  SettingsEditModel model;
  Holonight::ThemeConfig config = Holonight::ThemeConfig::defaults();
  config.base_font_size = 24;
  config.fixed_font_size = 19;

  model.setThemeConfigSnapshot(config);

  EXPECT_EQ(model.uiFontSize(), 18);
  EXPECT_EQ(model.fixedFontSize(), 18);
  EXPECT_FALSE(model.isDirty());
  EXPECT_EQ(model.toThemeConfig().base_font_size, 18);
  EXPECT_EQ(model.toThemeConfig().fixed_font_size, 18);
}

TEST(SettingsEditModelTest, ShapeValuesParticipateInDirtyStateAndNormalizeRanges) {
  SettingsEditModel model;
  model.setShapeAppearanceSnapshot(Holonight::AppearanceConfig::defaults());

  model.setShapeCornerStyle(QStringLiteral("chamfered"));
  model.setShapeScale(10.0);
  model.setBaseRadius(200.0);
  model.setBaseChamferEnabled(true);

  EXPECT_EQ(model.shapeCornerStyle(), QStringLiteral("chamfered"));
  EXPECT_EQ(model.shapeScale(), 4.0);
  EXPECT_TRUE(model.baseRadiusEnabled());
  EXPECT_EQ(model.baseRadius(), 128.0);
  EXPECT_TRUE(model.baseChamferEnabled());
  EXPECT_EQ(model.baseChamfer(), 0.0);
  EXPECT_TRUE(model.isDirty());

  model.setShapeAppearanceSnapshot(Holonight::AppearanceConfig::defaults());

  EXPECT_EQ(model.shapeCornerStyle(), QStringLiteral("inherit"));
  EXPECT_EQ(model.shapeScale(), 1.0);
  EXPECT_FALSE(model.baseRadiusEnabled());
  EXPECT_FALSE(model.baseChamferEnabled());
  EXPECT_FALSE(model.isDirty());
}

TEST(ConfigFileServiceTest, SaveWritesConfigAndClearsDirtyState) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());
  const ScopedXdgConfigHome xdg(temp_dir.path());
  const ScopedAppearanceFile appearance_file(temp_dir.filePath(QStringLiteral("appearance.json")));

  QDir().mkpath(QFileInfo(ConfigFileService::themeConfigPath()).absolutePath());
  QSettings initial_theme{ConfigFileService::themeConfigPath(), QSettings::IniFormat};
  initial_theme.setValue(QStringLiteral("fonts/header"), QStringLiteral("Header Face"));
  initial_theme.setValue(QStringLiteral("effects/custom"), 42);
  initial_theme.setValue(QStringLiteral("future/key"), QStringLiteral("preserve-me"));
  initial_theme.sync();

  SettingsEditModel model;
  ConfigFileService service(&model);
  ASSERT_TRUE(service.load());

  model.setWorkspaceCount(8);
  model.setTrayMaxItems(4);
  model.setThemeScheme(QStringLiteral("holonight-day"));
  model.setThemeAccent(QStringLiteral("yellow"));
  model.setUiFont(QStringLiteral("Settings Sans"));
  model.setUiFontSize(14);
  model.setFixedFont(QStringLiteral("Settings Mono"));
  model.setFixedFontSize(13);
  model.setShapeCornerStyle(QStringLiteral("rounded"));
  model.setShapeScale(1.25);
  model.setBaseRadius(9.0);
  ASSERT_TRUE(model.isDirty());

  ASSERT_TRUE(service.save());

  EXPECT_FALSE(model.isDirty());

  const ParsedConfig saved = readConfig(ConfigFileService::configPath());
  EXPECT_EQ(saved.bar_workspaces.count, 8);
  EXPECT_EQ(saved.bar_system_tray.max_items, 4);

  QSettings theme_settings{ConfigFileService::themeConfigPath(), QSettings::IniFormat};
  EXPECT_EQ(theme_settings.value(QStringLiteral("appearance/scheme")).toString(), QStringLiteral("holonight-day"));
  EXPECT_EQ(theme_settings.value(QStringLiteral("appearance/accent")).toString(), QStringLiteral("yellow"));
  EXPECT_EQ(theme_settings.value(QStringLiteral("appearance/mode")).toString(), QStringLiteral("light"));
  EXPECT_EQ(theme_settings.value(QStringLiteral("fonts/ui")).toString(), QStringLiteral("Settings Sans"));
  EXPECT_EQ(theme_settings.value(QStringLiteral("fonts/baseSize")).toInt(), 14);
  EXPECT_EQ(theme_settings.value(QStringLiteral("fonts/fixed")).toString(), QStringLiteral("Settings Mono"));
  EXPECT_EQ(theme_settings.value(QStringLiteral("fonts/fixedSize")).toInt(), 13);
  EXPECT_EQ(theme_settings.value(QStringLiteral("fonts/header")).toString(), QStringLiteral("Header Face"));
  EXPECT_EQ(theme_settings.value(QStringLiteral("effects/custom")).toInt(), 42);
  EXPECT_EQ(theme_settings.value(QStringLiteral("future/key")).toString(), QStringLiteral("preserve-me"));
  const Holonight::AppearanceConfig shape =
      Holonight::AppearanceConfig::load(ConfigFileService::appearanceConfigPath());
  EXPECT_EQ(shape.corner_style, Holonight::CornerStyle::Rounded);
  EXPECT_EQ(shape.shape_scale, 1.25);
  EXPECT_EQ(shape.base_radius, 9.0);
  EXPECT_TRUE(std::isnan(shape.base_chamfer));
  QFile shell_config{ConfigFileService::configPath()};
  ASSERT_TRUE(shell_config.open(QIODevice::ReadOnly | QIODevice::Text));
  EXPECT_FALSE(QString::fromUtf8(shell_config.readAll()).contains(QLatin1String("[theme]")));
}

TEST(ConfigFileServiceTest, MissingAppearanceFileLoadsDefaultsWithoutCreatingFile) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());
  const ScopedXdgConfigHome xdg(temp_dir.path());
  const QString appearance_path = temp_dir.filePath(QStringLiteral("nested/appearance.json"));
  const ScopedAppearanceFile appearance_file(appearance_path);

  SettingsEditModel model;
  ConfigFileService service(&model);
  ASSERT_TRUE(service.load());

  EXPECT_EQ(model.shapeCornerStyle(), QStringLiteral("inherit"));
  EXPECT_EQ(model.shapeScale(), 1.0);
  EXPECT_FALSE(model.baseRadiusEnabled());
  EXPECT_FALSE(model.baseChamferEnabled());
  EXPECT_FALSE(QFileInfo::exists(appearance_path));
  EXPECT_FALSE(model.isDirty());
}

TEST(ConfigFileServiceTest, LoadReadsSharedAppearanceAndDiscardRestoresIt) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());
  const ScopedXdgConfigHome xdg(temp_dir.path());
  const QString appearance_path = temp_dir.filePath(QStringLiteral("appearance.json"));
  const ScopedAppearanceFile appearance_file(appearance_path);

  Holonight::AppearanceConfig persisted;
  persisted.corner_style = Holonight::CornerStyle::Hybrid;
  persisted.shape_scale = 0.75;
  persisted.base_chamfer = 12.0;
  ASSERT_TRUE(persisted.save(appearance_path));

  SettingsEditModel model;
  ConfigFileService service(&model);
  ASSERT_TRUE(service.load());
  EXPECT_EQ(model.shapeCornerStyle(), QStringLiteral("hybrid"));
  EXPECT_EQ(model.shapeScale(), 0.75);
  EXPECT_TRUE(model.baseChamferEnabled());
  EXPECT_EQ(model.baseChamfer(), 12.0);

  model.setShapeCornerStyle(QStringLiteral("rounded"));
  model.setBaseChamferEnabled(false);
  ASSERT_TRUE(model.isDirty());
  ASSERT_TRUE(service.load());

  EXPECT_EQ(model.shapeCornerStyle(), QStringLiteral("hybrid"));
  EXPECT_TRUE(model.baseChamferEnabled());
  EXPECT_EQ(model.baseChamfer(), 12.0);
  EXPECT_FALSE(model.isDirty());
}

TEST(ConfigFileServiceTest, AppearanceWriteFailureKeepsModelDirtyAndNamesPath) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());
  const ScopedXdgConfigHome xdg(temp_dir.path());
  const QString appearance_path = temp_dir.filePath(QStringLiteral("target-directory"));
  ASSERT_TRUE(QDir().mkpath(appearance_path));
  const ScopedAppearanceFile appearance_file(appearance_path);

  SettingsEditModel model;
  ConfigFileService service(&model);
  ASSERT_TRUE(service.load());
  model.setShapeScale(2.0);
  QSignalSpy error_spy(&service, &ConfigFileService::saveError);

  EXPECT_FALSE(service.save());
  EXPECT_TRUE(model.isDirty());
  ASSERT_EQ(error_spy.count(), 1);
  EXPECT_TRUE(error_spy.at(0).at(0).toString().contains(appearance_path));
}

TEST(ConfigFileServiceTest, ThemeWriteFailureKeepsModelDirtyAndNamesPath) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());
  const ScopedXdgConfigHome xdg(temp_dir.path());
  const ScopedAppearanceFile appearance_file(temp_dir.filePath(QStringLiteral("appearance.json")));
  const QString theme_path = ConfigFileService::themeConfigPath();
  ASSERT_TRUE(QDir().mkpath(theme_path));

  SettingsEditModel model;
  ConfigFileService service(&model);
  ASSERT_TRUE(service.load());
  model.setUiFont(QStringLiteral("Unsaved Sans"));
  QSignalSpy error_spy(&service, &ConfigFileService::saveError);

  EXPECT_FALSE(service.save());
  EXPECT_TRUE(model.isDirty());
  ASSERT_EQ(error_spy.count(), 1);
  EXPECT_TRUE(error_spy.at(0).at(0).toString().contains(theme_path));
}

TEST(SettingsAppearanceQmlTest, UsesFourFramedSectionsWithInlinePaddedRows) {
  static const int registration = qmlRegisterType<FontListModel>("HolonightSettings", 1, 0, "FontListModel");
  Q_UNUSED(registration);
  static const int swatch_tokens_registration = qmlRegisterSingletonType<ThemeSwatchTokens>(
      "HolonightSettings", 1, 0, "ThemeSwatchTokens",
      [](QQmlEngine*, QJSEngine*) -> QObject* { return new ThemeSwatchTokens(); });
  Q_UNUSED(swatch_tokens_registration);

  SettingsEditModel model;
  QQmlEngine engine;
  engine.rootContext()->setContextProperty(QStringLiteral("editModel"), &model);
  QQmlComponent component(&engine,
                          QUrl::fromLocalFile(QStringLiteral(TEST_SOURCE_DIR "/apps/settings/qml/AppearancePage.qml")));
  std::unique_ptr<QObject> page{
      component.createWithInitialProperties({{QStringLiteral("editModel"), QVariant::fromValue(&model)}})};
  ASSERT_NE(page, nullptr) << component.errorString().toStdString();

  auto* root_item = qobject_cast<QQuickItem*>(page.get());
  ASSERT_NE(root_item, nullptr);
  root_item->setSize(QSizeF(1000, 1800));
  QQuickWindow window;
  window.resize(1000, 900);
  root_item->setParentItem(window.contentItem());
  window.show();
  ASSERT_TRUE(QTest::qWaitForWindowExposed(&window));
  QCoreApplication::processEvents();

  struct SectionExpectation {
    QString frame_name;
    QStringList row_names;
  };
  const QList<SectionExpectation> sections{
      {QStringLiteral("themeSectionFrame"),
       {QStringLiteral("colorSchemeRow"), QStringLiteral("accentColorRow"), QStringLiteral("darkModeRow"),
        QStringLiteral("transparencyRow"), QStringLiteral("blurStrengthRow")}},
      {QStringLiteral("typographySectionFrame"),
       {QStringLiteral("uiFontRow"), QStringLiteral("uiFontSizeRow"), QStringLiteral("fixedFontRow"),
        QStringLiteral("fixedFontSizeRow")}},
      {QStringLiteral("globalShapeSectionFrame"), {QStringLiteral("cornerStyleRow"), QStringLiteral("shapeScaleRow")}},
      {QStringLiteral("advancedShapeSectionFrame"),
       {QStringLiteral("baseRadiusRow"), QStringLiteral("baseChamferRow")}},
  };

  const auto verify_geometry = [&]() {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    for (const SectionExpectation& section : sections) {
      auto* frame = findVisualChild(root_item, section.frame_name);
      ASSERT_NE(frame, nullptr);
      EXPECT_EQ(frame->property("surfaceRole").toInt(), 3);
      for (qsizetype index = 0; index < section.row_names.size(); ++index) {
        auto* row = findVisualChild(root_item, section.row_names.at(index));
        ASSERT_NE(row, nullptr);
        EXPECT_TRUE(isVisualDescendantOf(row, frame));
        EXPECT_FALSE(row->property("stacked").toBool());
        EXPECT_EQ(row->property("contentHorizontalPadding").toReal(), 16);
        EXPECT_EQ(row->property("resolvedSizeRole").toInt(), 3);
        EXPECT_EQ(row->property("dividerVisible").toBool(), index + 1 < section.row_names.size());

        auto* control = qobject_cast<QQuickItem*>(row->property("controlItem").value<QObject*>());
        ASSERT_NE(control, nullptr);
        const QPointF control_top_left = control->mapToItem(frame, QPointF{});
        const QPointF control_bottom_right = control->mapToItem(frame, QPointF{control->width(), control->height()});
        EXPECT_GE(control_top_left.x(), 0);
        EXPECT_GE(control_top_left.y(), 0);
        EXPECT_LE(control_bottom_right.x(), frame->width() + 0.01);
        EXPECT_LE(control_bottom_right.y(), frame->height() + 0.01);
        EXPECT_NEAR(control->mapToItem(row, QPointF{0, control->height() / 2}).y(), row->height() / 2, 0.51);
      }
    }
  };

  EXPECT_EQ(findVisualChildren(root_item, QStringLiteral("appearanceSectionHeader")).size(), 4);
  verify_geometry();
  root_item->setWidth(420);
  window.resize(420, 900);
  verify_geometry();
}

TEST(SettingsAppearanceQmlTest, ShapeSettingsRowsPreserveControlsAndUpdateEditModel) {
  static const int registration = qmlRegisterType<FontListModel>("HolonightSettings", 1, 0, "FontListModel");
  Q_UNUSED(registration);
  static const int swatch_tokens_registration = qmlRegisterSingletonType<ThemeSwatchTokens>(
      "HolonightSettings", 1, 0, "ThemeSwatchTokens",
      [](QQmlEngine*, QJSEngine*) -> QObject* { return new ThemeSwatchTokens(); });
  Q_UNUSED(swatch_tokens_registration);

  SettingsEditModel model;
  QQmlEngine engine;
  engine.rootContext()->setContextProperty(QStringLiteral("editModel"), &model);
  QQmlComponent component(&engine,
                          QUrl::fromLocalFile(QStringLiteral(TEST_SOURCE_DIR "/apps/settings/qml/AppearancePage.qml")));
  std::unique_ptr<QObject> page{
      component.createWithInitialProperties({{QStringLiteral("editModel"), QVariant::fromValue(&model)}})};
  ASSERT_NE(page, nullptr) << component.errorString().toStdString();

  auto* root_item = qobject_cast<QQuickItem*>(page.get());
  ASSERT_NE(root_item, nullptr);
  root_item->setSize(QSizeF(1000, 1600));
  QQuickWindow window;
  window.resize(1000, 800);
  root_item->setParentItem(window.contentItem());
  window.show();
  ASSERT_TRUE(QTest::qWaitForWindowExposed(&window));
  QCoreApplication::processEvents();

  auto* shape_row = findVisualChild(root_item, QStringLiteral("shapeScaleRow"));
  auto* radius_row = findVisualChild(root_item, QStringLiteral("baseRadiusRow"));
  auto* chamfer_row = findVisualChild(root_item, QStringLiteral("baseChamferRow"));
  auto* shape_controls = findVisualChild(root_item, QStringLiteral("shapeScaleControls"));
  auto* radius_controls = findVisualChild(root_item, QStringLiteral("baseRadiusControls"));
  auto* chamfer_controls = findVisualChild(root_item, QStringLiteral("baseChamferControls"));
  auto* shape_scale = findVisualChild(root_item, QStringLiteral("shapeScaleSlider"));
  auto* radius_switch = findVisualChild(root_item, QStringLiteral("baseRadiusSwitch"));
  auto* radius_slider = findVisualChild(root_item, QStringLiteral("baseRadiusSlider"));
  auto* chamfer_switch = findVisualChild(root_item, QStringLiteral("baseChamferSwitch"));
  auto* chamfer_slider = findVisualChild(root_item, QStringLiteral("baseChamferSlider"));
  ASSERT_NE(shape_row, nullptr);
  ASSERT_NE(radius_row, nullptr);
  ASSERT_NE(chamfer_row, nullptr);
  ASSERT_NE(shape_controls, nullptr);
  ASSERT_NE(radius_controls, nullptr);
  ASSERT_NE(chamfer_controls, nullptr);
  ASSERT_NE(shape_scale, nullptr);
  ASSERT_NE(radius_switch, nullptr);
  ASSERT_NE(radius_slider, nullptr);
  ASSERT_NE(chamfer_switch, nullptr);
  ASSERT_NE(chamfer_slider, nullptr);
  EXPECT_EQ(shape_row->property("titleText").toString(), QStringLiteral("Shape scale"));
  EXPECT_EQ(radius_row->property("titleText").toString(), QStringLiteral("Override base radius"));
  EXPECT_EQ(chamfer_row->property("titleText").toString(), QStringLiteral("Override base chamfer"));
  EXPECT_FALSE(shape_row->property("stacked").toBool());
  EXPECT_FALSE(radius_row->property("stacked").toBool());
  EXPECT_FALSE(chamfer_row->property("stacked").toBool());
  EXPECT_FALSE(shape_row->property("activeFocusOnTab").toBool());
  EXPECT_FALSE(radius_row->property("activeFocusOnTab").toBool());
  EXPECT_FALSE(chamfer_row->property("activeFocusOnTab").toBool());
  EXPECT_TRUE(radius_switch->property("activeFocusOnTab").toBool());
  EXPECT_TRUE(chamfer_switch->property("activeFocusOnTab").toBool());
  EXPECT_NEAR(shape_controls->mapToScene(QPointF(shape_controls->width(), 0)).x(),
              shape_row->mapToScene(QPointF(shape_row->width() - 16, 0)).x(), 0.01);
  EXPECT_NEAR(radius_controls->mapToScene(QPointF(radius_controls->width(), 0)).x(),
              radius_row->mapToScene(QPointF(radius_row->width() - 16, 0)).x(), 0.01);
  EXPECT_NEAR(chamfer_controls->mapToScene(QPointF(chamfer_controls->width(), 0)).x(),
              chamfer_row->mapToScene(QPointF(chamfer_row->width() - 16, 0)).x(), 0.01);
  EXPECT_NEAR(radius_controls->mapToScene(QPointF{}).x(), chamfer_controls->mapToScene(QPointF{}).x(), 0.01);
  EXPECT_NEAR(radius_controls->width(), chamfer_controls->width(), 0.01);
  EXPECT_NEAR(radius_switch->mapToScene(QPointF(0, radius_switch->height() / 2.0)).y(),
              radius_slider->mapToScene(QPointF(0, radius_slider->height() / 2.0)).y(), 0.51);
  EXPECT_NEAR(chamfer_switch->mapToScene(QPointF(0, chamfer_switch->height() / 2.0)).y(),
              chamfer_slider->mapToScene(QPointF(0, chamfer_slider->height() / 2.0)).y(), 0.51);
  EXPECT_EQ(shape_scale->property("from").toReal(), 0.25);
  EXPECT_EQ(shape_scale->property("to").toReal(), 4.0);
  EXPECT_EQ(shape_scale->property("stepSize").toReal(), 0.05);
  EXPECT_EQ(radius_slider->property("from").toReal(), 0.0);
  EXPECT_EQ(radius_slider->property("to").toReal(), 128.0);
  EXPECT_EQ(radius_slider->property("stepSize").toReal(), 1.0);
  EXPECT_EQ(chamfer_slider->property("from").toReal(), 0.0);
  EXPECT_EQ(chamfer_slider->property("to").toReal(), 128.0);
  EXPECT_EQ(chamfer_slider->property("stepSize").toReal(), 1.0);
  EXPECT_FALSE(radius_slider->isEnabled());
  EXPECT_FALSE(chamfer_slider->isEnabled());
  QStringList section_titles;
  const auto section_headers = page->findChildren<QObject*>(QStringLiteral("appearanceSectionHeader"));
  for (QObject* section_header : section_headers) {
    section_titles.append(section_header->property("titleText").toString());
    EXPECT_FALSE(section_header->property("dividerVisible").toBool());
  }
  EXPECT_EQ(section_titles, QStringList({QStringLiteral("Theme"), QStringLiteral("Typography"),
                                         QStringLiteral("Global Shape"), QStringLiteral("Advanced Shape")}));

  shape_scale->setProperty("value", 1.5);
  ASSERT_TRUE(QMetaObject::invokeMethod(shape_scale, "moved"));
  radius_switch->setProperty("checked", true);
  radius_slider->setProperty("value", 14.0);
  ASSERT_TRUE(QMetaObject::invokeMethod(radius_slider, "moved"));
  chamfer_switch->setProperty("checked", true);
  chamfer_slider->setProperty("value", 18.0);
  ASSERT_TRUE(QMetaObject::invokeMethod(chamfer_slider, "moved"));

  EXPECT_EQ(model.shapeScale(), 1.5);
  EXPECT_TRUE(model.baseRadiusEnabled());
  EXPECT_EQ(model.baseRadius(), 14.0);
  EXPECT_TRUE(model.baseChamferEnabled());
  EXPECT_EQ(model.baseChamfer(), 18.0);
}

TEST(SettingsAppearanceQmlTest, ThemeVariantsPreserveCatalogAndUpdateEditModel) {
  static const int registration = qmlRegisterType<FontListModel>("HolonightSettings", 1, 0, "FontListModel");
  Q_UNUSED(registration);
  static const int swatch_tokens_registration = qmlRegisterSingletonType<ThemeSwatchTokens>(
      "HolonightSettings", 1, 0, "ThemeSwatchTokens",
      [](QQmlEngine*, QJSEngine*) -> QObject* { return new ThemeSwatchTokens(); });
  Q_UNUSED(swatch_tokens_registration);

  SettingsEditModel model;
  QQmlEngine engine;
  engine.rootContext()->setContextProperty(QStringLiteral("editModel"), &model);
  QQmlComponent component(&engine,
                          QUrl::fromLocalFile(QStringLiteral(TEST_SOURCE_DIR "/apps/settings/qml/AppearancePage.qml")));
  std::unique_ptr<QObject> page{
      component.createWithInitialProperties({{QStringLiteral("editModel"), QVariant::fromValue(&model)}})};
  ASSERT_NE(page, nullptr) << component.errorString().toStdString();

  auto* root_item = qobject_cast<QQuickItem*>(page.get());
  ASSERT_NE(root_item, nullptr);
  root_item->setSize(QSizeF(1000, 1600));
  QQuickWindow window;
  window.resize(1000, 800);
  root_item->setParentItem(window.contentItem());
  window.show();
  ASSERT_TRUE(QTest::qWaitForWindowExposed(&window));
  QCoreApplication::processEvents();

  const auto cards = findVisualChildren(root_item, QStringLiteral("themeFamilyCard"));
  ASSERT_EQ(cards.size(), 5);

  QStringList family_ids;
  QQuickItem* selected_card = nullptr;
  for (QQuickItem* card : cards) {
    family_ids.append(card->property("familyId").toString());
    EXPECT_FALSE(card->property("title").toString().isEmpty());
    EXPECT_TRUE(card->property("enabled").toBool());
    if (card->property("checked").toBool()) {
      selected_card = card;
    }
  }

  EXPECT_EQ(family_ids,
            QStringList({QStringLiteral("holonight"), QStringLiteral("catppuccin"), QStringLiteral("tokyonight"),
                         QStringLiteral("gruvbox"), QStringLiteral("cyber")}));
  ASSERT_NE(selected_card, nullptr);
  EXPECT_EQ(selected_card->property("familyId").toString(), QStringLiteral("holonight"));

  auto* swatch_flickable = findVisualChild(root_item, QStringLiteral("colorSchemeSwatchFlickable"));
  ASSERT_NE(swatch_flickable, nullptr);
  const auto scroll_into_view = [swatch_flickable](QQuickItem* card) {
    const qreal maximum_content_x =
        std::max(0.0, swatch_flickable->property("contentWidth").toReal() - swatch_flickable->width());
    swatch_flickable->setProperty("contentX", std::clamp(card->x(), 0.0, maximum_content_x));
    QCoreApplication::processEvents();
  };

  for (const QString& mode : {QStringLiteral("dark"), QStringLiteral("light")}) {
    model.setThemeMode(mode);
    QCoreApplication::processEvents();

    for (QQuickItem* card : cards) {
      // Swatch cards live in a clipped horizontal Flickable. Scroll each target into view before
      // computing scene coordinates so the synthesized click reaches the delegate.
      scroll_into_view(card);

      const QPoint card_center = card->mapToScene(QPointF(card->width() / 2.0, card->height() / 2.0)).toPoint();
      QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, card_center);
      EXPECT_EQ(model.themeScheme(), card->property("schemeId").toString());
      EXPECT_TRUE(card->property("checked").toBool());
    }
  }

  model.setThemeMode(QStringLiteral("dark"));
  scroll_into_view(selected_card);
  selected_card->forceActiveFocus();
  EXPECT_TRUE(selected_card->hasActiveFocus());
  QTest::keyClick(&window, Qt::Key_Space);
  EXPECT_EQ(model.themeScheme(), QStringLiteral("holonight-dark"));

  QQuickItem* target_card = cards.constLast();
  target_card->setEnabled(false);
  scroll_into_view(target_card);
  const QPoint target_center =
      target_card->mapToScene(QPointF(target_card->width() / 2.0, target_card->height() / 2.0)).toPoint();
  QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, target_center);
  EXPECT_EQ(model.themeScheme(), QStringLiteral("holonight-dark"));
}

TEST(SettingsAppearanceQmlTest, DarkModeTogglePreservesFamilyAndUpdatesEditModel) {
  static const int registration = qmlRegisterType<FontListModel>("HolonightSettings", 1, 0, "FontListModel");
  Q_UNUSED(registration);
  static const int swatch_tokens_registration = qmlRegisterSingletonType<ThemeSwatchTokens>(
      "HolonightSettings", 1, 0, "ThemeSwatchTokens",
      [](QQmlEngine*, QJSEngine*) -> QObject* { return new ThemeSwatchTokens(); });
  Q_UNUSED(swatch_tokens_registration);

  SettingsEditModel model;
  QQmlEngine engine;
  engine.rootContext()->setContextProperty(QStringLiteral("editModel"), &model);
  QQmlComponent component(&engine,
                          QUrl::fromLocalFile(QStringLiteral(TEST_SOURCE_DIR "/apps/settings/qml/AppearancePage.qml")));
  std::unique_ptr<QObject> page{
      component.createWithInitialProperties({{QStringLiteral("editModel"), QVariant::fromValue(&model)}})};
  ASSERT_NE(page, nullptr) << component.errorString().toStdString();

  auto* root_item = qobject_cast<QQuickItem*>(page.get());
  ASSERT_NE(root_item, nullptr);
  root_item->setSize(QSizeF(1000, 1600));
  QQuickWindow window;
  window.resize(1000, 800);
  root_item->setParentItem(window.contentItem());
  window.show();
  ASSERT_TRUE(QTest::qWaitForWindowExposed(&window));
  QCoreApplication::processEvents();

  auto* dark_mode_row = findVisualChild(root_item, QStringLiteral("darkModeRow"));
  auto* dark_mode_switch = findVisualChild(root_item, QStringLiteral("darkModeSwitch"));
  ASSERT_NE(dark_mode_row, nullptr);
  ASSERT_NE(dark_mode_switch, nullptr);
  EXPECT_EQ(dark_mode_row->property("titleText").toString(), QStringLiteral("Dark mode"));
  EXPECT_FALSE(dark_mode_row->property("stacked").toBool());

  // Default scheme is "holonight-dark" -> toggle starts on.
  EXPECT_TRUE(dark_mode_switch->property("checked").toBool());

  dark_mode_switch->setProperty("checked", false);
  ASSERT_TRUE(QMetaObject::invokeMethod(dark_mode_switch, "toggled"));
  EXPECT_EQ(model.themeScheme(), QStringLiteral("holonight-light"));
  EXPECT_EQ(model.themeMode(), QStringLiteral("light"));

  dark_mode_switch->setProperty("checked", true);
  ASSERT_TRUE(QMetaObject::invokeMethod(dark_mode_switch, "toggled"));
  EXPECT_EQ(model.themeScheme(), QStringLiteral("holonight-dark"));

  // Generalizes to non-"holonight" families: pick holonight-storm (dark), then toggle to light.
  model.setThemeScheme(QStringLiteral("holonight-storm"));
  EXPECT_TRUE(dark_mode_switch->property("checked").toBool());
  dark_mode_switch->setProperty("checked", false);
  ASSERT_TRUE(QMetaObject::invokeMethod(dark_mode_switch, "toggled"));
  EXPECT_EQ(model.themeScheme(), QStringLiteral("holonight-day"));
  dark_mode_switch->setProperty("checked", true);
  ASSERT_TRUE(QMetaObject::invokeMethod(dark_mode_switch, "toggled"));
  EXPECT_EQ(model.themeScheme(), QStringLiteral("holonight-storm"));
}

TEST(SettingsAppearanceQmlTest, TransparencyAndBlurSlidersUpdateEditModel) {
  static const int registration = qmlRegisterType<FontListModel>("HolonightSettings", 1, 0, "FontListModel");
  Q_UNUSED(registration);
  static const int swatch_tokens_registration = qmlRegisterSingletonType<ThemeSwatchTokens>(
      "HolonightSettings", 1, 0, "ThemeSwatchTokens",
      [](QQmlEngine*, QJSEngine*) -> QObject* { return new ThemeSwatchTokens(); });
  Q_UNUSED(swatch_tokens_registration);

  SettingsEditModel model;
  QQmlEngine engine;
  engine.rootContext()->setContextProperty(QStringLiteral("editModel"), &model);
  QQmlComponent component(&engine,
                          QUrl::fromLocalFile(QStringLiteral(TEST_SOURCE_DIR "/apps/settings/qml/AppearancePage.qml")));
  std::unique_ptr<QObject> page{
      component.createWithInitialProperties({{QStringLiteral("editModel"), QVariant::fromValue(&model)}})};
  ASSERT_NE(page, nullptr) << component.errorString().toStdString();

  auto* root_item = qobject_cast<QQuickItem*>(page.get());
  ASSERT_NE(root_item, nullptr);
  root_item->setSize(QSizeF(1000, 1600));
  QQuickWindow window;
  window.resize(1000, 800);
  root_item->setParentItem(window.contentItem());
  window.show();
  ASSERT_TRUE(QTest::qWaitForWindowExposed(&window));
  QCoreApplication::processEvents();

  auto* transparency_row = findVisualChild(root_item, QStringLiteral("transparencyRow"));
  auto* transparency_slider = findVisualChild(root_item, QStringLiteral("transparencySlider"));
  auto* transparency_value = findVisualChild(root_item, QStringLiteral("transparencyValue"));
  auto* blur_row = findVisualChild(root_item, QStringLiteral("blurStrengthRow"));
  auto* blur_slider = findVisualChild(root_item, QStringLiteral("blurStrengthSlider"));
  auto* blur_value = findVisualChild(root_item, QStringLiteral("blurStrengthValue"));
  ASSERT_NE(transparency_row, nullptr);
  ASSERT_NE(transparency_slider, nullptr);
  ASSERT_NE(transparency_value, nullptr);
  ASSERT_NE(blur_row, nullptr);
  ASSERT_NE(blur_slider, nullptr);
  ASSERT_NE(blur_value, nullptr);

  EXPECT_FALSE(transparency_row->property("stacked").toBool());
  EXPECT_FALSE(blur_row->property("stacked").toBool());
  EXPECT_EQ(transparency_slider->property("from").toDouble(), 0.0);
  EXPECT_EQ(transparency_slider->property("to").toDouble(), 100.0);
  EXPECT_EQ(blur_slider->property("from").toDouble(), 0.0);
  EXPECT_EQ(blur_slider->property("to").toDouble(), 64.0);
  EXPECT_EQ(transparency_slider->property("value").toInt(), model.transparency());
  EXPECT_EQ(blur_slider->property("value").toInt(), model.blurStrength());
  EXPECT_EQ(transparency_value->property("text").toString(), QStringLiteral("%1%").arg(model.transparency()));
  EXPECT_EQ(blur_value->property("text").toString(), QStringLiteral("%1 px").arg(model.blurStrength()));

  transparency_slider->setProperty("value", 60);
  ASSERT_TRUE(QMetaObject::invokeMethod(transparency_slider, "moved"));
  blur_slider->setProperty("value", 16);
  ASSERT_TRUE(QMetaObject::invokeMethod(blur_slider, "moved"));

  EXPECT_EQ(model.transparency(), 60);
  EXPECT_EQ(model.blurStrength(), 16);
  EXPECT_EQ(transparency_value->property("text").toString(), QStringLiteral("60%"));
  EXPECT_EQ(blur_value->property("text").toString(), QStringLiteral("16 px"));
}

TEST(SettingsAppearanceQmlTest, AccentChoicesPreserveCatalogAndUpdateEditModel) {
  static const int registration = qmlRegisterType<FontListModel>("HolonightSettings", 1, 0, "FontListModel");
  Q_UNUSED(registration);
  static const int swatch_tokens_registration = qmlRegisterSingletonType<ThemeSwatchTokens>(
      "HolonightSettings", 1, 0, "ThemeSwatchTokens",
      [](QQmlEngine*, QJSEngine*) -> QObject* { return new ThemeSwatchTokens(); });
  Q_UNUSED(swatch_tokens_registration);

  SettingsEditModel model;
  QQmlEngine engine;
  engine.rootContext()->setContextProperty(QStringLiteral("editModel"), &model);
  QQmlComponent component(&engine,
                          QUrl::fromLocalFile(QStringLiteral(TEST_SOURCE_DIR "/apps/settings/qml/AppearancePage.qml")));
  std::unique_ptr<QObject> page{
      component.createWithInitialProperties({{QStringLiteral("editModel"), QVariant::fromValue(&model)}})};
  ASSERT_NE(page, nullptr) << component.errorString().toStdString();

  auto* root_item = qobject_cast<QQuickItem*>(page.get());
  ASSERT_NE(root_item, nullptr);
  root_item->setSize(QSizeF(1000, 1600));
  QQuickWindow window;
  window.resize(1000, 800);
  root_item->setParentItem(window.contentItem());
  window.show();
  ASSERT_TRUE(QTest::qWaitForWindowExposed(&window));
  QCoreApplication::processEvents();

  auto* picker = findVisualChild(root_item, QStringLiteral("accentColorPicker"));
  ASSERT_NE(picker, nullptr);

  const auto colors = picker->property("colors").toList();
  ASSERT_EQ(colors.size(), 4);

  const auto all_children = findAllVisualChildren(picker);
  QList<QQuickItem*> swatches;
  for (QQuickItem* child : all_children) {
    if (child->property("modelData").isValid()) {
      swatches.append(child);
    }
  }
  ASSERT_EQ(swatches.size(), 4);

  // Default accent for holonight-dark maps to blue (swatch 1)
  EXPECT_TRUE(swatches.at(1)->property("selected").toBool());
  EXPECT_FALSE(swatches.at(0)->property("selected").toBool());
  EXPECT_EQ(picker->property("selectedColor").value<QColor>(), colors.at(1).value<QColor>());

  QQuickItem* target_swatch = swatches.at(0);  // Cyan
  const QPoint target_center =
      target_swatch->mapToScene(QPointF(target_swatch->width() / 2.0, target_swatch->height() / 2.0)).toPoint();
  QTest::mouseMove(&window, target_center);
  EXPECT_TRUE(target_swatch->property("hovered").toBool());
  QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, target_center);
  EXPECT_TRUE(target_swatch->property("down").toBool());
  QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, target_center);
  EXPECT_FALSE(target_swatch->property("down").toBool());

  EXPECT_EQ(model.themeAccent(), QStringLiteral("cyan"));
  EXPECT_TRUE(target_swatch->property("selected").toBool());
  EXPECT_FALSE(swatches.at(1)->property("selected").toBool());

  QMetaObject::invokeMethod(picker, "activate", Q_ARG(int, 1));
  EXPECT_EQ(model.themeAccent(), QStringLiteral("blue"));

  target_swatch->setEnabled(false);
  QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, target_center);
  EXPECT_EQ(model.themeAccent(), QStringLiteral("blue"));
}

TEST(SettingsAppearanceQmlTest, GlobalShapeSelectorPreservesValuesAndUpdatesEditModel) {
  static const int registration = qmlRegisterType<FontListModel>("HolonightSettings", 1, 0, "FontListModel");
  Q_UNUSED(registration);
  static const int swatch_tokens_registration = qmlRegisterSingletonType<ThemeSwatchTokens>(
      "HolonightSettings", 1, 0, "ThemeSwatchTokens",
      [](QQmlEngine*, QJSEngine*) -> QObject* { return new ThemeSwatchTokens(); });
  Q_UNUSED(swatch_tokens_registration);

  SettingsEditModel model;
  QQmlEngine engine;
  engine.rootContext()->setContextProperty(QStringLiteral("editModel"), &model);
  QQmlComponent component(&engine,
                          QUrl::fromLocalFile(QStringLiteral(TEST_SOURCE_DIR "/apps/settings/qml/AppearancePage.qml")));
  std::unique_ptr<QObject> page{
      component.createWithInitialProperties({{QStringLiteral("editModel"), QVariant::fromValue(&model)}})};
  ASSERT_NE(page, nullptr) << component.errorString().toStdString();

  auto* root_item = qobject_cast<QQuickItem*>(page.get());
  ASSERT_NE(root_item, nullptr);
  root_item->setSize(QSizeF(1000, 1600));
  QQuickWindow window;
  window.resize(1000, 800);
  root_item->setParentItem(window.contentItem());
  window.show();
  ASSERT_TRUE(QTest::qWaitForWindowExposed(&window));
  QCoreApplication::processEvents();

  auto* control = findVisualChild(root_item, QStringLiteral("shapeStyleControl"));
  ASSERT_NE(control, nullptr);
  EXPECT_EQ(control->property("currentIndex").toInt(), 0);
  EXPECT_EQ(control->property("currentValue").toString(), QStringLiteral("inherit"));

  QList<QQuickItem*> segments;
  for (int index = 0; index < 4; ++index) {
    auto* segment = findVisualChild(control, QStringLiteral("hnSegment%1").arg(index));
    ASSERT_NE(segment, nullptr);
    segments.append(segment);
  }
  EXPECT_EQ(segments.at(0)->property("text").toString(), QStringLiteral("Inherit"));
  EXPECT_EQ(segments.at(1)->property("text").toString(), QStringLiteral("Hybrid"));
  EXPECT_EQ(segments.at(2)->property("text").toString(), QStringLiteral("Rounded"));
  EXPECT_EQ(segments.at(3)->property("text").toString(), QStringLiteral("Chamfered"));

  const QPoint rounded_center =
      segments.at(2)->mapToScene(QPointF(segments.at(2)->width() / 2.0, segments.at(2)->height() / 2.0)).toPoint();
  QTest::mouseMove(&window, rounded_center);
  EXPECT_TRUE(segments.at(2)->property("hovered").toBool());
  QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, rounded_center);
  EXPECT_TRUE(segments.at(2)->property("down").toBool());
  QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, rounded_center);
  EXPECT_EQ(model.shapeCornerStyle(), QStringLiteral("rounded"));
  EXPECT_EQ(control->property("currentIndex").toInt(), 2);
  EXPECT_TRUE(segments.at(2)->property("checked").toBool());

  control->forceActiveFocus();
  EXPECT_TRUE(control->hasActiveFocus());
  QTest::keyClick(&window, Qt::Key_Right);
  EXPECT_EQ(model.shapeCornerStyle(), QStringLiteral("chamfered"));
  EXPECT_EQ(control->property("currentIndex").toInt(), 3);

  model.setShapeCornerStyle(QStringLiteral("hybrid"));
  EXPECT_EQ(control->property("currentIndex").toInt(), 1);
  EXPECT_EQ(control->property("currentValue").toString(), QStringLiteral("hybrid"));

  control->setEnabled(false);
  QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, rounded_center);
  EXPECT_EQ(model.shapeCornerStyle(), QStringLiteral("hybrid"));
}

TEST(SettingsAppearanceQmlTest, FontSelectorsPreserveModelsAndUpdateEditModel) {
  static const int registration = qmlRegisterType<FontListModel>("HolonightSettings", 1, 0, "FontListModel");
  Q_UNUSED(registration);
  static const int swatch_tokens_registration = qmlRegisterSingletonType<ThemeSwatchTokens>(
      "HolonightSettings", 1, 0, "ThemeSwatchTokens",
      [](QQmlEngine*, QJSEngine*) -> QObject* { return new ThemeSwatchTokens(); });
  Q_UNUSED(swatch_tokens_registration);

  SettingsEditModel model;
  QQmlEngine engine;
  engine.rootContext()->setContextProperty(QStringLiteral("editModel"), &model);
  QQmlComponent component(&engine,
                          QUrl::fromLocalFile(QStringLiteral(TEST_SOURCE_DIR "/apps/settings/qml/AppearancePage.qml")));
  std::unique_ptr<QObject> page{
      component.createWithInitialProperties({{QStringLiteral("editModel"), QVariant::fromValue(&model)}})};
  ASSERT_NE(page, nullptr) << component.errorString().toStdString();

  auto* root_item = qobject_cast<QQuickItem*>(page.get());
  ASSERT_NE(root_item, nullptr);
  root_item->setSize(QSizeF(1000, 1600));
  QQuickWindow window;
  window.resize(1000, 900);
  root_item->setParentItem(window.contentItem());
  window.show();
  ASSERT_TRUE(QTest::qWaitForWindowExposed(&window));
  QCoreApplication::processEvents();

  auto* ui_combo = findVisualChild(root_item, QStringLiteral("uiFontCombo"));
  auto* fixed_combo = findVisualChild(root_item, QStringLiteral("fixedFontCombo"));
  ASSERT_NE(ui_combo, nullptr);
  ASSERT_NE(fixed_combo, nullptr);

  auto* ui_model = qobject_cast<FontListModel*>(ui_combo->property("model").value<QObject*>());
  auto* fixed_model = qobject_cast<FontListModel*>(fixed_combo->property("model").value<QObject*>());
  ASSERT_NE(ui_model, nullptr);
  ASSERT_NE(fixed_model, nullptr);
  EXPECT_FALSE(ui_model->fixedPitchOnly());
  EXPECT_TRUE(fixed_model->fixedPitchOnly());
  EXPECT_GT(ui_model->rowCount(), 1);
  EXPECT_GT(fixed_model->rowCount(), 1);
  EXPECT_EQ(ui_combo->property("textRole").toString(), QStringLiteral("display"));
  EXPECT_EQ(fixed_combo->property("textRole").toString(), QStringLiteral("display"));
  EXPECT_EQ(ui_combo->property("currentIndex").toInt(), ui_model->indexOf(model.uiFont()));
  EXPECT_EQ(fixed_combo->property("currentIndex").toInt(), fixed_model->indexOf(model.fixedFont()));
  if (ui_combo->property("currentIndex").toInt() >= 0) {
    EXPECT_EQ(ui_combo->property("currentText").toString(), model.uiFont());
  }
  if (fixed_combo->property("currentIndex").toInt() >= 0) {
    EXPECT_EQ(fixed_combo->property("currentText").toString(), model.fixedFont());
  }
  EXPECT_TRUE(ui_combo->property("activeFocusOnTab").toBool());
  EXPECT_TRUE(fixed_combo->property("activeFocusOnTab").toBool());

  const int next_ui_index = ui_combo->property("currentIndex").toInt() == 0 ? 1 : 0;
  const QString next_ui_font = ui_model->data(ui_model->index(next_ui_index), Qt::DisplayRole).toString();
  ui_combo->setProperty("currentIndex", next_ui_index);
  ASSERT_TRUE(QMetaObject::invokeMethod(ui_combo, "activated", Q_ARG(int, next_ui_index)));
  EXPECT_EQ(model.uiFont(), next_ui_font);

  const int next_fixed_index = fixed_combo->property("currentIndex").toInt() == 0 ? 1 : 0;
  const QString next_fixed_font = fixed_model->data(fixed_model->index(next_fixed_index), Qt::DisplayRole).toString();
  fixed_combo->setProperty("currentIndex", next_fixed_index);
  ASSERT_TRUE(QMetaObject::invokeMethod(fixed_combo, "activated", Q_ARG(int, next_fixed_index)));
  EXPECT_EQ(model.fixedFont(), next_fixed_font);

  ui_combo->forceActiveFocus();
  EXPECT_TRUE(ui_combo->hasActiveFocus());
  QTest::keyClick(&window, Qt::Key_Space);
  EXPECT_TRUE(ui_combo->property("popup").value<QObject*>()->property("visible").toBool());
  QTest::keyClick(&window, Qt::Key_Escape);
  EXPECT_FALSE(ui_combo->property("popup").value<QObject*>()->property("visible").toBool());

  fixed_combo->setEnabled(false);
  const QPoint fixed_center =
      fixed_combo->mapToScene(QPointF(fixed_combo->width() / 2.0, fixed_combo->height() / 2.0)).toPoint();
  QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, fixed_center);
  EXPECT_FALSE(fixed_combo->property("popup").value<QObject*>()->property("visible").toBool());
}

TEST(SettingsAppearanceQmlTest, FontSizeRowsPreserveRangesAndUpdateEditModel) {
  static const int registration = qmlRegisterType<FontListModel>("HolonightSettings", 1, 0, "FontListModel");
  Q_UNUSED(registration);
  static const int swatch_tokens_registration = qmlRegisterSingletonType<ThemeSwatchTokens>(
      "HolonightSettings", 1, 0, "ThemeSwatchTokens",
      [](QQmlEngine*, QJSEngine*) -> QObject* { return new ThemeSwatchTokens(); });
  Q_UNUSED(swatch_tokens_registration);

  SettingsEditModel model;
  QQmlEngine engine;
  engine.rootContext()->setContextProperty(QStringLiteral("editModel"), &model);
  QQmlComponent component(&engine,
                          QUrl::fromLocalFile(QStringLiteral(TEST_SOURCE_DIR "/apps/settings/qml/AppearancePage.qml")));
  std::unique_ptr<QObject> page{
      component.createWithInitialProperties({{QStringLiteral("editModel"), QVariant::fromValue(&model)}})};
  ASSERT_NE(page, nullptr) << component.errorString().toStdString();

  auto* root_item = qobject_cast<QQuickItem*>(page.get());
  ASSERT_NE(root_item, nullptr);
  root_item->setSize(QSizeF(1000, 1600));
  QQuickWindow window;
  window.resize(1000, 900);
  root_item->setParentItem(window.contentItem());
  window.show();
  ASSERT_TRUE(QTest::qWaitForWindowExposed(&window));
  QCoreApplication::processEvents();

  auto* ui_row = findVisualChild(root_item, QStringLiteral("uiFontSizeRow"));
  auto* fixed_row = findVisualChild(root_item, QStringLiteral("fixedFontSizeRow"));
  auto* ui_controls = findVisualChild(root_item, QStringLiteral("uiFontSizeControls"));
  auto* fixed_controls = findVisualChild(root_item, QStringLiteral("fixedFontSizeControls"));
  auto* ui_slider = findVisualChild(root_item, QStringLiteral("uiFontSizeSlider"));
  auto* fixed_slider = findVisualChild(root_item, QStringLiteral("fixedFontSizeSlider"));
  auto* ui_value = findVisualChild(root_item, QStringLiteral("uiFontSizeValue"));
  auto* fixed_value = findVisualChild(root_item, QStringLiteral("fixedFontSizeValue"));
  ASSERT_NE(ui_row, nullptr);
  ASSERT_NE(fixed_row, nullptr);
  ASSERT_NE(ui_controls, nullptr);
  ASSERT_NE(fixed_controls, nullptr);
  ASSERT_NE(ui_slider, nullptr);
  ASSERT_NE(fixed_slider, nullptr);
  ASSERT_NE(ui_value, nullptr);
  ASSERT_NE(fixed_value, nullptr);

  EXPECT_EQ(ui_row->property("titleText").toString(), QStringLiteral("Interface font size"));
  EXPECT_EQ(fixed_row->property("titleText").toString(), QStringLiteral("Monospace font size"));
  EXPECT_FALSE(ui_row->property("stacked").toBool());
  EXPECT_FALSE(fixed_row->property("stacked").toBool());
  EXPECT_EQ(ui_row->property("controlItem").value<QObject*>(), ui_controls);
  EXPECT_EQ(fixed_row->property("controlItem").value<QObject*>(), fixed_controls);

  for (QQuickItem* slider : {ui_slider, fixed_slider}) {
    EXPECT_EQ(slider->property("from").toDouble(), 8.0);
    EXPECT_EQ(slider->property("to").toDouble(), 18.0);
    EXPECT_EQ(slider->property("stepSize").toDouble(), 1.0);
  }
  EXPECT_EQ(ui_slider->property("value").toInt(), model.uiFontSize());
  EXPECT_EQ(fixed_slider->property("value").toInt(), model.fixedFontSize());
  EXPECT_EQ(ui_value->property("text").toString(), QStringLiteral("%1 pt").arg(model.uiFontSize()));
  EXPECT_EQ(fixed_value->property("text").toString(), QStringLiteral("%1 pt").arg(model.fixedFontSize()));

  model.setUiFontSize(16);
  model.setFixedFontSize(18);
  EXPECT_EQ(ui_slider->property("value").toInt(), 16);
  EXPECT_EQ(fixed_slider->property("value").toInt(), 18);
  EXPECT_EQ(ui_value->property("text").toString(), QStringLiteral("16 pt"));
  EXPECT_EQ(fixed_value->property("text").toString(), QStringLiteral("18 pt"));

  ui_slider->setProperty("value", 17.4);
  ASSERT_TRUE(QMetaObject::invokeMethod(ui_slider, "moved"));
  fixed_slider->setProperty("value", 12.6);
  ASSERT_TRUE(QMetaObject::invokeMethod(fixed_slider, "moved"));
  EXPECT_EQ(model.uiFontSize(), 17);
  EXPECT_EQ(model.fixedFontSize(), 13);

  EXPECT_TRUE(ui_slider->property("activeFocusOnTab").toBool());
  EXPECT_TRUE(fixed_slider->property("activeFocusOnTab").toBool());
  ui_slider->forceActiveFocus();
  EXPECT_TRUE(ui_slider->hasActiveFocus());
}

TEST(SettingsBarQmlTest, WorkspaceAndTrayRowsPreserveRangesAndUpdateEditModel) {
  SettingsEditModel model;
  QQmlEngine engine;
  engine.rootContext()->setContextProperty(QStringLiteral("editModel"), &model);
  QQmlComponent component(&engine,
                          QUrl::fromLocalFile(QStringLiteral(TEST_SOURCE_DIR "/apps/settings/qml/BarPage.qml")));
  std::unique_ptr<QObject> page{
      component.createWithInitialProperties({{QStringLiteral("editModel"), QVariant::fromValue(&model)}})};
  ASSERT_NE(page, nullptr) << component.errorString().toStdString();

  auto* root_item = qobject_cast<QQuickItem*>(page.get());
  ASSERT_NE(root_item, nullptr);
  root_item->setSize(QSizeF(1000, 800));
  QQuickWindow window;
  window.resize(1000, 800);
  root_item->setParentItem(window.contentItem());
  window.show();
  ASSERT_TRUE(QTest::qWaitForWindowExposed(&window));
  QCoreApplication::processEvents();

  auto* workspace_row = findVisualChild(root_item, QStringLiteral("workspaceCountRow"));
  auto* tray_row = findVisualChild(root_item, QStringLiteral("trayMaxItemsRow"));
  auto* workspace_controls = findVisualChild(root_item, QStringLiteral("workspaceCountControls"));
  auto* tray_controls = findVisualChild(root_item, QStringLiteral("trayMaxItemsControls"));
  auto* workspace_slider = findVisualChild(root_item, QStringLiteral("workspaceCountSlider"));
  auto* tray_slider = findVisualChild(root_item, QStringLiteral("trayMaxItemsSlider"));
  auto* workspace_value = findVisualChild(root_item, QStringLiteral("workspaceCountValue"));
  auto* tray_value = findVisualChild(root_item, QStringLiteral("trayMaxItemsValue"));
  ASSERT_NE(workspace_row, nullptr);
  ASSERT_NE(tray_row, nullptr);
  ASSERT_NE(workspace_controls, nullptr);
  ASSERT_NE(tray_controls, nullptr);
  ASSERT_NE(workspace_slider, nullptr);
  ASSERT_NE(tray_slider, nullptr);
  ASSERT_NE(workspace_value, nullptr);
  ASSERT_NE(tray_value, nullptr);

  EXPECT_EQ(workspace_row->property("titleText").toString(), QStringLiteral("Workspace Count"));
  EXPECT_EQ(tray_row->property("titleText").toString(), QStringLiteral("System Tray Max Items"));
  EXPECT_FALSE(workspace_row->property("stacked").toBool());
  EXPECT_FALSE(tray_row->property("stacked").toBool());
  EXPECT_EQ(workspace_row->property("controlItem").value<QObject*>(), workspace_controls);
  EXPECT_EQ(tray_row->property("controlItem").value<QObject*>(), tray_controls);
  EXPECT_EQ(workspace_slider->property("from").toDouble(), 3.0);
  EXPECT_EQ(workspace_slider->property("to").toDouble(), 10.0);
  EXPECT_EQ(workspace_slider->property("stepSize").toDouble(), 1.0);
  EXPECT_EQ(tray_slider->property("from").toDouble(), 2.0);
  EXPECT_EQ(tray_slider->property("to").toDouble(), 5.0);
  EXPECT_EQ(tray_slider->property("stepSize").toDouble(), 1.0);
  EXPECT_EQ(workspace_slider->property("value").toInt(), model.workspaceCount());
  EXPECT_EQ(tray_slider->property("value").toInt(), model.trayMaxItems());
  EXPECT_EQ(workspace_value->property("text").toInt(), model.workspaceCount());
  EXPECT_EQ(tray_value->property("text").toInt(), model.trayMaxItems());

  model.setWorkspaceCount(8);
  model.setTrayMaxItems(4);
  EXPECT_EQ(workspace_slider->property("value").toInt(), 8);
  EXPECT_EQ(tray_slider->property("value").toInt(), 4);
  EXPECT_EQ(workspace_value->property("text").toInt(), 8);
  EXPECT_EQ(tray_value->property("text").toInt(), 4);

  workspace_slider->setProperty("value", 6.4);
  ASSERT_TRUE(QMetaObject::invokeMethod(workspace_slider, "moved"));
  tray_slider->setProperty("value", 2.6);
  ASSERT_TRUE(QMetaObject::invokeMethod(tray_slider, "moved"));
  EXPECT_EQ(model.workspaceCount(), 6);
  EXPECT_EQ(model.trayMaxItems(), 3);

  EXPECT_TRUE(workspace_slider->property("activeFocusOnTab").toBool());
  EXPECT_TRUE(tray_slider->property("activeFocusOnTab").toBool());
  workspace_slider->forceActiveFocus();
  EXPECT_TRUE(workspace_slider->hasActiveFocus());
}

TEST(SettingsWeatherQmlTest, WeatherRowsPreserveControlsAndUpdateEditModel) {
  SettingsEditModel model;
  QQmlEngine engine;
  engine.rootContext()->setContextProperty(QStringLiteral("editModel"), &model);
  QQmlComponent component(&engine,
                          QUrl::fromLocalFile(QStringLiteral(TEST_SOURCE_DIR "/apps/settings/qml/WeatherPage.qml")));
  std::unique_ptr<QObject> page{
      component.createWithInitialProperties({{QStringLiteral("editModel"), QVariant::fromValue(&model)}})};
  ASSERT_NE(page, nullptr) << component.errorString().toStdString();

  auto* root_item = qobject_cast<QQuickItem*>(page.get());
  ASSERT_NE(root_item, nullptr);
  root_item->setSize(QSizeF(1000, 1200));
  QQuickWindow window;
  window.resize(1000, 800);
  root_item->setParentItem(window.contentItem());
  window.show();
  ASSERT_TRUE(QTest::qWaitForWindowExposed(&window));
  QCoreApplication::processEvents();

  auto* provider_frame = findVisualChild(root_item, QStringLiteral("providerSectionFrame"));
  auto* location_frame = findVisualChild(root_item, QStringLiteral("locationSectionFrame"));
  auto* units_frame = findVisualChild(root_item, QStringLiteral("unitsSectionFrame"));
  auto* display_frame = findVisualChild(root_item, QStringLiteral("displaySectionFrame"));
  ASSERT_NE(provider_frame, nullptr);
  ASSERT_NE(location_frame, nullptr);
  ASSERT_NE(units_frame, nullptr);
  ASSERT_NE(display_frame, nullptr);

  auto* provider_combo = findVisualChild(root_item, QStringLiteral("weatherProviderComboBox"));
  auto* location_combo = findVisualChild(root_item, QStringLiteral("weatherLocationSourceComboBox"));
  auto* city_field = findVisualChild(root_item, QStringLiteral("weatherCityTextField"));
  auto* temp_combo = findVisualChild(root_item, QStringLiteral("weatherTempUnitComboBox"));
  auto* wind_combo = findVisualChild(root_item, QStringLiteral("weatherWindUnitComboBox"));
  auto* pressure_combo = findVisualChild(root_item, QStringLiteral("weatherPressureUnitComboBox"));
  auto* bar_switch = findVisualChild(root_item, QStringLiteral("weatherShowInBarSwitch"));
  auto* compact_switch = findVisualChild(root_item, QStringLiteral("weatherCompactModeSwitch"));
  auto* feels_switch = findVisualChild(root_item, QStringLiteral("weatherShowFeelsLikeSwitch"));
  auto* location_switch = findVisualChild(root_item, QStringLiteral("weatherShowLocationSwitch"));
  auto* interval_combo = findVisualChild(root_item, QStringLiteral("weatherRefreshIntervalComboBox"));

  ASSERT_NE(provider_combo, nullptr);
  ASSERT_NE(location_combo, nullptr);
  ASSERT_NE(city_field, nullptr);
  ASSERT_NE(temp_combo, nullptr);
  ASSERT_NE(wind_combo, nullptr);
  ASSERT_NE(pressure_combo, nullptr);
  ASSERT_NE(bar_switch, nullptr);
  ASSERT_NE(compact_switch, nullptr);
  ASSERT_NE(feels_switch, nullptr);
  ASSERT_NE(location_switch, nullptr);
  ASSERT_NE(interval_combo, nullptr);

  EXPECT_EQ(model.weatherProvider(), QStringLiteral("open-meteo"));
  EXPECT_EQ(model.weatherLocationSource(), QStringLiteral("manual"));
  EXPECT_TRUE(model.weatherShowInBar());
  EXPECT_FALSE(model.weatherCompactMode());

  city_field->setProperty("text", QStringLiteral("Berlin, Germany"));
  EXPECT_EQ(model.weatherCity(), QStringLiteral("Berlin, Germany"));
  EXPECT_TRUE(model.isDirty());

  bar_switch->setProperty("checked", false);
  EXPECT_FALSE(model.weatherShowInBar());

  compact_switch->setProperty("checked", true);
  EXPECT_TRUE(model.weatherCompactMode());
}

TEST(SettingsFooterQmlTest, ShellStatusUsesSharedSemanticIndicator) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());
  ScopedXdgConfigHome config_home(temp_dir.path());

  SettingsEditModel model;
  ConfigFileService file_service(&model);
  ShellStatusService shell_status;
  QQmlEngine engine;
  engine.rootContext()->setContextProperty(QStringLiteral("editModel"), &model);
  engine.rootContext()->setContextProperty(QStringLiteral("fileService"), &file_service);
  engine.rootContext()->setContextProperty(QStringLiteral("shellStatus"), &shell_status);
  engine.rootContext()->setContextProperty(QStringLiteral("appVersion"), QStringLiteral("0.0.0-test"));
  QQmlComponent component(&engine,
                          QUrl::fromLocalFile(QStringLiteral(TEST_SOURCE_DIR "/apps/settings/qml/FooterBar.qml")));
  std::unique_ptr<QObject> footer{component.createWithInitialProperties({
      {QStringLiteral("editModel"), QVariant::fromValue(&model)},
      {QStringLiteral("fileService"), QVariant::fromValue(&file_service)},
      {QStringLiteral("shellStatus"), QVariant::fromValue(&shell_status)},
      {QStringLiteral("appVersion"), QStringLiteral("0.0.0-test")},
  })};
  ASSERT_NE(footer, nullptr) << component.errorString().toStdString();

  auto* root_item = qobject_cast<QQuickItem*>(footer.get());
  ASSERT_NE(root_item, nullptr);
  root_item->setWidth(1000);
  EXPECT_EQ(root_item->property("implicitHeight").toReal(), 56.0);
  const Holonight::ColorTokens tokens = Holonight::tokensForScheme(Holonight::ThemeSchemeKind::HoloNightDark);
  EXPECT_EQ(root_item->property("color").value<QColor>(), tokens.surfaceRaised);
  auto* indicator = findVisualChild(root_item, QStringLiteral("shellStatusIndicator"));
  ASSERT_NE(indicator, nullptr);

  EXPECT_EQ(indicator->property("text").toString(), shell_status.statusText());
  EXPECT_TRUE(indicator->property("dotVisible").toBool());
  EXPECT_EQ(indicator->property("status").toInt(), shell_status.shellRunning() ? 2 : 3);
}

TEST(SettingsFooterQmlTest, ActionBarPreservesActionsAndTrailingAlignment) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());
  ScopedXdgConfigHome config_home(temp_dir.path());

  SettingsEditModel model;
  ConfigFileService file_service(&model);
  ShellStatusService shell_status;
  QQmlEngine engine;
  engine.rootContext()->setContextProperty(QStringLiteral("editModel"), &model);
  engine.rootContext()->setContextProperty(QStringLiteral("fileService"), &file_service);
  engine.rootContext()->setContextProperty(QStringLiteral("shellStatus"), &shell_status);
  engine.rootContext()->setContextProperty(QStringLiteral("appVersion"), QStringLiteral("0.0.0-test"));
  QQmlComponent component(&engine,
                          QUrl::fromLocalFile(QStringLiteral(TEST_SOURCE_DIR "/apps/settings/qml/FooterBar.qml")));
  std::unique_ptr<QObject> footer{component.createWithInitialProperties({
      {QStringLiteral("editModel"), QVariant::fromValue(&model)},
      {QStringLiteral("fileService"), QVariant::fromValue(&file_service)},
      {QStringLiteral("shellStatus"), QVariant::fromValue(&shell_status)},
      {QStringLiteral("appVersion"), QStringLiteral("0.0.0-test")},
  })};
  ASSERT_NE(footer, nullptr) << component.errorString().toStdString();

  auto* root_item = qobject_cast<QQuickItem*>(footer.get());
  ASSERT_NE(root_item, nullptr);
  root_item->setSize(QSizeF(1000, 56));
  QQuickWindow window;
  window.resize(1000, 56);
  root_item->setParentItem(window.contentItem());
  window.show();
  ASSERT_TRUE(QTest::qWaitForWindowExposed(&window));

  auto* action_bar = findVisualChild(root_item, QStringLiteral("footerActionBar"));
  auto* discard_button = findVisualChild(root_item, QStringLiteral("discardChangesButton"));
  auto* apply_button = findVisualChild(root_item, QStringLiteral("applyButton"));
  auto* save_and_apply_button = findVisualChild(root_item, QStringLiteral("saveAndApplyButton"));
  ASSERT_NE(action_bar, nullptr);
  ASSERT_NE(discard_button, nullptr);
  ASSERT_NE(apply_button, nullptr);
  ASSERT_NE(save_and_apply_button, nullptr);

  EXPECT_FALSE(discard_button->isEnabled());
  EXPECT_FALSE(apply_button->isEnabled());
  EXPECT_FALSE(save_and_apply_button->isEnabled());

  model.setThemeMode(QStringLiteral("light"));
  EXPECT_TRUE(discard_button->isEnabled());
  EXPECT_TRUE(apply_button->isEnabled());
  EXPECT_TRUE(save_and_apply_button->isEnabled());
  EXPECT_TRUE(discard_button->property("activeFocusOnTab").toBool());
  EXPECT_TRUE(apply_button->property("activeFocusOnTab").toBool());
  EXPECT_TRUE(save_and_apply_button->property("activeFocusOnTab").toBool());
  EXPECT_TRUE(discard_button->property("foregroundColor").isValid());
  EXPECT_TRUE(apply_button->property("foregroundColor").isValid());
  EXPECT_TRUE(save_and_apply_button->property("foregroundColor").isValid());
  EXPECT_FALSE(discard_button->property("highlighted").toBool());
  EXPECT_TRUE(apply_button->property("highlighted").toBool());
  EXPECT_TRUE(save_and_apply_button->property("highlighted").toBool());

  const QPointF trailing_edge = save_and_apply_button->mapToItem(root_item, QPointF(save_and_apply_button->width(), 0));
  EXPECT_NEAR(trailing_edge.x(), 984.0, 1.0);

  discard_button->forceActiveFocus();
  EXPECT_TRUE(discard_button->hasActiveFocus());
  QTest::keyClick(&window, Qt::Key_Space);
  EXPECT_FALSE(model.isDirty());
}

TEST(SettingsWindowQmlTest, NavigationWidthFollowsTheNonElidingTitleWithDocumentedMinimum) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());
  ScopedXdgConfigHome config_home(temp_dir.path());

  SettingsEditModel model;
  ConfigFileService file_service(&model);
  ShellStatusService shell_status;
  QQmlEngine engine;
  QQmlComponent component(&engine,
                          QUrl::fromLocalFile(QStringLiteral(TEST_SOURCE_DIR "/apps/settings/qml/SettingsWindow.qml")));
  std::unique_ptr<QObject> window_content{component.createWithInitialProperties({
      {QStringLiteral("editModel"), QVariant::fromValue(&model)},
      {QStringLiteral("fileService"), QVariant::fromValue(&file_service)},
      {QStringLiteral("shellStatus"), QVariant::fromValue(&shell_status)},
      {QStringLiteral("appVersion"), QStringLiteral("0.0.0-test")},
  })};
  ASSERT_NE(window_content, nullptr) << component.errorString().toStdString();

  auto* window = qobject_cast<QQuickWindow*>(window_content.get());
  ASSERT_NE(window, nullptr);
  window->resize(1600, 800);
  window->show();
  ASSERT_TRUE(QTest::qWaitForWindowExposed(window));
  EXPECT_EQ(window->minimumWidth(), 1244);
  EXPECT_EQ(window->minimumHeight(), 480);
  EXPECT_EQ(window->title(), QStringLiteral("HoloNight Settings"));
  auto* root_item = window->contentItem();

  auto* nav_frame = findVisualChild(root_item, QStringLiteral("settingsNavFrame"));
  auto* nav_panel = findVisualChild(root_item, QStringLiteral("settingsNavPanel"));
  auto* application_label = findVisualChild(root_item, QStringLiteral("applicationLabel"));
  ASSERT_NE(nav_frame, nullptr);
  ASSERT_NE(nav_panel, nullptr);
  ASSERT_NE(application_label, nullptr);
  EXPECT_GE(nav_frame->width(), 280.0);
  EXPECT_LE(application_label->property("contentWidth").toReal(), application_label->width());

  nav_panel->setProperty("applicationName", QStringLiteral("Localized Settings Application"));
  QTRY_VERIFY(nav_frame->width() >= nav_panel->property("minimumContentWidth").toReal());
  EXPECT_LE(application_label->property("contentWidth").toReal(), application_label->width());
}

TEST(SettingsWindowQmlTest, UsesUnifiedHeadersAndTracksTheSelectedPage) {
  static const int registration = qmlRegisterType<FontListModel>("HolonightSettings", 1, 0, "FontListModel");
  Q_UNUSED(registration);
  static const int swatch_tokens_registration = qmlRegisterSingletonType<ThemeSwatchTokens>(
      "HolonightSettings", 1, 0, "ThemeSwatchTokens",
      [](QQmlEngine*, QJSEngine*) -> QObject* { return new ThemeSwatchTokens(); });
  Q_UNUSED(swatch_tokens_registration);

  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());
  ScopedXdgConfigHome config_home(temp_dir.path());

  SettingsEditModel model;
  ConfigFileService file_service(&model);
  ShellStatusService shell_status;
  QQmlEngine engine;
  QQmlComponent component(&engine,
                          QUrl::fromLocalFile(QStringLiteral(TEST_SOURCE_DIR "/apps/settings/qml/SettingsWindow.qml")));
  std::unique_ptr<QObject> window_content{component.createWithInitialProperties({
      {QStringLiteral("editModel"), QVariant::fromValue(&model)},
      {QStringLiteral("fileService"), QVariant::fromValue(&file_service)},
      {QStringLiteral("shellStatus"), QVariant::fromValue(&shell_status)},
      {QStringLiteral("appVersion"), QStringLiteral("0.0.0-test")},
  })};
  ASSERT_NE(window_content, nullptr) << component.errorString().toStdString();

  auto* window = qobject_cast<QQuickWindow*>(window_content.get());
  ASSERT_NE(window, nullptr);
  window->show();
  ASSERT_TRUE(QTest::qWaitForWindowExposed(window));
  auto* root_item = window->contentItem();

  auto* nav_header = findVisualChild(root_item, QStringLiteral("settingsNavHeader"));
  auto* content_header = findVisualChild(root_item, QStringLiteral("settingsContentHeader"));
  auto* preview_header = findVisualChild(root_item, QStringLiteral("settingsPreviewHeader"));
  auto* app_title = findVisualChild(root_item, QStringLiteral("settingsAppTitle"));
  auto* content_title = findVisualChild(root_item, QStringLiteral("settingsContentHeaderTitle"));
  auto* preview_title = findVisualChild(root_item, QStringLiteral("settingsPreviewHeaderTitle"));
  auto* preview_placeholder = findVisualChild(root_item, QStringLiteral("previewPlaceholderLabel"));
  auto* nav_divider = findVisualChild(nav_header, QStringLiteral("headerDivider"));
  ASSERT_NE(nav_header, nullptr);
  ASSERT_NE(content_header, nullptr);
  ASSERT_NE(preview_header, nullptr);
  ASSERT_NE(app_title, nullptr);
  ASSERT_NE(content_title, nullptr);
  ASSERT_NE(preview_title, nullptr);
  ASSERT_NE(preview_placeholder, nullptr);
  ASSERT_NE(nav_divider, nullptr);

  EXPECT_EQ(nav_header->height(), content_header->height());
  EXPECT_EQ(content_header->height(), preview_header->height());
  EXPECT_EQ(nav_header->mapToItem(root_item, QPointF{}).y(), content_header->mapToItem(root_item, QPointF{}).y());
  EXPECT_EQ(content_header->mapToItem(root_item, QPointF{}).y(), preview_header->mapToItem(root_item, QPointF{}).y());
  EXPECT_EQ(nav_header->height(), nav_header->implicitHeight());
  EXPECT_FALSE(nav_divider->isVisible());
  EXPECT_EQ(app_title->property("applicationName").toString(), QStringLiteral("Settings"));
  EXPECT_EQ(app_title->property("iconSource").toUrl(),
            QUrl(QStringLiteral("qrc:/HolonightSettings/holonight-settings.svg")));
  EXPECT_EQ(content_title->property("title").toString(), QStringLiteral("Appearance"));
  EXPECT_EQ(preview_title->property("title").toString(), QStringLiteral("Preview"));
  EXPECT_EQ(preview_placeholder->property("text").toString(), QStringLiteral("Not yet implemented"));
  QTRY_VERIFY(findVisualChild(root_item, QStringLiteral("themeSectionFrame")) != nullptr);
  EXPECT_EQ(findVisualChild(root_item, QStringLiteral("appearancePageHeader")), nullptr);

  window_content->setProperty("currentPage", QStringLiteral("bar"));
  QTRY_COMPARE(content_title->property("title").toString(), QStringLiteral("Bar"));
  QTRY_VERIFY(findVisualChild(root_item, QStringLiteral("workspaceCountRow")) != nullptr);

  window_content->setProperty("currentPage", QStringLiteral("sidebar"));
  QTRY_COMPARE(content_title->property("title").toString(), QStringLiteral("Sidebar"));
  QTRY_VERIFY(findVisualChild(root_item, QStringLiteral("settingsPlaceholderLabel")) != nullptr);
  auto* deferred_placeholder = findVisualChild(root_item, QStringLiteral("settingsPlaceholderLabel"));
  ASSERT_NE(deferred_placeholder, nullptr);
  EXPECT_EQ(deferred_placeholder->property("text").toString(), QStringLiteral("Not yet implemented"));
}

TEST(SettingsNavPanelQmlTest, PreservesPageOrderAndRoutesActivation) {
  QQmlEngine engine;
  QQmlComponent component(&engine,
                          QUrl::fromLocalFile(QStringLiteral(TEST_SOURCE_DIR "/apps/settings/qml/NavPanel.qml")));
  std::unique_ptr<QObject> panel{
      component.createWithInitialProperties({{QStringLiteral("currentPage"), QStringLiteral("appearance")}})};
  ASSERT_NE(panel, nullptr) << component.errorString().toStdString();

  const QVariantList pages = panel->property("pages").toList();
  ASSERT_EQ(pages.size(), 13);
  EXPECT_EQ(pages.front().toMap().value(QStringLiteral("key")).toString(), QStringLiteral("appearance"));
  EXPECT_EQ(pages.at(1).toMap().value(QStringLiteral("key")).toString(), QStringLiteral("bar"));
  EXPECT_EQ(pages.back().toMap().value(QStringLiteral("key")).toString(), QStringLiteral("about"));

  auto* root_item = qobject_cast<QQuickItem*>(panel.get());
  ASSERT_NE(root_item, nullptr);
  root_item->setSize(QSizeF(200, 800));
  QQuickWindow window;
  window.resize(200, 800);
  root_item->setParentItem(window.contentItem());
  window.show();
  ASSERT_TRUE(QTest::qWaitForWindowExposed(&window));
  QSignalSpy requested_spy(panel.get(), SIGNAL(pageRequested(QString)));

  auto* app_title = findVisualChild(root_item, QStringLiteral("settingsAppTitle"));
  auto* appearance_delegate = findVisualChild(root_item, QStringLiteral("navDelegate-appearance"));
  auto* bar_delegate = findVisualChild(root_item, QStringLiteral("navDelegate-bar"));
  ASSERT_NE(app_title, nullptr);
  ASSERT_NE(appearance_delegate, nullptr);
  ASSERT_NE(bar_delegate, nullptr);
  EXPECT_EQ(app_title->property("applicationName").toString(), QStringLiteral("Settings"));
  EXPECT_EQ(app_title->property("iconSource").toUrl(),
            QUrl(QStringLiteral("qrc:/HolonightSettings/holonight-settings.svg")));
  EXPECT_EQ(appearance_delegate->property("resolvedSizeRole").toInt(), 2);
  EXPECT_GE(appearance_delegate->implicitHeight(), 40.0);

  const QPointF bar_center =
      bar_delegate->mapToItem(window.contentItem(), QPointF(bar_delegate->width() / 2, bar_delegate->height() / 2));
  QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, bar_center.toPoint());
  ASSERT_EQ(requested_spy.count(), 1);
  EXPECT_EQ(requested_spy.first().first().toString(), QStringLiteral("bar"));

  EXPECT_TRUE(appearance_delegate->property("checked").toBool());
  EXPECT_TRUE(appearance_delegate->property("selected").toBool());
  EXPECT_FALSE(bar_delegate->property("checked").toBool());
  EXPECT_FALSE(bar_delegate->property("selected").toBool());

  panel->setProperty("currentPage", QStringLiteral("bar"));
  EXPECT_FALSE(appearance_delegate->property("checked").toBool());
  EXPECT_FALSE(appearance_delegate->property("selected").toBool());
  EXPECT_TRUE(bar_delegate->property("checked").toBool());
  EXPECT_TRUE(bar_delegate->property("selected").toBool());

  appearance_delegate->forceActiveFocus();
  EXPECT_TRUE(appearance_delegate->hasActiveFocus());
  QTest::keyClick(&window, Qt::Key_Space);
  ASSERT_EQ(requested_spy.count(), 2);
  EXPECT_EQ(requested_spy.at(1).first().toString(), QStringLiteral("appearance"));

  bar_delegate->setEnabled(false);
  QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, bar_center.toPoint());
  EXPECT_EQ(requested_spy.count(), 2);
}

TEST(SettingsNavPanelQmlTest, KeepsEveryPageReachableAtMinimumWindowHeight) {
  QQmlEngine engine;
  QQmlComponent component(&engine,
                          QUrl::fromLocalFile(QStringLiteral(TEST_SOURCE_DIR "/apps/settings/qml/NavPanel.qml")));
  std::unique_ptr<QObject> panel{
      component.createWithInitialProperties({{QStringLiteral("currentPage"), QStringLiteral("appearance")}})};
  ASSERT_NE(panel, nullptr) << component.errorString().toStdString();

  auto* root_item = qobject_cast<QQuickItem*>(panel.get());
  ASSERT_NE(root_item, nullptr);
  root_item->setSize(QSizeF(184, 392));
  QQuickWindow window;
  window.resize(184, 392);
  root_item->setParentItem(window.contentItem());
  window.show();
  ASSERT_TRUE(QTest::qWaitForWindowExposed(&window));

  auto* page_list = findVisualChild(root_item, QStringLiteral("navPageList"));
  ASSERT_NE(page_list, nullptr);
  QTRY_VERIFY(page_list->property("contentHeight").toReal() > page_list->height());

  const qreal maximum_content_y = page_list->property("contentHeight").toReal() - page_list->height();
  page_list->setProperty("contentY", maximum_content_y);
  QTRY_COMPARE(page_list->property("contentY").toReal(), maximum_content_y);

  auto* about_delegate = findVisualChild(root_item, QStringLiteral("navDelegate-about"));
  ASSERT_NE(about_delegate, nullptr);
  const QRectF about_bounds(about_delegate->mapToItem(page_list, QPointF(0, 0)), about_delegate->size());
  EXPECT_GT(about_bounds.bottom(), 0.0);
  EXPECT_LE(about_bounds.bottom(), page_list->height());
}

TEST(SettingsContentStackQmlTest, ShowsInitialPageWithoutAnimationAndIgnoresSamePage) {
  SettingsEditModel model;
  QQmlEngine engine;
  QQmlComponent component(&engine,
                          QUrl::fromLocalFile(QStringLiteral(TEST_SOURCE_DIR "/apps/settings/qml/ContentStack.qml")));
  std::unique_ptr<QObject> content{
      component.createWithInitialProperties({{QStringLiteral("editModel"), QVariant::fromValue(&model)},
                                             {QStringLiteral("currentPage"), QStringLiteral("sidebar")},
                                             {QStringLiteral("currentPageTitle"), QStringLiteral("Sidebar")}})};
  ASSERT_NE(content, nullptr) << component.errorString().toStdString();

  auto* root_item = qobject_cast<QQuickItem*>(content.get());
  ASSERT_NE(root_item, nullptr);
  auto* stack = findVisualChild(root_item, QStringLiteral("settingsContentStack"));
  ASSERT_NE(stack, nullptr);
  EXPECT_EQ(stack->property("depth").toInt(), 1);
  EXPECT_FALSE(stack->property("busy").toBool());

  auto* initial_page = qvariant_cast<QObject*>(stack->property("currentItem"));
  ASSERT_NE(initial_page, nullptr);
  EXPECT_EQ(initial_page->objectName(), QStringLiteral("contentPage-sidebar"));

  ASSERT_TRUE(QMetaObject::invokeMethod(content.get(), "showPage", Q_ARG(QVariant, QStringLiteral("sidebar"))));
  EXPECT_EQ(qvariant_cast<QObject*>(stack->property("currentItem")), initial_page);
  EXPECT_EQ(stack->property("depth").toInt(), 1);
  EXPECT_FALSE(stack->property("busy").toBool());
}

TEST(SettingsContentStackQmlTest, ReplacesPagesInNavigationDirectionAndKeepsDepthOne) {
  SettingsEditModel model;
  QQmlEngine engine;
  QQmlComponent component(&engine,
                          QUrl::fromLocalFile(QStringLiteral(TEST_SOURCE_DIR "/apps/settings/qml/ContentStack.qml")));
  std::unique_ptr<QObject> content{
      component.createWithInitialProperties({{QStringLiteral("editModel"), QVariant::fromValue(&model)},
                                             {QStringLiteral("currentPage"), QStringLiteral("sidebar")},
                                             {QStringLiteral("currentPageTitle"), QStringLiteral("Sidebar")}})};
  ASSERT_NE(content, nullptr) << component.errorString().toStdString();

  auto* root_item = qobject_cast<QQuickItem*>(content.get());
  ASSERT_NE(root_item, nullptr);
  root_item->setSize(QSizeF(640, 480));
  QQuickWindow window;
  window.resize(640, 480);
  root_item->setParentItem(window.contentItem());
  window.show();
  ASSERT_TRUE(QTest::qWaitForWindowExposed(&window));

  auto* stack = findVisualChild(root_item, QStringLiteral("settingsContentStack"));
  ASSERT_NE(stack, nullptr);
  content->setProperty("currentPage", QStringLiteral("launcher"));
  EXPECT_EQ(content->property("transitionDirection").toInt(), 1);
  QTRY_VERIFY(!stack->property("busy").toBool());
  EXPECT_EQ(stack->property("depth").toInt(), 1);
  auto* launcher_page = qobject_cast<QQuickItem*>(qvariant_cast<QObject*>(stack->property("currentItem")));
  ASSERT_NE(launcher_page, nullptr);
  EXPECT_EQ(launcher_page->objectName(), QStringLiteral("contentPage-launcher"));
  EXPECT_TRUE(launcher_page->isVisible());
  EXPECT_EQ(launcher_page->opacity(), 1.0);
  EXPECT_GT(launcher_page->width(), 0.0);
  EXPECT_GT(launcher_page->height(), 0.0);

  content->setProperty("currentPage", QStringLiteral("sidebar"));
  EXPECT_EQ(content->property("transitionDirection").toInt(), -1);
  QTRY_VERIFY(!stack->property("busy").toBool());
  EXPECT_EQ(stack->property("depth").toInt(), 1);
  auto* sidebar_page = qobject_cast<QQuickItem*>(qvariant_cast<QObject*>(stack->property("currentItem")));
  ASSERT_NE(sidebar_page, nullptr);
  EXPECT_EQ(sidebar_page->objectName(), QStringLiteral("contentPage-sidebar"));
  EXPECT_TRUE(sidebar_page->isVisible());
  EXPECT_EQ(sidebar_page->opacity(), 1.0);
  EXPECT_EQ(sidebar_page->y(), 0.0);
}

TEST(SettingsContentStackQmlTest, RetainsOnlyLatestRequestDuringTransition) {
  SettingsEditModel model;
  QQmlEngine engine;
  QQmlComponent component(&engine,
                          QUrl::fromLocalFile(QStringLiteral(TEST_SOURCE_DIR "/apps/settings/qml/ContentStack.qml")));
  std::unique_ptr<QObject> content{
      component.createWithInitialProperties({{QStringLiteral("editModel"), QVariant::fromValue(&model)},
                                             {QStringLiteral("currentPage"), QStringLiteral("sidebar")},
                                             {QStringLiteral("currentPageTitle"), QStringLiteral("Sidebar")}})};
  ASSERT_NE(content, nullptr) << component.errorString().toStdString();

  auto* root_item = qobject_cast<QQuickItem*>(content.get());
  ASSERT_NE(root_item, nullptr);
  root_item->setSize(QSizeF(640, 480));
  QQuickWindow window;
  window.resize(640, 480);
  root_item->setParentItem(window.contentItem());
  window.show();
  ASSERT_TRUE(QTest::qWaitForWindowExposed(&window));

  auto* stack = findVisualChild(root_item, QStringLiteral("settingsContentStack"));
  ASSERT_NE(stack, nullptr);
  content->setProperty("currentPage", QStringLiteral("launcher"));
  ASSERT_TRUE(stack->property("busy").toBool());

  content->setProperty("currentPage", QStringLiteral("weather"));
  content->setProperty("currentPage", QStringLiteral("advanced"));

  QTRY_VERIFY(!stack->property("busy").toBool());
  EXPECT_EQ(stack->property("depth").toInt(), 1);
  EXPECT_EQ(content->property("displayedPage").toString(), QStringLiteral("advanced"));
  auto* advanced_page = qobject_cast<QQuickItem*>(qvariant_cast<QObject*>(stack->property("currentItem")));
  ASSERT_NE(advanced_page, nullptr);
  EXPECT_EQ(advanced_page->objectName(), QStringLiteral("contentPage-advanced"));
  EXPECT_TRUE(advanced_page->isVisible());
  EXPECT_EQ(advanced_page->opacity(), 1.0);
  EXPECT_EQ(advanced_page->y(), 0.0);
}

TEST(ConfigFileServiceTest, LoadDiscardsUnsavedChangesFromDisk) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());
  const ScopedXdgConfigHome xdg(temp_dir.path());

  SettingsEditModel model;
  ConfigFileService service(&model);
  ASSERT_TRUE(service.load());
  model.setWorkspaceCount(9);
  ASSERT_TRUE(service.save());

  model.setWorkspaceCount(3);
  ASSERT_TRUE(model.isDirty());

  ASSERT_TRUE(service.load());

  EXPECT_EQ(model.workspaceCount(), 9);
  EXPECT_FALSE(model.isDirty());
}

TEST(ConfigFileServiceTest, SavingUnrelatedSettingsPreservesDisabledTimeToEventWidget) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());
  const ScopedXdgConfigHome xdg(temp_dir.path());

  const QString path = ConfigFileService::configPath();
  QDir().mkpath(QFileInfo(path).absolutePath());
  QFile file{path};
  ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Text));
  file.write(R"toml(
[[widget]]
type = "time-to-event"
enabled = false
position = "center-center"
title = "Weekly Review"
deadline = "2026-12-25T14:00:00"
show_seconds = true
)toml");
  file.close();

  SettingsEditModel model;
  ConfigFileService service(&model);
  ASSERT_TRUE(service.load());

  model.setThemeScheme(QStringLiteral("holonight-light"));
  ASSERT_TRUE(service.save());

  const ParsedConfig saved = readConfig(path);
  ASSERT_EQ(saved.widgets.definitions.size(), 1);
  const WidgetDefinition& widget = saved.widgets.definitions.constFirst();
  EXPECT_FALSE(widget.enabled);
  EXPECT_EQ(widget.type, WidgetType::TimeToEvent);
  EXPECT_EQ(widget.time_to_event.title, QStringLiteral("Weekly Review"));
  EXPECT_EQ(widget.time_to_event.deadline, QDateTime(QDate(2026, 12, 25), QTime(14, 0)));
  EXPECT_TRUE(widget.time_to_event.has_time);
  EXPECT_TRUE(widget.time_to_event.show_seconds);
}

TEST(ConfigFileServiceTest, LoadReadsDerivedModeFromThemeConfScheme) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());
  const ScopedXdgConfigHome xdg(temp_dir.path());

  ASSERT_TRUE(ThemeConfigFile::writeAppearance(
      ThemeConfigFile::Appearance{.scheme = QStringLiteral("holonight-day"), .accent = QStringLiteral("violet")}));

  SettingsEditModel model;
  ConfigFileService service(&model);
  ASSERT_TRUE(service.load());

  EXPECT_EQ(model.themeMode(), QStringLiteral("light"));
  EXPECT_EQ(model.themeScheme(), QStringLiteral("holonight-day"));
  EXPECT_EQ(model.themeAccent(), QStringLiteral("violet"));
  EXPECT_FALSE(model.isDirty());
}

TEST(ConfigFileServiceTest, LoadIgnoresThemeAndTypographyFromToml) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());
  const ScopedXdgConfigHome xdg(temp_dir.path());

  const QString path = ConfigFileService::configPath();
  QDir().mkpath(QFileInfo(path).absolutePath());
  QFile file{path};
  ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Text));
  file.write(
      "[theme]\nmode = \"light\"\n[appearance]\nui_font = \"Toml Sans\"\nui_font_size = 18\n"
      "fixed_font = \"Toml Mono\"\nfixed_font_size = 17\n");
  file.close();

  QSettings theme_settings{ConfigFileService::themeConfigPath(), QSettings::IniFormat};
  theme_settings.setValue(QStringLiteral("appearance/scheme"), QStringLiteral("holonight-dark"));
  theme_settings.setValue(QStringLiteral("fonts/ui"), QStringLiteral("Theme Sans"));
  theme_settings.setValue(QStringLiteral("fonts/baseSize"), 11);
  theme_settings.setValue(QStringLiteral("fonts/fixed"), QStringLiteral("Theme Mono"));
  theme_settings.setValue(QStringLiteral("fonts/fixedSize"), 12);
  theme_settings.sync();

  SettingsEditModel model;
  ConfigFileService service(&model);
  ASSERT_TRUE(service.load());

  EXPECT_EQ(model.themeMode(), QStringLiteral("dark"));
  EXPECT_EQ(model.themeScheme(), QStringLiteral("holonight-dark"));
  EXPECT_EQ(model.uiFont(), QStringLiteral("Theme Sans"));
  EXPECT_EQ(model.uiFontSize(), 11);
  EXPECT_EQ(model.fixedFont(), QStringLiteral("Theme Mono"));
  EXPECT_EQ(model.fixedFontSize(), 12);
  EXPECT_FALSE(model.isDirty());
}

TEST(SettingsEditModelTest, SchemeDrivesDerivedMode) {
  SettingsEditModel model;
  ParsedConfig config;
  model.setFromParsedConfig(config);
  model.setThemeAppearanceSnapshot(
      ThemeConfigFile::Appearance{.scheme = QStringLiteral("holonight-dark"), .accent = QStringLiteral("cyan")});

  model.setThemeScheme(QStringLiteral("holonight-light"));

  EXPECT_EQ(model.themeScheme(), QStringLiteral("holonight-light"));
  EXPECT_EQ(model.themeMode(), QStringLiteral("light"));
  EXPECT_TRUE(model.isDirty());
}

TEST(SettingsEditModelTest, CatppuccinSchemesDriveDerivedMode) {
  SettingsEditModel model;
  ParsedConfig config;
  model.setFromParsedConfig(config);

  model.setThemeScheme(QStringLiteral("holonight-latte"));
  EXPECT_EQ(model.themeScheme(), QStringLiteral("holonight-latte"));
  EXPECT_EQ(model.themeMode(), QStringLiteral("light"));

  model.setThemeScheme(QStringLiteral("holonight-mocha"));
  EXPECT_EQ(model.themeScheme(), QStringLiteral("holonight-mocha"));
  EXPECT_EQ(model.themeMode(), QStringLiteral("dark"));
}

TEST(ThemeConfigFileTest, AcceptsAllCatalogSchemesAndDefaultsInvalidAccent) {
  const QStringList schemes = {
      QStringLiteral("holonight-dark"),  QStringLiteral("holonight-light"), QStringLiteral("holonight-mocha"),
      QStringLiteral("holonight-latte"), QStringLiteral("holonight-storm"), QStringLiteral("holonight-day"),
      QStringLiteral("holonight-ember"), QStringLiteral("holonight-sol"),
  };

  for (const QString& scheme : schemes) {
    EXPECT_EQ(ThemeConfigFile::normalizeScheme(scheme), scheme);
  }

  EXPECT_EQ(ThemeConfigFile::normalizeAccent(QString{}), QStringLiteral("default"));
  EXPECT_EQ(ThemeConfigFile::normalizeAccent(QStringLiteral("magenta")), QStringLiteral("default"));
  EXPECT_EQ(ThemeConfigFile::normalizeAccent(QStringLiteral("DEFAULT")), QStringLiteral("default"));
}

TEST(ThemeConfigFileTest, InvalidSchemeFallsBackFromLegacyMode) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());
  const ScopedXdgConfigHome xdg(temp_dir.path());

  QSettings settings{ThemeConfigFile::path(), QSettings::IniFormat};
  settings.setValue(QStringLiteral("appearance/scheme"), QStringLiteral("not-real"));
  settings.setValue(QStringLiteral("appearance/mode"), QStringLiteral("light"));
  settings.setValue(QStringLiteral("appearance/accent"), QStringLiteral("blue"));
  settings.sync();

  const ThemeConfigFile::Appearance appearance = ThemeConfigFile::loadAppearance();

  EXPECT_EQ(appearance.scheme, QStringLiteral("holonight-light"));
  EXPECT_EQ(appearance.mode, QStringLiteral("light"));
  EXPECT_EQ(appearance.accent, QStringLiteral("blue"));
}

TEST(ShellStatusServiceTest, StatusTextMatchesDetectedState) {
  ShellStatusService service;

  EXPECT_FALSE(service.statusText().isEmpty());
  if (service.shellRunning()) {
    EXPECT_EQ(service.statusText(), QStringLiteral("Shell is running"));
  } else {
    EXPECT_EQ(service.statusText(), QStringLiteral("Shell is not running"));
  }
}

TEST(FontListModelTest, RoleNamesExposeQmlContract) {
  FontListModel model;
  const QHash<int, QByteArray> roles = model.roleNames();

  EXPECT_EQ(roles.value(Qt::DisplayRole), "display");
  EXPECT_EQ(roles.size(), 1);
}

TEST(FontListModelTest, FixedPitchFilterRestoresCompleteCachedFamilyList) {
  FontListModel model;
  const int all_family_count = model.rowCount();

  model.setFixedPitchOnly(true);

  EXPECT_TRUE(model.fixedPitchOnly());
  EXPECT_LE(model.rowCount(), all_family_count);

  model.setFixedPitchOnly(false);

  EXPECT_FALSE(model.fixedPitchOnly());
  EXPECT_EQ(model.rowCount(), all_family_count);
}
