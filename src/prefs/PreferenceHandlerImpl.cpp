#include "PreferenceHandlerImpl.h"

bool PreferenceHandlerImpl::isPreferenceEnabled (CommunicatorPreference pref) {
  switch (pref) {
    case PreferenceMessageHistory:
      return chatter->getDeviceStore()->getMessageHistoryEnabled();
    case PreferenceKeyboardLandscape:
      return chatter->getDeviceStore()->getKeyboardOrientedLandscape();
    case PreferenceWifiEnabled:
      return chatter->getDeviceStore()->getWifiEnabled();
    case PreferenceMeshEnabled:
      return chatter->getDeviceStore()->getMeshEnabled();
    case PreferenceWiredEnabled:
      return chatter->getDeviceStore()->getUartEnabled();
    case PreferenceLoraEnabled:
      return chatter->getDeviceStore()->getLoraEnabled();
    case PreferenceMeshLearningEnabled:
      return chatter->getDeviceStore()->getMeshLearningEnabled();
    case PreferenceRemoteConfigEnabled:
      return chatter->getDeviceStore()->getRemoteConfigEnabled();
    case PreferenceDstEnabled:
      return chatter->getDeviceStore()->getDstEnabled();
    case PreferenceIgnoreExpiryEnabled:
      return chatter->getDeviceStore()->getAllowExpiredMessages();
    case PreferenceBackpacksEnabled:
      return chatter->getDeviceStore()->getCustomPreference(StoredPrefBackpacksEnabled) == 'T';
    case PreferenceBackpackThermalEnabled:
      return chatter->getDeviceStore()->getCustomPreference(StoredPrefBackpackThermalEnabled) == 'T';
    case PreferenceBackpackThermalRemoteEnabled:
      return chatter->getDeviceStore()->getCustomPreference(StoredPrefBackpackThermalRemoteEnabled) == 'T';
    case PreferenceBackpackThermalAutoEnabled:
      return chatter->getDeviceStore()->getCustomPreference(StoredPrefBackpackThermalAutoEnabled) == 'T';
    case PreferenceBackpackRelayEnabled:
      return chatter->getDeviceStore()->getCustomPreference(StoredPrefBackpackRelayEnabled) == 'T';
    case PreferenceBackpackRelayRemoteEnabled:
      return chatter->getDeviceStore()->getCustomPreference(StoredPrefBackpackRelayRemoteEnabled) == 'T';
    case PreferenceTruststoreLocked:
      return chatter->getDeviceStore()->getCustomPreference(StoredPrefTruststoreLocked) == 'T';
    case PreferenceKeyForwarding:
      return chatter->getDeviceStore()->getCustomPreference(StoredPrefKeyForwarding) != 'F';
    case PreferenceLocationSharingEnabled:
      return chatter->getDeviceStore()->getCustomPreference(StoredPrefLocationSharingEnabled) != 'F';
    case PreferenceLocationMapTypeLink:
      return chatter->getDeviceStore()->getCustomPreference(StoredPrefLocationMapTypeLink) != 'G'; // g for 'geo' link, vs 'l' for http link
  }

  Logger::info("Unknown preference read attempt", LogAppControl);
  return false;
}

void PreferenceHandlerImpl::enablePreference (CommunicatorPreference pref) {
  switch (pref) {
    case PreferenceMessageHistory:
      chatter->getDeviceStore()->setMessageHistoryEnabled(true);
      Logger::info("Message history enabled", LogAppControl);

      // reset device
      control->queueRestart();

      break;
    case PreferenceKeyboardLandscape:
      chatter->getDeviceStore()->setKeyboardOrientedLandscape(true);

      Logger::info("Keyboard landscape enabled", LogAppControl);
      control->queueRestart();
      break;
    case PreferenceWifiEnabled:
      chatter->getDeviceStore()->setWifiEnabled(true);
      Logger::info("Wifi enabled", LogAppControl);

      // reset device
      control->queueRestart();

      break;
    case PreferenceMeshEnabled:
      chatter->getDeviceStore()->setMeshEnabled(true);
      chatter->setMeshEnabled(true);
      Logger::info("Mesh enabled", LogAppControl);
      break;
    case PreferenceWiredEnabled:
      chatter->getDeviceStore()->setUartEnabled(true);
      Logger::info("Wired enabled", LogAppControl);
      // reset device
      control->queueRestart();
      break;
    case PreferenceLoraEnabled:
      chatter->getDeviceStore()->setLoraEnabled(true);
      Logger::info("Lora enabled", LogAppControl);
      // reset device
      control->queueRestart();
      break;
    case PreferenceMeshLearningEnabled:
      chatter->getDeviceStore()->setMeshLearningEnabled(true);
      control->setLearningEnabled(true);
      break;
    case PreferenceRemoteConfigEnabled:
      chatter->getDeviceStore()->setRemoteConfigEnabled(true);
      control->setRemoteConfigEnabled(true);
      break;
    case PreferenceDstEnabled:
        chatter->getDeviceStore()->setDstEnabled(true);
        chatter->getRtc()->setDstEnabled(true);
        break;
    case PreferenceIgnoreExpiryEnabled:
      chatter->getDeviceStore()->setAllowExpiredMessages(true);
      break;
    case PreferenceBackpacksEnabled:
      chatter->getDeviceStore()->setCustomPreference(StoredPrefBackpacksEnabled, 'T');
      control->queueRestart();
      break;

    case PreferenceBackpackThermalEnabled:
      chatter->getDeviceStore()->setCustomPreference(StoredPrefBackpackThermalEnabled, 'T');
      control->queueRestart();
      break;

    case PreferenceBackpackThermalRemoteEnabled:
      chatter->getDeviceStore()->setCustomPreference(StoredPrefBackpackThermalRemoteEnabled, 'T');
      control->queueRestart();
      break;

    case PreferenceBackpackThermalAutoEnabled:
      chatter->getDeviceStore()->setCustomPreference(StoredPrefBackpackThermalAutoEnabled, 'T');
      control->queueRestart();
      break;

    case PreferenceBackpackRelayEnabled:
      chatter->getDeviceStore()->setCustomPreference(StoredPrefBackpackRelayEnabled, 'T');
      control->queueRestart();
      break;

    case PreferenceBackpackRelayRemoteEnabled:
      chatter->getDeviceStore()->setCustomPreference(StoredPrefBackpackRelayRemoteEnabled, 'T');
      control->queueRestart();
      break;

    case PreferenceTruststoreLocked:
      chatter->getDeviceStore()->setCustomPreference(StoredPrefTruststoreLocked, 'T');
      chatter->setTruststoreLocked(true);
      break;

    case PreferenceKeyForwarding:
      chatter->getDeviceStore()->setCustomPreference(StoredPrefKeyForwarding, 'T');
      chatter->setKeyForwardingAllowed(true);
      break;

    case PreferenceLocationSharingEnabled:
      chatter->getDeviceStore()->setCustomPreference(StoredPrefLocationSharingEnabled, 'T');
      chatter->setLocationSharingEnabled(true);
      break;

    case PreferenceLocationMapTypeLink:
      chatter->getDeviceStore()->setCustomPreference(StoredPrefLocationMapTypeLink, 'L');
      break;

    default:
      Logger::info("Unknown preference enable attempt", LogAppControl);
  }
}

