// Demo: unique_ptr, shared_ptr, weak_ptr — what they are and when to use them.
//
// Build:  g++ -std=c++17 -Wall -Wextra -o smart_ptrs smart_pointers_demo.cpp
// Run:    ./smart_ptrs
//
// Read the output alongside the code section by section.

#include <iostream>
#include <memory>   // all smart pointers live here
#include <vector>
using namespace std;

// A simple class we'll use throughout. Prints when it's created/destroyed
// so you can SEE exactly when memory is freed.
struct Resource {
    string name;
    Resource(string n) : name(n) { cout << "    [+] Resource '" << name << "' created\n"; }
    ~Resource()                  { cout << "    [-] Resource '" << name << "' destroyed\n"; }
    void use() { cout << "    [ ] Using resource '" << name << "'\n"; }
};

// =====================================================================
// SECTION 1 — Raw pointer (the old way, for contrast)
// =====================================================================
void section_raw_pointer() {
    cout << "\n--- Raw pointer ---\n";

    // You are 100% responsible for calling delete. The compiler won't remind you.
    Resource* r = new Resource("raw");
    r->use();

    // If you forget `delete r` here (or an exception fires before it),
    // the memory leaks. The destructor never prints. Comment out the
    // next line and re-run to see the missing "destroyed" message.
    delete r;

    cout << "  (control returned from function -- raw pointer is gone,\n"
         << "   but ONLY because we remembered to delete it manually)\n";
}

// =====================================================================
// SECTION 2 — unique_ptr: SOLE ownership, automatic cleanup
// =====================================================================
// Rule: only ONE owner at a time. When that owner goes out of scope,
// the object is automatically deleted. Cannot be copied; can be moved.
void section_unique_ptr() {
    cout << "\n--- unique_ptr ---\n";

    // make_unique is the preferred way to create one (never need 'new').
    unique_ptr<Resource> u = make_unique<Resource>("unique");
    u->use();   // use it like a normal pointer with -> or *

    cout << "  (leaving function scope...)\n";
    // No delete needed. When 'u' goes out of scope here, it automatically
    // calls delete on the Resource. Watch the "destroyed" line print.


    // --- Ownership transfer (move) ---
    cout << "\n  Transfer of unique_ptr ownership:\n";
    unique_ptr<Resource> owner1 = make_unique<Resource>("transferred");
    cout << "  owner1 holds the resource\n";

    // unique_ptr cannot be COPIED (that would make two owners, violating uniqueness).
    // It can only be MOVED. After move, owner1 becomes null.
    unique_ptr<Resource> owner2 = move(owner1);
    cout << "  After move: owner1 is " << (owner1 == nullptr ? "null" : "not null") << "\n";
    cout << "  owner2 now holds the resource\n";
    // owner2 goes out of scope here, resource is freed.


    // --- When to use unique_ptr ---
    // Use it almost everywhere you used `new` before. It's the default smart
    // pointer. Zero overhead compared to a raw pointer.
}

// =====================================================================
// SECTION 3 — shared_ptr: SHARED ownership, reference counted
// =====================================================================
// Rule: multiple owners can hold the same object. A hidden counter tracks
// how many shared_ptrs point to it. The object is deleted only when the
// counter hits zero (i.e., the last owner releases it).
void section_shared_ptr() {
    cout << "\n--- shared_ptr ---\n";

    shared_ptr<Resource> sp1 = make_shared<Resource>("shared");
    cout << "  sp1 created.   use_count = " << sp1.use_count() << "\n";

    {
        // sp2 is a copy of sp1 -- both point to the same Resource,
        // reference count goes up to 2.
        shared_ptr<Resource> sp2 = sp1;
        cout << "  sp2 = sp1.     use_count = " << sp1.use_count() << "\n";
        sp2->use();

        {
            shared_ptr<Resource> sp3 = sp1;
            cout << "  sp3 = sp1.     use_count = " << sp1.use_count() << "\n";
            cout << "  (sp3 going out of scope...)\n";
            // sp3 destroyed here, count drops to 2.
        }
        cout << "  sp3 gone.      use_count = " << sp1.use_count() << "\n";
        cout << "  (sp2 going out of scope...)\n";
        // sp2 destroyed here, count drops to 1.
    }
    cout << "  sp2 gone.      use_count = " << sp1.use_count() << "\n";
    cout << "  (sp1 going out of scope -- last owner...)\n";
    // sp1 destroyed here, count drops to 0 --> Resource is deleted.


    // --- When to use shared_ptr ---
    // When genuinely multiple parts of your program share the same object
    // and you can't determine statically who will finish last.
    // It's heavier than unique_ptr (atomic reference count operations).
}

