/*
 * test_priority.h
 *
 *  Created on: Oct 14, 2022
 *      Author: HP
 */

#ifndef KERN_TESTS_TEST_PRIORITY_H_
#define KERN_TESTS_TEST_PRIORITY_H_

#ifndef FOS_KERNEL
# error "This is a FOS kernel header; user programs should not #include it"
#endif

void test_priority_normal_and_higher();
 void test_priority_normal_and_lower();

#endif /* KERN_TESTS_TEST_PRIORITY_H_ */
