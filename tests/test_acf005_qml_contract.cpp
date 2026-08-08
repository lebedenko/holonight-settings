#include <QFile>
#include <QString>

#include <algorithm>
#include <array>
#include <gtest/gtest.h>

namespace {

QString readProjectFile(const char* relative_path) {
  QFile file(QStringLiteral(TEST_SOURCE_DIR "/") + QString::fromUtf8(relative_path));
  if (!file.open(QIODevice::ReadOnly)) {
    return {};
  }
  return QString::fromUtf8(file.readAll());
}

template <std::size_t Size>
bool containsAllObjectNames(const QString& qml, const std::array<QString, Size>& object_names) {
  return std::ranges::all_of(object_names, [&qml](const QString& object_name) {
    return qml.contains(QStringLiteral("objectName: \"") + object_name + QLatin1Char('"'));
  });
}

}  // namespace

TEST(Acf005QmlContractTest, AppearancePageExposesEveryCanonicalControl) {
  const QString qml = readProjectFile("apps/settings/qml/AppearancePage.qml");
  ASSERT_FALSE(qml.isEmpty());

  const std::array object_names{
      QStringLiteral("colorSchemeRow"), QStringLiteral("accentColorRow"),   QStringLiteral("darkModeRow"),
      QStringLiteral("uiFontRow"),      QStringLiteral("monospaceFontRow"), QStringLiteral("titleFontRow"),
      QStringLiteral("displayFontRow"), QStringLiteral("iconThemeRow"),     QStringLiteral("fallbackIconThemeRow"),
      QStringLiteral("cursorThemeRow"), QStringLiteral("layoutScaleRow"),   QStringLiteral("cornerStyleRow"),
      QStringLiteral("shapeScaleRow"),  QStringLiteral("baseRadiusRow"),    QStringLiteral("baseChamferRow")};
  EXPECT_TRUE(containsAllObjectNames(qml, object_names));
  EXPECT_TRUE(qml.contains(QStringLiteral("retainedFamily: editModel.uiFont")));
  EXPECT_TRUE(qml.contains(QStringLiteral("retainedFamily: editModel.monospaceFont")));
  EXPECT_TRUE(qml.contains(QStringLiteral("lightModeAvailable")));
  EXPECT_TRUE(qml.contains(QStringLiteral("darkModeAvailable")));
}

TEST(Acf005QmlContractTest, LegacyControlsAndDuplicateActionsAreAbsent) {
  const QString appearance = readProjectFile("apps/settings/qml/AppearancePage.qml");
  const QString bar = readProjectFile("apps/settings/qml/BarPage.qml");
  const QString navigation = readProjectFile("apps/settings/qml/NavPanel.qml");
  const QString swatch = readProjectFile("apps/settings/qml/ColorSchemeSwatchCard.qml");
  const QString weather = readProjectFile("apps/settings/qml/WeatherPage.qml");
  const QString footer = readProjectFile("apps/settings/qml/FooterBar.qml");

  EXPECT_TRUE(appearance.contains(QStringLiteral("import QtQuick.Controls.Basic as QQC2")));
  EXPECT_FALSE(bar.contains(QStringLiteral("import QtQuick.Controls.Basic")));
  EXPECT_FALSE(weather.contains(QStringLiteral("import QtQuick.Controls.Basic")));
  EXPECT_FALSE(appearance.contains(QStringLiteral("transparency"), Qt::CaseInsensitive));
  EXPECT_FALSE(appearance.contains(QStringLiteral("blur"), Qt::CaseInsensitive));
  EXPECT_FALSE(navigation.contains(QStringLiteral("HnControlMetrics")));
  EXPECT_FALSE(swatch.contains(QStringLiteral("HoloniightPalette.borderWidth")));
  EXPECT_FALSE(swatch.contains(QStringLiteral("HoloniightPalette.focusBorderWidth")));
  EXPECT_TRUE(navigation.contains(QStringLiteral("HnMetrics.iconSize")));
  EXPECT_TRUE(swatch.contains(QStringLiteral("HnMetrics.borderWidth")));
  EXPECT_FALSE(footer.contains(QStringLiteral("Save & Apply")));
  EXPECT_FALSE(footer.contains(QStringLiteral("objectName: \"applyButton\"")));
  EXPECT_TRUE(footer.contains(QStringLiteral("objectName: \"saveButton\"")));
  EXPECT_TRUE(footer.contains(QStringLiteral("enabled: root.saveCoordinator.isDirty && !root.saveCoordinator.isBusy")));
  EXPECT_TRUE(footer.contains(QStringLiteral("rawText: root.saveCoordinator.resultText")));
  EXPECT_TRUE(footer.contains(QStringLiteral("Reload")));
  EXPECT_TRUE(footer.contains(QStringLiteral("Overwrite")));
  EXPECT_TRUE(footer.contains(QStringLiteral("Cancel")));
  EXPECT_TRUE(footer.contains(QStringLiteral("surfaceRole: HnSurfaceRole.Popup")));
  EXPECT_TRUE(footer.contains(QStringLiteral("fillColor: HoloniightPalette.surfaceRaised")));
}

TEST(Acf005QmlContractTest, PagesReceiveOnlyTheirOwningDomainModel) {
  const QString stack = readProjectFile("apps/settings/qml/ContentStack.qml");
  const QString window = readProjectFile("apps/settings/qml/SettingsWindow.qml");

  EXPECT_TRUE(stack.contains(QStringLiteral("properties.editModel = root.appearanceModel")));
  EXPECT_TRUE(stack.contains(QStringLiteral("properties.editModel = root.shellModel")));
  EXPECT_TRUE(window.contains(QStringLiteral("required property AppearanceEditModel appearanceModel")));
  EXPECT_TRUE(window.contains(QStringLiteral("required property ShellSettingsEditModel shellModel")));
  EXPECT_TRUE(window.contains(QStringLiteral("required property SettingsSaveCoordinator saveCoordinator")));
}
