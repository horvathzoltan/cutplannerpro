#pragma once

#include <csignal>
#include <functional>
#include <QString>
#include <QCoreApplication>
#include "common/logger.h"

class SignalHelper
{
public:
    static constexpr int SIGINT_  = SIGINT;
    static constexpr int SIGTERM_ = SIGTERM;

    /// Beállítja a kívánt leállító jelet (SIGINT, SIGTERM)
    static void setShutDownSignal(int signalId)
    {
#ifdef __linux__
        struct sigaction sa;
        sa.sa_flags = 0;
        sigemptyset(&sa.sa_mask);
        sa.sa_handler = handleShutDownSignal;
        if (sigaction(signalId, &sa, nullptr) == -1)
        {
            perror("setting up termination signal");
            ::exit(1);
        }
#elif defined(_WIN32)
        signal(signalId, handleShutDownSignal);
#else
        Q_UNUSED(signalId);
#endif
    }

    /// Opcionális cleanup callback beállítása (signalId‑val)
    static void setCleanupHandler(std::function<void(int)> fn)
    {
        cleanupHandler() = std::move(fn);
    }

private:
    static void handleShutDownSignal(int signalId)
    {
        zInfo(QStringLiteral("EXIT: %1").arg(signalId));

        // ha van cleanup callback, futtatjuk
        if (cleanupHandler()) {
            cleanupHandler()(signalId);
        }

        QCoreApplication::exit(0);
    }

    // statikus tároló a callbacknek
    static std::function<void(int)>& cleanupHandler()
    {
        static std::function<void(int)> fn;
        return fn;
    }
};
