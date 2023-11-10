//
//  types.hpp
//  CRDTs
//
//  Created by Camillo Schenone on 24/10/2023.
//

#ifndef types_hpp
#define types_hpp

#include <stdio.h>
#include <vector>
#include <algorithm>
#include <cassert>
#include <variant>
#include <string>
#include <utility>
#include <deque>
using namespace std::rel_ops;

enum OpType {
    DEL,
    INS
};

struct CharID {
    size_t site_id;
    size_t clock;
    
    bool operator<(const CharID& b) const {return (site_id < b.site_id) || (site_id == b.site_id && clock < b.clock); }
    bool operator==(const CharID& two) const  { return (site_id == two.site_id) && (clock == two.clock); }
};

struct WChar {
    CharID id;
    char value;
    bool visible;
    CharID prev;
    CharID next;
    
    bool operator<(const CharID &c) const {return  id < c;}
    bool operator<(const WChar  &c) const {return  id < c.id;}
    bool operator==(const CharID &c) const {return id == c;}
    bool operator==(const WChar  &c) const {return id == c.id;}
};

struct DelOp {
    WChar c;
    inline bool operator==(const DelOp& other) const {return c == other.c;}
};
struct InsOp {
    WChar c; WChar cp; WChar cn;
    inline bool operator==(const InsOp& other) const {return c == other.c && cp == other.cp && cn == other.cn;}
};
struct Op {
    std::variant<DelOp, InsOp> op;
    OpType type;
    inline bool operator==(const Op& other) const {return type == other.type && op == other.op;}
};

struct WBegin : WChar {
    WBegin() : WChar {
        CharID{std::numeric_limits<size_t>::min(), std::numeric_limits<size_t>::min()},
        '\0',
        false,
        CharID{},
        CharID{}
    } {}
};
struct WEnd : WChar {
    WEnd() : WChar {
        CharID{std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max()},
        '\0',
        false,
        CharID{},
        CharID{}
    } {}
};



// define the orders here!
inline bool operator<(const WBegin a, const CharID b)      {return true;}
inline bool operator<(const WBegin a, const WChar b)       {return true;}
inline bool operator<(const CharID a, const WBegin b)      {return false;}
inline bool operator<(const WChar a, const WBegin b)       {return false;}

inline bool operator==(const CharID a, const WBegin b)     {return false;}
inline bool operator==(const WChar a, const WBegin b)      {return false;}
inline bool operator==(const CharID a, const WEnd b)       {return false;}
inline bool operator==(const WChar a, const WEnd b)        {return false;}

inline bool operator<(const WEnd a, const CharID b)        { return false;}
inline bool operator<(const WEnd a, const WChar b)         { return false;}
inline bool operator<(const CharID a, const WEnd b)        { return true;}
inline bool operator<(const WChar a, const WEnd b)         { return true;}


class WString {
public:
    WString();
    inline WChar& operator[](int i) { return content_[i]; }

//    inline bool lt(CharID& a, CharID& b)  { return pos(a) < pos(b); }   // <_S
    inline bool lte(CharID& a, CharID& b) {
        return pos(a).value() <= pos(b).value();
    } // <=_S
    
    inline size_t   size()              { return content_.size(); }
    inline size_t   visibles()          { return num_visible_; }
    inline auto     end()               { return content_.end(); }
    inline auto     pos_it(CharID c)    {
        for (auto it = content_.begin(); it < content_.end(); ++it) {
            if (it->id == c) return it;
        }
        return content_.end();
    }
    inline bool contains(CharID c) { return pos(c).has_value(); }
    inline std::optional<size_t> pos(CharID c) {
        auto wc = pos_it(c);
        if (wc == content_.end()) return {};
        else return wc - content_.begin();
    }
    
    inline std::optional<WChar> del(WChar c) {
        auto wc = pos_it(c.id);
        if (wc == content_.end()) return {};
        
        if (wc->visible) {
            wc->visible = false;
            --num_visible_;
        }
        return *wc;
    }
    
    std::optional<WChar> insert(WChar c, size_t pos);
    std::vector<WChar> subseq(WChar c, WChar d);
    std::optional<WChar> ith_visible(size_t i);
    
    std::string     value();
private:
    size_t num_visible_;
    std::vector<WChar> content_;
};

class WOOTBuffer {
public:
    WOOTBuffer(const size_t id) 
    : id_{id}, clock_{}, pool_{}, buffer_{} { };
    WOOTBuffer(const size_t id, const std::string &str)
    : id_{id}, clock_{}, pool_{}, buffer_{}
    {
        // It is important when we inizialise buffers with some string, that the CharID
        // is the same across all buffers, as if the initialisation is receiving a stream of
        // characters from the same site.
        // I elected to just go with a id of 0.
        size_t pos {1};
        for (const auto c : str) {
            WChar wc {CharID{0, ++clock_}, c, true, std::prev(buffer_.end(),2)->id , std::prev(buffer_.end(),1)->id};
                buffer_.insert(wc, pos++);
            }
    }
    
    inline CharID site_char_id() { return {id_, clock_}; }

    inline void merge(Op op) {
        pool_.push_front(op);
        try_apply();
    }
    inline void merge(std::optional<Op> optop) {
        if (optop.has_value()) pool_.push_front(optop.value());
        try_apply();
    }
    
    //bool is_exacutable(Op); // deprecated
    std::optional<WChar> integrate_ins(WChar c, WChar cp, WChar cn);
    inline std::optional<WChar> integrate_del(WChar c) {
        return buffer_.del(c);
    }
    std::optional<Op> ins(size_t pos, char a);
    std::optional<Op> del(size_t pos);
    
    void try_apply();
    inline std::string value() { return buffer_.value(); }
    
private:
    size_t id_;
    size_t clock_;
    std::deque<Op> pool_;
    WString buffer_;
};



#endif /* types_hpp */
