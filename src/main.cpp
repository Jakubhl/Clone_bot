#include <QApplication>
#include "CloneBotWindow.h"



int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    CloneBotWindow dialog;

    QApplication::setStyle("Fusion");
    QPalette mNewPal;
    QColor textColor = QColor(223, 223, 223);
    QColor baseColor = QColor(72, 72, 72);
    mNewPal.setColorGroup(QPalette::Active,
            textColor,            // windowText
            baseColor.light(160), // button
            baseColor.light(180), // light
            baseColor.light(60),  // dark
            baseColor.light(155), // mid
            textColor,            // text
            baseColor,            // bright_text (Highlighted)
            baseColor.light(145), // base
            baseColor);           // window
    mNewPal.setColorGroup(QPalette::Inactive,
            textColor,            // windowText
            baseColor.light(160), // button
            baseColor.light(180), // light
            baseColor.light(60),  // dark
            baseColor.light(155), // mid
            textColor,            // text
            baseColor,            // bright_text (Highlighted)
            baseColor.light(145),   // base
            baseColor);           // window
    mNewPal.setColorGroup(QPalette::Disabled,
            QColor(textColor).dark(240), // windowText (foreground)
            baseColor.dark(),         // button
            baseColor.dark(150),      // light
            baseColor.dark(280),      // dark
            baseColor.dark(180),      // mid
            QColor(textColor).dark(140), // text
            baseColor.dark(),         // bright_text (Highlighted text)
            baseColor,                // base
            baseColor.dark(125));     // window (background)
    mNewPal.setColor(QPalette::Active, QPalette::Highlight, baseColor.light(210));
    app.setPalette(mNewPal); // nova paleta

    dialog.show();

    return app.exec();
}

