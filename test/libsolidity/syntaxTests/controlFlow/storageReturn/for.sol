pragma experimental ABIEncoderV2;
contract C {
	struct S { bool f; }
	S s;
    function f1() internal view returns (S storage c) {
        for(c = s;;) {
        }
    }
    function f2() internal view returns (S storage c) {
        for(; (c = s).f;) {
        }
    }
    function f3() internal view returns (S storage c) {
        for(;; c = s) {
        }
    }
    function f4() internal view returns (S storage c) {
        for(;;) {
            c = s;
        }
    }
}
// ----
// Warning: (0-33): Experimental features are turned on. Do not use experimental features on live deployments.
// Warning: (311-322): uninitialized storage pointer may be returned
// Warning: (407-418): uninitialized storage pointer may be returned
