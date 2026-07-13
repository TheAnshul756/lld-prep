#include<iostream>
#include<memory>
#include<vector>
using namespace std;


class Strategy {
public:
    virtual ~Strategy() = default;
    virtual void execute() = 0;
};

class Strategy1 : public Strategy {
public:
    void execute() override {
        cout << "This is Strategy 1 executing.\n";
    }
};

class Strategy2 : public Strategy {
public:
    void execute() override {
        cout << "This is Strategy 2 executing.\n";
    }
};

class Context {
    unique_ptr<Strategy> strategy;
public:
    Context(unique_ptr<Strategy> s) : strategy(move(s)) {}
    void setStrategy(unique_ptr<Strategy> s) {
        strategy = move(s);
    }
    void doSomething() {
        if(strategy) strategy->execute();
    }
};

int main() {
    Context c(make_unique<Strategy1>());
    c.doSomething();
    c.setStrategy(make_unique<Strategy2>());
    c.doSomething();
}