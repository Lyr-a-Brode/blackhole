#include <gtest/gtest.h>

#include <blackhole/attribute.hpp>

namespace blackhole {
namespace testing {
namespace attribute {

using ::blackhole::attribute::view_t;

TEST(view_t, FromInt) {
    view_t v(42);

    EXPECT_EQ(42, blackhole::attribute::get<std::int64_t>(v));
}

TEST(view_t, FromLong) {
    view_t v(42L);

    EXPECT_EQ(42L, blackhole::attribute::get<std::int64_t>(v));
}

}  // namespace attribute
}  // namespace testing
}  // namespace blackhole
