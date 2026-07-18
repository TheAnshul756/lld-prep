#include<iostream>
#include<memory>
using namespace std;


class Button {
public:
    virtual ~Button() = default;
    virtual void render() = 0;
    virtual void click() = 0;
};

class CheckBox {
public:
    virtual ~CheckBox() = default;
    virtual void render() = 0;
    virtual void fill() = 0;
};

class WinButton : public Button {
public:
    void render() override {
        cout << "Rendering Windows Button.\n";
    }
    void click() override {
        cout << "Clicking Windows Button.\n";
    }
};

class MacButton : public Button {
public:
    void render() override {
        cout << "Rendering MacOS Button.\n";
    }
    void click() override {
        cout << "Clicking MacOS Button.\n";
    }
};

class WinCheckBox : public CheckBox {
public:
    void render() override {
        cout << "Rendering Windows Checkbox.\n";
    }
    void fill() override {
        cout << "Filling Windows Button.\n";
    }
};

class MacCheckBox : public CheckBox {
public:
    void render() override {
        cout << "Rendering MacOS Checkbox.\n";
    }
    void fill() override {
        cout << "Filling MacOS Button.\n";
    }
};

class GUIFactory {
public:
    virtual ~GUIFactory() = default;
    virtual unique_ptr<Button> createButton() = 0;
    virtual unique_ptr<CheckBox> createCheckBox() = 0;
};

class MacGUIFactory : public GUIFactory {
public:
    unique_ptr<Button> createButton() override {
        return make_unique<MacButton>();
    }
    unique_ptr<CheckBox> createCheckBox() override {
        return make_unique<MacCheckBox>();
    }
};

class WinGUIFactory : public GUIFactory {
public:
    unique_ptr<Button> createButton() override {
        return make_unique<WinButton>();
    }
    unique_ptr<CheckBox> createCheckBox() override {
        return make_unique<WinCheckBox>();
    }
};

class Application {
    unique_ptr<GUIFactory> factory;
public:
    Application(unique_ptr<GUIFactory> factory) : factory(move(factory)) {}
    void renderUI() {
        unique_ptr<Button> btn = factory -> createButton();
        unique_ptr<CheckBox> box = factory -> createCheckBox();
        btn -> render();
        box -> render();
        btn -> click();
        box -> fill();
    }
};

int main() {
    Application winApplication(make_unique<WinGUIFactory>());
    Application macApplication(make_unique<MacGUIFactory>());

    winApplication.renderUI();
    cout << endl;
    macApplication.renderUI();
}