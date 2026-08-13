#pragma once

#include <QAbstractItemModel>
#include <QObject>
#include <QQmlEngine>

#include <AudioController.h>

class AudioControllerQml : public QObject {
  Q_OBJECT
  QML_NAMED_ELEMENT(AudioController)
  QML_UNCREATABLE("AudioController instances are owned by Settings")
  Q_PROPERTY(int volume READ volume NOTIFY volumeChanged)
  Q_PROPERTY(bool muted READ muted NOTIFY mutedChanged)
  Q_PROPERTY(bool available READ available NOTIFY availableChanged)
  Q_PROPERTY(QAbstractItemModel* outputs READ outputs CONSTANT)
  Q_PROPERTY(QAbstractItemModel* inputs READ inputs CONSTANT)

 public:
  explicit AudioControllerQml(QObject* parent = nullptr);

  [[nodiscard]] int volume() const { return controller_.volume(); }
  [[nodiscard]] bool muted() const { return controller_.muted(); }
  [[nodiscard]] bool available() const { return controller_.available(); }
  [[nodiscard]] QAbstractItemModel* outputs() const { return controller_.outputs(); }
  [[nodiscard]] QAbstractItemModel* inputs() const { return controller_.inputs(); }

  void start() { controller_.start(); }

  Q_INVOKABLE void setVolume(int percent) { controller_.setVolume(percent); }
  Q_INVOKABLE void setDefaultOutputMuted(bool muted) { controller_.setDefaultOutputMuted(muted); }
  Q_INVOKABLE void setDefaultOutput(uint32_t idx) { controller_.setDefaultOutput(idx); }
  Q_INVOKABLE void setDefaultInput(uint32_t idx) { controller_.setDefaultInput(idx); }

 Q_SIGNALS:
  void volumeChanged();
  void mutedChanged();
  void availableChanged();

 private:
  HoloNight::System::AudioController controller_;
};
