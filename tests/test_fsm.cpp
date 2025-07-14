#include <iostream>
#include <vector>
#include <random>
#include <climits>
#include <memory>
#include "../include/ZeroFlow.hpp"

using namespace zeroflow;

// FakeClock for timed tests
class FakeClock {
    unsigned long t = 0;
public:
    unsigned long now() const { return t; }
    void advance(unsigned long ms) { t += ms; }
};

// Utility to compare vectors and print details
bool compareLogs(const std::vector<char>& history, const std::vector<char>& expected) {
    if (history == expected) return true;
    std::cout << "  Expected: "; for (char c : expected) std::cout << c;
    std::cout << "\n  Got:      "; for (char c : history) std::cout << c;
    std::cout << std::endl;
    return false;
}

// Test 1: default waitCondition (no wait)
bool test_default_wait() {
    struct FSM : StateMachine<FSM> {
        State<FSM> s1, s2;
        std::vector<char> log;
        FSM(): s1(*this, &FSM::S1), s2(*this, &FSM::S2) { setInitialState(s1); }
        void S1() { log.push_back('1'); transitionTo(s2); }
        void S2() { log.push_back('2'); transitionTo(s1); }
        bool waitCondition(unsigned long) const { return false; }
    } fsm;
    for (int i = 0; i < 4; ++i) fsm.update();
    std::vector<char> expected = {'1','2','1','2'};
    std::cout << "test_default_wait: ";
    return compareLogs(fsm.log, expected);
}

// Test 2: timed waitCondition
bool test_timed_wait() {
    struct FSM : StateMachine<FSM> {
        FakeClock& clk; unsigned long start;
        State<FSM> s1, s2; std::vector<char> log;
        FSM(FakeClock& c): clk(c), s1(*this,&FSM::S1),s2(*this,&FSM::S2){ setInitialState(s1);}        
        void S1(){ log.push_back('A'); start=clk.now(); transitionTo(s2).wait(100);}         
        void S2(){ log.push_back('B'); transitionTo(s1).wait(100);}                         
        bool waitCondition(unsigned long ms) const{ return (clk.now()-start)<ms; }
    };
    FakeClock clk; FSM fsm(clk);
    fsm.update(); bool ok1=compareLogs(fsm.log, {'A'});
    clk.advance(50); fsm.update(); bool ok2=(fsm.log.size()==1);
    if(!ok2) std::cout<<"  Unexpected during wait: size="<<fsm.log.size()<<std::endl;
    clk.advance(50); fsm.update(); bool ok3=compareLogs(fsm.log, {'A','B'});
    std::cout<<"test_timed_wait: "; return ok1 && ok2 && ok3;
}

// Test 3: three-state cycle
bool test_three_state() {
    struct FSM : StateMachine<FSM> {
        FakeClock& clk; unsigned long start;
        State<FSM> s1,s2,s3; std::vector<char> log;
        FSM(FakeClock& c):clk(c),s1(*this,&FSM::S1),s2(*this,&FSM::S2),s3(*this,&FSM::S3){setInitialState(s1);}        
        void S1(){log.push_back('X');start=clk.now();transitionTo(s2).wait(10);}            
        void S2(){log.push_back('Y');start=clk.now();transitionTo(s3).wait(10);}            
        void S3(){log.push_back('Z');start=clk.now();transitionTo(s1).wait(10);}            
        bool waitCondition(unsigned long ms)const{return (clk.now()-start)<ms;}
    };
    FakeClock clk; FSM fsm(clk);
    fsm.update(); clk.advance(10);
    fsm.update(); clk.advance(10);
    fsm.update();
    std::vector<char> expected={'X','Y','Z'};
    std::cout<<"test_three_state: "; return compareLogs(fsm.log,expected);
}

// Test 4: interleaving two default FSMs
bool test_interleaving_default(){
    std::vector<char> history;
    struct FSM1:StateMachine<FSM1>{ State<FSM1> s1,s2; std::vector<char>& log;
        FSM1(std::vector<char>& h):s1(*this,&FSM1::S1),s2(*this,&FSM1::S2),log(h){setInitialState(s1);}        
        void S1(){log.push_back('1');transitionTo(s2);}        
        void S2(){log.push_back('2');transitionTo(s1);}        
        bool waitCondition(unsigned long)const{return false;}
    } f1(history);
    struct FSM2:StateMachine<FSM2>{ State<FSM2> s3,s4; std::vector<char>& log;
        FSM2(std::vector<char>& h):s3(*this,&FSM2::S3),s4(*this,&FSM2::S4),log(h){setInitialState(s3);}        
        void S3(){log.push_back('3');transitionTo(s4);}        
        void S4(){log.push_back('4');transitionTo(s3);}        
        bool waitCondition(unsigned long)const{return false;}
    } f2(history);
    for(int i=0;i<4;++i){f1.update();f2.update();}
    std::vector<char> expected={'1','3','2','4','1','3','2','4'};
    std::cout<<"test_interleaving_default: "; return compareLogs(history,expected);
}

