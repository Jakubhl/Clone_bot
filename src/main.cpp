#include <QApplication>
#include "CloneBotWindow.h"



int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    CloneBotWindow dialog;

    dialog.show();

    return app.exec();
}