void PreferenceHandlerImpl::disablePreference (CommunicatorPreference pref) {
  switch (pref) {
    case PreferenceMessageHistory:
      chatter->getDeviceStore()->setMessageHistoryEnabled(false);
      Logger::info("Message history disabled", LogAppControl);

      // reset device
      control->queueRestart();
      break;
    case PreferenceKeyboardLandscape:
      chatter->getDeviceStore()->setKeyboardOrientedLandscape(false);

      Logger::info("Keyboard landscape disabled", LogAppControl);
      control->queueRestart();
      break;
    case PreferenceWifiEnabled:
      chatter->getDeviceStore()->setWifiEnabled(false);
      Logger::info("Wifi disabled", LogAppControl);

      // reset device
      control->queueRestart();

      break;
    case PreferenceMeshEnabled:
      chatter->getDeviceStore()->setMeshEnabled(false);
      chatter->setMeshEnabled(false);
      Logger::info("Mesh disabled", LogAppControl);

      break;
    case PreferenceLoraEnabled:
      chatter->getDeviceStore()->setLoraEnabled(false);
      Logger::info("Lora disabled", LogAppControl);

      // reset device
      control->queueRestart();

      break;
    case PreferenceWiredEnabled:
      chatter->getDeviceStore()->setUartEnabled(false);
      Logger::info("Wired disabled", LogAppControl);

      // reset device
      control->queueRestart();

      break;
    case PreferenceMeshLearningEnabled:
      chatter->getDeviceStore()->setMeshLearningEnabled(false);
      control->setLearningEnabled(false);
      break;

    case PreferenceRemoteConfigEnabled:
      chatter->getDeviceStore()->setRemoteConfigEnabled(false);
      control->setRemoteConfigEnabled(false);
      break;
    case PreferenceDstEnabled:
        chatter->getDeviceStore()->setDstEnabled(false);
        chatter->getRtc()->setDstEnabled(false);
        break;
    case PreferenceIgnoreExpiryEnabled:
      chatter->getDeviceStore()->setAllowExpiredMessages(false);
      break;

    case PreferenceBackpacksEnabled:
      chatter->getDeviceStore()->setCustomPreference(StoredPrefBackpacksEnabled, 'F');
      control->queueRestart();
      break;

    case PreferenceBackpackThermalEnabled:
      chatter->getDeviceStore()->setCustomPreference(StoredPrefBackpackThermalEnabled, 'F');
      control->queueRestart();
      break;

    case PreferenceBackpackThermalRemoteEnabled:
      chatter->getDeviceStore()->setCustomPreference(StoredPrefBackpackThermalRemoteEnabled, 'F');
      control->queueRestart();
      break;

    case PreferenceBackpackThermalAutoEnabled:
      chatter->getDeviceStore()->setCustomPreference(StoredPrefBackpackThermalAutoEnabled, 'F');
      control->queueRestart();
      break;

    case PreferenceBackpackRelayEnabled:
      chatter->getDeviceStore()->setCustomPreference(StoredPrefBackpackRelayEnabled, 'F');
      control->queueRestart();
      break;

    case PreferenceBackpackRelayRemoteEnabled:
      chatter->getDeviceStore()->setCustomPreference(StoredPrefBackpackRelayRemoteEnabled, 'F');
      control->queueRestart();
      break;

    case PreferenceTruststoreLocked:
      chatter->getDeviceStore()->setCustomPreference(StoredPrefTruststoreLocked, 'F');
      chatter->setTruststoreLocked(false);
      break;

    case PreferenceKeyForwarding:
      chatter->getDeviceStore()->setCustomPreference(StoredPrefKeyForwarding, 'F');
      chatter->setKeyForwardingAllowed(false);
      break;

    case PreferenceLocationSharingEnabled:
      chatter->getDeviceStore()->setCustomPreference(StoredPrefLocationSharingEnabled, 'F');
      chatter->setLocationSharingEnabled(false);
      break;

    case PreferenceLocationMapTypeLink:
      chatter->getDeviceStore()->setCustomPreference(StoredPrefLocationMapTypeLink, 'G');
      break;

    default:
      Logger::info("Unknown preference disable attempt", LogAppControl);
  }
}
