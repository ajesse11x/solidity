pragma experimental ABIEncoderV2;
contract C {
    struct S { bool f; }
    S s;
    function g() internal view returns (S storage, uint) {
        return (s,2);
    }
    function f() internal view returns (S storage c) {
        uint a;
        (c, a) = g();
    }
}
// ----
// Warning: (0-33): Experimental features are turned on. Do not use experimental features on live deployments.
