#include <QFile>
#include <QTemporaryDir>

#include <gtest/gtest.h>
#include <holonight_config/config_parsers.h>
#include <holonight_config/config_writer.h>

namespace {

ParsedConfig writeAndReloadConfig(const ParsedConfig& config, const QString& path) {
  EXPECT_TRUE(ConfigWriter::write(config, path));

  QFile file(path);
  EXPECT_TRUE(file.open(QIODevice::ReadOnly | QIODevice::Text));
  const std::string content = file.readAll().toStdString();

  MissingDefaults missing;
  return parseConfigTable(toml::parse(content), missing);
}

}  // namespace

TEST(ConfigWriterWeatherPreservationTest, PinnedCoordinatesAndCityAreWrittenAndReloaded) {
  QTemporaryDir tmp;
  const QString path = tmp.path() + "/config.toml";

  ParsedConfig config;
  config.weather.latitude = 49.83968;
  config.weather.longitude = 24.02972;
  config.weather.city = QStringLiteral("Lviv");
  config.weather.country = QStringLiteral("Ukraine");

  const ParsedConfig reloaded = writeAndReloadConfig(config, path);

  ASSERT_TRUE(reloaded.weather.latitude.has_value());
  ASSERT_TRUE(reloaded.weather.longitude.has_value());
  EXPECT_DOUBLE_EQ(*reloaded.weather.latitude, 49.83968);
  EXPECT_DOUBLE_EQ(*reloaded.weather.longitude, 24.02972);
  EXPECT_EQ(reloaded.weather.city, QStringLiteral("Lviv"));
  EXPECT_EQ(reloaded.weather.country, QStringLiteral("Ukraine"));
}

TEST(ConfigWriterWeatherPreservationTest, UnsetCoordinatesAndCityStayUnsetAfterReload) {
  QTemporaryDir tmp;
  const QString path = tmp.path() + "/config.toml";

  ParsedConfig config;
  ASSERT_FALSE(config.weather.latitude.has_value());
  ASSERT_FALSE(config.weather.longitude.has_value());
  ASSERT_TRUE(config.weather.city.isEmpty());
  ASSERT_TRUE(config.weather.country.isEmpty());

  const ParsedConfig reloaded = writeAndReloadConfig(config, path);

  EXPECT_FALSE(reloaded.weather.latitude.has_value());
  EXPECT_FALSE(reloaded.weather.longitude.has_value());
  EXPECT_TRUE(reloaded.weather.city.isEmpty());
  EXPECT_TRUE(reloaded.weather.country.isEmpty());
}

TEST(ConfigWriterWeatherPreservationTest, SecondSaveWithoutChangesPreservesPinnedLocation) {
  QTemporaryDir tmp;
  const QString path = tmp.path() + "/config.toml";

  ParsedConfig config;
  config.weather.latitude = 40.7128;
  config.weather.longitude = -74.0060;
  config.weather.city = QStringLiteral("New York");

  const ParsedConfig first_reload = writeAndReloadConfig(config, path);
  const ParsedConfig second_reload = writeAndReloadConfig(first_reload, path);

  ASSERT_TRUE(second_reload.weather.latitude.has_value());
  ASSERT_TRUE(second_reload.weather.longitude.has_value());
  EXPECT_DOUBLE_EQ(*second_reload.weather.latitude, 40.7128);
  EXPECT_DOUBLE_EQ(*second_reload.weather.longitude, -74.0060);
  EXPECT_EQ(second_reload.weather.city, QStringLiteral("New York"));
}

TEST(ConfigWriterWeatherPreservationTest, ExtendedWeatherSettingsAreWrittenAndReloaded) {
  QTemporaryDir tmp;
  const QString path = tmp.path() + "/config.toml";

  ParsedConfig config;
  config.weather.provider = QStringLiteral("openweathermap");
  config.weather.location_source = QStringLiteral("auto");
  config.weather.api_key = QStringLiteral("test_owm_key");
  config.weather.temp_unit = QStringLiteral("fahrenheit");
  config.weather.wind_unit = QStringLiteral("mph");
  config.weather.pressure_unit = QStringLiteral("inhg");
  config.weather.show_in_bar = false;
  config.weather.compact_mode = true;
  config.weather.show_feels_like = false;
  config.weather.show_location = false;
  config.weather.refresh_interval = 900;

  const ParsedConfig reloaded = writeAndReloadConfig(config, path);

  EXPECT_EQ(reloaded.weather.provider, QStringLiteral("openweathermap"));
  EXPECT_EQ(reloaded.weather.location_source, QStringLiteral("auto"));
  EXPECT_EQ(reloaded.weather.api_key, QStringLiteral("test_owm_key"));
  EXPECT_EQ(reloaded.weather.temp_unit, QStringLiteral("fahrenheit"));
  EXPECT_EQ(reloaded.weather.wind_unit, QStringLiteral("mph"));
  EXPECT_EQ(reloaded.weather.pressure_unit, QStringLiteral("inhg"));
  EXPECT_FALSE(reloaded.weather.show_in_bar);
  EXPECT_TRUE(reloaded.weather.compact_mode);
  EXPECT_FALSE(reloaded.weather.show_feels_like);
  EXPECT_FALSE(reloaded.weather.show_location);
  EXPECT_EQ(reloaded.weather.refresh_interval, 900);
}
