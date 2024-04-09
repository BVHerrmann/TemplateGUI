#include "crashhandler.h"

#include <iostream>

#include <QtCore>

#ifdef Q_OS_WIN
#pragma warning(push)
#pragma warning(disable: 4005)
#endif
#include <client/windows/handler/exception_handler.h>
#ifdef Q_OS_WIN
#pragma warning(pop)
#endif

static bool MinidumpCallback(const wchar_t* dump_dir, const wchar_t* minidump_id, void* context, EXCEPTION_POINTERS *exinfo, MDRawAssertionInfo *assertion, bool succeeded);

CrashHandler::CrashHandler(const QString dumpFolderPath, const QString crashReporter)
{
    qDebug() << "Installing crash handler";

    m_crash_handler = new google_breakpad::ExceptionHandler(dumpFolderPath.toStdWString(), 0, MinidumpCallback, this, true, 0 );

    if (!crashReporter.isEmpty()) {
        setCrashReporter(crashReporter);
    } else {
        m_crashReporterChar = new char[0];
        m_crashReporterWChar = new wchar_t[0];
    }
}

CrashHandler::~CrashHandler()
{
    delete m_crash_handler;
}

void CrashHandler::setCrashReporter(const QString crashReporter)
{
    QString crashReporterPath;
    QString localReporter = QString( "%1/%2" ).arg( QCoreApplication::instance()->applicationDirPath() ).arg( crashReporter );
    QString globalReporter = QString( "%1/../libexec/%2" ).arg( QCoreApplication::instance()->applicationDirPath() ).arg( crashReporter );

    if ( QFileInfo( localReporter ).exists() )
        crashReporterPath = localReporter;
    else if ( QFileInfo( globalReporter ).exists() )
        crashReporterPath = globalReporter;
    else {
        qDebug() << "Could not find" << crashReporter << "in ../libexec or application path";
        crashReporterPath = crashReporter;
    }

    // cache reporter path as char*
    char *creporter;
    std::string sreporter = crashReporter.toStdString();
    creporter = new char[sreporter.size() + 1];
    strcpy_s(creporter, sizeof(creporter), sreporter.c_str());
    m_crashReporterChar = creporter;

    // cache reporter path as wchart_t*
    wchar_t *wreporter;
    std::wstring wsreporter = crashReporter.toStdWString();
    wreporter = new wchar_t[wsreporter.size() + 10];
    wcscpy_s(wreporter, sizeof(wreporter), wsreporter.c_str());
    m_crashReporterWChar = wreporter;
}

static bool MinidumpCallback(const wchar_t* dump_dir, const wchar_t* minidump_id, void* context, EXCEPTION_POINTERS *exinfo, MDRawAssertionInfo *assertion, bool succeeded)
{
    (void)assertion;
    (void)exinfo;

    if (!succeeded)
        return false;

    // DON'T USE THE HEAP!!!
    // So that indeed means, no QStrings, no qDebug(), no QAnything, seriously!

    const wchar_t *crashReporter = static_cast<CrashHandler *>(context)->crashReporterWChar();
    if (crashReporter == NULL || wcslen(crashReporter) == 0)
        return false;

    wchar_t command[MAX_PATH * 3 + 6];
    wcscpy_s(command, crashReporter);
    wcscat_s(command, L" \"");
    wcscat_s(command, dump_dir);
    wcscat_s(command, L"/" );
    wcscat_s(command, minidump_id );
    wcscat_s(command, L".dmp\"" );

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOWNORMAL;
    ZeroMemory( &pi, sizeof(pi) );

    if (CreateProcess(NULL, command, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        TerminateProcess(GetCurrentProcess(), 1);
    }

    return succeeded;
}
