/**
 * @file unity.h
 * @brief Minimal host-only Unity compatibility shim for local GCC coverage runs.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once
#include <cstdint>
#include <iostream>
namespace fakeunity { inline int failures=0; inline int tests=0; inline std::uint64_t assertions=0; inline void fail(const char* expr,const char* file,int line){++failures;std::cerr<<file<<':'<<line<<" assertion failed: "<<expr<<'\n';} }
#define UNITY_BEGIN() (fakeunity::failures=0, fakeunity::tests=0, fakeunity::assertions=0, 0)
#define UNITY_END() (std::cout<<"Unity tests: "<<fakeunity::tests<<" tests, "<<fakeunity::assertions<<" assertions, "<<fakeunity::failures<<" failures\n", fakeunity::failures)
#define RUN_TEST(fn) do { ++fakeunity::tests; setUp(); fn(); tearDown(); } while(0)
#define TEST_ASSERT_TRUE(x) do{++fakeunity::assertions;if(!(x))fakeunity::fail(#x,__FILE__,__LINE__);}while(0)
#define TEST_ASSERT_FALSE(x) TEST_ASSERT_TRUE(!(x))
#define TEST_ASSERT_EQUAL_UINT8(a,b) TEST_ASSERT_TRUE(static_cast<std::uint8_t>(a)==static_cast<std::uint8_t>(b))
#define TEST_ASSERT_EQUAL_UINT32(a,b) TEST_ASSERT_TRUE(static_cast<std::uint32_t>(a)==static_cast<std::uint32_t>(b))
#define TEST_ASSERT_EQUAL_UINT64(a,b) TEST_ASSERT_TRUE(static_cast<std::uint64_t>(a)==static_cast<std::uint64_t>(b))
#define TEST_ASSERT_EQUAL_HEX32(a,b) TEST_ASSERT_EQUAL_UINT32(a,b)
#define TEST_ASSERT_EQUAL(a,b) TEST_ASSERT_TRUE((a)==(b))
