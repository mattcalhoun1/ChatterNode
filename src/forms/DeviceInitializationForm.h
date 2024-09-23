#include "NewClusterForm.h"

#ifndef DEVICEINITIALIZATIONFORM_H
#define DEVICEINITIALIZATIONFORM_H

struct DeviceInitializationForm : public NewClusterForm {
    char daylightSavings[7];
    char timeZone[24];
    char deviceAlias[13];
    char passwordProtected[7];
    char devicePassword[17];
};

#endif