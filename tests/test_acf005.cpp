#include "AppearanceEditModel.h"
#include "AppearanceFileService.h"
#include "FontListModel.h"
#include "SettingsSaveCoordinator.h"
#include "ShellConfigFileService.h"
#include "ShellSettingsEditModel.h"

#include <QFile>
#include <QTemporaryDir>

#include <gtest/gtest.h>

TEST(AppearanceEditModelTest, UsesCanonicalDefaultsAndRanges) {
  AppearanceEditModel model;
  EXPECT_EQ(model.themeScheme(), QStringLiteral("holonight-dark"));
  EXPECT_EQ(model.uiFontSize(), 12);
  model.setUiFontSize(2);
  EXPECT_EQ(model.uiFontSize(), 6);
  model.setLayoutScale(9.0);
  EXPECT_DOUBLE_EQ(model.layoutScale(), 3.0);
  model.setShapeScale(0.1);
  EXPECT_DOUBLE_EQ(model.shapeScale(), 0.25);
  EXPECT_TRUE(model.isDirty());
  model.markSaved();
  EXPECT_FALSE(model.isDirty());
}

TEST(AppearanceEditModelTest, OptionalShapeOverridesRoundTrip) {
  AppearanceEditModel model;
  EXPECT_FALSE(model.baseRadiusEnabled());
  model.setBaseRadiusEnabled(true);
  model.setBaseRadius(200.0);
  EXPECT_TRUE(model.value().shape.base_radius.has_value());
  EXPECT_DOUBLE_EQ(*model.value().shape.base_radius, 128.0);
  model.setBaseRadiusEnabled(false);
  EXPECT_FALSE(model.value().shape.base_radius.has_value());
}

TEST(FontListModelTest, RetainsUnavailableConfiguredFamily) {
  FontListModel model;
  const QString unavailable = QStringLiteral("HoloNight Definitely Missing Font");
  model.setRetainedFamily(unavailable);
  EXPECT_GE(model.indexOf(unavailable), 0);
}

TEST(AppearanceFileServiceTest, MissingLoadsDefaultsWithoutCreatingFileOrDirtying) {
  QTemporaryDir directory;
  AppearanceEditModel model;
  const QString path = directory.filePath(QStringLiteral("appearance.toml"));
  AppearanceFileService service(&model, path);
  EXPECT_TRUE(service.load());
  EXPECT_FALSE(QFile::exists(path));
  EXPECT_FALSE(model.isDirty());
}

TEST(AppearanceFileServiceTest, DetectsExternalIdentityChangeAndRequiresStableOverwrite) {
  QTemporaryDir directory;
  AppearanceEditModel model;
  const QString path = directory.filePath(QStringLiteral("appearance.toml"));
  AppearanceFileService service(&model, path);
  ASSERT_TRUE(service.load());
  model.setThemeAccent(QStringLiteral("cyan"));
  QFile external(path);
  ASSERT_TRUE(external.open(QIODevice::WriteOnly));
  external.write("external");
  external.close();
  EXPECT_EQ(service.save(), AppearanceFileService::SaveResult::Conflict);
  ASSERT_TRUE(external.open(QIODevice::WriteOnly));
  external.write("changed again");
  external.close();
  EXPECT_EQ(service.save(true), AppearanceFileService::SaveResult::Conflict);
  EXPECT_TRUE(model.isDirty());
}

TEST(ShellConfigFileServiceTest, ProductSaveDoesNotCreateAppearanceFile) {
  QTemporaryDir directory;
  ShellSettingsEditModel model;
  const QString product = directory.filePath(QStringLiteral("config.toml"));
  ShellConfigFileService service(&model, product);
  ASSERT_TRUE(service.load());
  model.setworkspaceCount(7);
  EXPECT_EQ(service.save(), ShellConfigFileService::SaveResult::Success);
  EXPECT_TRUE(QFile::exists(product));
  EXPECT_FALSE(QFile::exists(directory.filePath(QStringLiteral("appearance.toml"))));
}
