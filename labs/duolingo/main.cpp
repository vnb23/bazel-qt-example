#include "mainwindow.h"
#include <QApplication>
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    LanguageApp window;
    window.setWindowTitle("Language Learning App");
    window.resize(600, 400);
    window.show();

    return app.exec();
}