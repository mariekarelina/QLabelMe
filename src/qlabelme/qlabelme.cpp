#include "widgets/main_window.h"

#include "shared/defmac.h"
#include "shared/utils.h"
#include "shared/logger/logger.h"
#include "shared/logger/config.h"
#include "shared/logger/format.h"
#include "shared/config/appl_conf.h"
#include "shared/config/logger_conf.h"
#include "shared/qt/quuidex.h"
#include "shared/qt/logger_operators.h"
#include "shared/qt/version_number.h"

// #include "pproto/commands/base.h"
// #include "pproto/commands/pool.h"
// #include "pproto/transport/tcp.h"
// #include "pproto/transport/udp.h"

#include <QApplication>
#include <QNetworkProxy>

#include <csignal>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

using namespace std;

/**
 Используется для уведомления основного потока о завершении работы программы.
*/
void stopProgramHandler(int sig)
{
    if ((sig == SIGTERM) || (sig == SIGINT))
    {
        const char* sigName = (sig == SIGTERM) ? "SIGTERM" : "SIGINT";
        log_verbose << "Signal " << sigName << " is received. Program will be stopped";
        QApplication::exit();
    }
    else
        log_verbose << "Signal " << sig << " is received";
}

void stopProgram()
{
    #define STOP_THREAD(THREAD_FUNC, NAME, TIMEOUT) \
        if (!THREAD_FUNC.stop(TIMEOUT * 1000)) { \
            log_info << "Thread '" NAME "': Timeout expired, thread will be terminated"; \
            THREAD_FUNC.terminate(); \
        }

    //STOP_THREAD(video::transport::recvPool(),  "VideoRecvPool", 20)

    #undef STOP_THREAD

    log_info << log_format("'%?' is stopped", APPLICATION_NAME);
    alog::stop();
}

int main(int argc, char *argv[])
{
    // Устанавливаем в качестве разделителя целой и дробной части символ '.',
    // если этого не сделать - функции преобразования строк в числа (std::atof)
    // буду неправильно работать.
    qputenv("LC_NUMERIC", "C");

    int ret = 0;
    try
    {
        alog::logger().start();
#ifdef NDEBUG
        alog::logger().addSaverStdOut(alog::Level::Info, true);
#else
        alog::logger().addSaverStdOut(alog::Level::Debug2);
#endif
        signal(SIGTERM, &stopProgramHandler);
        signal(SIGINT,  &stopProgramHandler);

#ifdef MINGW
        // Получаем APPDATA сразу как Unicode-строку
        const QString appData = qEnvironmentVariable("APPDATA");
        if (appData.isEmpty())
        {
            log_error << "Environment variable APPDATA is empty";
            alog::stop();
            return 1;
        }

        const QString configDir = QDir(appData).filePath("qlabelme");
#else
        const QString configDir =
            QDir::home().filePath(".config/qlabelme");
#endif

        // mkpath() также возвращает true, если каталог уже существует
        if (!QDir().mkpath(configDir))
        {
            log_error << "Failed create config directory: " << configDir;
            alog::stop();
            return 1;
        }

        const QString configFile =
            QDir(configDir).filePath("qlabelme.conf");

        // Подготавливаем путь для файловых функций сабмодуля
        const QByteArray encodedConfigFile =
            QFile::encodeName(QDir::toNativeSeparators(configFile));

        // QByteArray::toStdString() копирует байты без перекодирования
        const std::string configFilePath = encodedConfigFile.toStdString();
        if (!QFile::exists(configFile))
        {
            QFile file;
            QByteArray conf;

            file.setFileName(":/config/resources/qlabelme.base.conf");
            file.open(QIODevice::ReadOnly);
            conf = file.readAll();

            if (!config::base().readString(conf.toStdString()))
            {
                alog::stop();
                return 1;
            }
            // Используем подготовленный путь в системной кодировке
            if (!config::base().saveFile(configFilePath))
            {
                alog::stop();
                return 1;
            }
        }
        else
        {
            // При ошибке чтения прекращаем запуск
            if (!config::base().readFile(configFilePath))
            {
                alog::stop();
                return 1;
            }
        }

        // Создаем сэйвер по умолчанию для логгера
        if (!alog::configDefaultSaver())
        {
            alog::stop();
            return 1;
        }

        log_info << log_format(
            "'%?' is running (version: %?; protocol version: %?-%?; gitrev: %?)",
            APPLICATION_NAME, productVersion().toString(),
            PPROTO_VERSION_LOW, PPROTO_VERSION_HIGH, GIT_REVISION);
        alog::logger().flush();

        alog::logger().removeSaverStdOut();
        alog::logger().removeSaverStdErr();

        // Создаем дополнительные сэйверы для логгера
        // alog::configExtensionSavers();
        // alog::printSaversInfo();

        // if (!pproto::command::checkUnique())
        // {
        //     stopProgram();
        //     return 1;
        // }

        // if (!pproto::error::checkUnique())
        // {
        //     stopProgram();
        //     return 1;
        // }

        QApplication appl {argc, argv};

        appl.setApplicationName(APPLICATION_NAME);
        //appl.setApplicationDisplayName(APPLICATION_NAME);
        appl.setApplicationVersion(VERSION_PROJECT);

        // Устанавливаем текущую директорию. Эта конструкция работает только
        // когда создан экземпляр QCoreApplication.
        if (QDir::setCurrent(QCoreApplication::applicationDirPath()))
        {
            log_debug << "Set work directory: " << QCoreApplication::applicationDirPath();
        }
        else
        {
            log_error << "Failed set work directory";
            stopProgram();
            return 1;
        }

        QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);

        MainWindow mw;
        if (!mw.init())
        {
            stopProgram();
            return 1;
        }
        mw.show();

        alog::logger().removeSaverStdOut();
        alog::logger().removeSaverStdErr();

        ret = appl.exec();
        mw.deinit();

        if (config::base().changed())
            config::base().saveFile();
    }
    catch (std::exception& e)
    {
        log_error << "Failed initialization. Detail: " << e.what();
        ret = 1;
    }
    catch (...)
    {
        log_error << "Failed initialization. Unknown error";
        ret = 1;
    }

    stopProgram();
    return ret;
}
