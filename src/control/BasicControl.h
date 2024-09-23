#ifndef BASICCONTROL_H
#define BASICCONTROL_H

class BasicControl {
    public:
        virtual void queueRestart () = 0;
        virtual void setLearningEnabled (bool _enabled) = 0;
        virtual void setRemoteConfigEnabled (bool _enabled) = 0;
};

#endif