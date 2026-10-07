// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 HoloNight contributors
#include "WallpaperController.h"
#include "WallpaperModel.h"

#include <QFile>
#include <QImage>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#include <gtest/gtest.h>
#include <holonight_shell_config/config_schema.h>

namespace Config = HoloNight::Config;
namespace {
void write(const QString& path, const QByteArray& bytes) {
  QFile file(path);
  ASSERT_TRUE(file.open(QIODevice::WriteOnly));
  ASSERT_EQ(file.write(bytes), bytes.size());
}
QString image(const QString& folder, const QString& name) {
  const QString path = folder + '/' + name;
  QImage picture(24, 16, QImage::Format_RGB32);
  picture.fill(Qt::blue);
  EXPECT_TRUE(picture.save(path));
  return path;
}
}  // namespace
TEST(WallpaperControllerTest, EncodesEscapesAsARealStringArray) {
  const QStringList paths{QStringLiteral("/a/quote\"slash\\line\n\t.png"), QString::fromUtf8("/a/🌙.png")};
  const auto parsed = Config::parseDocument("[background]\nimages = []\n");
  ASSERT_TRUE(parsed);
  const auto patched = Config::patchDocument(*parsed.value,
                                             {
                                                 {
                                                     .key = {"background", "images"},
                                                     .baseline = parsed.value->value({"background", "images"}),
                                                     .pending = WallpaperController::encodeImages(paths),
                                                 },
                                             },
                                             HoloNight::ShellConfig::documentSchema());
  ASSERT_EQ(patched.status, Config::SaveStatus::Success);
  const auto decoded = HoloNight::ShellConfig::decodeDocument(*patched.snapshot);
  ASSERT_TRUE(decoded);
  EXPECT_EQ(decoded.value->background.images, paths);
}
TEST(WallpaperControllerTest, MaterializesFallbackAndPreservesTrailingAssignments) {
  EXPECT_EQ(WallpaperController::assign({"a"}, 3, 1, "b", false), (QStringList{"a", "b", "a"}));
  EXPECT_EQ(WallpaperController::assign({"a", "b", "c", "d"}, 2, 1, "x", false), (QStringList{"a", "x", "c", "d"}));
  EXPECT_EQ(WallpaperController::assign({}, 2, 1, "x", false), (QStringList{"", "x"}));
  EXPECT_EQ(WallpaperController::assign({"a", "b"}, 2, 1, "x", true), (QStringList{"x"}));
  EXPECT_EQ(WallpaperController::assign({"a"}, 2, -1, "x", false), (QStringList{"a"}));
}
TEST(WallpaperControllerTest, SavesOnlyWallpaperAndPreservesConcurrentUnrelatedEdits) {
  QTemporaryDir folder;
  const auto config = folder.filePath("shell.toml");
  write(config, "# keep comment\n[custom]\nvalue = 1\n");
  WallpaperController controller(nullptr, config);
  controller.select(image(folder.path(), "one.png"));
  ASSERT_TRUE(controller.canApply());
  write(config, "# keep comment\n[custom]\nvalue = 2\n");
  ASSERT_TRUE(controller.apply());
  QFile file(config);
  ASSERT_TRUE(file.open(QIODevice::ReadOnly));
  const auto bytes = file.readAll();
  EXPECT_TRUE(bytes.contains("# keep comment"));
  EXPECT_TRUE(bytes.contains("value = 2"));
  EXPECT_FALSE(controller.canApply());
}
TEST(WallpaperControllerTest, ExternalWallpaperConflictRetainsPendingUntilExplicitResolution) {
  QTemporaryDir folder;
  const auto config = folder.filePath("shell.toml");
  write(config, "[background]\nimages = []\n");
  WallpaperController controller(nullptr, config);
  const auto selected = image(folder.path(), "one.png");
  controller.select(selected);
  write(config, "[background]\nimages = [\"/external.png\"]\n");
  controller.reload();
  EXPECT_TRUE(controller.conflict());
  EXPECT_EQ(controller.selectedPath(), selected);
  EXPECT_FALSE(controller.apply());
  controller.resolve(true);
  EXPECT_FALSE(controller.conflict());
  EXPECT_TRUE(controller.apply());
  controller.select(image(folder.path(), "two.png"));
  controller.discard();
  EXPECT_EQ(controller.selectedPath(), selected);
  EXPECT_FALSE(controller.canApply());
}
TEST(WallpaperControllerTest, InvalidConfigurationAndMissingImagesDisableApply) {
  QTemporaryDir folder;
  const auto config = folder.filePath("shell.toml");
  write(config, "[background]\nimages = [\n");
  WallpaperController controller(nullptr, config);
  controller.select(image(folder.path(), "one.png"));
  EXPECT_FALSE(controller.canApply());
  EXPECT_FALSE(controller.apply());
  write(config, "[background]\nimages = []\n");
  controller.reload();
  controller.select(folder.filePath("missing.png"));
  EXPECT_FALSE(controller.canApply());
  EXPECT_FALSE(controller.apply());
}
TEST(WallpaperControllerTest, StorageFailureRetainsSelection) {
  QTemporaryDir folder;
  const auto config = folder.filePath("missing/shell.toml");
  WallpaperController controller(nullptr, config);
  const auto selected = image(folder.path(), "one.png");
  controller.select(selected);
  // A file prevents creation of the configuration's parent directory.
  write(folder.filePath("missing"), "blocked");
  EXPECT_FALSE(controller.apply());
  EXPECT_EQ(controller.selectedPath(), selected);
}
TEST(WallpaperModelTest, DeduplicatesRecursiveSourcesAndPersistsFavorites) {
  QTemporaryDir folder;
  ASSERT_TRUE(QDir(folder.path()).mkdir("child"));
  const auto path = image(folder.filePath("child"), "wall.png");
  write(folder.filePath("child/corrupt.png"), "not an image");
  const auto settings = folder.filePath("picker.ini");
  {
    WallpaperModel model(nullptr, settings);
    WallpaperFilter proxy(&model);
    model.addFolder(QUrl::fromLocalFile(folder.path()));
    model.addFolder(QUrl::fromLocalFile(folder.filePath("child")));
    model.refresh();
    QTRY_VERIFY_WITH_TIMEOUT(!model.loading(), 5000);
    proxy.setCollection(folder.path());
    ASSERT_EQ(proxy.rowCount(), 1);
    EXPECT_EQ(proxy.data(proxy.index(0, 0), WallpaperModel::Path).toString(), path);
    model.toggleFavorite(path);
    proxy.setCollection("favorites");
    EXPECT_EQ(proxy.rowCount(), 1);
  }
  WallpaperModel restored(nullptr, settings);
  restored.refresh();
  QTRY_VERIFY_WITH_TIMEOUT(!restored.loading(), 5000);
  WallpaperFilter favorites(&restored);
  favorites.setCollection("favorites");
  EXPECT_EQ(favorites.rowCount(), 1);
}
TEST(WallpaperModelTest, RefreshInvalidatesThumbnailAndRemovesDeletedImages) {
  QTemporaryDir folder;
  const auto path = image(folder.path(), "wall.png");
  WallpaperModel model(nullptr, folder.filePath("picker.ini"));
  WallpaperFilter proxy(&model);
  proxy.setCollection(folder.path());
  model.addFolder(QUrl::fromLocalFile(folder.path()));
  QTRY_VERIFY_WITH_TIMEOUT(!model.loading(), 5000);
  ASSERT_EQ(proxy.rowCount(), 1);
  const auto original = proxy.data(proxy.index(0, 0), WallpaperModel::Thumbnail);
  QFile file(path);
  ASSERT_TRUE(file.open(QIODevice::ReadWrite));
  ASSERT_TRUE(file.setFileTime(QDateTime::currentDateTime().addSecs(5), QFileDevice::FileModificationTime));
  file.close();
  model.refresh();
  QTRY_VERIFY_WITH_TIMEOUT(!model.loading(), 5000);
  EXPECT_NE(proxy.data(proxy.index(0, 0), WallpaperModel::Thumbnail), original);
  ASSERT_TRUE(QFile::remove(path));
  model.refresh();
  QTRY_VERIFY_WITH_TIMEOUT(!model.loading(), 5000);
  EXPECT_EQ(proxy.rowCount(), 0);
}

