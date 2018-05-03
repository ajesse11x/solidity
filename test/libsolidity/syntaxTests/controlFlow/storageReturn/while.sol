contract C {
	struct S { bool f; }
	S s;
    function f1() internal view returns (S storage c) {
        while((c = s).f) {
        }
    }
    function f2() internal view returns (S storage c) {
        while(false) {
            c = s; // warning expected
        }
    }
    function f3() internal view returns (S storage c) {
        c = s;
        while(false) {
        }
    }
    function f4() internal view returns (S storage c) {
        while(false) {
        }
        c = s;
    }
}
// ----
// Warning: (181-192): uninitialized storage pointer may be returned
