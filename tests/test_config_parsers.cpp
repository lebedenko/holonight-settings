#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include <array>
#include <gtest/gtest.h>
#include <holonight_config/config_parsers.h>
#include <holonight_config/config_writer.h>
#include <string>
#include <string_view>

namespace {

ParsedConfig parseConfig(std::string_view content, MissingDefaults& missing) {
  return parseConfigTable(toml::parse(std::string{content}), missing);
}

TEST(ConfigParsersTest, EmptyTableReturnsDefaultsAndMarksWritableMissingDefaults) {
  MissingDefaults missing;
  const ParsedConfig parsed = parseConfigTable(toml::table{}, missing);

  EXPECT_EQ(parsed.appearance, AppearanceConfig{});
  EXPECT_EQ(parsed.bar_workspaces, BarWorkspacesConfig{});
  EXPECT_EQ(parsed.bar_system_tray, BarSystemTrayConfig{});
  EXPECT_EQ(parsed.background, BackgroundConfig{});
  EXPECT_EQ(parsed.weather, WeatherConfig{});
  EXPECT_EQ(parsed.notifications, NotificationsConfig{});
  EXPECT_EQ(parsed.notification_history, NotificationHistoryConfig{});

  EXPECT_TRUE(missing.ui_font);
  EXPECT_TRUE(missing.ui_font_size);
  EXPECT_TRUE(missing.fixed_font);
  EXPECT_TRUE(missing.fixed_font_size);
  EXPECT_TRUE(missing.clock_font);
  EXPECT_TRUE(missing.clock_font_size);
  EXPECT_TRUE(missing.title_font);
  EXPECT_TRUE(missing.title_font_size);
  EXPECT_TRUE(missing.transparency);
  EXPECT_TRUE(missing.blur_strength);
  EXPECT_TRUE(missing.workspace_count);
  EXPECT_TRUE(missing.tray_max_items);
  EXPECT_TRUE(missing.background_images);
  EXPECT_TRUE(missing.weather_api_key);
  EXPECT_TRUE(missing.weather_geo_api_key);
  EXPECT_TRUE(missing.weather_units);
  EXPECT_TRUE(missing.weather_lang);
  EXPECT_TRUE(missing.weather_refresh_interval);
  EXPECT_TRUE(missing.notif_default_timeout);
  EXPECT_TRUE(missing.notif_max_visible);
  EXPECT_TRUE(missing.notif_history_enabled);
  EXPECT_TRUE(missing.notif_history_max_items);
  EXPECT_TRUE(missing.notif_history_max_age_days);
  EXPECT_TRUE(missing.notif_history_persist_body);
}

TEST(ConfigParsersTest, LegacyThemeSectionIsIgnored) {
  MissingDefaults missing;
  const ParsedConfig parsed = parseConfig(R"toml(
[theme]
variant = "Aurora"
mode = "system"
accent = "violet"
)toml",
                                          missing);

  EXPECT_EQ(parsed.appearance, AppearanceConfig{});
  EXPECT_TRUE(missing.ui_font);
}

