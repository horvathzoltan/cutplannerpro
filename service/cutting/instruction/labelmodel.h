#pragma once
#include <QDateTime>

struct LabelPart {
    QString text;

    bool trimmable = false;   // rövidíthető
    bool jumpable = false;    // kiugorhat új sorba
    int targetRow = 0;        // melyik sorba szeretnénk alapból
    Qt::Alignment align = Qt::AlignLeft;  // bal/közép/jobb

};


struct LabelModel {
    QVector<LabelPart> parts;
    QString priorityIcon;   // 🔥💧☁️🪨
    QString groupIcon;      // 🦌🐸🐱… ABC állatok
    QString barcode;

    QString toString() const {
        QString out;
        for (const auto& p : parts)
            out += p.text;
        return out;
    }

    int length() const {
        int len = 0;
        for (const auto& p : parts)
            len += p.text.length();
        return len;
    }
};