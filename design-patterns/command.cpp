#include<iostream>
#include<memory>
#include<string>
#include<stack>
using namespace std;

class Application;

class Editor {
    string text;
    string selection;
public:
    Editor() : text("") {}
    string getSelection() {
        cout << "Getting Selected text.\n";
        return selection;
    }
    void deleteSelection() {
        cout << "Deleting Selection.\n";
        selection = "";
        return;
    }
    void replaceSelection(string txt) {
        cout << "Replacing selection with " << txt << endl;
        selection = txt;
    }
    string getText() {
        return text;
    }
    void setText(string text) {
        this -> text = text;
    }
};

class Command {
protected:
    weak_ptr<Application> app;
    weak_ptr<Editor> editor;
    string backup;
public:
    Command(shared_ptr<Editor> editor, shared_ptr<Application> app): app(move(app)), editor(move(editor)) {}
    virtual ~Command() = default;
    virtual void execute() = 0;
    void saveBackup() {
        if(shared_ptr<Editor> locked =  editor.lock())
            backup = locked -> getText();
    }
    void undo() {
        if(shared_ptr<Editor> locked =  editor.lock())
            locked -> setText(backup);
    }
};

class CommandHistory {
    stack<shared_ptr<Command>> history;
public:
    void push(shared_ptr<Command> c) {
        history.push(c);
    }
    shared_ptr<Command> pop() {
        if(history.empty()) return nullptr;
        shared_ptr<Command> c = history.top();
        history.pop();
        return c;
    }
};

class Application {
    shared_ptr<Editor> editor;
    unique_ptr<CommandHistory>  history;
    string clipboard;
public:
    Application(shared_ptr<Editor> e, unique_ptr<CommandHistory> h) : editor(move(e)), history(move(h)) {}
    void executeCommand(shared_ptr<Command> c) {
        history -> push(c); 
        c -> execute();
    }
    void undo() {
        auto c = history -> pop();
        if(c != nullptr) {
            c -> undo();
        }
    }
    string getClipBoard() {
        return clipboard;
    }
    void setClipBoard(string str) {
        clipboard = str;
    }
};

class CopyCommand : public Command {
public:
    CopyCommand(shared_ptr<Editor> editor, shared_ptr<Application> app) : Command(editor, app) {}
    void execute() override {
        shared_ptr<Editor> editorLocked =  editor.lock();
        shared_ptr<Application> appLocked =  app.lock();
        if(editorLocked && appLocked)
            appLocked -> setClipBoard(editorLocked -> getSelection());
    }
    void undo() {
        return;
    }
};

class CutCommand : public Command {
public:
    CutCommand(shared_ptr<Editor> editor, shared_ptr<Application> app) : Command(editor, app) {}
    void execute() override {
        shared_ptr<Editor> editorLocked =  editor.lock();
        shared_ptr<Application> appLocked =  app.lock();
        if(editorLocked && appLocked) {
            saveBackup();
            appLocked -> setClipBoard(editorLocked -> getSelection());
            editorLocked -> deleteSelection();
        }
    }
};

class PasteCommand : public Command {
public:
    PasteCommand(shared_ptr<Editor> editor, shared_ptr<Application> app) : Command(editor, app) {}
    void execute() override {
        shared_ptr<Editor> editorLocked =  editor.lock();
        shared_ptr<Application> appLocked =  app.lock();
        if(editorLocked && appLocked) {
            saveBackup();
            editorLocked -> setText(editorLocked -> getText() + appLocked -> getClipBoard());
        }
    }
};


int main() {
    shared_ptr<Editor> e = make_shared<Editor>();
    shared_ptr<Application> app = make_shared<Application>(e, make_unique<CommandHistory>());
    e -> setText("abcd");
    e -> replaceSelection("ttt");
    cout << "Copying " << endl;
    app -> executeCommand(make_shared<CopyCommand>(e, app));

    cout << "Pasting " << endl;
    app -> executeCommand(make_shared<PasteCommand>(e, app));
    cout << e -> getText() << endl;

}