TEST(ConfigParsersTest, AppearanceParserAppliesOverridesAndTracksMissingKeys) {
  MissingDefaults missing;
  const ParsedConfig parsed = parseConfig(R"toml(
[appearance]
ui_font = "Fira Code"
ui_font_size = 14
fixed_font = "Inconsolata"
fixed_font_size = "bad"
clock_font = "Orbitron"
clock_font_size = 20
title_font = "Exo"
)toml",
                                          missing);

  EXPECT_EQ(parsed.appearance.ui_font, QStringLiteral("Fira Code"));
  EXPECT_EQ(parsed.appearance.ui_font_size, 14);
  EXPECT_EQ(parsed.appearance.fixed_font, QStringLiteral("Inconsolata"));
  EXPECT_EQ(parsed.appearance.fixed_font_size, AppearanceConfig{}.fixed_font_size);
  EXPECT_EQ(parsed.appearance.clock_font, QStringLiteral("Orbitron"));
  EXPECT_EQ(parsed.appearance.clock_font_size, 20);
  EXPECT_EQ(parsed.appearance.title_font, QStringLiteral("Exo"));
  EXPECT_EQ(parsed.appearance.title_font_size, AppearanceConfig{}.title_font_size);
  EXPECT_EQ(parsed.appearance.transparency, AppearanceConfig{}.transparency);
  EXPECT_EQ(parsed.appearance.blur_strength, AppearanceConfig{}.blur_strength);

  EXPECT_FALSE(missing.ui_font);
  EXPECT_FALSE(missing.ui_font_size);
  EXPECT_FALSE(missing.fixed_font);
  EXPECT_FALSE(missing.fixed_font_size);
  EXPECT_FALSE(missing.clock_font);
  EXPECT_FALSE(missing.clock_font_size);
  EXPECT_FALSE(missing.title_font);
  EXPECT_TRUE(missing.title_font_size);
  EXPECT_TRUE(missing.transparency);
  EXPECT_TRUE(missing.blur_strength);
}

TEST(ConfigParsersTest, AppearanceParserClampsTransparencyAndBlurStrength) {
  struct Case {
    std::string_view toml;
    int expected_transparency;
    int expected_blur_strength;
  };

  const std::array<Case, 5> cases = {
      Case{.toml = R"toml(
[appearance]
transparency = -10
blur_strength = -5
)toml",
           .expected_transparency = AppearanceConfig::kMinTransparency,
           .expected_blur_strength = AppearanceConfig::kMinBlurStrength},
      Case{.toml = R"toml(
[appearance]
transparency = 60
blur_strength = 16
)toml",
           .expected_transparency = 60,
           .expected_blur_strength = 16},
      Case{.toml = R"toml(
[appearance]
transparency = 500
blur_strength = 500
)toml",
           .expected_transparency = AppearanceConfig::kMaxTransparency,
           .expected_blur_strength = AppearanceConfig::kMaxBlurStrength},
      Case{.toml = R"toml(
[appearance]
transparency = -9223372036854775808
blur_strength = -9223372036854775808
)toml",
           .expected_transparency = AppearanceConfig::kMinTransparency,
           .expected_blur_strength = AppearanceConfig::kMinBlurStrength},
      Case{.toml = R"toml(
[appearance]
transparency = 9223372036854775807
blur_strength = 9223372036854775807
)toml",
           .expected_transparency = AppearanceConfig::kMaxTransparency,
           .expected_blur_strength = AppearanceConfig::kMaxBlurStrength},
  };

  for (const Case& test_case : cases) {
    MissingDefaults missing;
    const ParsedConfig parsed = parseConfig(test_case.toml, missing);

    EXPECT_EQ(parsed.appearance.transparency, test_case.expected_transparency);
    EXPECT_EQ(parsed.appearance.blur_strength, test_case.expected_blur_strength);
    EXPECT_FALSE(missing.transparency);
    EXPECT_FALSE(missing.blur_strength);
  }
}

TEST(ConfigParsersTest, AppearanceParserAcceptsZeroTransparencyAndBlurStrength) {
  MissingDefaults missing;
  const ParsedConfig parsed = parseConfig(R"toml(
[appearance]
transparency = 0
blur_strength = 0
)toml",
                                          missing);

  EXPECT_EQ(parsed.appearance.transparency, 0);
  EXPECT_EQ(parsed.appearance.blur_strength, 0);
  EXPECT_FALSE(missing.transparency);
  EXPECT_FALSE(missing.blur_strength);
}

