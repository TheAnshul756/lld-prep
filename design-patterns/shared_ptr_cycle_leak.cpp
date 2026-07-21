// CONCEPT: shared_ptr reference cycles (a.k.a. ownership cycles)
// ================================================================
//
// std::shared_ptr manages an object's lifetime by reference counting:
// the object is destroyed the instant its count hits 0. That works
// great for a tree of ownership, but it breaks the moment two objects
// hold shared_ptrs to *each other* (directly, or through a chain of
// containers). Each one's refcount always has at least 1 contributed
// by the other, so neither can ever reach 0 -- both leak, silently,
// with no error, no crash, no warning. Valgrind/ASan won't even flag
// it as "definitely lost" in many cases, because the memory is still
// technically reachable through the cycle itself.
//
// This file demonstrates the problem with a minimal Owner/Child pair,
// fixes it two different ways, and explains when to reach for each fix.
//
// Compile & run:
//   g++ -std=c++17 -o shared_ptr_cycle_leak shared_ptr_cycle_leak.cpp && ./shared_ptr_cycle_leak

#include <iostream>
#include <memory>
#include <vector>
using namespace std;

// ================================================================
// 1) BROKEN: Child holds a shared_ptr back to its Owner.
//
//    Ownership graph:  Owner --(shared_ptr, in vector)--> Child
//                       Child --(shared_ptr)--------------> Owner
//
//    That's a cycle. Owner "owns" Child, but Child also insists on
//    co-owning Owner. Neither will ever see its refcount drop to 0
//    on its own.
// ================================================================
namespace broken {

struct Owner {
    vector<shared_ptr<struct Child>> children;
    ~Owner() { cout << "[broken] Owner destroyed\n"; }
};

struct Child {
    shared_ptr<Owner> owner; // <-- the mistake: Child shares ownership of the thing that owns it
    Child(shared_ptr<Owner> o) : owner(move(o)) {}
    ~Child() { cout << "[broken] Child destroyed\n"; }
};

void run() {
    cout << "\n=== 1) BROKEN: shared_ptr cycle ===\n";
    auto owner = make_shared<Owner>();
    cout << "use_count right after creation:      " << owner.use_count() << "\n";

    auto child = make_shared<Child>(owner);   // Child copies the shared_ptr -> refcount +1
    owner->children.push_back(child);         // Owner keeps its own shared_ptr to Child

    cout << "use_count after wiring the cycle:    " << owner.use_count() << "\n";

    child.reset(); // the *local* shared_ptr<Child> goes away; Owner's copy still lives on
    cout << "use_count after local child.reset(): " << owner.use_count() << "\n";

    cout << "leaving broken::run() -- owner's local shared_ptr is about to be destroyed...\n";
    // Watch the output: neither "Owner destroyed" nor "Child destroyed" will ever
    // print. The refcount can't drop below 1 because Owner's own Child holds a
    // reference back to it. Both objects are leaked for the rest of the program.
}

} // namespace broken


// ================================================================
// 2) FIXED (option A): Child holds a raw, non-owning pointer.
//
//    Use this when the relationship is structurally one-directional:
//    Owner's lifetime always fully contains Child's lifetime, so
//    Child never needs to keep Owner alive -- it just needs to *see*
//    it while it's around, which it always is.
// ================================================================
namespace fixed_raw_ptr {

struct Owner {
    vector<shared_ptr<struct Child>> children;
    ~Owner() { cout << "[fixed:raw]  Owner destroyed\n"; }
};

struct Child {
    Owner* owner; // non-owning: doesn't touch the refcount at all
    Child(Owner* o) : owner(o) {}
    ~Child() { cout << "[fixed:raw]  Child destroyed\n"; }
};

void run() {
    cout << "\n=== 2) FIXED (raw pointer): no cycle ===\n";
    auto owner = make_shared<Owner>();
    cout << "use_count right after creation:      " << owner.use_count() << "\n";

    auto child = make_shared<Child>(owner.get()); // pass the raw address, not the shared_ptr
    owner->children.push_back(child);

    cout << "use_count after wiring:              " << owner.use_count() << "\n"; // stays 1

    child.reset();
    cout << "use_count after local child.reset(): " << owner.use_count() << "\n";

    cout << "leaving fixed_raw_ptr::run() -- owner's local shared_ptr is about to be destroyed...\n";
    // Owner's refcount correctly hits 0 here. Its destructor runs, which destroys
    // its `children` vector, which destroys the Child stored in it. Deterministic,
    // no leak. Both messages print, in the correct order (Owner first, since its
    // destructor is what triggers the vector's destruction).
}

} // namespace fixed_raw_ptr