// Test 5: three interleaved Blink FSMs sequence
bool test_three_blink(){
    FakeClock clk; std::vector<char> log;
    struct BlinkTest:StateMachine<BlinkTest>{ FakeClock& clk; unsigned long interval,t0; char id; std::vector<char>& log;
        State<BlinkTest> sLow,sHigh;
        BlinkTest(char id,unsigned long intv,FakeClock& c,std::vector<char>& l):clk(c),interval(intv),t0(0),id(id),log(l),sLow(*this,&BlinkTest::low),sHigh(*this,&BlinkTest::high){setInitialState(sLow);}        
        void low(){log.push_back(id);t0=clk.now();transitionTo(sHigh).wait(interval);}        
        void high(){log.push_back(id);t0=clk.now();transitionTo(sLow).wait(interval);}        
        bool waitCondition(unsigned long ms)const{return (clk.now()-t0)<ms;}
    } bA('A',300,clk,log),bB('B',500,clk,log),bC('C',700,clk,log);
    for(int step=0;step<20;++step){bA.update();bB.update();bC.update();clk.advance(100);}    
    // build expected
    std::vector<char> expected; unsigned long t=0,nextA=0,nextB=0,nextC=0;
    for(int step=0;step<20;++step){if(t>=nextA){expected.push_back('A');nextA+=300;}if(t>=nextB){expected.push_back('B');nextB+=500;}if(t>=nextC){expected.push_back('C');nextC+=700;}t+=100;}    
    std::cout<<"test_three_blink: "; return compareLogs(log,expected);
}

// Test 6: no initial state
bool test_no_initial(){
    struct FSM: StateMachine<FSM> {
        // no State members
        bool waitCondition(unsigned long) const { return false; }
    } fsm;
    // should not crash or invoke
    fsm.update(); fsm.update();
    std::cout << "test_no_initial: ";
    return true;
}

// Test 7: double setInitialState
bool test_double_set_initial() {
    struct FSM : StateMachine<FSM> {
        State<FSM> s1, s2;
        std::vector<char> log;
        FSM() : s1(*this, &FSM::S1), s2(*this, &FSM::S2) {
            setInitialState(s1);   // first call – should stick
            setInitialState(s2);   // second call – must be ignored
        }
        void S1() { log.push_back('1'); }
        void S2() { log.push_back('2'); }
        bool waitCondition(unsigned long) const { return false; }
    } fsm;

    fsm.update();  // should invoke ONLY S1

    std::cout << "test_double_set_initial: ";
    return (fsm.log.size() == 1 && fsm.log[0] == '1');
}

// Test 8: zero-delay transition
bool test_zero_delay(){
    struct FSM:StateMachine<FSM>{ State<FSM> s1,s2; std::vector<char> log;
        FSM():s1(*this,&FSM::S1),s2(*this,&FSM::S2){ setInitialState(s1);}        
        void S1(){log.push_back('X'); transitionTo(s2).wait(0);}        
        void S2(){log.push_back('Y');}
        bool waitCondition(unsigned long)const{return false;}
    } fsm;
    fsm.update(); // S1->schedule S2
    fsm.update(); // immediate S2
    std::cout<<"test_zero_delay: ";
    return (fsm.log.size()==2 && fsm.log[0]=='X'&&fsm.log[1]=='Y');
}

// Test 9: rapid-fire transitions
bool test_rapid_fire() {
    struct FSM : StateMachine<FSM> {
        State<FSM> s1, s2, s3;
        std::vector<char> log;
        FSM() : s1(*this, &FSM::S1), s2(*this, &FSM::S2), s3(*this, &FSM::S3) {
            setInitialState(s1);
        }
        void S1() {
            log.push_back('1');
            // Issue two consecutive transitions; only the LAST one should be effective
            transitionTo(s2);
            transitionTo(s3); // this should win
        }
        void S2() { log.push_back('2'); }
        void S3() { log.push_back('3'); }
        bool waitCondition(unsigned long) const { return false; }
    } fsm;

    fsm.update(); // invokes S1, schedules S3
    fsm.update(); // should invoke ONLY S3

    std::vector<char> expected = {'1', '3'};
    std::cout<<"test_rapid_fire: ";
    return compareLogs(fsm.log, expected);
}

