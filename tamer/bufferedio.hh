#ifndef TAMER_BUFFEREDIO_HH
#define TAMER_BUFFEREDIO_HH
#include <string>
#include <tamer/tamer.hh>
namespace tamer {
namespace fileio {

class buffer { public:

    buffer(size_t = 1024);
    ~buffer();

    void fill_until(int fd, char c, size_t max_size, size_t &out_size, event<int> done);
    void take_until(int fd, char c, size_t max_size, std::string &str, event<int> done);
    
  private:

    char *_buf;
    size_t _size;
    size_t _head;
    size_t _tail;
    size_t _until_pos;

    ssize_t fill_more(int fd, const event<int> &done, const event<> &closewatch);

    class fill_untilickRkQ__closure;
    void fill_until(fill_untilickRkQ__closure &, unsigned);
    class take_untilickRN3std6stringEQ__closure;
    void take_until(take_untilickRN3std6stringEQ__closure &, unsigned);

};

}}
#endif /* TAMER_BUFFEREDIO_HH */