TEST(ConfigParsersTest, BarParsersClampWorkspaceAndTrayCounts) {
  struct Case {
    std::string_view toml;
    int expected_workspaces;
    int expected_tray_items;
  };

  const std::array<Case, 3> cases = {
      Case{.toml = R"toml(
[bar.workspaces]
count = 1
[bar.systemtray]
max_items = 1
)toml",
           .expected_workspaces = BarWorkspacesConfig::kMinCount,
           .expected_tray_items = BarSystemTrayConfig::kMinMaxItems},
      Case{.toml = R"toml(
[bar.workspaces]
count = 7
[bar.systemtray]
max_items = 4
)toml",
           .expected_workspaces = 7,
           .expected_tray_items = 4},
      Case{.toml = R"toml(
[bar.workspaces]
count = 99
[bar.systemtray]
max_items = 99
)toml",
           .expected_workspaces = BarWorkspacesConfig::kMaxCount,
           .expected_tray_items = BarSystemTrayConfig::kMaxMaxItems},
  };

  for (const Case& test_case : cases) {
    MissingDefaults missing;
    const ParsedConfig parsed = parseConfig(test_case.toml, missing);

    EXPECT_EQ(parsed.bar_workspaces.count, test_case.expected_workspaces);
    EXPECT_EQ(parsed.bar_system_tray.max_items, test_case.expected_tray_items);
    EXPECT_FALSE(missing.workspace_count);
    EXPECT_FALSE(missing.tray_max_items);
  }
}

TEST(ConfigParsersTest, TrayIconOverridesKeepValidRulesAndSkipIncompleteRules) {
  MissingDefaults missing;
  const ParsedConfig parsed = parseConfig(R"toml(
[tray.icon_overrides.slack]
id = "Slack_status_icon_1"
icon = "slack-indicator"
attention_icon = "slack-indicator-attention"

[tray.icon_overrides.no_icon]
id = "broken"

[tray.icon_overrides.no_matcher]
icon = "broken-icon"
)toml",
                                          missing);

  ASSERT_EQ(parsed.tray_icon_overrides.items.size(), 1);
  const TrayIconOverrideConfig& override = parsed.tray_icon_overrides.items.at(0);
  EXPECT_EQ(override.name, QStringLiteral("slack"));
  EXPECT_EQ(override.id, QStringLiteral("Slack_status_icon_1"));
  EXPECT_EQ(override.icon, QStringLiteral("slack-indicator"));
  EXPECT_EQ(override.attention_icon, QStringLiteral("slack-indicator-attention"));
}

TEST(ConfigParsersTest, BackgroundParserExpandsTildeAndSkipsInvalidEntries) {
  MissingDefaults missing;
  const ParsedConfig parsed = parseConfig(R"toml(
[background]
images = ["~/wallpapers/a.png", 42, "/opt/wallpapers/b.png"]
)toml",
                                          missing);

  ASSERT_EQ(parsed.background.images.size(), 2);
  EXPECT_EQ(parsed.background.images.at(0), QDir::homePath() + QStringLiteral("/wallpapers/a.png"));
  EXPECT_EQ(parsed.background.images.at(1), QStringLiteral("/opt/wallpapers/b.png"));
  EXPECT_FALSE(missing.background_images);
}

TEST(ConfigParsersTest, WeatherParserHandlesOverridesInvalidValuesAndOptionalCoordinates) {
  MissingDefaults missing;
  const ParsedConfig parsed = parseConfig(R"toml(
[weather]
api_key = "weather-key"
geo_api_key = "geo-key"
latitude = 50.45
longitude = 30.52
city = "Kyiv"
country = "Ukraine"
units = "imperial"
lang = "uk"
refresh_interval = 0
)toml",
                                          missing);

  EXPECT_EQ(parsed.weather.api_key, QStringLiteral("weather-key"));
  EXPECT_EQ(parsed.weather.geo_api_key, QStringLiteral("geo-key"));
  ASSERT_TRUE(parsed.weather.latitude.has_value());
  ASSERT_TRUE(parsed.weather.longitude.has_value());
  EXPECT_DOUBLE_EQ(*parsed.weather.latitude, 50.45);
  EXPECT_DOUBLE_EQ(*parsed.weather.longitude, 30.52);
  EXPECT_EQ(parsed.weather.city, QStringLiteral("Kyiv"));
  EXPECT_EQ(parsed.weather.country, QStringLiteral("Ukraine"));
  EXPECT_EQ(parsed.weather.units, QStringLiteral("imperial"));
  EXPECT_EQ(parsed.weather.lang, QStringLiteral("uk"));
  EXPECT_EQ(parsed.weather.refresh_interval, WeatherConfig{}.refresh_interval);

  EXPECT_FALSE(missing.weather_api_key);
  EXPECT_FALSE(missing.weather_geo_api_key);
  EXPECT_FALSE(missing.weather_units);
  EXPECT_FALSE(missing.weather_lang);
  EXPECT_FALSE(missing.weather_refresh_interval);
}

