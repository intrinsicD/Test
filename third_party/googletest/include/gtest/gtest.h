#pragma once

#include <cmath>
#include <cstring>
#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace testing
{
    class Test
    {
    public:
        virtual ~Test() = default;

        void Run()
        {
            SetUp();
            TestBody();
            TearDown();
        }

    protected:
        virtual void SetUp()
        {
        }

        virtual void TearDown()
        {
        }

        virtual void TestBody()
        {
        }
    };

    namespace internal
    {
        struct TestDescriptor
        {
            std::string suite_name;
            std::string test_name;
            std::function<void()> factory;
        };

        inline std::vector<TestDescriptor>& Registry()
        {
            static std::vector<TestDescriptor> tests;
            return tests;
        }

        struct TestContext
        {
            int failed_expectations = 0;
            bool aborted = false;
        };

        inline thread_local TestContext* g_current_context = nullptr;

        class AssertionFailure : public std::exception
        {
        public:
            [[nodiscard]] const char* what() const noexcept override { return "fatal assertion"; }
        };

        inline void ReportFailure(const char* file, int line, const std::string& message)
        {
            std::cerr << file << ":" << line << ": Failure\n" << message << std::endl;
            if (g_current_context != nullptr)
            {
                ++g_current_context->failed_expectations;
            }
        }

        [[noreturn]] inline void ReportFatalFailure(const char* file, int line, const std::string& message)
        {
            ReportFailure(file, line, message);
            if (g_current_context != nullptr)
            {
                g_current_context->aborted = true;
            }
            throw AssertionFailure{};
        }

        inline std::string FormatBoolMessage(const char* expression)
        {
            std::ostringstream stream;
            stream << "Expected: " << expression << "\n  Actual: false";
            return stream.str();
        }

        template <typename Lhs, typename Rhs>
        inline std::string FormatComparison(const char* lhs_expression, const char* rhs_expression, const Lhs& lhs,
                                            const Rhs& rhs)
        {
            std::ostringstream stream;
            stream << "Expected equality of these values:\n  " << lhs_expression << "\n  " << rhs_expression
                << "\n    Actual: " << lhs << " vs " << rhs;
            return stream.str();
        }

        template <typename Lhs, typename Rhs>
        inline bool EqHelper(const Lhs& lhs, const Rhs& rhs)
        {
            return lhs == rhs;
        }

        inline bool FloatEqHelper(float lhs, float rhs)
        {
            constexpr float tolerance = 1e-5F;
            return std::fabs(lhs - rhs) <= tolerance;
        }

        inline bool CStringEqHelper(const char* lhs, const char* rhs)
        {
            if (lhs == nullptr || rhs == nullptr)
            {
                return lhs == rhs;
            }
            return std::strcmp(lhs, rhs) == 0;
        }

        class FatalFailureMessage
        {
        public:
            FatalFailureMessage(const char* file, int line) : file_(file), line_(line)
            {
            }

            [[noreturn]] ~FatalFailureMessage()
            {
                ReportFatalFailure(file_, line_, stream_.str());
            }

            template <typename T>
            FatalFailureMessage& operator<<(const T& value)
            {
                stream_ << value;
                return *this;
            }

        private:
            const char* file_;
            int line_;
            std::ostringstream stream_;
        };
    } // namespace internal

    class TestRegistrar
    {
    public:
        TestRegistrar(const char* suite_name, const char* test_name, std::function<void()> factory)
        {
            internal::Registry().push_back(internal::TestDescriptor{suite_name, test_name, std::move(factory)});
        }
    };

    inline void InitGoogleTest(int*, char**)
    {
    }

    inline int RunAllTests()
    {
        auto& tests = internal::Registry();
        std::cout << "[==========] Running " << tests.size() << " tests." << std::endl;

        int failed_tests = 0;
        for (const auto& test : tests)
        {
            std::cout << "[ RUN      ] " << test.suite_name << '.' << test.test_name << std::endl;
            internal::TestContext context{};
            internal::g_current_context = &context;
            try
            {
                test.factory();
            }
            catch (const internal::AssertionFailure&)
            {
                // Failure already reported.
            }
            catch (const std::exception& error)
            {
                internal::ReportFailure("<unknown>", 0, std::string{"Uncaught exception: "} + error.what());
                context.aborted = true;
            }
            catch (...)
            {
                internal::ReportFailure("<unknown>", 0, "Uncaught non-standard exception");
                context.aborted = true;
            }
            internal::g_current_context = nullptr;

            if (context.failed_expectations > 0 || context.aborted)
            {
                ++failed_tests;
                std::cout << "[  FAILED  ] " << test.suite_name << '.' << test.test_name << std::endl;
            }
            else
            {
                std::cout << "[       OK ] " << test.suite_name << '.' << test.test_name << std::endl;
            }
        }

        std::cout << "[==========] " << tests.size() << " tests ran." << std::endl;
        std::cout << "[  PASSED  ] " << (tests.size() - failed_tests) << " tests." << std::endl;
        if (failed_tests > 0)
        {
            std::cout << "[  FAILED  ] " << failed_tests << " tests." << std::endl;
        }
        return failed_tests;
    }
} // namespace testing

