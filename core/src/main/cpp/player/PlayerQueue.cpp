//
// Created by maomao on 2020/1/15.
//

#include "PlayerQueue.h"

//pthread 文档：
//1、pthread_t :用于声明一个线程对象如：pthread_t thread;
//2、pthread_create :用于创建一个实际的线程如：pthread_create(&pthread,NULL,threadCallBack,NULL);其总共接收4个参数，第一个参数为pthread_t对象，第二个参数为线程的一些属性我们一般传NULL就行，第三个参数为线程执行的函数（ void* threadCallBack(void *data) ），第四个参数是传递给线程的参数是void*类型的既可以传任意类型。
//3、pthread_exit :用于退出线程如：pthread_exit(&thread)，参数也可以传NULL。注：线程回调函数最后必须调用此方法，不然APP会退出（挂掉）。
//4、pthread_mutex_t :用于创建线程锁对象如：pthread_mutex_t mutex;
//5、pthread_mutex_init :用于初始化pthread_mutex_t锁对象如：pthread_mutex_init(&mutex, NULL);
//6、pthread_mutex_destroy :用于销毁pthread_mutex_t锁对象如：pthread_mutex_destroy(&mutex);
//7、pthread_cond_t :用于创建线程条件对象如：pthread_cond_t cond;
//8、pthread_cond_init :用于初始化pthread_cond_t条件对象如：pthread_cond_init(&cond, NULL);
//9、pthread_cond_destroy :用于销毁pthread_cond_t条件对象如：pthread_cond_destroy(&cond);
//10、pthread_mutex_lock :用于上锁mutex，本线程上锁后的其他变量是不能被别的线程操作的如：pthread_mutex_lock(&mutex);
//11、pthread_mutex_unlock :用于解锁mutex，解锁后的其他变量可以被其他线程操作如：pthread_mutex_unlock(&mutex);
//12、pthread_cond_signal :用于发出条件信号如：pthread_cond_signal(&mutex, &cond);
//13、pthread_cond_wait :用于线程阻塞等待，直到pthread_cond_signal发出条件信号后才执行退出线程阻塞执行后面的操作。

PlayerQueue::PlayerQueue(PlayerStatus *status) {
    playerStatus = status;
    pthread_mutex_init(&mutexFrame, NULL);
    pthread_cond_init(&condFrame, NULL);
    pthread_cond_init(&condPacket, NULL);
    pthread_mutex_init(&mutexPacket, NULL);
}


PlayerQueue::~PlayerQueue() {
    playerStatus = NULL;
    pthread_cond_destroy(&condPacket);
    pthread_cond_destroy(&condFrame);
    pthread_mutex_destroy(&mutexPacket);
    pthread_mutex_destroy(&mutexFrame);
    LOGE("PlayerQueue Has Success Destructed");
}

int PlayerQueue::putAvPacket(AVPacket *avPacket) {
    pthread_mutex_lock(&mutexPacket);
    queuePacket.push(avPacket);
    pthread_cond_signal(&condPacket);
    pthread_mutex_unlock(&mutexPacket);
    return 0;
}

int PlayerQueue::getAvPacket(AVPacket *avPacket) {
    pthread_mutex_lock(&mutexPacket);
    while (playerStatus != NULL && !playerStatus->exit) {
        if (queuePacket.size() > 0) {
            AVPacket *pkt = queuePacket.front();
            // 对入队列的Packet调用av_packet_ref增加引用计数的方法来复制Packet中的数据
            if (av_packet_ref(avPacket, pkt) == 0) {
                queuePacket.pop();
            }
            // 调用av_packet_free来释放AVPacket本身所占用的空间
            av_packet_free(&pkt);
            // 调用av_free()用于释放申请的内存
            av_free(pkt);
            pkt = NULL;
            break;
        } else {
            if (!playerStatus->exit) {
                pthread_cond_wait(&condPacket, &mutexPacket);
            }
        }
    }
    pthread_mutex_unlock(&mutexPacket);
    return 0;
}

int PlayerQueue::clearAvPacket() {
    pthread_mutex_lock(&mutexPacket);
    while (!queuePacket.empty()) {
        AVPacket *pkt = queuePacket.front();
        queuePacket.pop();
        av_free(pkt->data);
        av_free(pkt->buf);
        av_free(pkt->side_data);
        pkt = NULL;
    }
    pthread_cond_signal(&condPacket);
    pthread_mutex_unlock(&mutexPacket);
    return 0;
}

int PlayerQueue::getAvPacketSize() {
    int size = 0;
    pthread_mutex_lock(&mutexPacket);
    size = queuePacket.size();
    pthread_mutex_unlock(&mutexPacket);
    return size;
}

int PlayerQueue::putAvFrame(AVFrame *avFrame) {
    pthread_mutex_lock(&mutexFrame);
    queueFrame.push(avFrame);
    pthread_cond_signal(&condFrame);
    pthread_mutex_unlock(&mutexFrame);
    return 0;
}

int PlayerQueue::getAvFrame(AVFrame *avFrame) {
    pthread_mutex_lock(&mutexFrame);
    while (playerStatus != NULL && !playerStatus->exit) {
        if (queueFrame.size() > 0) {
            AVFrame *frame = queueFrame.front();
            if (av_frame_ref(avFrame, frame) == 0) {
                queueFrame.pop();
            }
            av_frame_free(&frame);
            av_free(frame);
            frame = NULL;
            break;
        } else {
            if (!playerStatus->exit) {
                pthread_cond_wait(&condFrame, &mutexFrame);
            }
        }
    }
    pthread_mutex_unlock(&mutexFrame);
    return 0;
}

int PlayerQueue::clearAvFrame() {
    pthread_mutex_lock(&mutexFrame);
    while (!queueFrame.empty()) {
        AVFrame *frame = queueFrame.front();
        queueFrame.pop();
        av_frame_free(&frame);
        av_free(frame);
        frame = NULL;
    }
    pthread_cond_signal(&condFrame);
    pthread_mutex_unlock(&mutexFrame);
    return 0;
}

int PlayerQueue::getAvFrameSize() {
    int size = 0;
    pthread_mutex_lock(&mutexFrame);
    size = queueFrame.size();
    pthread_mutex_unlock(&mutexFrame);
    return size;
}

int PlayerQueue::clearToKeyFrame() {
    pthread_mutex_lock(&mutexPacket);
    while (!queuePacket.empty())
    {
        AVPacket *pkt = queuePacket.front();
        if(pkt->flags != AV_PKT_FLAG_KEY)
        {
            queuePacket.pop();
            av_free(pkt->data);
            av_free(pkt->buf);
            av_free(pkt->side_data);
            pkt = NULL;
        } else{
            break;
        }
    }
    pthread_cond_signal(&condPacket);
    pthread_mutex_unlock(&mutexPacket);
    return 0;
}

int PlayerQueue::noticeThread() {
    pthread_cond_signal(&condFrame);
    pthread_cond_signal(&condPacket);
    return 0;
}

void PlayerQueue::release() {
    LOGI("Player Queue Released！");
    noticeThread();
    clearAvPacket();
    clearAvFrame();
}