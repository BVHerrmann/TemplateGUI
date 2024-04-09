#ifndef INSPECTORDEFINES_H
#define INSPECTORDEFINES_H

#include <QString>

const QString kOrganizationName     = "Bertram Bildverarbeitung GmbH";
const QString kOrganizationDomain   = "bertram.eu";
const QString kApplicationName      = TARGET;
const QString kApplicationVersion   = VERSION;

const QString kVirtualKeyboard      = "Inspector/VirtualKeyboard";
const bool kDefaultVirtualKeyboard  = true;
const QString kCustomAppName        = "Inspector/ApplicationName";
const QString kAllowMachineState    = "Inspector/MachineStateChangeUserLevel";
const QString kLocale               = "Inspector/Language";
const QString kFullscreen           = "Inspector/FullScreen";
const QString kSystemTrayIcon		= "Inspector/SystemTrayIcon";
const bool kDefaultSystemTrayIcon	= false;
#if defined(Q_OS_MACX)
const bool kDefaultFullscreen       = false;
#else
const bool kDefaultFullscreen       = true;
#endif
const QString kCheckPriority        = "Inspector/CheckPriority";
const bool kDefaultCheckPriority    = true;
const QString kHideLogo				= "Inspector/HideLogo";
const QString kWindowGeometry       = "Inspector/WindowGeometry";
const QString kWindowState          = "Inspector/WindowState";
const QString kLogoutTimeout        = "Inspector/LogoutTimeout";
const uint32_t kDefaultLogoutTimeout = 5 * 60 * 1000; // 5 min

const QString kDeviceComPort		= "Inspector/COM-Port";
const QString kUsers				= "Inspector/Users";
const QString kPassword				= "Inspector/PasswordLevel%1";

#endif // INSPECTORDEFINES_H
