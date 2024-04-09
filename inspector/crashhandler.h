#ifndef CRASHHANDLER_H
#define CRASHHANDLER_H

#include <string>

class QString;

namespace google_breakpad {
    class ExceptionHandler;
}


class CrashHandler
{
    const char* m_crashReporterChar; // yes! It MUST be const char[]
    const wchar_t* m_crashReporterWChar;

public:
    CrashHandler(const QString dumpFolderPath, const QString crashReporter);
    virtual ~CrashHandler();

    void setCrashReporter(const QString crashReporter);
    const char* crashReporterChar() const { return m_crashReporterChar; }
    const wchar_t* crashReporterWChar() const { return m_crashReporterWChar; }

private:
    google_breakpad::ExceptionHandler *m_crash_handler;
};

#endif // CRASHHANDLER_H
