// Demo: why a base class used polymorphically needs a VIRTUAL destructor.
//
// Build:  g++ -std=c++17 -Wall -Wextra -o vdtor_demo virtual_destructor_demo.cpp
// Run:    ./vdtor_demo
//
// Watch the console output closely -- in Case 1 you'll see the derived
// class's constructor message print, but its destructor message NEVER
// prints. That's a real, silent memory leak.

#include <iostream>
using namespace std;

// =====================================================================
// CASE 1: Base destructor is NOT virtual.  (the broken version)
// =====================================================================

struct BaseNoVirtual {
    // A plain, non-virtual destructor. Looks harmless on its own.
    ~BaseNoVirtual() { cout << "  ~BaseNoVirtual()\n"; }
};

struct DerivedNoVirtual : BaseNoVirtual {
    int* data; // some resource DerivedNoVirtual owns and must clean up

    DerivedNoVirtual() {
        data = new int[100];
        cout << "  DerivedNoVirtual() allocated data\n";
    }

    // This destructor exists and is correct -- the problem isn't here,
    // it's in whether C++ actually calls it (see main() below).
    ~DerivedNoVirtual() {
        delete[] data;
        cout << "  ~DerivedNoVirtual() freed data\n";
    }
};

// =====================================================================
// CASE 2: Base destructor IS virtual.  (the correct version)
// =====================================================================

struct BaseVirtual {
    // The 'virtual' keyword here is the entire fix. It tells the compiler:
    // "when deleting through a Base* pointer, look up the REAL type of
    // the object at runtime (via the vtable) and call ITS destructor,
    // not just mine."
    virtual ~BaseVirtual() { cout << "  ~BaseVirtual()\n"; }
};

struct DerivedVirtual : BaseVirtual {
    int* data;

    DerivedVirtual() {
        data = new int[100];
        cout << "  DerivedVirtual() allocated data\n";
    }

    // 'override' is optional here but good practice: it makes the compiler
    // verify this really does override a virtual function in the base.
    ~DerivedVirtual() override {
        delete[] data;
        cout << "  ~DerivedVirtual() freed data\n";
    }
};

int main() {
    // ------------------------------------------------------------------
    // Both cases below do the SAME thing structurally:
    //   1. Create a Derived object on the heap.
    //   2. Store it in a Base* pointer (this is normal, intended usage --
    //      it's exactly how you'd hold a Subscriber* that actually points
    //      to a Subscriber1, Subscriber2, etc.)
    //   3. delete it through that Base* pointer.
    // ------------------------------------------------------------------

    cout << "=== Case 1: delete through BaseNoVirtual* (non-virtual dtor) ===\n";
    BaseNoVirtual* b1 = new DerivedNoVirtual();
    delete b1;
    // Expected if destructors chained correctly: constructor line, then
    // "~DerivedNoVirtual() freed data", then "~BaseNoVirtual()".
    // ACTUAL: "~DerivedNoVirtual()" line is MISSING. The compiler only
    // knows the STATIC type of b1 (BaseNoVirtual*), so it hard-codes a
    // call to ~BaseNoVirtual() only. DerivedNoVirtual's destructor --
    // and the delete[] data inside it -- never runs. That's a leak.
    // (Formally this is undefined behavior per the C++ standard; in
    // practice, this "silently skip the derived destructor" is what
    // every mainstream compiler does.)

    cout << "\n=== Case 2: delete through BaseVirtual* (virtual dtor) ===\n";
    BaseVirtual* b2 = new DerivedVirtual();
    delete b2;
    // Here the destructor call is resolved at RUNTIME via the vtable,
    // based on the object's ACTUAL type (DerivedVirtual), not the
    // pointer's declared type. So ~DerivedVirtual() runs first (freeing
    // data), and it automatically chains up to ~BaseVirtual() after --
    // that chaining to the base always happens, virtual or not.

    // ------------------------------------------------------------------
    // The cost of 'virtual': one hidden pointer per object (the vptr),
    // which points to a per-CLASS table of function addresses (vtable).
    // That pointer is what makes runtime dispatch possible. You only pay
    // for it once a class has at least one virtual function -- so adding
    // a virtual destructor on top of an already-virtual class (like your
    // Subscriber, which has a pure virtual notify()) costs nothing extra.
    // ------------------------------------------------------------------
    cout << "\nsizeof(BaseNoVirtual) = " << sizeof(BaseNoVirtual)
         << " bytes (empty class)\n";
    cout << "sizeof(BaseVirtual)   = " << sizeof(BaseVirtual)
         << " bytes (holds a vptr)\n";

    // ------------------------------------------------------------------
    // Rule of thumb: if a class has ANY virtual function (like your
    // Subscriber::notify or Publisher::sendNotification), it is meant to
    // be used polymorphically -- through base pointers/references. Give
    // it a virtual destructor too, even if it does nothing:
    //
    //     virtual ~Subscriber() = default;
    //
    // Without it, `Subscriber* s = new Subscriber1(); delete s;`
    // reproduces exactly the bug in Case 1 above.
    // ------------------------------------------------------------------
}
