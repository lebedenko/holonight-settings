#include "AppearanceEditModel.h"
#include "AppearanceFileService.h"
#include "FontListModel.h"
#include "SettingsSaveCoordinator.h"
#include "ShellConfigFileService.h"
#include "ShellSettingsEditModel.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>

#include <gtest/gtest.h>
#include <holonight/config/store.h>
#include <holonight_shell_config/config_writer.h>

namespace {

QByteArray fileHash(const QString& path) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    return {};
  }
  return QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha256);
}

void writeBytes(const QString& path, const QByteArray& contents) {
  QFile file(path);
  ASSERT_TRUE(file.open(QIODevice::WriteOnly));
  ASSERT_EQ(file.write(contents), contents.size());
}

}  // namespace

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
  QSignalSpy radius_enabled_spy(&model, &AppearanceEditModel::baseRadiusEnabledChanged);
  EXPECT_FALSE(model.baseRadiusEnabled());
  model.setBaseRadiusEnabled(true);
  model.setBaseRadius(200.0);
  EXPECT_TRUE(model.value().shape.base_radius.has_value());
  EXPECT_DOUBLE_EQ(*model.value().shape.base_radius, 128.0);
  EXPECT_EQ(radius_enabled_spy.count(), 1);
  model.setBaseRadiusEnabled(false);
  EXPECT_FALSE(model.value().shape.base_radius.has_value());
}

TEST(AppearanceEditModelTest, SettingOptionalExtentEnablesItAndNotifies) {
  AppearanceEditModel model;
  QSignalSpy chamfer_enabled_spy(&model, &AppearanceEditModel::baseChamferEnabledChanged);

  model.setBaseChamfer(12.0);

  EXPECT_TRUE(model.baseChamferEnabled());
  EXPECT_DOUBLE_EQ(model.baseChamfer(), 12.0);
  EXPECT_EQ(chamfer_enabled_spy.count(), 1);
}

