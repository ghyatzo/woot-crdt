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

struct WBegin : WChar {};
struct WEnd : WChar {};

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
//    inline bool lt(WChar a, WChar b) { return pos(a) < pos(b); }   // <_S
//    inline bool lte(WChar a, WChar b) { return pos(a) <= pos(b); } // <=_S
//
    inline bool lt(CharID& a, CharID& b)  { return pos(a) < pos(b); }   // <_S
    inline bool lte(CharID& a, CharID& b) { return pos(a) <= pos(b); } // <=_S
    
    inline size_t   size()              { return content_.size(); }
    inline auto     end()               { return content_.end(); }
    inline auto     pos_it(CharID c)    {
        for (auto it = content_.begin(); it <content_.end(); ++it) {
            if (it->id == c) return it;
        }
        return content_.end();
    }
    inline bool     contains(CharID c)  { return pos_it(c) != std::end(content_); }
    inline size_t   pos(CharID c)       { return pos_it(c) - content_.begin(); }
    inline void     del(WChar c)        { pos_it(c.id)->visible = false; --num_visible_; }
    
    void            insert(WChar c, size_t pos);
    std::vector<WChar> subseq(WChar c, WChar d);
    WChar&          ith_visible(size_t i);
    
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
            WChar wc {CharID{0, ++clock_}, c, true, std::prev(buffer_.end(),2)->id , buffer_.end()->id};
                buffer_.insert(wc, pos++);
            }
    }
    
    inline CharID site_char_id() { return {id_, clock_}; }
    inline void add_op(Op op) { pool_.push_back(op); }
    
    bool is_exacutable(Op);
    void integrate_ins(WChar c, WChar cp, WChar cn);
    void integrate_del(WChar c);
    Op generate_ins(size_t pos, char a);
    Op generate_del(size_t pos);
    
    void try_apply();
    inline std::string value() { return buffer_.value(); }
    
private:
    size_t id_;
    size_t clock_;
    std::vector<Op> pool_;
    WString buffer_;
};



#endif /* types_hpp */
