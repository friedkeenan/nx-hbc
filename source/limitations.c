#include "limitations.h"

bool has_limitations() {
    AppletType type = appletGetAppletType();

    return type != AppletType_Application && type != AppletType_SystemApplication;
}