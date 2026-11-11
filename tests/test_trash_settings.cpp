#include "TrashSettingsModel.h"

#include <QScopeGuard>
#include <QTemporaryDir>

#include <gtest/gtest.h>

TEST(TrashSettings, SaveAndDiscardRoundTrip) {
  const QTemporaryDir config;
  ASSERT_TRUE(config.isValid());
  const QScopeGuard guard([previous = qEnvironmentVariable("XDG_CONFIG_HOME")] {
    if (previous.isNull()) {
      qunsetenv("XDG_CONFIG_HOME");
    } else {
      qputenv("XDG_CONFIG_HOME", previous.toUtf8());
    }
  });
  ASSERT_TRUE(qputenv("XDG_CONFIG_HOME", config.path().toUtf8()));
  TrashSettingsModel model;
  EXPECT_FALSE(model.enabled());
  EXPECT_EQ(model.retentionDays(), 7);
  model.setEnabled(true);
  model.setRetentionDays(14);
  EXPECT_TRUE(model.isDirty());
  ASSERT_TRUE(model.save()) << model.error().toStdString();
  EXPECT_FALSE(model.isDirty());
  model.setEnabled(false);
  model.setRetentionDays(365);
  model.load();
  EXPECT_TRUE(model.enabled());
  EXPECT_EQ(model.retentionDays(), 14);
  EXPECT_FALSE(model.isDirty());
  const auto policy = holonightd::trash::readPolicy(holonightd::trash::policyPath());
  ASSERT_TRUE(policy);
  EXPECT_TRUE(policy->enabled);
  EXPECT_EQ(policy->retention_days, 14);
}
