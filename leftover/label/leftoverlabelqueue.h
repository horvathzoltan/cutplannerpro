#pragma once

#include <QVector>
#include <QString>
#include "service/cutting/instruction/labelmodel.h"

class LeftoverLabelQueue
{
public:
    static LeftoverLabelQueue& instance()
    {
        static LeftoverLabelQueue inst;
        return inst;
    }

    void append(const LabelModel& lm);
    QVector<LabelModel> load() const;
    void clear();

private:
    LeftoverLabelQueue() = default;

    QString serialize(const LabelModel& lm) const;
    LabelModel deserialize(const QString& line) const;
};
