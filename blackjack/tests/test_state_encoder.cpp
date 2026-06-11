#include "doctest.h"
#include "StateEncoder.h"
#include "Types.h"
#include <stdexcept>
#include <string>
#include <unordered_set>

static GameStateSnapshot makeSnap(int total, bool soft, int upcard, int count = 0) {
    GameStateSnapshot s;
    s.playerTotal       = total;
    s.isSoftHand        = soft;
    s.dealerUpcardValue = upcard;
    s.runningCount      = count;
    return s;
}

// ── encode() format ───────────────────────────────────────────────────────────

TEST_CASE("StateEncoder: encode() contains exactly 3 pipe characters") {
    StateEncoder enc;
    std::string key = enc.encode(makeSnap(16, true, 7, 2));
    int pipes = 0;
    for (char c : key) { if (c == '|') { ++pipes; } }
    CHECK(pipes == 3);
}

TEST_CASE("StateEncoder: encode() known values") {
    StateEncoder enc;
    CHECK(enc.encode(makeSnap(16, true,  7,  2)) == "16|1|7|+1");
    CHECK(enc.encode(makeSnap(16, false, 7,  4)) == "16|0|7|+2");
    CHECK(enc.encode(makeSnap( 4, false, 2, -5)) == "4|0|2|-2");
    CHECK(enc.encode(makeSnap(21, true, 11, 10)) == "21|1|11|+2");
    CHECK(enc.encode(makeSnap(20, false,11,  0)) == "20|0|11|0");
}

// ── encodeSimple() format ─────────────────────────────────────────────────────

TEST_CASE("StateEncoder: encodeSimple() contains exactly 2 pipe characters") {
    StateEncoder enc;
    std::string key = enc.encodeSimple(makeSnap(16, true, 7));
    int pipes = 0;
    for (char c : key) { if (c == '|') { ++pipes; } }
    CHECK(pipes == 2);
}

TEST_CASE("StateEncoder: encodeSimple() known values") {
    StateEncoder enc;
    CHECK(enc.encodeSimple(makeSnap(16, true,  7)) == "16|1|7");
    CHECK(enc.encodeSimple(makeSnap( 4, false, 2)) == "4|0|2");
    CHECK(enc.encodeSimple(makeSnap(21, true, 11)) == "21|1|11");
}

// ── countBucket() ─────────────────────────────────────────────────────────────

TEST_CASE("StateEncoder: countBucket() boundary values") {
    StateEncoder enc;
    CHECK(enc.countBucket(-10) == "-2");
    CHECK(enc.countBucket(-4)  == "-2");
    CHECK(enc.countBucket(-3)  == "-1");
    CHECK(enc.countBucket(-1)  == "-1");
    CHECK(enc.countBucket( 0)  == "0");
    CHECK(enc.countBucket( 1)  == "+1");
    CHECK(enc.countBucket( 3)  == "+1");
    CHECK(enc.countBucket( 4)  == "+2");
    CHECK(enc.countBucket(10)  == "+2");
}

// ── validateState() error cases ───────────────────────────────────────────────

TEST_CASE("StateEncoder: throws on playerTotal out of range") {
    StateEncoder enc;
    CHECK_THROWS_AS(enc.encode(makeSnap(22, false, 7)), std::logic_error);
    CHECK_THROWS_AS(enc.encode(makeSnap( 3, false, 7)), std::logic_error);
    CHECK_THROWS_AS(enc.encode(makeSnap( 0, false, 7)), std::logic_error);
}

TEST_CASE("StateEncoder: throws on dealerUpcardValue out of range") {
    StateEncoder enc;
    CHECK_THROWS_AS(enc.encode(makeSnap(16, false,  1)), std::logic_error);
    CHECK_THROWS_AS(enc.encode(makeSnap(16, false, 12)), std::logic_error);
    CHECK_THROWS_AS(enc.encode(makeSnap(16, false,  0)), std::logic_error);
}

TEST_CASE("StateEncoder: error message includes bad value") {
    StateEncoder enc;
    try {
        enc.encode(makeSnap(22, false, 7));
        FAIL("expected throw");
    } catch (const std::logic_error& e) {
        std::string msg = e.what();
        CHECK(msg.find("22") != std::string::npos);
    }
}

// ── collision test — all 1800 combinations ───────────────────────────────────

TEST_CASE("StateEncoder: no two different states produce the same key") {
    StateEncoder enc;
    std::unordered_set<std::string> seen;
    seen.reserve(1800);

    // counts that map to each of the 5 buckets: -5, -2, 0, 2, 5
    const int counts[] = {-5, -2, 0, 2, 5};

    for (int total = 4; total <= 21; ++total) {
        for (int soft = 0; soft <= 1; ++soft) {
            for (int upcard = 2; upcard <= 11; ++upcard) {
                for (int cnt : counts) {
                    GameStateSnapshot s = makeSnap(total, soft != 0, upcard, cnt);
                    std::string key = enc.encode(s);
                    CHECK_FALSE(seen.count(key));
                    seen.insert(key);
                }
            }
        }
    }
    CHECK(static_cast<int>(seen.size()) == 1800);
}

// ── stateSpaceSize() ─────────────────────────────────────────────────────────

TEST_CASE("StateEncoder: stateSpaceSize() returns 1800") {
    CHECK(StateEncoder::stateSpaceSize() == 1800);
}
