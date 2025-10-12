#pragma once

#include <unordered_map>
#include <memory>

#include <QMutex>
#include <QRecursiveMutex>
#include <QThread>
#include <QDebug>

// ScopedPerThreadLock: per-thread depth tracking mutex-cím szerint.
// Usage:
//   ScopedPerThreadLock locker(static_cast<void*>(&_mutex), /*recursive=*/true);
//   ScopedPerThreadLock locker(static_cast<void*>(&_mutex), /*recursive=*/false);

class ScopedPerThreadLock {
public:
    explicit ScopedPerThreadLock(void* mutexPtr, bool recursive = false)
        : m_mutexPtr(mutexPtr), m_recursive(recursive), m_owned(false)
    {
        auto &depthMap = getDepthMap();
        int depth = 0;
        auto it = depthMap.find(m_mutexPtr);
        if (it != depthMap.end()) depth = it->second;

        if (depth == 0) {
            // tényleges lockolás: rekurzív mutexhez manuális RAII, sima mutexhez QMutexLocker<QMutex>
            if (m_recursive) {
                // QRecursiveMutex esetén QMutexLocker<QRecursiveMutex> működik (Qt6)
                m_recursiveLocker.reset(new QMutexLocker<QRecursiveMutex>(reinterpret_cast<QRecursiveMutex*>(m_mutexPtr)));
            } else {
                m_mutexLocker.reset(new QMutexLocker<QMutex>(reinterpret_cast<QMutex*>(m_mutexPtr)));
            }
            m_owned = true;
        }
        ++depthMap[m_mutexPtr];
    }

    ~ScopedPerThreadLock()
    {
        auto &depthMap = getDepthMap();
        auto it = depthMap.find(m_mutexPtr);
        if (it != depthMap.end()) {
            --(it->second);
            if (it->second <= 0) depthMap.erase(it);
        }

        // locker objektumok destruktora oldja a lockot, ha létre lettek hozva
        m_mutexLocker.reset();
        m_recursiveLocker.reset();
    }

    ScopedPerThreadLock(const ScopedPerThreadLock&) = delete;
    ScopedPerThreadLock& operator=(const ScopedPerThreadLock&) = delete;
    ScopedPerThreadLock(ScopedPerThreadLock&&) = delete;
    ScopedPerThreadLock& operator=(ScopedPerThreadLock&&) = delete;

    bool ownsLock() const { return m_owned; }

private:
    // per-thread map: mutex pointer -> depth
    static std::unordered_map<const void*, int>& getDepthMap() {
        thread_local std::unordered_map<const void*, int> depthMap;
        return depthMap;
    }

    void* m_mutexPtr;
    bool m_recursive;
    bool m_owned;
    std::unique_ptr<QMutexLocker<QMutex>> m_mutexLocker;                     // used when recursive == false
    std::unique_ptr<QMutexLocker<QRecursiveMutex>> m_recursiveLocker;        // used when recursive == true
};
