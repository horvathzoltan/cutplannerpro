#pragma once

#include <QString>
#include <QStringList>

struct CountPerType {
private:
    QStringList _goodRefs;
    QStringList _badRefs;

public:
    int good() const { return _goodRefs.size(); }
    int bad()  const { return _badRefs.size(); }
    int total() const { return _goodRefs.size() + _badRefs.size(); }

    void addReference_Good(const QString& r){ _goodRefs.append(r);}
    void addReference_Bad(const QString& r){ _badRefs.append(r);}

    QStringList goodRefs() const {return _goodRefs;}
    QStringList badRefs() const {return _badRefs;}
};

