#ifndef CALLBACKREGISTRY_H
#define CALLBACKREGISTRY_H

#include <Arduino.h>

#define CALLBACK_REG_MAX 2

enum CallbackType {
    CallbackChatStatus = 0
};

typedef void (*initFunc)();


class CallbackRegistry {
    public:
        void addCallback (CallbackType callbackType, void* _callback) { callbacks[callbackType] = _callback; }
        void* getCallback (CallbackType callbackType) { return callbacks[callbackType]; }

    protected:
        void* callbacks[CALLBACK_REG_MAX];
};

#endif