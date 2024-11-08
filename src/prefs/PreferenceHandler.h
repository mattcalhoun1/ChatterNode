#ifndef COMMUNICATORPREFHANDLER_H
#define COMMUNICATORPREFHANDLER_H

enum CommunicatorPreference {
    PreferenceKeyboardLandscape = 0,
    PreferenceMessageHistory = 1,
    PreferenceWifiEnabled = 2,
    PreferenceMeshEnabled = 3,
    PreferenceWiredEnabled = 4,
    PreferenceLoraEnabled = 5,
    PreferenceMeshLearningEnabled = 6,
    PreferenceRemoteConfigEnabled = 7,
    PreferenceDstEnabled = 8,
    PreferenceIgnoreExpiryEnabled = 9,
    PreferenceBackpacksEnabled = 10,
    PreferenceBackpackThermalEnabled = 11,
    PreferenceBackpackThermalRemoteEnabled = 12,
    PreferenceBackpackThermalAutoEnabled = 13,
    PreferenceBackpackRelayEnabled = 14,
    PreferenceBackpackRelayRemoteEnabled = 15,
    PreferenceTruststoreLocked = 16,
    PreferenceKeyForwarding = 17,
    PreferenceLocationSharingEnabled = 18,
    PreferenceLocationMapTypeLink = 19,
    PreferenceGnssEnabled = 20, // gps remain on after initial startup
    PreferenceGnssGPSEnabled = 21,
    PreferenceGnssGlonassEnabled = 22,
    PreferenceGnssBeiDouEnabled = 23,
    PreferenceExperimentalFeaturesEnabled = 24,
    PreferenceAnalysisEnabled = 25 
};

enum StoredPreference {
    StoredPrefBackpacksEnabled = 0,
    StoredPrefBackpackThermalEnabled = 1,
    StoredPrefBackpackThermalRemoteEnabled = 2,
    StoredPrefBackpackThermalAutoEnabled = 3,
    StoredPrefBackpackRelayEnabled = 4,
    StoredPrefBackpackRelayRemoteEnabled = 5,
    StoredPrefTruststoreLocked = 6,
    StoredPrefKeyForwarding = 7,
    StoredPrefLocationSharingEnabled = 8,
    StoredPrefLocationMapTypeLink = 9,
    StoredPrefGnssEnabled = 10,
    StoredPrefGnssGPSEnabled = 11,
    StoredPrefGnssGlonassEnabled = 12,
    StoredPrefGnssBeiDouEnabled = 13,
    StoredPrefExperimentalFeaturesEnabled = 14,
    StoredPrefAnalysisEnabled = 15
};

class PreferenceHandler {
    public:
        virtual bool isPreferenceEnabled (CommunicatorPreference pref) = 0;
        virtual void enablePreference (CommunicatorPreference pref) = 0;
        virtual void disablePreference (CommunicatorPreference pref) = 0;
        virtual void applyGnssConfig ();
};

#endif