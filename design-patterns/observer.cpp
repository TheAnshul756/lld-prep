#include<string>
#include<vector>
#include<iostream>
#include<memory>
using namespace std;

class Subscriber {
public:
    virtual ~Subscriber() = default;
    virtual void notify(string) = 0;
};

class Publisher {
public:
    virtual ~Publisher() = default;
    virtual void addSubscriber(shared_ptr<Subscriber>) = 0;
    virtual void removeSubscriber(shared_ptr<Subscriber>) = 0;
    virtual void sendNotification(string) = 0;
};

class Subscriber1 : public Subscriber {
public:
    void notify(string message) override {
        cout << "Recived notification in class of type SubScriber1 with message : " << message << endl;
    }
};

class Publisher1 : public Publisher {
    vector<weak_ptr<Subscriber>> subscribers;
    public:
    void addSubscriber(shared_ptr<Subscriber> s) override {
        subscribers.push_back(s);
    }
    void removeSubscriber(shared_ptr<Subscriber> s) override {
        for(uint i = 0; i < subscribers.size(); i++) {
            if(!subscribers[i].lock() || subscribers[i].lock() == s) {
                subscribers.erase(subscribers.begin() + i);
                i--;
            }
        }
    }
    void sendNotification(string message) override {
        cout << "sending notification.\n";
        for(auto i : subscribers) {
            if(shared_ptr<Subscriber> p = i.lock())
                p -> notify(message);
        }
    }
};

int main() {
    unique_ptr<Publisher> pub = make_unique<Publisher1>();
    shared_ptr<Subscriber> sub = make_shared<Subscriber1>();
    pub -> addSubscriber(sub);
    pub -> sendNotification("Notification 1");
    pub -> removeSubscriber(sub);
    pub -> sendNotification("Notification 2");
}