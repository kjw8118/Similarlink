#include "similarlink.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <iostream>
int main(int argc, char *argv[])
{
    std::cout << "Start" << std::endl;
    QApplication a(argc, argv);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "Similarlink_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }
    Similarlink w;
    w.show();
    return a.exec();
}
