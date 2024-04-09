#ifdef CVL_USED
#include <ch_cvl/defs.h>
#include <ch_cvl/windisp.h>
#endif

#include <QtCore>
#include <QtWidgets>
#include <QtVirtualKeyboard>
#include <QSerialPort>

#ifdef Q_OS_WIN
#pragma warning(push)
#pragma warning(disable: 4005)
#include <Windows.h>
#pragma warning(pop)

#ifdef _DEBUG
#ifdef VLD_FOUND
#include <vld.h>
#endif
#endif

#endif

#include <QtSingleApplication>

#ifdef Q_OS_WIN
#include "crashhandler.h"
#endif

#include <BSimaticident.h>

#include "inspectordefines.h"
#include "mainwindow.h"
#include "pluginscontroller.h"
#include "translator.h"


inline bool isArgumentEnabled(QStringList arguments, QString argument)
{
    return arguments.contains(argument, Qt::CaseInsensitive)
            || arguments.contains("/" + argument, Qt::CaseInsensitive)
            || arguments.contains("-" + argument, Qt::CaseInsensitive)
            || arguments.contains("--" + argument, Qt::CaseInsensitive);
}

int main(int argc, char *argv[])
{
#if defined(DEBUG)
    qputenv("QT_DEBUG_PLUGINS", QByteArray("1"));
    qputenv("QT_LOGGING_RULES", QByteArray("qt.virtualkeyboard=true"));
#endif
    
#if defined(Q_OS_MACX)
    QSettings settings(QSettings::UserScope, kOrganizationDomain, kApplicationName);
#else
    QSettings settings(QSettings::UserScope, kOrganizationName, kApplicationName);
#endif
    if (settings.value(kVirtualKeyboard, kDefaultVirtualKeyboard).toBool()) {
        qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));
    }
    
    QtSingleApplication a(argc, argv);
    
    // check if an instance of the application is already running
    if (a.isRunning()) {
        if (isArgumentEnabled(a.arguments(), "hide")) {
            a.sendMessage("hide");
        } else {
            a.sendMessage("activate");
        }
        return 0;
    }

#ifndef DEBUG
#ifdef Q_OS_WIN
    // init crash handler
    QString dumpPath = "C:\\Bertram\\CrashDump\\";
    QDir dir(dumpPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    new CrashHandler(dumpPath, ""); // TODO: create crash uploader
#endif
#endif

#ifdef Q_OS_WIN
#ifndef DEBUG
	HANDLE process = GetCurrentProcess();

    // set to real time priority
    if (!SetPriorityClass(process, REALTIME_PRIORITY_CLASS)) {
        DWORD error = GetLastError();
        qDebug() << "Failed to change process priority (%d)" << error;

        // set main thread to idel priority
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_IDLE);
    } else {
        // set main thread to idel priority
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_IDLE);
    }

	// increase working set size
	bool success = SetProcessWorkingSetSizeEx(process, (SIZE_T)512 * 1024 * 1024, (SIZE_T)2048 * 1024 * 1024, QUOTA_LIMITS_HARDWS_MIN_ENABLE | QUOTA_LIMITS_HARDWS_MAX_DISABLE);
	if (!success) {
		DWORD error = GetLastError();
		qDebug() << "Failed to change process working set (%d)" << error;
	}
#endif
#endif

#ifdef CVL_USED
	cfInitializeDisplayResources();
#endif

    // register metatypes to be usable in connections
    //registerMetaTypes();

    QCoreApplication::setOrganizationName(kOrganizationName);
    QCoreApplication::setOrganizationDomain(kOrganizationDomain);
    QCoreApplication::setApplicationName(kApplicationName);
    QCoreApplication::setApplicationVersion(kApplicationVersion);
    QCoreApplication::instance()->thread()->setObjectName("MainThread");

    // load style sheet
    QFile styleSheetFile(":/style.css");
    styleSheetFile.open(QFile::ReadOnly);
    a.setStyleSheet(QLatin1String(styleSheetFile.readAll()));
    // load translations
    Q_INIT_RESOURCE(translations_bg);
    Translator::getInstance()->updateTranslations();
    
    // initialize main window
    MainWindow mainWindow;

    if (isArgumentEnabled(a.arguments(), "hide")) {
#ifdef Q_OS_WIN
        mainWindow.hide();
#else
        mainWindow.showMinimized();
#endif
    } else {
        mainWindow.show();
    }

    // register as main window of application
    QObject::connect(&a, &QtSingleApplication::messageReceived, &mainWindow, &MainWindow::handleMessage);
    a.setActivationWindow(&mainWindow, true);

    // initialize and load all plugins
    PluginsController pluginsController(&mainWindow);
    pluginsController.loadAllPlugins();

	//initialize cardreader
	QString comPort = settings.value(kDeviceComPort).toString();
	if (!comPort.isEmpty()) {
		BSimaticIdent* cardReader = new BSimaticIdent();
		QThread *thread = new QThread();
		thread->start();
		thread->setObjectName("CardReader");
		thread->setPriority(QThread::IdlePriority);

		cardReader->moveToThread(thread);

		QObject::connect(cardReader, &BSimaticIdent::login, &mainWindow, static_cast<void (MainWindow::*)(const QString)>(&MainWindow::changeUser));
		QObject::connect(&mainWindow, &MainWindow::accessStatus, cardReader, &BSimaticIdent::setLed);
		QMetaObject::invokeMethod(cardReader, "setDevice", Q_ARG(QString, comPort));
	}
    
    // adjust virtual keyboard size
    QObject::connect(qApp->inputMethod(), &QInputMethod::visibleChanged, [=](){
        for (const QWindow *w : qApp->allWindows()) {
            QObject *kb = w->findChild<QObject *>("keyboard");
            if (kb) {
                double screen = qApp->primaryScreen()->size().width();
                double width = screen / 2;
                kb->parent()->setProperty("x", (screen - width) / 2);
                kb->parent()->setProperty("width", width);
            }
        }
    });

    int result = a.exec();
    qDebug() << "Exit with code" << result;

    return result;
}