// =====================================================================
// SECTION 4 — weak_ptr: NON-OWNING reference to a shared_ptr object
// =====================================================================
// Problem shared_ptr has: if two objects hold shared_ptrs to EACH OTHER
// (a cycle), their counts never reach zero and they both leak forever.
//
// weak_ptr solves this. It points to the same object a shared_ptr does,
// but does NOT increment the reference count. So it doesn't keep the
// object alive.
//
// Before using a weak_ptr you must 'lock()' it -- this gives you a
// temporary shared_ptr. If the object was already deleted, lock() returns
// null, so you can check safely.

struct Node {
    string name;
    shared_ptr<Node> next;   // <-- try making this weak_ptr and see the cycle break
    // weak_ptr<Node> next;  // uncomment this, comment above line, re-run

    Node(string n) : name(n) { cout << "    [+] Node '" << name << "' created\n"; }
    ~Node()                  { cout << "    [-] Node '" << name << "' destroyed\n"; }
};

void section_shared_ptr_cycle_leak() {
    cout << "\n--- shared_ptr cycle (LEAK -- destructors will NOT print) ---\n";
    {
        shared_ptr<Node> a = make_shared<Node>("A");
        shared_ptr<Node> b = make_shared<Node>("B");
        a->next = b;  // A holds a shared_ptr to B  (B count = 2)
        b->next = a;  // B holds a shared_ptr to A  (A count = 2)
        cout << "  a use_count=" << a.use_count()
             << "  b use_count=" << b.use_count() << "\n";
        // Scope ends: a destroyed (A count 2->1), b destroyed (B count 2->1).
        // Neither reaches 0. Neither destructor runs. Both nodes leak.
    }
    cout << "  (scope ended -- notice NO 'destroyed' lines above)\n";
}

void section_weak_ptr() {
    cout << "\n--- weak_ptr breaking the cycle ---\n";
    {
        shared_ptr<Node> a = make_shared<Node>("A");

        // Node B uses a weak_ptr to point back to A.
        // We use a small lambda/struct trick: embed weak_ptr directly.
        // For simplicity, we just show the standalone weak_ptr pattern here.
        weak_ptr<Node> weak_a = a;  // does NOT increment a's ref count

        cout << "  a use_count after weak_ptr = " << a.use_count()
             << "  (still 1, weak_ptr doesn't count)\n";

        // To use a weak_ptr, you must lock() it first.
        // lock() returns a shared_ptr. If the object is alive, it's valid.
        // If the object was already destroyed, it returns nullptr.
        if (shared_ptr<Node> locked = weak_a.lock()) {
            cout << "  lock() succeeded, object is alive: " << locked->name << "\n";
        }

        cout << "  (a going out of scope -- only owner)\n";
        // a goes out of scope, count drops to 0, Node A is deleted.
    }

    cout << "  (scope ended -- now try to lock the dangling weak_ptr)\n";
    // We can't access weak_a here (it's out of scope), but the pattern is:
    // if (auto locked = weak_a.lock()) { /* object alive */ }
    // else                             { /* object was deleted, locked is null */ }
}

void section_weak_ptr_standalone() {
    cout << "\n--- weak_ptr: safe access after source is gone ---\n";

    weak_ptr<Resource> wp;

    {
        shared_ptr<Resource> sp = make_shared<Resource>("ephemeral");
        wp = sp;  // weak_ptr points at same object, count still 1
        cout << "  Inside scope: lock valid? "
             << (wp.lock() ? "yes" : "no") << "\n";
        cout << "  (sp going out of scope...)\n";
    }
    // sp destroyed here, Resource deleted (count hit 0).

    cout << "  Outside scope: lock valid? "
         << (wp.lock() ? "yes" : "no") << "\n";
    // lock() returns null because the Resource is gone.
    // No crash, no undefined behavior -- weak_ptr handles this safely.
}

// =====================================================================
// SECTION 5 — Quick cheatsheet (printed at the end)
// =====================================================================
void print_cheatsheet() {
    cout << R"(
======================================================
CHEATSHEET
------------------------------------------------------
unique_ptr<T>   Sole ownership. Auto-deletes on scope
                exit. Cannot copy, only move.
                Use: default choice everywhere.

shared_ptr<T>   Shared ownership via ref count.
                Deleted when last owner releases it.
                Use: when multiple owners genuinely
                     need the same object.

weak_ptr<T>     Non-owning. Doesn't affect ref count.
                Must lock() before using; returns null
                if object already deleted.
                Use: break shared_ptr cycles, or hold
                     a "maybe-alive" reference safely.

Raw T*          No ownership semantics at all. You
                manually manage lifetime. Avoid for
                owning pointers; fine for non-owning
                observers when lifetime is clear.
======================================================
)";
}

int main() {
    section_raw_pointer();
    section_unique_ptr();
    section_shared_ptr();
    section_shared_ptr_cycle_leak();
    section_weak_ptr();
    section_weak_ptr_standalone();
    print_cheatsheet();
}