TEST(ConfigParsersTest, WeatherParserRejectsOutOfRangeCoordinatesAndKeepsBoundaries) {
  MissingDefaults missing;
  const ParsedConfig parsed = parseConfig(R"toml(
[weather]
latitude = -90.0
longitude = 180.0
)toml",
                                          missing);

  ASSERT_TRUE(parsed.weather.latitude.has_value());
  ASSERT_TRUE(parsed.weather.longitude.has_value());
  EXPECT_DOUBLE_EQ(*parsed.weather.latitude, -90.0);
  EXPECT_DOUBLE_EQ(*parsed.weather.longitude, 180.0);

  const ParsedConfig invalid = parseConfig(R"toml(
[weather]
latitude = 90.01
longitude = -180.01
)toml",
                                           missing);

  EXPECT_FALSE(invalid.weather.latitude.has_value());
  EXPECT_FALSE(invalid.weather.longitude.has_value());
}

TEST(ConfigParsersTest, NotificationParsersApplyRangesAndHistoryBooleans) {
  MissingDefaults missing;
  const ParsedConfig parsed = parseConfig(R"toml(
[notifications]
default_timeout_ms = 2500
max_visible = 99

[notifications.history]
enabled = false
max_items = 250
max_age_days = 30
persist_body = false
)toml",
                                          missing);

  EXPECT_EQ(parsed.notifications.default_timeout_ms, 2500);
  EXPECT_EQ(parsed.notifications.max_visible, NotificationsConfig::kMaxVisible);
  EXPECT_FALSE(parsed.notification_history.enabled);
  EXPECT_EQ(parsed.notification_history.max_items, 250);
  EXPECT_EQ(parsed.notification_history.max_age_days, 30);
  EXPECT_FALSE(parsed.notification_history.persist_body);

  EXPECT_FALSE(missing.notif_default_timeout);
  EXPECT_FALSE(missing.notif_max_visible);
  EXPECT_FALSE(missing.notif_history_enabled);
  EXPECT_FALSE(missing.notif_history_max_items);
  EXPECT_FALSE(missing.notif_history_max_age_days);
  EXPECT_FALSE(missing.notif_history_persist_body);
}

