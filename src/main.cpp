#include <qapplication.h>
#include <Windows.h>
#include <qlogcollector/server/logcollector.h>
#include <qlogcollector/server/colors/styledstring.h>
#include <qlogcollector/server/outputs/fileoutputtarget.h>
#include <qicon.h>
#include <qdebug.h>
#include <qfont.h>

#include "app.h"

QLOGCOLLECTOR_USE_NAMESPACE

int main(int argc, char* argv[]) {
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication a(argc, argv);

#ifdef Q_OS_WIN
    /**
     * 设置系统默认字体
    */
    NONCLIENTMETRICS ncm;
    ncm.cbSize = sizeof(ncm);
    HRESULT hr;
    hr = SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
    if (hr != 0) {
        auto font = QApplication::font();
        font.setFamily(QString::fromLocal8Bit(ncm.lfMenuFont.lfFaceName));
        QApplication::setFont(font);
    }
#endif

    /**
     * 初始化日志收集器
     */
    LogCollector::quickStart()
        .style(ROOT_PROJECT_PATH, 120)
        .console(Ide::clion)
        .bindFatalSignal(true)
        .start();

    /**
     * 打印应用版本等信息
     */
    qDebug() << styled("------application version:").g() << styled(APP_VERSION, true).g() << styled("------").g();

    /**
     * 应用程序初始化在此处开始
     */
    a.setWindowIcon(QIcon(":/res/logo.ico"));

    App app;
    app.setWindowTitle("备忘录");
    app.show();chushihua

    return a.exec();
}