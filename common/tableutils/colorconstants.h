#pragma once
#include <QColor>

namespace ColorConstants {

constexpr int YellowR = 255, YellowG = 243, YellowB = 205;
constexpr int GreenStandardR = 212, GreenStandardG = 237, GreenStandardB = 218;
constexpr int GreenSuperR = 195, GreenSuperG = 230, GreenSuperB = 203;
constexpr int OrangeR = 255, OrangeG = 165, OrangeB = 0;

inline const QColor ColorYellow{YellowR, YellowG, YellowB};
inline const QColor ColorGreenStandard{GreenStandardR, GreenStandardG, GreenStandardB};
inline const QColor ColorGreenSuper{GreenSuperR, GreenSuperG, GreenSuperB};
inline const QColor ColorOrange{OrangeR, OrangeG, OrangeB};
inline const QColor ColorRed{Qt::red}; // Qt saját példánya
inline const QColor TextBlack{Qt::black};

}
