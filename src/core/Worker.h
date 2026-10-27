#pragma once

#include <QObject>
#include <QPointer>
#include <QThread>

#include <functional>

#include "platform/Platform.h"

// A background thread that runs jobs one after another (slow Windows calls: DDC/CI,
// Bluetooth, WinRT...). Results come back with `then`, on the thread that owns `context`.
//
//   m_worker.run([] { return platform::listDisplays(); },
//                this, [this](auto displays) { ... update UI state ... });
class Worker : public QObject {
public:
  explicit Worker(const char* name, QObject* parent = nullptr) : QObject(parent) {
    m_thread.setObjectName(name);
    m_receiver.moveToThread(&m_thread);
    m_thread.start();
    post([] { platform::initializeThread(); });
  }

  ~Worker() override {
    m_thread.quit();
    m_thread.wait();
  }

  void post(std::function<void()> job) { QMetaObject::invokeMethod(&m_receiver, std::move(job), Qt::QueuedConnection); }

  template <typename Job, typename Then> void run(Job job, QObject* context, Then then) {
    QPointer<QObject> ctx(context);
    post([job = std::move(job), ctx, then = std::move(then)]() mutable {
      auto result = job();
      if (!ctx) {
        return;
      }
      QMetaObject::invokeMethod(
          ctx.data(), [ctx, then = std::move(then), result = std::move(result)]() mutable {
            if (ctx) {
              then(std::move(result));
            }
          },
          Qt::QueuedConnection);
    });
  }

private:
  QThread m_thread;
  QObject m_receiver;
};
