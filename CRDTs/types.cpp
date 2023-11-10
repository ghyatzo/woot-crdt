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
#include <limits>

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

std::optional<WChar> WString::insert(WChar c, size_t pos)
{
    assert(pos < content_.size());
    content_.insert(std::next(content_.begin(), pos), c);
    if (c.visible) ++num_visible_;
    return c;
}

std::vector<WChar> WString::subseq(WChar c, WChar d)
{
    // we are gonna assume that c and d actually are in the string... we're playing.
    auto vb = std::next(pos_it(c.id), 1);
    auto ve = pos_it(d.id); // we exclude the first one as well.
    
//    if (vb == ve) return {};
    return {vb, ve};
}

std::optional<WChar> WString::ith_visible(size_t i)
{
//    assert(i < content_.size() && i <= num_visible_);
    auto it = content_.begin();
    for (size_t j {}; it < content_.end(); ++it) {
        if (it->visible) {
            if (++j == i) return *it;
        }
    }

    return {};
}

std::string WString::value() {
    std::stringstream ss {};
    for (const auto c : content_ | std::views::filter([](WChar c){ return c.visible; })) {
        ss << c;
    }
    return ss.str();
}

//bool WOOTBuffer::is_exacutable(Op op)
//{
//    if (op.type == DEL) return buffer_.contains(std::get<DelOp>(op.op).c.id);
//    if (op.type == INS) {
//        auto insop = std::get<InsOp>(op.op);
//        return buffer_.contains(insop.cn.id) && buffer_.contains(insop.cp.id);
//    }
//    return false;
//} DEPRECATED

std::optional<WChar> WOOTBuffer::integrate_ins(WChar c, WChar cp, WChar cn)
{
    auto Sp = buffer_.subseq(cp, cn);
    if (Sp.empty()) {
        auto pos = buffer_.pos(cn.id);
        if (pos.has_value()) return buffer_.insert(c, pos.value());
        else return {};
    } else {
        if (not buffer_.contains(cp.id) || not buffer_.contains(cn.id)) return {};
        auto L_filter = [&](WChar d){
            return buffer_.lte(d.prev, cp.id) && buffer_.lte(cn.id, d.next);
        };
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
        return integrate_ins(c, L[i-1], L[i]);
    }
}

std::optional<Op> WOOTBuffer::ins(size_t pos, char a)
{
    if (pos > buffer_.visibles()) return std::nullopt;
    
    ++clock_;
    auto cpopt = buffer_.ith_visible(pos);
    auto cnopt = buffer_.ith_visible(pos + 1);
    
    auto cp = cpopt.has_value() ? cpopt.value() : WBegin();
    auto cn = cnopt.has_value() ? cnopt.value() : WEnd();
    auto c = WChar{ site_char_id(), a, true, cp.id, cn.id };
    integrate_ins(c, cp, cn);
    
    std::variant<DelOp, InsOp> op = InsOp{c, cp, cn};
    
    return Op{op, OpType::INS};
}

std::optional<Op> WOOTBuffer::del(size_t pos)
{
    if (pos >= buffer_.visibles()) return std::nullopt;
    
    auto c = buffer_.ith_visible(pos+1);
    assert(c.has_value());
    integrate_del(c.value());
    
    std::variant<DelOp, InsOp> op = DelOp{c.value()};
    return Op{op, OpType::DEL};
}

void WOOTBuffer::try_apply()
{
    // this could also be a worker that loops over, or polls
    // for new operations and goes on until pool_ is empty.
    
    // this is ugly as fuck, probably smarted to use a deque or something.
    // erase-remove breaks if we loop through it while doing it (duh)
    std::vector<Op> remove_these {};
    for (auto& op : pool_) {
        if (op.type == DEL) {
            auto op_cont = std::get<DelOp>(op.op);
            if (integrate_del(op_cont.c).has_value())
                remove_these.push_back(op);
        } else {
            auto op_cont = std::get<InsOp>(op.op);
            if (integrate_ins(op_cont.c, op_cont.cp, op_cont.cn).has_value())
                remove_these.push_back(op);
        }
    }
    for (const auto& op: remove_these) {
        pool_.erase(std::remove(pool_.begin(), pool_.end(), op));
    }
}