// ================================================================
// 3) FIXED (option B): Child holds a weak_ptr instead of a raw pointer.
//
//    Use this when you want the same "don't extend Owner's lifetime"
//    behavior as a raw pointer, but ALSO want to safely detect if
//    Owner has already been destroyed before you dereference it (e.g.
//    the Child might outlive Owner in some code paths, or you're not
//    100% sure of the lifetime guarantee and want a checked failure
//    mode instead of undefined behavior). You pay for that safety with
//    a `.lock()` call (and a null-check) every time you need to use it.
// ================================================================
namespace fixed_weak_ptr {

struct Owner {
    vector<shared_ptr<struct Child>> children;
    ~Owner() { cout << "[fixed:weak] Owner destroyed\n"; }
};

struct Child {
    weak_ptr<Owner> owner; // observes Owner without extending its lifetime
    Child(weak_ptr<Owner> o) : owner(move(o)) {}
    ~Child() { cout << "[fixed:weak] Child destroyed\n"; }

    void useOwner() {
        if (auto locked = owner.lock()) { // succeeds only if Owner is still alive
            cout << "[fixed:weak] Owner is still alive, safe to use it\n";
        } else {
            cout << "[fixed:weak] Owner is already gone -- would have been a dangling raw pointer!\n";
        }
    }
};

void run() {
    cout << "\n=== 3) FIXED (weak_ptr): no cycle, plus a safety check ===\n";
    auto owner = make_shared<Owner>();
    auto child = make_shared<Child>(owner); // weak_ptr construction does NOT bump use_count

    cout << "use_count after wiring:              " << owner.use_count() << "\n"; // stays 1
    child->useOwner();

    owner.reset(); // Owner is destroyed right now -- nothing is keeping it alive
    child->useOwner(); // still safe: weak_ptr::lock() just returns nullptr, no UB
}

} // namespace fixed_weak_ptr


int main() {
    broken::run();
    fixed_raw_ptr::run();
    fixed_weak_ptr::run();

    // --------------------------------------------------------------
    // TAKEAWAYS
    // --------------------------------------------------------------
    // - shared_ptr expresses SHARED, ambiguous ownership: "I don't know
    //   who else needs this alive, so let's all vote and the last one
    //   out turns off the lights." Give it to two objects that point
    //   at each other and nobody ever turns off the lights.
    //
    // - Before adding a shared_ptr member, ask: "does this object need
    //   to keep the other one alive, or does it just need to *use* it
    //   while it happens to be alive?" If it's the latter, shared_ptr
    //   is the wrong tool.
    //
    // - Raw pointer: use when the referenced object's lifetime
    //   provably outlives (contains) the pointer's lifetime -- e.g. a
    //   child that only ever exists while stored inside its parent.
    //   Zero overhead, but dereferencing after the owner is gone is
    //   undefined behavior, so only use it when that's structurally
    //   impossible.
    //
    // - weak_ptr: use when you want the same non-owning relationship
    //   but the lifetime guarantee isn't ironclad, or you want a
    //   checked, defensive way to detect "the thing I'm pointing at
    //   might already be gone" instead of risking a dangling pointer.
    //
    // - Real-world example this file was written for: a Command
    //   pattern where an Application (invoker) owns a CommandHistory
    //   which owns executed Command objects. If Command stores a
    //   shared_ptr<Application> back to its invoker, you get exactly
    //   this cycle: Application -> CommandHistory -> Command ->
    //   Application. Since Application always outlives every Command
    //   it holds, Command should store a raw Application* (or
    //   weak_ptr<Application> if that guarantee ever gets shaky).
}
