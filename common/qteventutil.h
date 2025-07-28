#pragma once

#include <QObject>
#include <QCoreApplication>
#include <QEvent>
#include <functional>

//
// 🚀 Minimál lambda-esemény osztály – csak egy QEvent és egy futtatandó lambda
//
class LambdaEvent : public QEvent {
public:
    using Func = std::function<void()>;

    explicit LambdaEvent(Func func)
        : QEvent(QEvent::User), m_func(std::move(func)) {} // QEvent::User: egyszerű típus

    void execute() { m_func(); }

private:
    Func m_func;
};

//
// 🔧 Használatra kész util namespace, ami lambdákat postol az event queue-ba
//
namespace QtEventUtil {

// 📬 Lambda postolása az objektumnak
inline void post(QObject* receiver, std::function<void()> func) {
    QCoreApplication::postEvent(receiver, new LambdaEvent(std::move(func)));
}
}
