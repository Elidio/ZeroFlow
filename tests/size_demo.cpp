#include <iostream>
#include "../include/ZeroFlow.hpp"

using namespace zeroflow;

// -----------------------------------------------------------------------------
// Caso 1: FSM “vazia” – só herda StateMachine, sem membros próprios.
// -----------------------------------------------------------------------------
class EmptyFSM : public StateMachine<EmptyFSM> {
public:
    // Nenhum estado real; sem waitCondition custom.
};

// -----------------------------------------------------------------------------
// Caso 2: FSM simples com dois estados e um inteiro extra.
// -----------------------------------------------------------------------------
class TwoState : public StateMachine<TwoState> {
private:
    State<TwoState> s1, s2;
    int counter;                          // payload extra
public:
    TwoState() : s1(*this,&TwoState::A),
                 s2(*this,&TwoState::B),
                 counter(0)
    { setInitialState(s1); }

    void A() { ++counter; transitionTo(s2).wait(0); }
    void B() { --counter; transitionTo(s1).wait(0); }

    bool waitCondition(unsigned long) const { return false; }
};

// -----------------------------------------------------------------------------
// Main: imprime sizeof(...) para cada classe
// -----------------------------------------------------------------------------
int main() {
    std::cout << "sizeof(StateMachine<void>): "
              << sizeof(StateMachine<EmptyFSM>) << " bytes\n";

    std::cout << "sizeof(EmptyFSM):           "
              << sizeof(EmptyFSM) << " bytes\n";

    std::cout << "sizeof(TwoState):           "
              << sizeof(TwoState) << " bytes\n";

#if defined(__GNUG__)
    std::cout << "\n[Compilador: GCC/Clang detectado; 64-bit ptr = 8 bytes]\n";
#endif
}