TEST(ConfigParsersTest, WidgetsParserKeepsValidDefinitionsAndSkipsInvalidOnes) {
  MissingDefaults missing;
  const ParsedConfig parsed = parseConfig(R"toml(
[widgets]
margin = -4

[[widget]]
type = "time-to-event"
title = "Release"
deadline = "2026-07-01T12:30:00"
show_seconds = true
position = "right-bottom"
monitors = ["eDP-1", 99, "HDMI-A-1"]

[[widget]]
type = "clock"
show_seconds = false
date_format = "yyyy-MM-dd"
locale = "uk_UA"
position = "center-top"

[[widget]]
type = "time-to-event"
title = "Missing deadline"

[[widget]]
type = "clock"
enabled = false
)toml",
                                          missing);

  EXPECT_EQ(parsed.widgets.margin, WidgetsConfig{}.margin);
  ASSERT_EQ(parsed.widgets.definitions.size(), 3);

  const WidgetDefinition& event = parsed.widgets.definitions.at(0);
  EXPECT_EQ(event.type, WidgetType::TimeToEvent);
  EXPECT_TRUE(event.enabled);
  EXPECT_EQ(event.position, WidgetPosition::RightBottom);
  EXPECT_EQ(event.monitors, (QStringList{QStringLiteral("eDP-1"), QStringLiteral("HDMI-A-1")}));
  EXPECT_EQ(event.time_to_event.title, QStringLiteral("Release"));
  EXPECT_TRUE(event.time_to_event.has_time);
  EXPECT_TRUE(event.time_to_event.show_seconds);
  EXPECT_TRUE(event.time_to_event.deadline.isValid());

  const WidgetDefinition& clock = parsed.widgets.definitions.at(1);
  EXPECT_EQ(clock.type, WidgetType::Clock);
  EXPECT_TRUE(clock.enabled);
  EXPECT_EQ(clock.position, WidgetPosition::CenterTop);
  EXPECT_FALSE(clock.clock.show_seconds);
  EXPECT_EQ(clock.clock.date_format, QStringLiteral("yyyy-MM-dd"));
  EXPECT_EQ(clock.clock.locale, QStringLiteral("uk_UA"));

  const WidgetDefinition& disabled = parsed.widgets.definitions.at(2);
  EXPECT_EQ(disabled.type, WidgetType::Clock);
  EXPECT_FALSE(disabled.enabled);
}

TEST(ConfigParsersTest, DisabledTimeToEventPreservesValidFields) {
  MissingDefaults missing;
  const ParsedConfig parsed = parseConfig(R"toml(
[[widget]]
type = "time-to-event"
enabled = false
title = "Standup"
deadline = "2026-03-01T09:00:00"
)toml",
                                          missing);

  ASSERT_EQ(parsed.widgets.definitions.size(), 1);
  const WidgetDefinition& def = parsed.widgets.definitions.at(0);
  EXPECT_FALSE(def.enabled);
  EXPECT_EQ(def.type, WidgetType::TimeToEvent);
  EXPECT_EQ(def.time_to_event.title, QStringLiteral("Standup"));
  EXPECT_TRUE(def.time_to_event.deadline.isValid());
}

TEST(ConfigParsersTest, DisabledClockPreservesFields) {
  MissingDefaults missing;
  const ParsedConfig parsed = parseConfig(R"toml(
[[widget]]
type = "clock"
enabled = false
show_seconds = false
date_format = "yyyy-MM-dd"
locale = "uk_UA"
)toml",
                                          missing);

  ASSERT_EQ(parsed.widgets.definitions.size(), 1);
  const WidgetDefinition& def = parsed.widgets.definitions.at(0);
  EXPECT_FALSE(def.enabled);
  EXPECT_EQ(def.type, WidgetType::Clock);
  EXPECT_FALSE(def.clock.show_seconds);
  EXPECT_EQ(def.clock.date_format, QStringLiteral("yyyy-MM-dd"));
  EXPECT_EQ(def.clock.locale, QStringLiteral("uk_UA"));
}

TEST(ConfigParsersTest, MprisWidgetParsesDefaultAndExplicitPauseHideMinutes) {
  MissingDefaults missing;
  const ParsedConfig parsed = parseConfig(R"toml(
[[widget]]
type = "mpris"
position = "left-center"

[[widget]]
type = "mpris"
position = "right-center"
pause_hide_minutes = 15
)toml",
                                          missing);

  ASSERT_EQ(parsed.widgets.definitions.size(), 2);

  const WidgetDefinition& defaulted = parsed.widgets.definitions.at(0);
  EXPECT_EQ(defaulted.type, WidgetType::Mpris);
  EXPECT_TRUE(defaulted.enabled);
  EXPECT_EQ(defaulted.position, WidgetPosition::LeftCenter);
  EXPECT_EQ(defaulted.mpris.pause_hide_minutes, 10);

  const WidgetDefinition& explicitly_set = parsed.widgets.definitions.at(1);
  EXPECT_EQ(explicitly_set.type, WidgetType::Mpris);
  EXPECT_EQ(explicitly_set.position, WidgetPosition::RightCenter);
  EXPECT_EQ(explicitly_set.mpris.pause_hide_minutes, 15);
}

