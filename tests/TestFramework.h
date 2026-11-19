#ifndef MIDIPIANO_TESTS_TESTFRAMEWORK_H
#define MIDIPIANO_TESTS_TESTFRAMEWORK_H

#include <cmath>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

// Een heel klein testframework: TEST(naam) registreert een test, CHECK(...) controleert iets.
namespace midipiano::test {

class Registry {
 public:
  using TestFunction = std::function<void()>;

  static Registry& instance() {
    static Registry registry;  // static in een functie in plaats van een globale variabele
    return registry;
  }
  void add(const std::string& name, TestFunction function) { tests_.push_back({name, std::move(function)}); }
  void fail(const std::string& message) {
    ++failures_;
    std::cerr << "  FAIL " << message << '\n';
  }
  int runAll() {
    for (const auto& test : tests_) {
      std::cout << "[ RUN ] " << test.name << '\n';
      const int before = failures_;
      test.function();
      std::cout << (failures_ == before ? "[  OK ] " : "[FAILED] ") << test.name << '\n';
    }
    std::cout << '\n' << tests_.size() << " tests, " << failures_ << " failed checks\n";
    return failures_ == 0 ? 0 : 1;
  }

 private:
  struct Entry {
    std::string name;
    TestFunction function;
  };
  std::vector<Entry> tests_;
  int failures_ = 0;
};

struct AutoRegister {
  AutoRegister(const std::string& name, Registry::TestFunction function) {
    Registry::instance().add(name, std::move(function));
  }
};

}  // namespace midipiano::test

#define TEST(name)                                                                \
  static void name();                                                             \
  static const midipiano::test::AutoRegister name##_registration(#name, &name); \
  static void name()

#define CHECK(condition)                                                                     \
  do {                                                                                       \
    if (!(condition))                                                                        \
      midipiano::test::Registry::instance().fail(std::string(__FILE__) + ":" +              \
                                                 std::to_string(__LINE__) + "  " #condition); \
  } while (false)

#define CHECK_NEAR(a, b, tolerance) CHECK(std::fabs((a) - (b)) <= (tolerance))

#define CHECK_THROWS(statement, ExceptionType) \
  do {                                         \
    bool thrown = false;                       \
    try {                                      \
      statement;                               \
    } catch (const ExceptionType&) {           \
      thrown = true;                           \
    }                                          \
    CHECK(thrown && #statement " throws " #ExceptionType); \
  } while (false)

#endif  // MIDIPIANO_TESTS_TESTFRAMEWORK_H
