//
//  main.cpp
//  CRDTs
//
//  Created by Camillo Schenone on 24/10/2023.
//

#include <iostream>
#include "types.hpp"

bool insert_char_from_empty_string() {
    std::cout << "insert char from empty string" << std::endl;
    WOOTBuffer site0 {99, ""};
    [[maybe_unused]] auto op0 = site0.ins(0, 'a');
    std::cout << site0.value() << std::endl;
    return site0.value() == "a";
}

bool insert_out_of_bounds_fails() {
    std::cout << "insert out of bounds fails" << std::endl;
    WOOTBuffer site0 {99, "a"};
    auto op0 = site0.ins(2, 'a');
    std::cout << site0.value() << std::endl;
    return !op0.has_value();
}

bool delete_out_of_bounds_fails() {
    std::cout << "delete out of bounds fails" << std::endl;
    WOOTBuffer site0 {99, "a"};
    auto op0 = site0.del(2);
    std::cout << site0.value() << std::endl;
    return !op0.has_value();
}

bool delete_char_in_string() {
    std::cout << "delete char in string" << std::endl;
    WOOTBuffer site0 {99, "a"};
    [[maybe_unused]] auto op0 = site0.del(0);
    std::cout << site0.value() << std::endl;
    return site0.value() == "";
}
bool insert_after_deleting() {
    std::cout << "insert after deleting" << std::endl;
    WOOTBuffer site0 {99, "ab"};
    [[maybe_unused]] auto op0 = site0.del(0);
    [[maybe_unused]] auto op1 = site0.ins(0, 'c');
    std::cout << site0.value() << std::endl;
    return site0.value() == "cb";
}
bool integrating_remote_work() {
    std::cout << "integrating remote work" << std::endl;
    WOOTBuffer site0 {99, "a"};
    WOOTBuffer site1 {88, "a"};
    auto op0 = site0.ins(1, 'b');
    site1.merge(op0);
    std::cout << site0.value() << " " << site1.value() << std::endl;
    return site0.value() == site1.value();
}
bool inserting_in_buffer_with_all_invisible_chars() {
    std::cout << "insertion in buffer with all deleted chars" << std::endl;
    WOOTBuffer site0 {99, "a"};
    [[maybe_unused]] auto op0 = site0.del(0);
    [[maybe_unused]] auto op1 = site0.ins(0, 'b');
    std::cout << site0.value() << std::endl;
    return site0.value() == "b";
}

bool multiple_insertions_converge() {
    std::cout << "multiple_insertion_converge" << std::endl;
    WOOTBuffer site0 {99, ""};
    WOOTBuffer site1 {88, ""};
    auto op0 = site0.ins(0, 'a');
    site1.merge(op0);
    auto op1 = site0.ins(1, 'b');
    auto op2 = site1.ins(1, 'c');
    site0.merge(op2);
    site1.merge(op1);
    
    std::cout << site0.value() << " " << site1.value() << std::endl;
    return site0.value() == "acb" && site0.value() == site1.value();
}

bool deleting_while_inserting_converges() {
    std::cout << "deleting while inserting converges" << std::endl;
    WOOTBuffer site1 {99, ""};
    WOOTBuffer site2 {88, ""};
    auto op1 = site1.ins(0, 'a');
    site2.merge(op1);
    auto op2 = site1.ins(1, 'b');
    site2.merge(op2);
    auto op3 = site1.ins(2, 'c');
    
    auto del = site2.del(1);
    site2.merge(op3);
    site1.merge(del);
    
    std::cout << site1.value() << " " << site2.value() << std::endl;
    return site1.value() == "ac" && site1.value() == site2.value();
}

bool receiving_many_operations_brings_in_sync() {
    std::cout << "deleting while inserting converges" << std::endl;
    WOOTBuffer site1 {99, ""};
    WOOTBuffer site2 {88, ""};
    auto ops = std::vector<std::optional<Op>> {
        site1.ins(0, 'a'),
        site1.ins(1, 'b'),
        site1.ins(2, 'c'),
        site1.ins(3, ' '),
        site1.ins(4, 'd'),
        site1.del(3),
        site1.ins(3, 'c'),
        site1.ins(0, ' '),
    };
    for (const auto& op : ops) {
        site2.merge(op);
    }
    
    std::cout << site1.value() << " " << site2.value() << std::endl;
    return site1.value() == " abccd" && site1.value() == site2.value();
}

//bool incompatible_work_converges_as_soon_as_it_is_integrable() {
//    
//}

int main(int argc, const char * argv[]) {
    
    std::cout<< insert_char_from_empty_string() << std::endl;
    std::cout<< insert_out_of_bounds_fails() << std::endl;
    std::cout<< delete_out_of_bounds_fails() << std::endl;
    std::cout<< delete_char_in_string() << std::endl;
    std::cout<< insert_after_deleting() << std::endl;
    std::cout<< integrating_remote_work() << std::endl;
    std::cout<< inserting_in_buffer_with_all_invisible_chars() << std::endl;
    std::cout<< multiple_insertions_converge() << std::endl;
    std::cout<< deleting_while_inserting_converges() << std::endl;
    std::cout<< receiving_many_operations_brings_in_sync() << std::endl;


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
    auto op1 = site1.ins(2, '1');
    auto op2 = site2.del(3);
    site1_timeline += "\t\t"+site1.value();
    site2_timeline += "\t\t"+site2.value();
    site1.merge(op2);
    site2.merge(op1);
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
    auto op3 = site3.del(2);
    auto op4 = site4.del(3);
    site3_timeline += "\t\t"+site3.value();
    site4_timeline += "\t\t"+site4.value();
    site3.merge(op4);
    site4.merge(op3);
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
    
    auto op5_1 = site5.ins(1, '1');
    site5_timeline += "\t\t"+site5.value();
    auto op5_2 = site5.ins(1, '3');
    site5_timeline += "\t\t"+site5.value();

    auto op6_1 = site6.ins(1, '2');
    site6_timeline += "\t\t"+site6.value();
    site7_timeline += "\t\t"+site7.value();
    
    site5.merge(op6_1); // 1 operation to apply
    site5_timeline += "\t\t"+site5.value();
    
    // 2 operations to apply, only one is executable immediately
    site6.merge(op5_2); //this merge won't happen: we need op5_1 first
    site6_timeline += "\t\t"+site6.value();
    site6.merge(op5_1); //adding this operation automatically merges the new op and all the operations that depends on it.
    site6_timeline += "\t\t"+site6.value();
    
    // 3 operations applied immediately
    site7.merge(op5_1);
    site7.merge(op6_1);
    site7.merge(op5_2);
    site7_timeline += "\t\t"+site7.value();

    std::cout << site5_timeline << std::endl; // ab     a1b     a31b    a312b
    std::cout << site6_timeline << std::endl; // ab     a2b     a12b    a312b
    std::cout << site7_timeline << std::endl; // ab     ab      a312b   (all at once)
    
    return 0;
}