// Test 10: overflow timing
bool test_overflow(){
    struct FSM:StateMachine<FSM>{ FakeClock& clk; unsigned long start; State<FSM> s1;
        std::vector<char> log;
        FSM(FakeClock& c):clk(c),s1(*this,&FSM::S1){ setInitialState(s1);}        
        void S1(){log.push_back('O'); start=clk.now(); transitionTo(s1).wait(10);}        
        bool waitCondition(unsigned long ms)const{
            unsigned long now = clk.now();
            // simulate unsigned wrap safe: (now - start) < ms even if now<start
            return (static_cast<long long>(now) - static_cast<long long>(start)) < static_cast<long long>(ms);
        }
    };
    FakeClock clk;
    // place start near max
    clk.advance(ULLONG_MAX - 5);
    FSM fsm(clk);
    fsm.update(); // log 'O'
    // advance past wrap
    clk.advance(20);
    fsm.update(); // should log again if wrap safe
    std::cout<<"test_overflow: ";
    return (fsm.log.size()==2);
}
// -----------------------------------------------------------------------------
// 11. fuzzing — 100 Blink com intervalos aleatórios
// -----------------------------------------------------------------------------
bool test_fuzzing() {
    FakeClock clk;
    std::mt19937 rng(123);
    std::uniform_int_distribution<int> jitter(10, 150);

    struct FSM : StateMachine<FSM> {
        FakeClock& clk;
        unsigned long last;
        int toggles = 0;
        State<FSM> self;

        FSM(FakeClock& c)
          : clk(c),
            last(0),
            self(*this, &FSM::run)
        {
            setInitialState(self);
        }

        void run() {
            ++toggles;                   // count a toggle
            last = clk.now();            // reset timer
            transitionTo(self).wait(500);
        }

        bool waitCondition(unsigned long ms) const  {
            return (clk.now() - last) < ms;
        }
    } fsm(clk);

    // Simulate until ~5000 ms real time
    unsigned long simulated = 0;
    while (simulated < 5000) {
        int dt = jitter(rng);
        clk.advance(dt);
        simulated += dt;
        fsm.update();
    }

    // Compute expected toggles = floor(simulated/500)
    int expected =  (simulated / 500);
    std::cout << "    simulated=" << simulated
              << "ms, toggles=" << fsm.toggles
              << " (expected=" << expected << ")\n";

    std::cout << "test_fuzzing: ";
    return (fsm.toggles == expected);
}

// -----------------------------------------------------------------------------
// 12. mass_allocation — stress de heap (50k instâncias BlinkF curtas)
// -----------------------------------------------------------------------------
// 12. mass_allocation — stress de heap com 50k FSMs (C++11-safe)
bool test_mass_allocation() {
    struct Dummy : StateMachine<Dummy> {
        bool waitCondition(unsigned long) const { return false; }
    };

    {
        std::vector<std::unique_ptr<Dummy>> vec;
        vec.reserve(50000);

        for (int i = 0; i < 50000; ++i) {
            vec.emplace_back(new Dummy());   // C++11: constrói unique_ptr a partir de raw ptr
        }
        for (auto& p : vec) p->update();
    } // destrói todas aqui

    std::cout << "test_mass_allocation: PASS (alloc/dealloc)";
    return true;
}

// -----------------------------------------------------------------------------
// MAIN – executa todos os 12 testes
// -----------------------------------------------------------------------------
int main() {
    struct Test { const char* name; bool (*fn)(); } tests[] = {
        {"test_default_wait",           test_default_wait},
        {"test_timed_wait",             test_timed_wait},
        {"test_three_state",            test_three_state},
        {"test_interleaving_default",   test_interleaving_default},
        {"test_three_blink",            test_three_blink},
        {"test_no_initial",             test_no_initial},
        {"test_double_set_initial",     test_double_set_initial},
        {"test_zero_delay",             test_zero_delay},
        {"test_rapid_fire",             test_rapid_fire},
        {"test_overflow",               test_overflow},
        {"test_mass_allocation",        test_mass_allocation},
        {"test_fuzzing",                test_fuzzing}
    };

    bool all = true;
    for (auto& t : tests) {
        bool res = t.fn();
        std::cout << (res ? "PASS" : "FAIL") << '\n';
        all &= res;
    }
    return all ? 0 : 1;
}
