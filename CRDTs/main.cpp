//
//  main.cpp
//  CRDTs
//
//  Created by Camillo Schenone on 24/10/2023.
//

#include <iostream>
#include "types.hpp"


int main(int argc, const char * argv[]) {
    
    

    std::cout << "(del, ins) commuting: " << std::endl;
    // (del, ins) commutation
    /*        site1                       site2
            ["abc"]                     ["abc"]
                |                           |
        op1 = insert(b < 1 < c)         op2 = del(c)
            ["ab1c"]                       ["ab(c)"]  // the (c) represents the tombstone of that char.
                |                           |
                |                           |
            apply(op2)                    apply(op1)
            ["ab1"]                         ["ab1(c)"]
     */
    WOOTBuffer site1 {1, "abc"};
    WOOTBuffer site2 {2, "abc"};
    std::string site1_timeline {site1.value()};
    std::string site2_timeline {site2.value()};
    auto op1 = site1.generate_ins(2, '1');
    auto op2 = site2.generate_del(3);
    site1_timeline += "\t\t"+site1.value();
    site2_timeline += "\t\t"+site2.value();
    site1.add_op(op2);
    site2.add_op(op1);
    site1.try_apply();
    site2.try_apply();
    site1_timeline += "\t\t"+site1.value();
    site2_timeline += "\t\t"+site2.value();
    
    std::cout << site1_timeline << std::endl; // abc    ab1c    ab1
    std::cout << site2_timeline << std::endl; // abc    ab      ab1

    
    std::cout << "(del, del) commuting: " << std::endl;
    // (del, del) commutation
    /*        
     site3                       site4
     ["abc"]                     ["abc"]
        |                           |
     op3 = del(b)               op4 = del(c)
     ["a(b)c"]                    ["ab(c)"]  // the (c) represents the tombstone of that char.
        |                           |
        |                           |
     apply(op4)                  apply(op3)
     ["a(b)(c)"]                 ["a(b)(c)"]
     */
    WOOTBuffer site3 {3, "abc"};
    WOOTBuffer site4 {4, "abc"};
    std::string site3_timeline {site3.value()};
    std::string site4_timeline {site4.value()};
    auto op3 = site3.generate_del(2);
    auto op4 = site4.generate_del(3);
    site3_timeline += "\t\t"+site3.value();
    site4_timeline += "\t\t"+site4.value();
    site3.add_op(op4);
    site4.add_op(op3);
    site3.try_apply();
    site4.try_apply();
    site3_timeline += "\t\t"+site3.value();
    site4_timeline += "\t\t"+site4.value();
    
    std::cout << site3_timeline << std::endl; // abc    ac    a
    std::cout << site4_timeline << std::endl; // abc    ab    a
    
    
    std::cout << "(ins, ins) commuting: " << std::endl;
    // (ins, ins) commutation
    /*
     site5                       site6                    site7
     ["ab"]                      ["ab"]                   ["ab"]
        |                           |                       |
     op5 = ins(a<1<b)            op6 = ins(a<2<b)           |
     ["a1b"]                     ["a2b"]                    |
        |                           |                       |
     op7 = ins(a<3<1)               |                     apply(op5)
     ["a31b"]                       |                     ["a1b"]
        |                      apply(op7)                   |
        |               ['1' is missing, wait op5]        apply(op6)
     apply(op6)                     |                     ["a12b"]
     ["a312b"]                 apply(op5)                   |
        |                       ["a12b"]                  apply(op7)
        |                           |                     ["a312b"]
        |                       reapply(op7)                |
        |                       ["a312b"]                   |
     */
    WOOTBuffer site5 {5, "ab"};
    WOOTBuffer site6 {6, "ab"};
    WOOTBuffer site7 {7, "ab"};
    std::string site5_timeline {site5.value()};
    std::string site6_timeline {site6.value()};
    std::string site7_timeline {site7.value()};
    
    auto op5 = site5.generate_ins(1, '1');
    site5_timeline += "\t\t"+site5.value();
    auto op7 = site5.generate_ins(1, '3');
    site5_timeline += "\t\t"+site5.value();

    auto op6 = site6.generate_ins(1, '2');
    site6_timeline += "\t\t"+site6.value();
    
    site7_timeline += "\t\t"+site7.value();
    
    site5.add_op(op6);
    
    site6.add_op(op7);
    site6.add_op(op5);
    
    site7.add_op(op5);
    site7.add_op(op6);
    site7.add_op(op7);
    
    site5.try_apply(); // 1 operation to apply
    site5_timeline += "\t\t"+site5.value();

    site6.try_apply(); // 2 operations to apply, only one is executable immediately
    site6_timeline += "\t\t"+site6.value();
    site6.try_apply(); // we try to apply the one we couldn't before.
    site6_timeline += "\t\t"+site6.value();

    site7.try_apply(); // 3 operations applied immediately
    site7_timeline += "\t\t"+site7.value();

    std::cout << site5_timeline << std::endl; // ab     a1b     a31b    a312b
    std::cout << site6_timeline << std::endl; // ab     a2b     a12b    a312b
    std::cout << site7_timeline << std::endl; // ab     ab      a312b   (all at once)
    
    return 0;
}
