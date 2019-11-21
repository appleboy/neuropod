//
// Uber, Inc. (c) 2018
//

#include "gtest/gtest.h"
#include "neuropods/backends/test_backend/test_neuropod_backend.hh"
#include "neuropods/internal/neuropod_tensor.hh"

#include <gmock/gmock.h>

using ::testing::HasSubstr;

class uint8_tensor_fixture : public ::testing::Test
{
public:
    uint8_tensor_fixture()
    {
        untyped_tensor =
            test_backend_.get_tensor_allocator()->allocate_tensor({EXPECTED_SIZE}, neuropods::UINT8_TENSOR);
        const_untyped_tensor = untyped_tensor.get();

        tensor       = untyped_tensor->as_typed_tensor<uint8_t>();
        const_tensor = tensor;

        auto accessor = tensor->accessor<1>();
        for (size_t i = 0; i < EXPECTED_SIZE; ++i)
        {
            accessor[i] = i;
        }
    }

protected:
    static int                                     EXPECTED_SIZE;
    neuropods::TestNeuropodBackend                 test_backend_;
    std::unique_ptr<neuropods::NeuropodTensor>     untyped_tensor;
    const neuropods::NeuropodTensor *              const_untyped_tensor;
    neuropods::TypedNeuropodTensor<uint8_t> *      tensor;
    const neuropods::TypedNeuropodTensor<uint8_t> *const_tensor;
};
int uint8_tensor_fixture::EXPECTED_SIZE = 10;

class uint8_scalar_fixture : public ::testing::Test
{
public:
    uint8_scalar_fixture()
    {
        untyped_tensor = test_backend_.get_tensor_allocator()->allocate_tensor({1}, neuropods::UINT8_TENSOR);
        untyped_tensor->as_scalar<uint8_t>() = 42;
        const_untyped_tensor                 = untyped_tensor.get();
        tensor                               = untyped_tensor->as_typed_tensor<uint8_t>();
        const_tensor                         = tensor;
    }

protected:
    neuropods::TestNeuropodBackend                 test_backend_;
    std::unique_ptr<neuropods::NeuropodTensor>     untyped_tensor;
    const neuropods::NeuropodTensor *              const_untyped_tensor;
    neuropods::TypedNeuropodTensor<uint8_t> *      tensor;
    const neuropods::TypedNeuropodTensor<uint8_t> *const_tensor;
};

TEST(test_stream_operator, untyped_tensor)
{
    std::stringstream              ss;
    neuropods::TestNeuropodBackend test_backend;
    const auto untyped_tensor = test_backend.get_tensor_allocator()->allocate_tensor({3}, neuropods::UINT8_TENSOR);
    ss << *untyped_tensor;

    EXPECT_THAT(ss.str(), HasSubstr("NeuropodTensor"));
}

TEST(test_stream_operator, typed_tensor)
{
    std::stringstream              ss;
    neuropods::TestNeuropodBackend test_backend;
    auto untyped_tensor = test_backend.get_tensor_allocator()->allocate_tensor({3}, neuropods::UINT8_TENSOR);

    auto &typed_tensor = *untyped_tensor->as_typed_tensor<uint8_t>();
    auto  accessor     = typed_tensor.accessor<1>();

    accessor[0] = 10;
    accessor[1] = 11;
    accessor[2] = 12;

    ss << typed_tensor;
    EXPECT_THAT(ss.str(), HasSubstr("NeuropodTensor"));
    EXPECT_THAT(ss.str(), HasSubstr("[10, 11, 12]"));
}

TEST(test_stream_operator, typed_float_tensor)
{
    std::stringstream              ss;
    neuropods::TestNeuropodBackend test_backend;
    constexpr int                  TENSOR_SIZE = 8;
    auto untyped_tensor = test_backend.get_tensor_allocator()->allocate_tensor({TENSOR_SIZE}, neuropods::FLOAT_TENSOR);

    auto &typed_tensor = *untyped_tensor->as_typed_tensor<float>();
    auto  accessor     = typed_tensor.accessor<1>();

    for (int i = 0; i < TENSOR_SIZE; ++i)
    {
        accessor[i] = i + 0.5;
    }

    ss << typed_tensor;
    EXPECT_THAT(ss.str(), HasSubstr("NeuropodTensor"));
    EXPECT_THAT(ss.str(), HasSubstr("[0.5, 1.5, 2.5 ... 5.5, 6.5, 7.5]"));
}

TEST(test_typed_neuropod_tensor, downcast_failulre)
{
    neuropods::TestNeuropodBackend test_backend;
    constexpr int                  TENSOR_SIZE = 8;
    auto untyped_tensor = test_backend.get_tensor_allocator()->allocate_tensor({TENSOR_SIZE}, neuropods::FLOAT_TENSOR);

    EXPECT_THROW(untyped_tensor->as_typed_tensor<int8_t>(), std::runtime_error);
}

TEST_F(uint8_tensor_fixture, forloop)
{
    uint8_t i = 0;
    for (auto element : *tensor)
    {
        EXPECT_EQ(element, i);
        ++i;
    }
    EXPECT_EQ(EXPECTED_SIZE, i);
}

