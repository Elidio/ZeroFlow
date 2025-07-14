#ifndef ZEROFLOW_HPP_INCLUDED
#define ZEROFLOW_HPP_INCLUDED

namespace zeroflow {

/**
 * @brief Represents a state in the finite‑state machine.
 *
 * @tparam T Concrete FSM class that owns the state methods.
 */
template <typename T>
class State {
private:
    T&  instance;                     ///< Reference to the owning FSM
    void (T::*method)();              ///< Pointer to the member function that implements the state

public:
    State(T& obj, void (T::*m)())
        : instance(obj), method(m) {}

    /// Invoke the state's behaviour.
    void invoke() const {
        (instance.*method)();
    }
};

/**
 * @brief Minimal header‑only FSM engine using CRTP and an implicit “waiting” sub‑state.
 *
 * No dynamic allocation and no virtual functions.
 *
 * @tparam Derived The concrete FSM class that inherits from this template.
 */
template <typename Derived>
class StateMachine {
private:
    const State<Derived>* currentState = nullptr; ///< Active state
    const State<Derived>* nextState    = nullptr; ///< State scheduled for transition
    unsigned long         waitTime     = 0;       ///< Wait interval in milliseconds
    bool                  waiting      = false;   ///< Are we currently in the waiting sub‑state?

public:
    StateMachine() = default;

    /**
     * @brief Assign the initial state. Further calls are ignored.
     */
    void setInitialState(const State<Derived>& state) {
        if (!currentState) {
            currentState = &state;
        }
    }

    /**
     * @brief Schedule a transition to @p state.
     *
     * The transition takes effect once the waiting period (if any) expires.
     */
    Derived& transitionTo(const State<Derived>& state) {
        nextState = &state;
        return static_cast<Derived&>(*this);
    }

    /**
     * @brief Start a waiting period before the pending transition is executed.
     *
     * @param ms Duration in milliseconds.
     */
    Derived& wait(unsigned long ms) {
        waitTime = ms;
        waiting  = true;
        return static_cast<Derived&>(*this);
    }

    /**
     * @brief Override in the derived class to implement custom timing.
     *
     * @param ms Duration that was passed to wait().
     * @return true  – still waiting
     * @return false – waiting period completed
     */
    bool waitCondition(unsigned long /*ms*/) const {
        return false;            // Default implementation: never wait
    }

    /**
     * @brief Execute one update cycle.
     *
     * 1. If we are in the implicit waiting sub‑state, poll @c waitCondition().
     *    • @c true  → remain in wait mode, return immediately.  
     *    • @c false → exit wait mode and continue.
     *
     * 2. If a transition has been scheduled, apply it.
     *
     * 3. Invoke the current state's behaviour exactly once.
     */
    void update() {
        // Step 1 – waiting?
        if (waiting) {
            if (static_cast<const Derived*>(this)->waitCondition(waitTime)) {
                return;          // still waiting
            }
            waiting = false;     // waiting finished
        }

        // Step 2 – perform pending transition
        if (nextState) {
            currentState = nextState;
            nextState    = nullptr;
        }

        // Step 3 – invoke current state
        if (currentState) {
            currentState->invoke();
        }
    }
};

} // namespace zeroflow

#endif // ZEROFLOW_HPP_INCLUDED