TEST(AppearanceEditModelTest, ConvertsEveryCanonicalSelectionWithoutPersistingMode) {
  AppearanceEditModel model;
  model.setThemeMode(QStringLiteral("light"));
  model.setThemeAccent(QStringLiteral("violet"));
  model.setUiFont(QStringLiteral("UI Test"));
  model.setUiFontSize(48);
  model.setMonospaceFont(QStringLiteral("Mono Test"));
  model.setMonospaceFontSize(6);
  model.setTitleFont(QStringLiteral("Title Test"));
  model.setTitleFontSize(19);
  model.setDisplayFont(QStringLiteral("Display Test"));
  model.setDisplayFontSize(31);
  model.setIconTheme(QStringLiteral("Icons Test"));
  model.setFallbackIconTheme(QStringLiteral("Fallback Test"));
  model.setCursorTheme(QStringLiteral("Cursor Test"));
  model.setLayoutScale(0.5);
  model.setShapeStyle(QStringLiteral("chamfered"));
  model.setShapeScale(4.0);
  model.setBaseRadius(128.0);
  model.setBaseChamfer(64.0);

  const auto& value = model.value();
  EXPECT_EQ(model.themeMode(), QStringLiteral("light"));
  EXPECT_EQ(value.theme.accent, "violet");
  EXPECT_EQ(value.typography.ui_family, "UI Test");
  EXPECT_EQ(value.typography.ui_size, 48);
  EXPECT_EQ(value.typography.monospace_family, "Mono Test");
  EXPECT_EQ(value.typography.monospace_size, 6);
  EXPECT_EQ(value.typography.title_family, "Title Test");
  EXPECT_EQ(value.typography.title_size, 19);
  EXPECT_EQ(value.typography.display_family, "Display Test");
  EXPECT_EQ(value.typography.display_size, 31);
  EXPECT_EQ(value.icons.theme, "Icons Test");
  EXPECT_EQ(value.icons.fallback, "Fallback Test");
  EXPECT_EQ(value.icons.cursor, "Cursor Test");
  EXPECT_DOUBLE_EQ(value.layout.scale, 0.5);
  EXPECT_EQ(value.shape.style, HoloNight::Config::ShapeStyle::Chamfered);
  EXPECT_DOUBLE_EQ(value.shape.scale, 4.0);
  EXPECT_DOUBLE_EQ(*value.shape.base_radius, 128.0);
  EXPECT_DOUBLE_EQ(*value.shape.base_chamfer, 64.0);
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

TEST(AppearanceFileServiceTest, InvalidStartupUsesDirtyDefaultsAndRedactsDocumentContents) {
  QTemporaryDir directory;
  const QString path = directory.filePath(QStringLiteral("appearance.toml"));
  writeBytes(path, QByteArrayLiteral("not valid toml SECRET_VALUE"));
  AppearanceEditModel model;
  AppearanceFileService service(&model, path);

  EXPECT_FALSE(service.load());

  EXPECT_EQ(model.themeScheme(), QStringLiteral("holonight-dark"));
  EXPECT_TRUE(model.isDirty());
  EXPECT_FALSE(service.error().contains(QStringLiteral("SECRET_VALUE")));
  EXPECT_EQ(service.save(), AppearanceFileService::SaveResult::Success);
  EXPECT_FALSE(model.isDirty());
  EXPECT_TRUE(HoloNight::Config::load(path.toStdString()));
}

TEST(AppearanceFileServiceTest, ExplicitOverwriteSucceedsWhenConflictRevisionIsStable) {
  QTemporaryDir directory;
  const QString path = directory.filePath(QStringLiteral("appearance.toml"));
  AppearanceEditModel model;
  AppearanceFileService service(&model, path);
  ASSERT_TRUE(service.load());
  model.setThemeAccent(QStringLiteral("cyan"));
  writeBytes(path, QByteArrayLiteral("external"));
  ASSERT_EQ(service.save(), AppearanceFileService::SaveResult::Conflict);

  EXPECT_EQ(service.save(true), AppearanceFileService::SaveResult::Success);
  EXPECT_FALSE(model.isDirty());
  ASSERT_TRUE(HoloNight::Config::load(path.toStdString()));
  EXPECT_EQ(HoloNight::Config::load(path.toStdString()).value->appearance.theme.accent, "cyan");
}

TEST(AppearanceFileServiceTest, FailedDiscardRetainsEditedValueAndSnapshot) {
  QTemporaryDir directory;
  const QString path = directory.filePath(QStringLiteral("appearance.toml"));
  ASSERT_TRUE(HoloNight::Config::writeAtomically(HoloNight::Config::defaults(), path.toStdString()));
  AppearanceEditModel model;
  AppearanceFileService service(&model, path);
  ASSERT_TRUE(service.load());
  model.setThemeAccent(QStringLiteral("cyan"));
  writeBytes(path, QByteArrayLiteral("invalid"));

  EXPECT_FALSE(service.load());
  EXPECT_EQ(model.themeAccent(), QStringLiteral("cyan"));
  EXPECT_TRUE(model.isDirty());
}

TEST(AppearanceFileServiceTest, AppearanceOnlySaveDoesNotCreateProductConfig) {
  QTemporaryDir directory;
  AppearanceEditModel model;
  const QString appearance_path = directory.filePath(QStringLiteral("appearance.toml"));
  AppearanceFileService service(&model, appearance_path);
  ASSERT_TRUE(service.load());
  model.setThemeAccent(QStringLiteral("cyan"));

  EXPECT_EQ(service.save(), AppearanceFileService::SaveResult::Success);
  EXPECT_FALSE(QFile::exists(directory.filePath(QStringLiteral("config.toml"))));
}

TEST(AppearanceFileServiceTest, AtomicWriteFailureLeavesDomainDirty) {
  QTemporaryDir directory;
  const QString unwritable_path = directory.path();
  AppearanceEditModel model;
  AppearanceFileService service(&model, unwritable_path);
  model.setThemeAccent(QStringLiteral("cyan"));

  EXPECT_EQ(service.save(), AppearanceFileService::SaveResult::Error);
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

TEST(ShellConfigFileServiceTest, FailedDiscardRetainsProductEdit) {
  QTemporaryDir directory;
  const QString path = directory.filePath(QStringLiteral("config.toml"));
  ASSERT_TRUE(HoloNight::ShellConfig::ProductConfigWriter::write({}, path));
  ShellSettingsEditModel model;
  ShellConfigFileService service(&model, path);
  ASSERT_TRUE(service.load());
  model.setworkspaceCount(7);
  writeBytes(path, QByteArrayLiteral("invalid"));

  EXPECT_FALSE(service.load());
  EXPECT_EQ(model.workspaceCount(), 7);
  EXPECT_TRUE(model.isDirty());
}

TEST(SettingsSaveCoordinatorTest, AppearanceSuccessAndShellFailureAreIndependentAndRetryable) {
  QTemporaryDir directory;
  const QString appearance_path = directory.filePath(QStringLiteral("appearance.toml"));
  const QString shell_path = directory.filePath(QStringLiteral("blocked-product"));
  ASSERT_TRUE(QDir().mkdir(shell_path));
  AppearanceEditModel appearance;
  ShellSettingsEditModel shell;
  AppearanceFileService appearance_files(&appearance, appearance_path);
  ShellConfigFileService shell_files(&shell, shell_path);
  ASSERT_TRUE(appearance_files.load());
  ASSERT_TRUE(shell_files.load());
  appearance.setThemeAccent(QStringLiteral("cyan"));
  shell.setworkspaceCount(7);
  SettingsSaveCoordinator coordinator(&appearance, &appearance_files, &shell, &shell_files);

  coordinator.save();

  EXPECT_FALSE(appearance.isDirty());
  EXPECT_TRUE(shell.isDirty());
  EXPECT_TRUE(coordinator.resultText().contains(QStringLiteral("Some changes saved")));
  const QByteArray appearance_hash = fileHash(appearance_path);
  ASSERT_TRUE(QDir().rmdir(shell_path));
  coordinator.save();
  EXPECT_FALSE(shell.isDirty());
  EXPECT_EQ(fileHash(appearance_path), appearance_hash);
}

TEST(SettingsSaveCoordinatorTest, AppearanceFailureDoesNotPreventShellSuccess) {
  QTemporaryDir directory;
  const QString appearance_path = directory.filePath(QStringLiteral("blocked-appearance"));
  const QString shell_path = directory.filePath(QStringLiteral("config.toml"));
  ASSERT_TRUE(QDir().mkdir(appearance_path));
  AppearanceEditModel appearance;
  ShellSettingsEditModel shell;
  AppearanceFileService appearance_files(&appearance, appearance_path);
  ShellConfigFileService shell_files(&shell, shell_path);
  ASSERT_TRUE(shell_files.load());
  appearance.setThemeAccent(QStringLiteral("cyan"));
  shell.setworkspaceCount(7);
  SettingsSaveCoordinator coordinator(&appearance, &appearance_files, &shell, &shell_files);

  coordinator.save();

  EXPECT_TRUE(appearance.isDirty());
  EXPECT_FALSE(shell.isDirty());
  EXPECT_TRUE(QFile::exists(shell_path));
  EXPECT_TRUE(coordinator.resultText().contains(QStringLiteral("Some changes saved")));
}

TEST(SettingsSaveCoordinatorTest, DiscardReloadsDirtyDomainsIndependently) {
  QTemporaryDir directory;
  const QString appearance_path = directory.filePath(QStringLiteral("appearance.toml"));
  const QString shell_path = directory.filePath(QStringLiteral("config.toml"));
  ASSERT_TRUE(HoloNight::Config::writeAtomically(HoloNight::Config::defaults(), appearance_path.toStdString()));
  ASSERT_TRUE(HoloNight::ShellConfig::ProductConfigWriter::write({}, shell_path));
  AppearanceEditModel appearance;
  ShellSettingsEditModel shell;
  AppearanceFileService appearance_files(&appearance, appearance_path);
  ShellConfigFileService shell_files(&shell, shell_path);
  ASSERT_TRUE(appearance_files.load());
  ASSERT_TRUE(shell_files.load());
  appearance.setThemeAccent(QStringLiteral("cyan"));
  shell.setworkspaceCount(7);
  writeBytes(appearance_path, QByteArrayLiteral("invalid"));
  SettingsSaveCoordinator coordinator(&appearance, &appearance_files, &shell, &shell_files);

  coordinator.discard();

  EXPECT_TRUE(appearance.isDirty());
  EXPECT_EQ(appearance.themeAccent(), QStringLiteral("cyan"));
  EXPECT_FALSE(shell.isDirty());
  EXPECT_EQ(shell.workspaceCount(), 5);
  EXPECT_TRUE(coordinator.resultText().contains(QStringLiteral("Appearance")));
}

TEST(SettingsSaveCoordinatorTest, ConflictCommandsCancelReloadAndOverwritePerDomain) {
  QTemporaryDir directory;
  const QString appearance_path = directory.filePath(QStringLiteral("appearance.toml"));
  AppearanceEditModel appearance;
  ShellSettingsEditModel shell;
  AppearanceFileService appearance_files(&appearance, appearance_path);
  ShellConfigFileService shell_files(&shell, directory.filePath(QStringLiteral("config.toml")));
  ASSERT_TRUE(appearance_files.load());
  appearance.setThemeAccent(QStringLiteral("cyan"));
  writeBytes(appearance_path, QByteArrayLiteral("external"));
  SettingsSaveCoordinator coordinator(&appearance, &appearance_files, &shell, &shell_files);

  coordinator.save();
  ASSERT_EQ(coordinator.conflictDomain(), QStringLiteral("Appearance"));
  coordinator.cancelConflict();
  EXPECT_TRUE(coordinator.conflictDomain().isEmpty());
  EXPECT_TRUE(appearance.isDirty());
  coordinator.save();
  coordinator.overwriteConflict();
  EXPECT_FALSE(appearance.isDirty());
  EXPECT_TRUE(coordinator.conflictDomain().isEmpty());
}

TEST(SettingsSaveCoordinatorTest, ReloadConflictAcceptsExternalAppearance) {
  QTemporaryDir directory;
  const QString appearance_path = directory.filePath(QStringLiteral("appearance.toml"));
  AppearanceEditModel appearance;
  ShellSettingsEditModel shell;
  AppearanceFileService appearance_files(&appearance, appearance_path);
  ShellConfigFileService shell_files(&shell, directory.filePath(QStringLiteral("config.toml")));
  ASSERT_TRUE(appearance_files.load());
  appearance.setThemeAccent(QStringLiteral("cyan"));
  auto external = HoloNight::Config::defaults();
  external.theme.accent = "violet";
  ASSERT_TRUE(HoloNight::Config::writeAtomically(external, appearance_path.toStdString()));
  SettingsSaveCoordinator coordinator(&appearance, &appearance_files, &shell, &shell_files);
  coordinator.save();
  ASSERT_EQ(coordinator.conflictDomain(), QStringLiteral("Appearance"));

  coordinator.reloadConflict();

  EXPECT_TRUE(coordinator.conflictDomain().isEmpty());
  EXPECT_FALSE(appearance.isDirty());
  EXPECT_EQ(appearance.themeAccent(), QStringLiteral("violet"));
}

TEST(SettingsSaveCoordinatorTest, SaveReentryWhileBusyIsIgnored) {
  QTemporaryDir directory;
  AppearanceEditModel appearance;
  ShellSettingsEditModel shell;
  AppearanceFileService appearance_files(&appearance, directory.filePath(QStringLiteral("appearance.toml")));
  ShellConfigFileService shell_files(&shell, directory.filePath(QStringLiteral("config.toml")));
  ASSERT_TRUE(appearance_files.load());
  appearance.setThemeAccent(QStringLiteral("cyan"));
  SettingsSaveCoordinator coordinator(&appearance, &appearance_files, &shell, &shell_files);
  int busy_entries = 0;
  QObject::connect(&coordinator, &SettingsSaveCoordinator::isBusyChanged, [&]() {
    if (coordinator.isBusy()) {
      ++busy_entries;
      coordinator.save();
    }
  });

  coordinator.save();

  EXPECT_EQ(busy_entries, 1);
  EXPECT_FALSE(appearance.isDirty());
}