TEST_F(uint8_tensor_fixture, forloop_const_tensor)
{
    uint8_t i = 0;
    for (const auto &element : *const_tensor)
    {
        EXPECT_EQ(element, i);
        ++i;
    }
    EXPECT_EQ(EXPECTED_SIZE, i);
}

TEST_F(uint8_tensor_fixture, forloop_const_reference)
{
    uint8_t i = 0;
    for (const auto &element : *tensor)
    {
        EXPECT_EQ(element, i);
        ++i;
    }
    EXPECT_EQ(EXPECTED_SIZE, i);
}

TEST_F(uint8_tensor_fixture, can_not_cast_to_scalar)
{
    EXPECT_THROW(untyped_tensor->as_scalar<uint16_t>(), std::runtime_error);
}

TEST_F(uint8_tensor_fixture, wrong_dimensions)
{
    EXPECT_THROW(untyped_tensor->as_scalar<uint8_t>(), std::runtime_error);
}

TEST_F(uint8_tensor_fixture, const_wrong_dimensions)
{
    EXPECT_THROW(const_untyped_tensor->as_scalar<uint8_t>(), std::runtime_error);
}

TEST_F(uint8_scalar_fixture, non_const)
{
    untyped_tensor->as_scalar<uint8_t>() = 10;
    EXPECT_EQ(untyped_tensor->as_scalar<uint8_t>(), 10);
}

TEST_F(uint8_scalar_fixture, const_access)
{
    const auto &actual = const_untyped_tensor->as_scalar<uint8_t>();
    EXPECT_EQ(actual, 42);
}

TEST_F(uint8_scalar_fixture, wrong_type)
{
    EXPECT_THROW(const_untyped_tensor->as_scalar<uint16_t>(), std::runtime_error);
}

TEST_F(uint8_scalar_fixture, typed_non_const)
{
    tensor->as_scalar() = 10;
    EXPECT_EQ(tensor->as_scalar(), 10);
}

TEST_F(uint8_scalar_fixture, typed_const_access)
{
    const auto &actual = const_tensor->as_scalar();
    EXPECT_EQ(actual, 42);
}

TEST(test_value_equality, non_tensor_error)
{
    // We don't currently have any NeuropodValues that are not
    // tensors so create one in order to test
    class SomeNonTensorValue : public neuropods::NeuropodValue
    {
    public:
        SomeNonTensorValue() : neuropods::NeuropodValue(false) {}
        ~SomeNonTensorValue() = default;

        SET_SERIALIZE_TAG("something");
    };

    neuropods::TestNeuropodBackend backend;
    auto                           allocator = backend.get_tensor_allocator();

    // Create a tensor
    auto val1 = allocator->allocate_tensor<float>({5});

    // Create a non-tensor value
    SomeNonTensorValue val2;

    // We shouldn't be able to convert this to a tensor
    EXPECT_THROW(val2.as_tensor(), std::runtime_error);

    // Comparing with a NeuropodValue that is not a tensor should throw
    auto should_throw = [&val1, &val2]() { return *val1 == val2; };

    EXPECT_THROW(should_throw(), std::runtime_error);
}

TEST(test_tensor_equality, basic_equality)
{
    neuropods::TestNeuropodBackend backend;
    auto                           allocator = backend.get_tensor_allocator();

    auto t1 = allocator->ones<float>({5});
    auto t2 = allocator->ones<float>({5});

    // Self equality
    EXPECT_EQ(*t1, *t1);

    // t1 and t2 should be equal
    EXPECT_EQ(*t1, *t2);
}

TEST(test_tensor_equality, different_types)
{
    neuropods::TestNeuropodBackend backend;
    auto                           allocator = backend.get_tensor_allocator();

    auto t1 = allocator->ones<float>({5});
    auto t2 = allocator->ones<double>({5});

    EXPECT_FALSE(*t1 == *t2);
}

TEST(test_tensor_equality, different_dims)
{
    neuropods::TestNeuropodBackend backend;
    auto                           allocator = backend.get_tensor_allocator();

    auto t1 = allocator->ones<float>({5});
    auto t2 = allocator->ones<float>({6});

    EXPECT_FALSE(*t1 == *t2);
}

TEST(test_tensor_equality, different_ranks)
{
    neuropods::TestNeuropodBackend backend;
    auto                           allocator = backend.get_tensor_allocator();

    auto t1 = allocator->ones<float>({30});
    auto t2 = allocator->ones<float>({5, 6});

    EXPECT_FALSE(*t1 == *t2);
}

TEST(test_copy_from, different_numel)
{
    neuropods::TestNeuropodBackend backend;
    std::vector<float>             data(4);

    auto allocator = backend.get_tensor_allocator();
    auto t1        = allocator->allocate_tensor<float>({5});

    // The number of elements in data doesn't match the number of elements
    // in the tensor
    EXPECT_THROW(t1->copy_from(data), std::runtime_error);
}