TEST(ConfigParsersTest, MprisWidgetClampsPauseHideMinutesToValidRange) {
  MissingDefaults missing;
  const ParsedConfig parsed = parseConfig(R"toml(
[[widget]]
type = "mpris"
pause_hide_minutes = 0

[[widget]]
type = "mpris"
pause_hide_minutes = 999
)toml",
                                          missing);

  ASSERT_EQ(parsed.widgets.definitions.size(), 2);
  EXPECT_EQ(parsed.widgets.definitions.at(0).mpris.pause_hide_minutes, MprisWidgetConfig::kMinPauseHideMinutes);
  EXPECT_EQ(parsed.widgets.definitions.at(1).mpris.pause_hide_minutes, MprisWidgetConfig::kMaxPauseHideMinutes);
}

TEST(ConfigParsersTest, DisabledMprisPreservesPauseHideMinutes) {
  MissingDefaults missing;
  const ParsedConfig parsed = parseConfig(R"toml(
[[widget]]
type = "mpris"
enabled = false
pause_hide_minutes = 20
)toml",
                                          missing);

  ASSERT_EQ(parsed.widgets.definitions.size(), 1);
  const WidgetDefinition& def = parsed.widgets.definitions.at(0);
  EXPECT_FALSE(def.enabled);
  EXPECT_EQ(def.type, WidgetType::Mpris);
  EXPECT_EQ(def.mpris.pause_hide_minutes, 20);
}

TEST(ConfigParsersTest, DisabledEntryPreservesPositionAndMonitors) {
  MissingDefaults missing;
  const ParsedConfig parsed = parseConfig(R"toml(
[[widget]]
type = "time-to-event"
enabled = false
title = "Standup"
deadline = "2026-03-01T09:00:00"
position = "left-bottom"
monitors = ["eDP-1", "HDMI-A-1"]
)toml",
                                          missing);

  ASSERT_EQ(parsed.widgets.definitions.size(), 1);
  const WidgetDefinition& def = parsed.widgets.definitions.at(0);
  EXPECT_EQ(def.position, WidgetPosition::LeftBottom);
  EXPECT_EQ(def.monitors, (QStringList{QStringLiteral("eDP-1"), QStringLiteral("HDMI-A-1")}));
}

TEST(ConfigParsersTest, DisabledEntryWithMissingTitleDefaultsToEmptyStringWithoutWarning) {
  MissingDefaults missing;
  QTest::failOnWarning("Config: time-to-event widget requires a non-empty title; skipping");
  const ParsedConfig parsed = parseConfig(R"toml(
[[widget]]
type = "time-to-event"
enabled = false
deadline = "2026-03-01T09:00:00"
)toml",
                                          missing);

  ASSERT_EQ(parsed.widgets.definitions.size(), 1);
  const WidgetDefinition& def = parsed.widgets.definitions.at(0);
  EXPECT_TRUE(def.time_to_event.title.isEmpty());
  EXPECT_TRUE(def.time_to_event.deadline.isValid());
}

TEST(ConfigParsersTest, DisabledEntryWithInvalidDeadlineDefaultsWithoutWarning) {
  MissingDefaults missing;
  QTest::failOnWarning("Config: time-to-event widget \"Standup\" has unparseable deadline \"not-a-date\" — skipping");
  const ParsedConfig parsed = parseConfig(R"toml(
[[widget]]
type = "time-to-event"
enabled = false
title = "Standup"
deadline = "not-a-date"
)toml",
                                          missing);

  ASSERT_EQ(parsed.widgets.definitions.size(), 1);
  const WidgetDefinition& def = parsed.widgets.definitions.at(0);
  EXPECT_EQ(def.time_to_event.title, QStringLiteral("Standup"));
  EXPECT_FALSE(def.time_to_event.deadline.isValid());
}