TEST(WallpaperModelTest, LatestRefreshDiscardsOlderScansAndSkipsDirectorySymlinks) {
  QTemporaryDir folder;
  ASSERT_TRUE(QDir(folder.path()).mkdir("source"));
  ASSERT_TRUE(QDir(folder.path()).mkdir("outside"));
  image(folder.filePath("source"), "one.png");
  image(folder.filePath("outside"), "excluded.png");
  ASSERT_TRUE(QFile::link(folder.filePath("outside"), folder.filePath("source/link")));
  WallpaperModel model(nullptr, folder.filePath("picker.ini"));
  QSignalSpy resets(&model, &QAbstractItemModel::modelReset);
  model.addFolder(QUrl::fromLocalFile(folder.filePath("source")));
  model.refresh();
  model.refresh();
  QTRY_VERIFY_WITH_TIMEOUT(!model.loading(), 5000);
  ASSERT_EQ(resets.count(), 1);
  ASSERT_EQ(model.rowCount(), 1);
  EXPECT_EQ(model.data(model.index(0), WallpaperModel::Title).toString(), QStringLiteral("one"));
}

TEST(WallpaperControllerTest, TargetMustBeReselectedAfterItBecomesUnavailable) {
  QTemporaryDir folder;
  WallpaperController controller(nullptr, folder.filePath("shell.toml"));
  controller.select(image(folder.path(), "one.png"));
  ASSERT_TRUE(controller.canApply());
  controller.setTarget(-1);
  EXPECT_FALSE(controller.canApply());
  controller.setTarget(0);
  EXPECT_TRUE(controller.canApply());
}
