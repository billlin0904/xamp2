#include "applicationservices.h"
#include <QCoreApplication>
#include <QEvent>
#include <base/threadpoolbuilder.h>
#include <widget/worker/filesystemservice.h>
#include <widget/worker/albumcoverservice.h>
#include <widget/worker/backgroundservice.h>
namespace {
    template <typename Service>
    void destroyWorkerService(QScopedPointer<Service>& service, QThread& thread) {
        auto* object = service.take();
        if (object == nullptr) {
            return;
        }

        const auto destroy_object = [object]() {
            object->cancelRequested();
            object->deleteLater();
            QCoreApplication::sendPostedEvents(object, QEvent::DeferredDelete);
        };

        if (thread.isRunning() && object->thread() != QThread::currentThread()) {
            QMetaObject::invokeMethod(object, destroy_object, Qt::BlockingQueuedConnection);
        }
        else {
            destroy_object();
        }
    }
}
ApplicationServices::ApplicationServices() {
    files_.reset(new FileSystemService());
    files_->setScannerThreadPool(ThreadPoolBuilder::makeBackgroundThreadPool());
    covers_.reset(new AlbumCoverService());
    background_.reset(new BackgroundService());
    files_->moveToThread(&files_thread_);
    covers_->moveToThread(&covers_thread_);
    background_->moveToThread(&background_thread_);
    files_thread_.start(QThread::LowestPriority);
    covers_thread_.start(QThread::LowestPriority);
    background_thread_.start(QThread::LowestPriority);
}
ApplicationServices::~ApplicationServices() { shutdown(); }
void ApplicationServices::shutdown() {
    destroyWorkerService(files_, files_thread_);
    destroyWorkerService(covers_, covers_thread_);
    destroyWorkerService(background_, background_thread_);
    for (auto* thread : {&files_thread_, &covers_thread_, &background_thread_}) {
        if (thread->isRunning()) { thread->requestInterruption(); thread->quit(); thread->wait(); }
    }
}
