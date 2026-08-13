#include "AudioControllerQml.h"

AudioControllerQml::AudioControllerQml(QObject* parent) : QObject(parent) {
  connect(&controller_, &HoloNight::System::AudioController::volumeChanged, this, &AudioControllerQml::volumeChanged);
  connect(&controller_, &HoloNight::System::AudioController::mutedChanged, this, &AudioControllerQml::mutedChanged);
  connect(&controller_, &HoloNight::System::AudioController::availableChanged, this,
          &AudioControllerQml::availableChanged);
}
