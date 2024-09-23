#ifndef CHATTERUSEREVENT_H
#define CHATTERUSEREVENT_H

#define MAX_NUM_EVENTS 29

enum ChatterUserEvent {
    UserEventNone = 0,
    UserEnteredLicenseKey = 1,
    UserTextInputSave = 2,
    UserTextInputCancel = 3,
    UserSelectOptionSave = 4,
    UserSelectOptionCancel = 5,
    UserYesSelect = 6,
    UserNoSelect = 7,
    ViewChangeHome = 8,
    ViewChangeNeighbors = 9,
    ViewChangeSettings = 10,
    ViewChangeNewMessage = 11,
    UserHomeSwiped = 12,
    UserNeighborsSwiped = 13,
    ViewChangeDeviceFilter = 14,
    ViewChangeFindPath = 15,
    UserSettingEditClicked = 16,
    UserBusyCancelClick = 17,
    UserClickedSend = 18,
    UserClickedDeviceLocation = 19,
    UserClickedLocationClose = 20,
    HomeScreenLoaded = 21,
    UserDeviceYesClicked = 22,
    UserDeviceNoClicked = 23,
    UserClickedHomeDeviceSetFilter = 24,
    UserClickedHomeDeviceClearFilter = 25,
    UserClickedLockScreen = 26,
    UserClickedNeighborLocation = 27,
    UserClickedSelfLocation = 28
};

#endif