#include<iostream>
#include<memory>
using namespace std;

class Product {
public:
    virtual ~Product() = default;
    virtual void usecase1() = 0;
    virtual void usecase2() = 0;
};

class Product1 : public Product {
public:
    void usecase1() override {
        cout << "Product 1 is performing use case 1\n";
    }
    void usecase2() override {
        cout << "Product 1 is performing use case 2\n";
    }
};

class Product2 : public Product {
public:
    void usecase1() override {
        cout << "Product 2 is performing use case 1\n";
    }
    void usecase2() override {
        cout << "Product 2 is performing use case 2\n";
    }
};

class Creator {
public:
    virtual ~Creator() = default;
    virtual unique_ptr<Product> createProduct() = 0;
    void execute() {
        unique_ptr<Product> product = createProduct();
        product -> usecase1();
        product -> usecase2();
    }
};

class ConcreteCreator1 : public Creator {
public:
    unique_ptr<Product> createProduct() override {
        return make_unique<Product1>();
    }
};

class ConcreteCreator2 : public Creator {
public:
    unique_ptr<Product> createProduct() override {
        return make_unique<Product2>();
    }
};

int main() {
    unique_ptr<Creator> creator1 = make_unique<ConcreteCreator1>();
    creator1 -> execute();
    unique_ptr<Creator> creator2 = make_unique<ConcreteCreator2>();
    creator2 -> execute();
}