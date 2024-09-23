#ifndef CHATTERVIEWCALLBACK_H
#define CHATTERVIEWCALLBACK_H

enum ChatViewStatus {
    ChatViewDisconnected = 0,
    ChatViewConnecting = 1,
    ChatViewConnected = 2,
    ChatViewReceiving = 3,
    ChatViewReceived = 4,
    ChatViewSending = 5,
    ChatViewSent = 6,
    ChatViewFailed = 7,
    ChatViewNoDevice = 8
};

class ChatterViewCallback {
    public:
        virtual void updateChatViewStatus (const char* status) = 0;
        virtual void updateChatViewProgress (float progress) = 0;
        virtual void updateCacheUsed (float progress) = 0;
        virtual void updateChatViewStatus (uint8_t channelNum, ChatViewStatus newStatus) = 0;
        virtual void pingReceived (uint8_t deviceAddress) = 0;
        virtual void subChannelHopped () = 0;
        virtual bool userInterrupted () = 0;
        virtual void yieldForProcessing () = 0;
        virtual void messageReceived () = 0;
};

#endif