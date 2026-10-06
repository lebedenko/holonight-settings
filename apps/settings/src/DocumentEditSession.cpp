// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 HoloNight contributors
#include "DocumentEditSession.h"

#include <QMetaMethod>
#include <QMetaProperty>

#include <algorithm>
#include <holonight/config/appearance_document.h>

namespace Config = HoloNight::Config;
namespace {
QVariant variant(const Config::Value& value) {
  return std::visit(
      [](const auto& item) -> QVariant {
        using T = std::decay_t<decltype(item)>;
        if constexpr (std::is_same_v<T, std::string>) {
          return QString::fromStdString(item);
        } else if constexpr (std::is_same_v<T, Config::AggregateValue>) {
          return QString::fromStdString(item.toml);
        } else {
          return QVariant::fromValue(item);
        }
      },
      value);
}
Config::Value value(const QVariant& item) {
  switch (item.metaType().id()) {
    case QMetaType::Bool:
      return item.toBool();
    case QMetaType::Int:
    case QMetaType::LongLong:
      return static_cast<std::int64_t>(item.toLongLong());
    case QMetaType::Double:
      return item.toDouble();
    default:
      return item.toString().toStdString();
  }
}
QString notes(const std::vector<Config::Diagnostic>& diagnostics) {
  QStringList result;
  for (const auto& diagnostic : diagnostics) {
    result << QString::fromStdString(diagnostic.message);
  }
  return result.join(QLatin1Char('\n'));
}
QString display(const std::optional<Config::Value>& item, bool sensitive) {
  if (!item) {
    return QStringLiteral("Default (no override)");
  }
  return sensitive ? QStringLiteral("••••••") : variant(*item).toString();
}
}  // namespace

