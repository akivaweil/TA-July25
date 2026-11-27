#pragma once

#include "LineLink.h"

namespace linelink {

// The Transfer Arm has no wired Start input. Observe release before arming
// after boot or link loss, and allow one pickup per debounced hold.
template <class Debouncer>
class SwitchButton {
public:
    explicit SwitchButton(uint8_t stateBit) : stateBit_(stateBit) {}

    void begin() { remote_.release(); }
    void interval(uint16_t intervalMs) { remote_.interval(intervalMs); }

    void update() {
        PeerStats peer{};
        const bool valid = !flashGate().flashPending() &&
            link().getPeerStats(ID_SWITCH_CONTROLLER, peer) && peer.alive &&
            (peer.state & SWITCH_STATE_VALID);
        if (!valid || peer.restarts != peerRestarts_ ||
            peer.switchControlGapCount != peerGapCount_) {
            remote_.release();
            armed_ = false;
        }
        if (valid) {
            peerRestarts_ = peer.restarts;
            peerGapCount_ = peer.switchControlGapCount;
            const bool pressed = (peer.state & stateBit_) != 0;
            if (!pressed) armed_ = true;
            remote_.setLevel(armed_ && pressed);
            remote_.update();
        }
        if (!read()) consumed_ = false;
    }

    bool read() const { return remote_.read(); }
    bool unusedPress() const { return read() && !consumed_; }
    void consumePress() { consumed_ = read(); }

private:
    class RemoteDebouncer : public Debouncer {
    public:
        void setLevel(bool pressed) { pressed_ = pressed; }

        void release() {
            pressed_ = false;
            this->begin();
        }

    protected:
        bool readCurrentState() override { return pressed_; }

    private:
        bool pressed_ = false;
    };

    RemoteDebouncer remote_;
    const uint8_t stateBit_;
    uint32_t peerRestarts_ = 0;
    uint32_t peerGapCount_ = 0;
    bool armed_ = false;
    bool consumed_ = false;
};

}  // namespace linelink
