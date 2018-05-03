contract C {
	struct S { bool f; }
	S s;
    function f(bool flag) internal view returns (S storage c) {
        if (flag) c = s;
        else c = s;
    }
    function f2(bool flag) internal view returns (S storage c) {
        if (flag) c = s;
        else { c = s; }
    }
    function g(bool flag) internal view returns (S storage c) {
        if (flag) c = s;
    }
    function h(bool flag) internal view returns (S storage c) {
        if (flag) c = s;
        else
        {
            if (!flag) c = s;
            else c = s;
        }
    }
    function i(bool flag) internal returns (S storage c) {
        if (flag) c = s;
        else
        {
            if (!flag) c = s;
            else s.f = true;
        }
    }
    function j() internal view returns (S storage c) {
        if ((c = s).f) {
        }
    }
    function k() internal view returns (S storage c) {
        if ((c = s).f && !(c = s).f) {
        }
    }
}
// ----
// Warning: (325-336): uninitialized storage pointer may be returned
// Warning: (597-608): uninitialized storage pointer may be returned
