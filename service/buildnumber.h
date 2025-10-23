#include <QString>

namespace Buildnumber {

#define BUILDNUMBER -1

inline const QString value = QString::number(BUILDNUMBER);
}
