#include "service/buildnumber.h"

#define SYSINFO_STRINGIFY(x) #x
#define SYSINFO_TOSTRING(x) SYSINFO_STRINGIFY(x)

class SysInfoHelper {
    QString _target;
    QString _buildNumber;
    QString _user;
    QString _hostName;

    SysInfoHelper() {
#ifdef TARGI
        _target = QStringLiteral(SYSINFO_TOSTRING(TARGI));
#else
        _target = QStringLiteral("ApplicationNameString");
#endif

        _buildNumber = Buildnumber::value;

        _user = qEnvironmentVariable("USER");
        if (_user.isEmpty())
            _user = qEnvironmentVariable("USERNAME");

        _hostName = QSysInfo::machineHostName();
    }

public:
    static SysInfoHelper& instance() {
        static SysInfoHelper inst;
        return inst;
    }

    QString sysInfo() const {
        QString msg = QStringLiteral("started ") + _target;
        if (!_buildNumber.isEmpty() && _buildNumber != "-1")
            msg += "(" + _buildNumber + ")";
        if (!_user.isEmpty())
            msg += " as " + _user;
        if (!_hostName.isEmpty())
            msg += "@" + _hostName;
        return msg;
    }

    QString target() const {
        return _target;
    }
};

#undef SYSINFO_STRINGIFY
#undef SYSINFO_TOSTRING
