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

int main(int argc, char *argv[]) {
    (void) argc, (void) argv;
    test_debuffer1();
    test_debuffer2();
}
