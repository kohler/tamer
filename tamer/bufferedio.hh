#ifndef TAMER_BUFFEREDIO_HH
#define TAMER_BUFFEREDIO_HH
#include <string>
#include <tamer/tamer.hh>
#include <tamer/fd.hh>
namespace tamer {

class buffer { public:

    buffer(size_t = 1024);
    ~buffer();

    void fill_until(fd &f, char c, size_t max_size, size_t &out_size, event<int> done);
    void take_until(fd &f, char c, size_t max_size, std::string &str, event<int> done);
    
  private:

    char *_buf;
    size_t _size;
    size_t _head;
    size_t _tail;
    size_t _until_pos;

    ssize_t fill_more(fd &f, const event<int> &done, const event<> &closewatch);

    class closure__fill_until__R2fdckRkQ;
    void fill_until(closure__fill_until__R2fdckRkQ &, unsigned);
    class closure__take_until__R2fdckRSsQ;
    void take_until(closure__take_until__R2fdckRSsQ &, unsigned);

};

}
#endif /* TAMER_BUFFEREDIO_HH */
