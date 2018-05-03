pragma experimental ABIEncoderV2;
contract C {
    struct S { bool f; }
    S s;
    function f() internal pure returns (S storage) {
        throw;
    }
}
// ----
// Warning: (0-33): Experimental features are turned on. Do not use experimental features on live deployments.
// Warning: (142-147): "throw" is deprecated in favour of "revert()", "require()" and "assert()".