TEST(ConfigParsersTest, DisabledEntryWithInvalidPositionDefaultsToCenterCenter) {
  MissingDefaults missing;
  QTest::ignoreMessage(QtWarningMsg,
                       "Config: widget \"Standup\" has invalid position \"invalid-position-name\" — skipping");
  const ParsedConfig parsed = parseConfig(R"toml(
[[widget]]
type = "time-to-event"
enabled = false
title = "Standup"
deadline = "2026-03-01T09:00:00"
position = "invalid-position-name"
)toml",
                                          missing);

  ASSERT_EQ(parsed.widgets.definitions.size(), 1);
  const WidgetDefinition& def = parsed.widgets.definitions.at(0);
  EXPECT_EQ(def.position, WidgetPosition::CenterCenter);
}

TEST(ConfigParsersTest, DisabledEntryMonitorsSkipsNonStringEntriesOnly) {
  MissingDefaults missing;
  const ParsedConfig parsed = parseConfig(R"toml(
[[widget]]
type = "time-to-event"
enabled = false
title = "Standup"
deadline = "2026-03-01T09:00:00"
monitors = ["eDP-1", 99, "HDMI-A-1"]
)toml",
                                          missing);

  ASSERT_EQ(parsed.widgets.definitions.size(), 1);
  const WidgetDefinition& def = parsed.widgets.definitions.at(0);
  EXPECT_EQ(def.monitors, (QStringList{QStringLiteral("eDP-1"), QStringLiteral("HDMI-A-1")}));
}

TEST(ConfigParsersTest, EnabledEntryWithInvalidFieldStillRejectsWholeEntry) {
  MissingDefaults missing;
  const ParsedConfig parsed = parseConfig(R"toml(
[[widget]]
type = "time-to-event"
title = "Standup"
deadline = "2026-03-01T09:00:00"
position = "invalid-position-name"
)toml",
                                          missing);

  EXPECT_TRUE(parsed.widgets.definitions.isEmpty());
}

TEST(ConfigParsersTest, DisabledEntryWithUnknownTypeStaysUnchanged) {
  MissingDefaults missing;
  const ParsedConfig parsed = parseConfig(R"toml(
[[widget]]
type = "bogus"
enabled = false
title = "Standup"
)toml",
                                          missing);

  ASSERT_EQ(parsed.widgets.definitions.size(), 1);
  const WidgetDefinition& def = parsed.widgets.definitions.at(0);
  EXPECT_FALSE(def.enabled);
  EXPECT_EQ(def.type, WidgetType::TimeToEvent);  // default member value; never set for unknown type
  EXPECT_TRUE(def.time_to_event.title.isEmpty());
}

TEST(ConfigParsersTest, CalendarParserAcceptsOnlySundayOverride) {
  MissingDefaults missing;
  EXPECT_EQ(parseConfig("[calendar]\nweek_start_day = \"Sun\"\n", missing).calendar.week_start_day,
            WeekStartDay::Sunday);

  missing = MissingDefaults{};
  EXPECT_EQ(parseConfig("[calendar]\nweek_start_day = \"Fri\"\n", missing).calendar.week_start_day,
            WeekStartDay::Monday);
}

TEST(ConfigParsersTest, OsdParserTreatsAbsentAndEmptySectionIdentically) {
  MissingDefaults missing;
  const OsdConfig absent = parseConfig("[appearance]\nui_font = \"Inter\"\n", missing).osd;

  missing = MissingDefaults{};
  const OsdConfig empty = parseConfig("[osd]\n", missing).osd;

  EXPECT_EQ(absent, OsdConfig{});
  EXPECT_EQ(empty, OsdConfig{});
  EXPECT_TRUE(absent.enabled);
  EXPECT_EQ(absent.timeout_ms, 1500);
  EXPECT_EQ(absent.position, WidgetPosition::CenterBottom);
  EXPECT_TRUE(absent.volume.enabled);
  EXPECT_TRUE(absent.brightness.enabled);
  EXPECT_TRUE(absent.keyboard_layout.enabled);
}

