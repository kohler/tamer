#include <tamer/util.hh>
#include <stdio.h>
#include <string>
using namespace tamer::tamerutil;

void test_debuffer1() {
    debuffer<std::string> db;
    assert(db.front_ptr() == 0 && db.size() == 0);
    db.push_back("A");
    db.push_back("B");
    assert(*db.front_ptr() == "A" && db.size() == 2);
    db.push_front("X");
    assert(*db.front_ptr() == "X" && db.size() == 3);
    db.pop_front();
    assert(*db.front_ptr() == "A" && db.size() == 2);
    db.clear();
    assert(db.front_ptr() == 0 && db.size() == 0);
    db.push_back("A");
    db.push_back("B");
    assert(*db.front_ptr() == "A" && db.size() == 2);
    db.push_front("X");
    assert(*db.front_ptr() == "X" && db.size() == 3);
    db.push_front("W");
    assert(*db.front_ptr() == "W" && db.size() == 4);
    db.pop_front();
    assert(*db.front_ptr() == "X" && db.size() == 3);
    db.pop_front();
    assert(*db.front_ptr() == "A" && db.size() == 2);
    db.pop_front();
    assert(*db.front_ptr() == "B" && db.size() == 1);
    db.pop_front();
    assert(db.front_ptr() == 0 && db.size() == 0);
    db.push_back("A");
    db.push_back("B");
    assert(db.front_ptr() && (void *) db.front_ptr() >= (void *) &db && (void *) db.front_ptr() <= (void *) (&db + 1));
}

void test_debuffer2() {
    debuffer<std::string> db;
    assert(db.front_ptr() == 0 && db.size() == 0);
    db.push_back("A");
    db.push_back("B");
    assert(*db.front_ptr() == "A" && db.size() == 2);
    db.push_front("X");
    assert(*db.front_ptr() == "X" && db.size() == 3);
    db.pop_front();
    assert(*db.front_ptr() == "A" && db.size() == 2);
    db.clear();
    assert(db.front_ptr() == 0 && db.size() == 0);
    db.push_back("A");
    db.push_back("B");
    assert(*db.front_ptr() == "A" && db.size() == 2);
    db.push_front("X");
    assert(*db.front_ptr() == "X" && db.size() == 3);
    for (size_t i = 3; i < 9; i++)
	db.push_front("W");
    for (size_t i = 9; i > 3; --i) {
	assert(*db.front_ptr() == "W" && db.size() == i);
	db.pop_front();
    }
    assert(*db.front_ptr() == "X" && db.size() == 3);
    db.pop_front();
    assert(*db.front_ptr() == "A" && db.size() == 2);
    db.pop_front();
    assert(*db.front_ptr() == "B" && db.size() == 1);
    db.pop_front();
    assert(db.front_ptr() == 0 && db.size() == 0);
    db.push_back("A");
    db.push_back("B");
    assert(db.front_ptr() && !((void *) db.front_ptr() >= (void *) &db && (void *) db.front_ptr() <= (void *) (&db + 1)));
}

void test_ready_set1() {
    ready_set<std::string> rs;
    assert(rs.front_ptr() == 0 && rs.size() == 0 && rs.multiset_size() == 0);
    size_t i1 = rs.insert("ABCDE", 2);
    size_t i2 = rs.insert("ABCDE", 4);
    size_t i3 = rs.insert("ABCDE");
    assert(rs.front_ptr() == 0 && rs.size() == 0 && rs.multiset_size() == 3);
    size_t i4 = rs.insert("ABCDE");
    rs.erase(i3);
    assert(rs.front_ptr() == 0 && rs.size() == 0 && rs.multiset_size() == 3);
    rs.push_back_element(i2);
    assert(*rs.front_ptr() == "ABCD" && rs.size() == 1 && rs.multiset_size() == 2);
    size_t i5 = rs.insert("ABCDEF");
    rs.push_back_element(i5);
    rs.push_back_element(i1);
    rs.push_back_element(i4);
    assert(*rs.front_ptr() == "ABCD" && rs.size() == 4 && rs.multiset_size() == 0);
    rs.pop_front();
    assert(*rs.front_ptr() == "ABCDEF" && rs.size() == 3 && rs.multiset_size() == 0);
    rs.pop_front();
    assert(*rs.front_ptr() == "AB" && rs.size() == 2 && rs.multiset_size() == 0);
    rs.pop_front();
    assert(*rs.front_ptr() == "ABCDE" && rs.size() == 1 && rs.multiset_size() == 0);
    rs.pop_front();
    assert(rs.front_ptr() == 0 && rs.size() == 0 && rs.multiset_size() == 0);
}

void test_ready_set2() {
    ready_set<std::string> rs;
    assert(rs.front_ptr() == 0 && rs.size() == 0 && rs.multiset_size() == 0);
    size_t i1 = rs.insert("ABCDE", 2);
    size_t i2 = rs.insert("ABCDE", 4);
    size_t i3 = rs.insert("ABCDE");
    assert(rs.front_ptr() == 0 && rs.size() == 0 && rs.multiset_size() == 3);
    size_t i4 = rs.insert("ABCDE");
    rs.erase(i3);
    assert(rs.front_ptr() == 0 && rs.size() == 0 && rs.multiset_size() == 3);
    rs.push_back_element(i2);
    assert(*rs.front_ptr() == "ABCD" && rs.size() == 1 && rs.multiset_size() == 2);
    size_t i5 = rs.insert("ABCDEF");

    size_t x[6];
    for (size_t i = 4; i <= 9; i++) {
	x[i - 4] = rs.insert("X" + std::string(1, i + '0'));
	assert(rs.size() == 1 && rs.multiset_size() == i);
    }
    for (size_t i = 9; i >= 4; i--)
	rs.erase(x[i - 4]);
    
    rs.push_back_element(i5);
    rs.push_back_element(i1);
    rs.push_back_element(i4);
    assert(*rs.front_ptr() == "ABCD" && rs.size() == 4 && rs.multiset_size() == 0);
    rs.pop_front();
    assert(*rs.front_ptr() == "ABCDEF" && rs.size() == 3 && rs.multiset_size() == 0);
    rs.pop_front();
    assert(*rs.front_ptr() == "AB" && rs.size() == 2 && rs.multiset_size() == 0);
    rs.pop_front();
    assert(*rs.front_ptr() == "ABCDE" && rs.size() == 1 && rs.multiset_size() == 0);
    rs.pop_front();
    assert(rs.front_ptr() == 0 && rs.size() == 0 && rs.multiset_size() == 0);
}

int main(int argc, char *argv[]) {
    (void) argc, (void) argv;
    test_debuffer1();
    test_debuffer2();
    test_ready_set1();
    test_ready_set2();
}