DocumentEditSession::DocumentEditSession(QObject* model, QString path, std::vector<Field> fields,
                                         Config::DocumentSchema schema, Projection projection, bool appearance)
    : model_(model),
      fields_(std::move(fields)),
      schema_(std::move(schema)),
      projection_(std::move(projection)),
      appearance_(appearance),
      watcher_(std::move(path)) {
  const auto slot = staticMetaObject.method(staticMetaObject.indexOfSlot("capture()"));
  for (const auto& field : fields_) {
    for (const auto& property : {field.property, field.enabled_property}) {
      const int index = model_->metaObject()->indexOfProperty(property.constData());
      if (index >= 0) {
        connect(model_, model_->metaObject()->property(index).notifySignal(), this, slot);
      }
    }
    shown_[field.key] = controlValue(field);
  }
  connect(&watcher_, &Holonight::DocumentWatcher::documentChanged, this, [this] {
    if (!suspended_) {
      static_cast<void>(reload());
    }
  });
}
bool DocumentEditSession::dirty() const { return retry_ || !edits_.empty(); }
std::optional<Config::Value> DocumentEditSession::controlValue(const Field& field) const {
  if (!field.enabled_property.isEmpty() && !model_->property(field.enabled_property.constData()).toBool()) {
    return std::nullopt;
  }
  return value(model_->property(field.property.constData()));
}
void DocumentEditSession::notify() {
  emit stateChanged();
  if (last_dirty_ != dirty()) {
    last_dirty_ = dirty();
    emit dirtyChanged();
  }
}
bool DocumentEditSession::reload(bool discard) {
  const auto loaded = watcher_.read();
  watcher_.refresh();
  if (!loaded) {
    blocked_ = true;
    diagnostics_ = notes(loaded.diagnostics);
    notify();
    return false;
  }
  const auto projected = projection_(*loaded.value);
  if (!projected) {
    blocked_ = true;
    diagnostics_ = notes(projected.diagnostics);
    notify();
    return false;
  }
  blocked_ = false;
  diagnostics_ = notes(projected.diagnostics);
  snapshot_ = *loaded.value;
  effective_ = *projected.value;
  if (discard) {
    edits_.clear();
    retry_ = false;
  } else {
    reconcile();
  }
  show();
  notify();
  return true;
}
void DocumentEditSession::reconcile() {
  std::erase_if(edits_, [this](const Config::Edit& edit) { return snapshot_.value(edit.key) == edit.pending; });
}
void DocumentEditSession::show() {
  QVariantMap desired = effective_;
  auto preview = edits_;
  for (auto& edit : preview) {
    edit.baseline = snapshot_.value(edit.key);
  }
  if (appearance_ && !preview.empty()) {
    preview.push_back(
        {.key = {"version"}, .baseline = snapshot_.value({"version"}), .pending = Config::Value{std::int64_t{2}}});
  }
  const auto patched = Config::patchDocument(snapshot_, preview, schema_);
  if (patched.status == Config::SaveStatus::Success) {
    const auto projected = projection_(*patched.snapshot);
    if (projected) {
      desired = *projected.value;
    }
  } else {
    diagnostics_ = notes(patched.diagnostics);
    for (const auto& field : fields_) {
      const auto edit = std::ranges::find(edits_, field.key, &Config::Edit::key);
      if (edit == edits_.end()) {
        continue;
      }
      if (edit->pending) {
        desired[field.property] = variant(*edit->pending);
      }
      if (!field.enabled_property.isEmpty()) {
        desired[field.enabled_property] = edit->pending.has_value();
      }
    }
  }
  refreshing_ = true;
  // Enabled flags follow values so optional fields can be removed without setters re-enabling them.
  for (const auto& field : fields_) {
    if (desired.contains(field.property) && model_->property(field.property.constData()) != desired[field.property]) {
      model_->setProperty(field.property.constData(), desired[field.property]);
    }
    if (!field.enabled_property.isEmpty()) {
      model_->setProperty(field.enabled_property.constData(), desired[field.enabled_property]);
    }
  }
  for (const auto& field : fields_) {
    shown_[field.key] = controlValue(field);
  }
  refreshing_ = false;
}
void DocumentEditSession::capture() {
  if (refreshing_) {
    return;
  }
  for (const auto& field : fields_) {
    const auto pending = controlValue(field);
    if (pending == shown_[field.key]) {
      continue;
    }
    shown_[field.key] = pending;
    auto edit = std::ranges::find(edits_, field.key, &Config::Edit::key);
    const auto disk = snapshot_.value(field.key);
    const bool effective_match = field.enabled_property.isEmpty()
                                     ? pending == std::optional<Config::Value>{value(effective_.value(field.property))}
                                     : effective_.value(field.enabled_property).toBool() == pending.has_value() &&
                                           (!pending || *pending == value(effective_.value(field.property)));
    if (!suspended_ && (pending == disk || effective_match)) {
      if (edit != edits_.end()) {
        edits_.erase(edit);
      }
    } else if (edit == edits_.end()) {
      edits_.push_back({.key = field.key, .baseline = disk, .pending = pending});
    } else {
      edit->pending = pending;
    }
  }
  show();
  notify();
}
QVariantList DocumentEditSession::fields() const {
  QVariantList result;
  for (const auto& field : fields_) {
    const auto pending = std::ranges::find(edits_, field.key, &Config::Edit::key);
    QVariantMap row{
        {"property", QString::fromUtf8(field.property)},
        {"label", field.label},
        {"overridden", pending == edits_.end() ? snapshot_.value(field.key).has_value() : pending->pending.has_value()},
        {"pending", pending != edits_.end()},
    };
    for (auto it = field.constraints.begin(); it != field.constraints.end(); ++it) {
      row.insert(it.key(), it.value());
    }
    result.push_back(row);
  }
  return result;
}
QVariantList DocumentEditSession::conflicts() const {
  QVariantList result;
  for (const auto& field : fields_) {
    const auto edit = std::ranges::find(edits_, field.key, &Config::Edit::key);
    const auto disk = snapshot_.value(field.key);
    if (edit == edits_.end() || disk == edit->baseline || disk == edit->pending) {
      continue;
    }
    result.push_back(QVariantMap{
        {"property", QString::fromUtf8(field.property)},
        {"label", field.label},
        {"baseline", display(edit->baseline, field.sensitive)},
        {"disk", display(disk, field.sensitive)},
        {"pending", display(edit->pending, field.sensitive)},
    });
  }
  return result;
}
void DocumentEditSession::reset(const QString& property) {
  const auto field = std::ranges::find(fields_, property.toUtf8(), &Field::property);
  if (field == fields_.end()) {
    return;
  }
  auto edit = std::ranges::find(edits_, field->key, &Config::Edit::key);
  if (!snapshot_.value(field->key)) {
    if (edit != edits_.end()) {
      edits_.erase(edit);
    }
  } else if (edit == edits_.end()) {
    edits_.push_back({.key = field->key, .baseline = snapshot_.value(field->key), .pending = std::nullopt});
  } else {
    edit->pending = std::nullopt;
  }
  show();
  notify();
}
void DocumentEditSession::resolve(const QString& property, bool keepPending) {
  if (!reload()) {
    return;
  }
  const auto field = std::ranges::find(fields_, property.toUtf8(), &Field::property);
  if (field == fields_.end()) {
    return;
  }
  const auto edit = std::ranges::find(edits_, field->key, &Config::Edit::key);
  if (edit == edits_.end()) {
    return;
  }
  if (keepPending) {
    edit->baseline = snapshot_.value(field->key);
  } else {
    edits_.erase(edit);
  }
  show();
  notify();
}
void DocumentEditSession::accept(const Config::DocumentSnapshot& snapshot, const Config::EditBatch& saved) {
  for (auto& edit : edits_) {
    if (std::ranges::find(saved, edit.key, &Config::Edit::key) != saved.end()) {
      edit.baseline = snapshot.value(edit.key);
    }
  }
  snapshot_ = snapshot;
  retry_ = false;
  reconcile();
  suspended_ = false;
  static_cast<void>(reload());
}
void DocumentEditSession::suspend(bool value) {
  suspended_ = value;
  if (!value) {
    static_cast<void>(reload());
  }
}
void DocumentEditSession::retry() {
  retry_ = true;
  notify();
}
void DocumentEditSession::report(const Config::SaveResult& result) {
  static_cast<void>(reload());
  if (result.status == Config::SaveStatus::DurabilityFailure) {
    retry();
  }
  diagnostics_ = notes(result.diagnostics);
  if (diagnostics_.isEmpty()) {
    diagnostics_ = result.status == Config::SaveStatus::Conflict
                       ? QStringLiteral("Resolve the conflicting values before saving")
                       : QStringLiteral("Configuration save failed");
  }
  notify();
}

QVariantMap DocumentEditSession::properties(const QObject& model, const std::vector<Field>& fields) {
  QVariantMap result;
  for (const auto& field : fields) {
    result[field.property] = model.property(field.property.constData());
    if (!field.enabled_property.isEmpty()) {
      result[field.enabled_property] = model.property(field.enabled_property.constData());
    }
  }
  return result;
}

QVariantMap DocumentEditSession::metadata(const QString& property) const {
  const auto field = std::ranges::find(fields_, property.toUtf8(), &Field::property);
  return field == fields_.end() ? QVariantMap{} : field->constraints;
}
