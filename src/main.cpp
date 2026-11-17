#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/GJEffectManager.hpp>
#include <chrono>
#include <cstdint>

using namespace geode::prelude;

// FrameProbe: logs once per second how much time per frame is spent in the
// main non-inline entry points of the update loop. Diagnostic only, it does
// not change game behaviour.
namespace probe {
    using Clock = std::chrono::steady_clock;

    struct Stat {
        double ms = 0.0;
        uint64_t calls = 0;
    };

    inline Stat s_update, s_commands, s_moveActions, s_effects, s_pulse;
    inline uint64_t s_groupColorCalls = 0;
    inline uint64_t s_pulseColorCalls = 0;
    inline uint64_t s_frames = 0;
    inline Clock::time_point s_windowStart = Clock::now();

    struct Scope {
        Stat& stat;
        Clock::time_point start = Clock::now();
        explicit Scope(Stat& s) : stat(s) {}
        ~Scope() {
            stat.ms += std::chrono::duration<double, std::milli>(Clock::now() - start).count();
            stat.calls++;
        }
    };

    inline void reset() {
        s_update = s_commands = s_moveActions = s_effects = s_pulse = Stat{};
        s_groupColorCalls = 0;
        s_pulseColorCalls = 0;
        s_frames = 0;
        s_windowStart = Clock::now();
    }

    inline void tick() {
        s_frames++;
        double sec = std::chrono::duration<double>(Clock::now() - s_windowStart).count();
        if (sec < 1.0) return;

        double f = static_cast<double>(s_frames);
        log::info(
            "[FrameProbe] {:.0f} fps | ms/frame: update {:.2f}, commands {:.2f}, moveActions {:.2f}, "
            "effects {:.2f}, pulse {:.2f} | calls/frame: colorForGroupID {:.0f}, colorForPulseEffect {:.0f}",
            f / sec,
            s_update.ms / f, s_commands.ms / f, s_moveActions.ms / f,
            s_effects.ms / f, s_pulse.ms / f,
            s_groupColorCalls / f, s_pulseColorCalls / f
        );
        reset();
    }
}

class $modify(ProbeBaseLayer, GJBaseGameLayer) {
    void update(float dt) {
        {
            probe::Scope s(probe::s_update);
            GJBaseGameLayer::update(dt);
        }
        probe::tick();
    }

    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        probe::Scope s(probe::s_commands);
        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
    }

    void processMoveActions() {
        probe::Scope s(probe::s_moveActions);
        GJBaseGameLayer::processMoveActions();
    }
};

class $modify(ProbeEffects, GJEffectManager) {
    void updateEffects(float dt) {
        probe::Scope s(probe::s_effects);
        GJEffectManager::updateEffects(dt);
    }

    void updatePulseEffects(float dt) {
        probe::Scope s(probe::s_pulse);
        GJEffectManager::updatePulseEffects(dt);
    }

    cocos2d::ccColor3B colorForGroupID(int id, cocos2d::ccColor3B const& color, bool mainColor) {
        probe::s_groupColorCalls++;
        return GJEffectManager::colorForGroupID(id, color, mainColor);
    }

    cocos2d::ccColor3B colorForPulseEffect(cocos2d::ccColor3B const& color, PulseEffectAction* action) {
        probe::s_pulseColorCalls++;
        return GJEffectManager::colorForPulseEffect(color, action);
    }
};
