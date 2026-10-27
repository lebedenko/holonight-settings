// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 HoloNight contributors
#include "SettingsSessionContext.h"

#include <gtest/gtest.h>

TEST(SettingsSessionContext, SessionIdentityControlsApplicability) {
  for (const auto& desktop : {"Hyprland", "sway", "labwc", "", "Hyprland:sway"}) {
    SCOPED_TRACE(desktop);
    QProcessEnvironment environment;
    environment.insert("XDG_CURRENT_DESKTOP", desktop);
    SettingsSessionContext context(environment);
    const bool supported = QString(desktop) == "Hyprland" || QString(desktop) == "sway" || QString(desktop) == "labwc";
    EXPECT_EQ(context.isApplicable("workspaceCount"), supported);
    for (const auto& property : {"taskbarEnabled", "taskbarGrouped", "windowOverviewAccess", "desktopMenuEnabled"}) {
      EXPECT_EQ(context.isApplicable(property), QString(desktop) == "labwc");
    }
    EXPECT_TRUE(context.isApplicable("trayMaxItems"));
    EXPECT_TRUE(context.isApplicable("weatherCity"));
  }
}
TEST(SettingsSessionContext, MarkersAreFallbackAndDesktopTokensTakePrecedence) {
  QProcessEnvironment environment;
  environment.insert("SWAYSOCK", "/unavailable");
  SettingsSessionContext sway(environment);
  EXPECT_EQ(sway.compositor(), "sway");
  EXPECT_TRUE(sway.isApplicable("workspaceCount"));
  environment.insert("HYPRLAND_INSTANCE_SIGNATURE", "stale");
  SettingsSessionContext conflicting(environment);
  EXPECT_FALSE(conflicting.isApplicable("workspaceCount"));
  environment.insert("XDG_CURRENT_DESKTOP", "HoloNight:LABWC");
  SettingsSessionContext labwc(environment);
  EXPECT_EQ(labwc.compositor(), "labwc");
  EXPECT_TRUE(labwc.isApplicable("taskbarEnabled"));
  EXPECT_EQ(sway.compositor(), "sway");
}
