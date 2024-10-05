#include "Flatfield.h"

#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyle(QStyleFactory::create("windowsvista"));
    Flatfield w;
    w.show();
    return a.exec();
}