TEST(ConfigParsersTest, OsdParserAppliesEverySupportedKey) {
  MissingDefaults missing;
  const OsdConfig cfg = parseConfig(R"(
[osd]
enabled = false
timeout = 2500
position = "right-top"

[osd.volume]
enabled = false

[osd.brightness]
enabled = false

[osd.keyboard_layout]
enabled = false
)",
                                    missing)
                            .osd;

  EXPECT_FALSE(cfg.enabled);
  EXPECT_EQ(cfg.timeout_ms, 2500);
  EXPECT_EQ(cfg.position, WidgetPosition::RightTop);
  EXPECT_FALSE(cfg.volume.enabled);
  EXPECT_FALSE(cfg.brightness.enabled);
  EXPECT_FALSE(cfg.keyboard_layout.enabled);
}

TEST(ConfigParsersTest, OsdParserClampsTimeoutAndFallsBackOnInvalidPosition) {
  MissingDefaults missing;
  EXPECT_EQ(parseConfig("[osd]\ntimeout = 10\n", missing).osd.timeout_ms, OsdConfig::kMinTimeoutMs);

  missing = MissingDefaults{};
  EXPECT_EQ(parseConfig("[osd]\ntimeout = 999999\n", missing).osd.timeout_ms, OsdConfig::kMaxTimeoutMs);

  missing = MissingDefaults{};
  EXPECT_EQ(parseConfig("[osd]\nposition = \"middle-of-nowhere\"\n", missing).osd.position,
            WidgetPosition::CenterBottom);
}

TEST(ConfigParsersTest, OsdSectionSurvivesAWriterRoundTrip) {
  QTemporaryDir tmp;
  ASSERT_TRUE(tmp.isValid());
  const QString path = tmp.path() + "/config.toml";

  ParsedConfig config;
  config.osd.enabled = false;
  config.osd.timeout_ms = 4200;
  config.osd.position = WidgetPosition::LeftCenter;
  config.osd.volume.enabled = false;
  config.osd.brightness.enabled = true;
  config.osd.keyboard_layout.enabled = false;

  ASSERT_TRUE(ConfigWriter::write(config, path));

  QFile file(path);
  ASSERT_TRUE(file.open(QIODevice::ReadOnly | QIODevice::Text));
  MissingDefaults missing;
  const ParsedConfig reloaded = parseConfigTable(toml::parse(file.readAll().toStdString()), missing);

  EXPECT_EQ(reloaded.osd, config.osd);
}

TEST(ConfigParsersTest, MprisWidgetSurvivesAWriterRoundTrip) {
  QTemporaryDir tmp;
  ASSERT_TRUE(tmp.isValid());
  const QString path = tmp.path() + "/config.toml";

  ParsedConfig config;
  WidgetDefinition def;
  def.type = WidgetType::Mpris;
  def.enabled = true;
  def.position = WidgetPosition::RightBottom;
  def.monitors = {QStringLiteral("eDP-1")};
  def.mpris.pause_hide_minutes = 25;
  config.widgets.definitions = {def};

  ASSERT_TRUE(ConfigWriter::write(config, path));

  QFile file(path);
  ASSERT_TRUE(file.open(QIODevice::ReadOnly | QIODevice::Text));
  MissingDefaults missing;
  const ParsedConfig reloaded = parseConfigTable(toml::parse(file.readAll().toStdString()), missing);

  ASSERT_EQ(reloaded.widgets.definitions.size(), 1);
  EXPECT_EQ(reloaded.widgets.definitions.at(0), def);
}

}  // namespace
