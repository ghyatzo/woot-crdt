//
//  types.cpp
//  CRDTs
//
//  Created by Camillo Schenone on 24/10/2023.
//

#include <sstream>
#include <string>
#include <variant>
#include <vector>
#include <ranges>
#include <algorithm>

#include "types.hpp"

std::ostream& operator<<(std::ostream& os, const WChar c) {
    if (c.visible) {
        os << c.value;
    } else {
        os << "(" << c.value << ")";
    }
    
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const std::vector<WChar> chars) {
    for (const auto c : chars) {
        os << c;
    }
    return os;
}

WString::WString()
: num_visible_(0), content_()
{
    
    WBegin cb {};
    WEnd ce {};
    // add beginning special character
    content_.push_back(cb);
    // add end special character
    content_.push_back(ce);
}

void WString::insert(WChar c, size_t pos)
{
    assert(pos < content_.size());
    content_.insert(std::next(content_.begin(), pos), c);
    if (c.visible) ++num_visible_;
}

std::vector<WChar> WString::subseq(WChar c, WChar d)
{
    // we are gonna assume that c and d actually are in the string... we're playing.
    auto vb = std::next(pos_it(c.id), 1);
    auto ve = pos_it(d.id); // we exclude the first one as well.
    
//    if (vb == ve) return {};
    return {vb, ve};
}

WChar& WString::ith_visible(size_t i)
{
    assert(i < content_.size() && i <= num_visible_);
    auto it = content_.begin();
    for (size_t j {}; it < content_.end(); ++it) {
        if (it->visible) {
            if (++j == i) return *it;
        }
    }
    throw std::runtime_error("no visible characters");
}

std::string WString::value() {
    std::stringstream ss {};
    for (const auto c : content_ | std::views::filter([](WChar c){ return c.visible; })) {
        ss << c;
    }
    return ss.str();
}

bool WOOTBuffer::is_exacutable(Op op)
{
    if (op.type == DEL) return buffer_.contains(std::get<DelOp>(op.op).c.id);
    if (op.type == INS) {
        auto insop = std::get<InsOp>(op.op);
        return buffer_.contains(insop.cn.id) && buffer_.contains(insop.cp.id);
    }
    return false;
}

void WOOTBuffer::integrate_ins(WChar c, WChar cp, WChar cn)
{
    auto Sp = buffer_.subseq(cp, cn);
    if (Sp.empty()) buffer_.insert(c, buffer_.pos(cn.id));
    else {

        auto L_filter = [&](WChar d){ return buffer_.lte(d.prev, cp.id) && buffer_.lte(cn.id, d.next); };
        std::vector<WChar> L;
        std::copy_if(Sp.begin(), Sp.end(), std::back_inserter(L), L_filter);
        
        L.insert(L.begin(), cp);
        L.insert(L.end(), cn);
        
        assert(L.size() >= 3);
        int i = 1;
        auto li = std::next(L.begin(), 1);
        while( (i < L.size()-1) && (li->id < c.id) ) {
            ++i; ++li;
        }
        integrate_ins(c, L[i-1], L[i]);
    }
}
void WOOTBuffer::integrate_del(WChar c)
{
    buffer_.del(c);
}
Op WOOTBuffer::generate_ins(size_t pos, char a)
{
    ++clock_;
    auto cp = buffer_.ith_visible(pos);
    auto cn = buffer_.ith_visible(pos + 1);
    auto c = WChar{ site_char_id(), a, true, cp.id, cn.id };
    integrate_ins(c, cp, cn);
    
    std::variant<DelOp, InsOp> op = InsOp{c, cp, cn};
    
    return {op, OpType::INS};
}

Op WOOTBuffer::generate_del(size_t pos)
{
    auto c = buffer_.ith_visible(pos);
    integrate_del(c);
    
    std::variant<DelOp, InsOp> op = DelOp{c};
    return {op, OpType::DEL};
}

void WOOTBuffer::try_apply()
{
    // this could also be a worker that loops over, or polls
    // for new operations and goes on until pool_ is empty.
    
    // this is ugly as fuck, probably smarted to use a deque or something.
    std::vector<Op> remove_these {};
    for (auto& op : pool_) {
        if (!is_exacutable(op)) continue;
        remove_these.push_back(op);
        
        if (op.type == DEL) {
            auto op_cont = std::get<DelOp>(op.op);
            integrate_del(op_cont.c);
        } else {
            auto op_cont = std::get<InsOp>(op.op);
            integrate_ins(op_cont.c, op_cont.cp, op_cont.cn);
        }
    }
    for (const auto& op: remove_these) {
        pool_.erase(std::remove(pool_.begin(), pool_.end(), op));
    }
}
