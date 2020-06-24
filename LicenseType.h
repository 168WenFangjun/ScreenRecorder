#pragma once

enum LicenseType
{
    LicenseType_Invalid,
    LicenseType_ScreenRecorder,
    LicenseType_Max
};
const char LICENSE_TYPE[LicenseType_Max][64] = {
    "invalid",
    "oosman\\'s screen recorder app"
};
