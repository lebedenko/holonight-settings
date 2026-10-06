// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 HoloNight contributors
#include "AppearanceEditModel.h"
#include "AppearanceFileService.h"
#include "ShellConfigFileService.h"
#include "ShellSettingsEditModel.h"

#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <filesystem>
#include <gtest/gtest.h>
#include <holonight/config/appearance_document.h>
#include <holonight/config/codec.h>

namespace {
void write(const QString& path, const QByteArray& bytes) {
  QFile file(path);
  ASSERT_TRUE(file.open(QIODevice::WriteOnly));
  ASSERT_EQ(file.write(bytes), bytes.size());
}
QByteArray read(const QString& path) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    return {};
  }
  return file.readAll();
}
}  // namespace

TEST(ConfigurationDocuments, AppearanceFirstSaveUpgradesV1AndPreservesOtherExplicitValues) {
  QTemporaryDir directory;
  const auto path = directory.filePath("appearance.toml");
  auto original = HoloNight::Config::defaults();
  original.typography.ui_size = 18;
  const auto serialized = HoloNight::Config::serialize(original);
  ASSERT_TRUE(serialized);
  const auto bytes = QByteArray::fromStdString(*serialized.value) + "# retained tail\n";
  write(path, bytes);
  AppearanceEditModel model;
  AppearanceFileService files(&model, path);
  ASSERT_TRUE(files.load());
  model.setThemeAccent("cyan");
  ASSERT_EQ(files.save(), AppearanceFileService::SaveResult::Success) << files.error().toStdString();
  const auto result = HoloNight::Config::readAppearanceDocument(path.toStdString());
  ASSERT_TRUE(result);
  EXPECT_EQ(result.value->document_version, 2);
  EXPECT_EQ(result.value->appearance.typography.ui_size, 18);
  EXPECT_TRUE(read(path).endsWith("# retained tail\n"));
  EXPECT_FALSE(model.isDirty());
}
TEST(ConfigurationDocuments, ShellResetRemovesAnExplicitDefaultAndPreservesUnknownBytes) {
  QTemporaryDir directory;
  const auto path = directory.filePath("config.toml");
  write(path, "# header\n[bar.workspaces]\ncount = 5 # keep comment\n[unknown]\nquoted = 'untouched'\n");
  ShellSettingsEditModel model;
  ShellConfigFileService files(&model, path);
  ASSERT_TRUE(files.load());
  model.document()->reset("workspaceCount");
  EXPECT_EQ(model.workspaceCount(), 5);
  EXPECT_TRUE(model.isDirty());
  ASSERT_EQ(files.save(), ShellConfigFileService::SaveResult::Success);
  EXPECT_FALSE(read(path).contains("count ="));
  EXPECT_TRUE(read(path).contains("# keep comment"));
  EXPECT_TRUE(read(path).contains("[unknown]\nquoted = 'untouched'\n"));
}
TEST(ConfigurationDocuments, ExternalChangesUpdateUntouchedControlsAndKeepConflictingPendingValues) {
  QTemporaryDir directory;
  const auto path = directory.filePath("config.toml");
  write(path, "[bar.workspaces]\ncount = 5\n[bar.systemtray]\nmax_items = 3\n");
  ShellSettingsEditModel model;
  ShellConfigFileService files(&model, path);
  ASSERT_TRUE(files.load());
  model.setworkspaceCount(7);
  write(path, "# external\n[bar.workspaces]\ncount = 8\n[bar.systemtray]\nmax_items = 4\n");
  ASSERT_TRUE(QTest::qWaitFor([&] { return model.trayMaxItems() == 4; }));
  EXPECT_EQ(model.workspaceCount(), 7);
  ASSERT_EQ(model.document()->conflicts().size(), 1);
  EXPECT_EQ(files.save(), ShellConfigFileService::SaveResult::Conflict);
  model.document()->resolve("workspaceCount", true);
  ASSERT_EQ(files.save(), ShellConfigFileService::SaveResult::Success);
  EXPECT_TRUE(read(path).startsWith("# external\n"));
  EXPECT_TRUE(read(path).contains("max_items = 4"));
}
TEST(ConfigurationDocuments, ConflictResolutionChangesOnlyTheSelectedPendingValue) {
  QTemporaryDir directory;
  const auto path = directory.filePath("config.toml");
  ShellSettingsEditModel model;
  ShellConfigFileService files(&model, path);
  ASSERT_TRUE(files.load());
  model.setworkspaceCount(7);
  model.settrayMaxItems(4);
  write(path, "[bar.workspaces]\ncount = 8\n[bar.systemtray]\nmax_items = 5\n");
  EXPECT_EQ(files.save(), ShellConfigFileService::SaveResult::Conflict);
  ASSERT_EQ(model.document()->conflicts().size(), 2);
  model.document()->resolve("workspaceCount", false);
  EXPECT_EQ(model.workspaceCount(), 8);
  EXPECT_EQ(model.trayMaxItems(), 4);
  ASSERT_EQ(model.document()->conflicts().size(), 1);
  model.document()->resolve("trayMaxItems", true);
  ASSERT_EQ(files.save(), ShellConfigFileService::SaveResult::Success);
}
TEST(ConfigurationDocuments, UnsupportedInlinePatchLeavesBytesAndPendingEditUnchanged) {
  QTemporaryDir directory;
  const auto path = directory.filePath("config.toml");
  const QByteArray original = "bar = {workspaces = {count = 5}} # keep\n";
  write(path, original);
  ShellSettingsEditModel model;
  ShellConfigFileService files(&model, path);
  ASSERT_TRUE(files.load());
  model.setworkspaceCount(7);
  EXPECT_EQ(files.save(), ShellConfigFileService::SaveResult::Error);
  EXPECT_EQ(read(path), original);
  EXPECT_EQ(model.workspaceCount(), 7);
  EXPECT_TRUE(model.isDirty());
}
TEST(ConfigurationDocuments, InvalidExternalDocumentRetainsLastValidModelAndRecoversAfterRecreation) {
  QTemporaryDir directory;
  const auto path = directory.filePath("config.toml");
  write(path, "[bar.workspaces]\ncount = 7\n");
  ShellSettingsEditModel model;
  ShellConfigFileService files(&model, path);
  ASSERT_TRUE(files.load());
  QSignalSpy changes(&model, &ShellSettingsEditModel::workspaceCountChanged);
  write(path, "[bar.workspaces]\ncount = 999\n");
  ASSERT_TRUE(QTest::qWaitFor([&] { return model.document()->blocked(); }));
  EXPECT_EQ(model.workspaceCount(), 7);
  EXPECT_EQ(changes.size(), 0);
  ASSERT_TRUE(QFile::remove(path));
  ASSERT_TRUE(QTest::qWaitFor([&] { return model.workspaceCount() == 5; }));
  write(path, "[bar.workspaces]\ncount = 8\n");
  ASSERT_TRUE(QTest::qWaitFor([&] { return model.workspaceCount() == 8; }));
  EXPECT_FALSE(model.document()->blocked());
}
TEST(ConfigurationDocuments, EqualEffectiveExternalChangesDoNotEmitControlChanges) {
  QTemporaryDir directory;
  const auto path = directory.filePath("config.toml");
  ShellSettingsEditModel model;
  ShellConfigFileService files(&model, path);
  ASSERT_TRUE(files.load());
  QSignalSpy changes(&model, &ShellSettingsEditModel::workspaceCountChanged);
  write(path, "# explicit default\n[bar.workspaces]\ncount = 5\n");
  ASSERT_TRUE(QTest::qWaitFor([&] {
    for (const auto& field : model.document()->fields()) {
      const auto row = field.toMap();
      if (row["property"] == "workspaceCount") {
        return row["overridden"].toBool();
      }
    }
    return false;
  }));
  EXPECT_EQ(changes.size(), 0);
  EXPECT_FALSE(model.isDirty());
}
TEST(ConfigurationDocuments, AppearanceRollbackPreservesExternalEditsAndCommitRejectsChangedRevision) {
  QTemporaryDir directory;
  const auto path = directory.filePath("appearance.toml");
  AppearanceEditModel model;
  AppearanceFileService files(&model, path);
  ASSERT_TRUE(files.load());
  model.setThemeAccent("cyan");
  ASSERT_EQ(files.stage(), AppearanceFileService::SaveResult::Success);
  const QByteArray external = "version = 2\n# intervening editor\n[theme]\naccent = 'violet'\n";
  write(path, external);
  EXPECT_FALSE(files.rollback());
  EXPECT_EQ(read(path), external);
  EXPECT_TRUE(model.isDirty());
  model.document()->resolve("themeAccent", true);
  ASSERT_EQ(files.stage(), AppearanceFileService::SaveResult::Success);
  write(path, external);
  EXPECT_FALSE(files.commit());
  EXPECT_EQ(read(path), external);
  EXPECT_TRUE(model.isDirty());
}
TEST(ConfigurationDocuments, AppearanceRollbackKeepsUnrelatedChangesMergedDuringStaging) {
  QTemporaryDir directory;
  const auto path = directory.filePath("appearance.toml");
  write(path, "version = 2\n");
  AppearanceEditModel model;
  AppearanceFileService files(&model, path);
  ASSERT_TRUE(files.load());
  model.setThemeAccent("cyan");
  const QByteArray external = "version = 2\n# another writer\n[typography]\nui_size = 18\n";
  write(path, external);
  ASSERT_EQ(files.stage(), AppearanceFileService::SaveResult::Success);
  ASSERT_TRUE(files.rollback());
  EXPECT_EQ(read(path), external);
  EXPECT_EQ(model.uiFontSize(), 18);
  EXPECT_EQ(model.themeAccent(), "cyan");
}
TEST(ConfigurationDocuments, RetargetedSymlinkCannotAuthorizeAppearanceRollback) {
  QTemporaryDir directory;
  const auto first = directory.filePath("first.toml");
  const auto second = directory.filePath("second.toml");
  const auto alias = directory.filePath("appearance.toml");
  write(first, "version = 2\n");
  std::filesystem::create_symlink(first.toStdString(), alias.toStdString());
  AppearanceEditModel model;
  AppearanceFileService files(&model, alias);
  ASSERT_TRUE(files.load());
  model.setThemeAccent("cyan");
  ASSERT_EQ(files.stage(), AppearanceFileService::SaveResult::Success);
  const auto staged = read(first);
  write(second, staged);
  ASSERT_TRUE(QFile::remove(alias));
  std::filesystem::create_symlink(second.toStdString(), alias.toStdString());
  EXPECT_FALSE(files.rollback());
  EXPECT_EQ(read(first), staged);
  EXPECT_EQ(read(second), staged);
}
TEST(ConfigurationDocuments, ChangesMadeDuringApplicationRemainPendingWithoutSelfConflict) {
  QTemporaryDir directory;
  const auto path = directory.filePath("appearance.toml");
  AppearanceEditModel model;
  AppearanceFileService files(&model, path);
  ASSERT_TRUE(files.load());
  model.setThemeAccent("cyan");
  ASSERT_EQ(files.stage(), AppearanceFileService::SaveResult::Success);
  model.setThemeAccent("violet");
  ASSERT_TRUE(files.commit());
  EXPECT_TRUE(model.isDirty());
  EXPECT_EQ(model.themeAccent(), "violet");
  EXPECT_TRUE(model.document()->conflicts().isEmpty());
  EXPECT_EQ(files.save(), AppearanceFileService::SaveResult::Success);
}
TEST(ConfigurationDocuments, UnsupportedAppearanceVersionCannotBeOverwritten) {
  QTemporaryDir directory;
  const auto path = directory.filePath("appearance.toml");
  const QByteArray original = "version = 99\n";
  write(path, original);
  AppearanceEditModel model;
  AppearanceFileService files(&model, path);
  EXPECT_FALSE(files.load());
  EXPECT_FALSE(model.isDirty());
  model.setThemeAccent("cyan");
  EXPECT_EQ(files.save(), AppearanceFileService::SaveResult::Error);
  EXPECT_EQ(read(path), original);
}