#define RUN_ALL_TESTS() ::testing::RunAllTests()

#define TEST(test_suite_name, test_name)                                                                   \
    class test_suite_name##_##test_name##_Test : public ::testing::Test {                                  \
    protected:                                                                                            \
        void TestBody() override;                                                                         \
    };                                                                                                    \
    static ::testing::TestRegistrar test_suite_name##_##test_name##_Test_registrar(                        \
        #test_suite_name, #test_name, []() {                                                               \
            test_suite_name##_##test_name##_Test test_instance;                                            \
            test_instance.Run();                                                                           \
        });                                                                                               \
    void test_suite_name##_##test_name##_Test::TestBody()

#define EXPECT_TRUE(condition)                                                                            \
    do {                                                                                                  \
        const bool gtest_condition = static_cast<bool>(condition);                                        \
        if (!gtest_condition) {                                                                           \
            ::testing::internal::ReportFailure(__FILE__, __LINE__,                                        \
                                              ::testing::internal::FormatBoolMessage(#condition));        \
        }                                                                                                 \
    } while (false)

#define EXPECT_FALSE(condition)                                                                           \
    do {                                                                                                  \
        const bool gtest_condition = static_cast<bool>(condition);                                        \
        if (gtest_condition) {                                                                            \
            ::testing::internal::ReportFailure(__FILE__, __LINE__,                                        \
                                              ::testing::internal::FormatBoolMessage("!(" #condition ")")); \
        }                                                                                                 \
    } while (false)


#define EXPECT_EQ(val1, val2)                                                                             \
    do {                                                                                                  \
        const auto gtest_val1 = (val1);                                                                   \
        const auto gtest_val2 = (val2);                                                                   \
        if (!::testing::internal::EqHelper(gtest_val1, gtest_val2)) {                                     \
            ::testing::internal::ReportFailure(                                                           \
                __FILE__, __LINE__,                                                                       \
                ::testing::internal::FormatComparison(#val1, #val2, gtest_val1, gtest_val2));            \
        }                                                                                                 \
    } while (false)

#define EXPECT_FLOAT_EQ(val1, val2)                                                                       \
    do {                                                                                                  \
        const float gtest_val1 = static_cast<float>(val1);                                                \
        const float gtest_val2 = static_cast<float>(val2);                                                \
        if (!::testing::internal::FloatEqHelper(gtest_val1, gtest_val2)) {                                \
            std::ostringstream gtest_message;                                                             \
            gtest_message << "Expected nearly equal values:\n  " << #val1 << " vs " << #val2           \
                          << "\n    Actual: " << gtest_val1 << " vs " << gtest_val2;                     \
            ::testing::internal::ReportFailure(__FILE__, __LINE__, gtest_message.str());                  \
        }                                                                                                 \
    } while (false)

#define EXPECT_STREQ(val1, val2)                                                                          \
    do {                                                                                                  \
        const char* gtest_val1 = (val1);                                                                  \
        const char* gtest_val2 = (val2);                                                                  \
        if (!::testing::internal::CStringEqHelper(gtest_val1, gtest_val2)) {                              \
            std::ostringstream gtest_message;                                                             \
            gtest_message << "Expected strings to be equal:\n  " << #val1 << " vs " << #val2           \
                          << "\n    Actual: "                                                         \
                          << (gtest_val1 ? gtest_val1 : "(null)") << " vs "                            \
                          << (gtest_val2 ? gtest_val2 : "(null)");                                      \
            ::testing::internal::ReportFailure(__FILE__, __LINE__, gtest_message.str());                  \
        }                                                                                                 \
    } while (false)

#define EXPECT_GT(val1, val2)                                                                             \
    do {                                                                                                  \
        const auto gtest_val1 = (val1);                                                                   \
        const auto gtest_val2 = (val2);                                                                   \
        if (!(gtest_val1 > gtest_val2)) {                                                                 \
            std::ostringstream gtest_message;                                                             \
            gtest_message << "Expected: (" << #val1 << ") > (" << #val2 << "), actual: " << gtest_val1  \
                          << " vs " << gtest_val2;                                                        \
            ::testing::internal::ReportFailure(__FILE__, __LINE__, gtest_message.str());                  \
        }                                                                                                 \
    } while (false)

#define EXPECT_GE(val1, val2)                                                                            \
    do {                                                                                                  \
        const auto gtest_val1 = (val1);                                                                   \
        const auto gtest_val2 = (val2);                                                                   \
        if (!(gtest_val1 >= gtest_val2)) {                                                                \
            std::ostringstream gtest_message;                                                             \
            gtest_message << "Expected: (" << #val1 << ") >= (" << #val2 << "), actual: " << gtest_val1 \
                          << " vs " << gtest_val2;                                                        \
            ::testing::internal::ReportFailure(__FILE__, __LINE__, gtest_message.str());                  \
        }                                                                                                 \
    } while (false)

#define EXPECT_LT(val1, val2)                                                                             \
    do {                                                                                                  \
        const auto gtest_val1 = (val1);                                                                   \
        const auto gtest_val2 = (val2);                                                                   \
        if (!(gtest_val1 < gtest_val2)) {                                                                 \
            std::ostringstream gtest_message;                                                             \
            gtest_message << "Expected: (" << #val1 << ") < (" << #val2 << "), actual: " << gtest_val1  \
                          << " vs " << gtest_val2;                                                        \
            ::testing::internal::ReportFailure(__FILE__, __LINE__, gtest_message.str());                  \
        }                                                                                                 \
    } while (false)

#define EXPECT_LE(val1, val2)                                                                             \
    do {                                                                                                  \
        const auto gtest_val1 = (val1);                                                                   \
        const auto gtest_val2 = (val2);                                                                   \
        if (!(gtest_val1 <= gtest_val2)) {                                                                 \
            std::ostringstream gtest_message;                                                             \
            gtest_message << "Expected: (" << #val1 << ") <= (" << #val2 << "), actual: " << gtest_val1  \
            << " vs " << gtest_val2;                                                        \
            ::testing::internal::ReportFailure(__FILE__, __LINE__, gtest_message.str());                  \
        }                                                                                                 \
    } while (false)

#define EXPECT_NEAR(val, exp, tol)                                                      \
    do {                                                                                \
        const auto gtest_val1 = (val);                                                  \
        const auto gtest_val2 = (exp);                                                  \
        const auto gtest_tol  = (tol);                                                  \
        using std::abs;                                                                 \
        if (!(abs(gtest_val1 - gtest_val2) <= gtest_tol)) {                             \
            std::ostringstream gtest_message;                                           \
            gtest_message << "Expected: |" << #val << " - " << #exp << "| <= " << #tol  \
                          << "\n  Actual: | " << gtest_val1 << " - " << gtest_val2 << " | = | " \
                          << (gtest_val1 - gtest_val2) << " | = " << abs(gtest_val1 - gtest_val2) \
                          << " > " << gtest_tol;                                        \
            ::testing::internal::ReportFailure(__FILE__, __LINE__, gtest_message.str());\
        }                                                                               \
    } while (false)

#define EXPECT_THROW(statement, expected_exception)                                                   \
    do {                                                                                              \
        bool gtest_caught_expected = false;                                                            \
        bool gtest_caught_other = false;                                                               \
        try {                                                                                          \
            (void)(statement);                                                                         \
        } catch (const expected_exception&) {                                                          \
            gtest_caught_expected = true;                                                              \
        } catch (const std::exception& gtest_other) {                                                  \
            gtest_caught_other = true;                                                                 \
            std::ostringstream gtest_message;                                                          \
            gtest_message << "Expected exception of type " << #expected_exception                      \
                          << " but caught std::exception: " << gtest_other.what();                    \
            ::testing::internal::ReportFailure(__FILE__, __LINE__, gtest_message.str());               \
        } catch (...) {                                                                                \
            gtest_caught_other = true;                                                                 \
            std::ostringstream gtest_message;                                                          \
            gtest_message << "Expected exception of type " << #expected_exception                      \
                          << " but caught an unknown exception type.";                               \
            ::testing::internal::ReportFailure(__FILE__, __LINE__, gtest_message.str());               \
        }                                                                                              \
        if (!gtest_caught_expected && !gtest_caught_other) {                                           \
            std::ostringstream gtest_message;                                                          \
            gtest_message << "Expected exception of type " << #expected_exception                      \
                          << " to be thrown.";                                                        \
            ::testing::internal::ReportFailure(__FILE__, __LINE__, gtest_message.str());               \
        }                                                                                              \
    } while (false)

#define ASSERT_TRUE(condition)                                                                           \
    do {                                                                                                  \
        const bool gtest_condition = static_cast<bool>(condition);                                        \
        if (!gtest_condition) {                                                                           \
            ::testing::internal::ReportFatalFailure(__FILE__, __LINE__,                                   \
                                                    ::testing::internal::FormatBoolMessage(#condition));  \
        }                                                                                                 \
    } while (false)

#define ASSERT_EQ(val1, val2)                                                                             \
    do {                                                                                                  \
        const auto gtest_val1 = (val1);                                                                   \
        const auto gtest_val2 = (val2);                                                                   \
        if (!::testing::internal::EqHelper(gtest_val1, gtest_val2)) {                                     \
            ::testing::internal::ReportFatalFailure(                                                      \
                __FILE__, __LINE__,                                                                       \
                ::testing::internal::FormatComparison(#val1, #val2, gtest_val1, gtest_val2));            \
        }                                                                                                 \
    } while (false)

#define FAIL() ::testing::internal::FatalFailureMessage(__FILE__, __LINE__)
