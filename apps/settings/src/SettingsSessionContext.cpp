// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 HoloNight contributors
#include "SettingsSessionContext.h"

#include <holonight_system/compositor/CompositorFactory.h>

SettingsSessionContext::SettingsSessionContext(QObject* parent)
    : SettingsSessionContext(QProcessEnvironment::systemEnvironment(), parent) {}
SettingsSessionContext::SettingsSessionContext(const QProcessEnvironment& environment, QObject* parent)
    : QObject(parent), compositor_(selectCompositorBackend(environment)) {}
QString SettingsSessionContext::compositor() const { return compositor_; }
bool SettingsSessionContext::isApplicable(const QString& property) const {
  if (property == "workspaceCount") {
    return compositor_ == "hyprland" || compositor_ == "sway" || compositor_ == "labwc";
  }
  if (property == "taskbarEnabled" || property == "taskbarGrouped" || property == "windowOverviewAccess" ||
      property == "desktopMenuEnabled") {
    return compositor_ == "labwc";
  }
  return true;
}
