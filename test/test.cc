#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include "gtest/gtest.h"

#include "dcpl/any.h"
#include "dcpl/coro/coro.h"
#include "dcpl/coro/utils.h"
#include "dcpl/file.h"
#include "dcpl/fs.h"
#include "dcpl/ivector.h"
#include "dcpl/json/json.h"
#include "dcpl/memory.h"
#include "dcpl/stdns_override.h"
#include "dcpl/storage_span.h"
#include "dcpl/string_formatter.h"
#include "dcpl/temp_file.h"
#include "dcpl/temp_path.h"
#include "dcpl/threadpool.h"
#include "dcpl/types.h"
#include "dcpl/utils.h"
#include "dcpl/varint.h"

namespace dcpl_test {

TEST(StringFormatter, API) {
  dcpl::string_formatter sf;

  sf << "This " << 1 << " is a test for " << 2.3;
  EXPECT_EQ(sf.str(), std::string("This 1 is a test for 2.3"));
}

TEST(VectorStreamTest, TestInt) {
  std::vector<int> vec{1, 2, 3, 4, 5, 6, 7, 8};
  std::stringstream ss;

  ss << vec;
  EXPECT_EQ(ss.str(), std::string("(1, 2, 3, 4, 5, 6, 7, 8)"));
}

TEST(VectorStreamTest, TestString) {
  std::vector<std::string> vec{"ABC", "DEF", "GHI"};
  std::stringstream ss;

  ss << vec;
  EXPECT_EQ(ss.str(), std::string("(\"ABC\", \"DEF\", \"GHI\")"));
}

TEST(VectorStreamTest, TestCharPtr) {
  std::vector<const char*> vec{"ABC", "DEF", "GHI"};
  std::stringstream ss;

  ss << vec;
  EXPECT_EQ(ss.str(), std::string("(\"ABC\", \"DEF\", \"GHI\")"));
}

TEST(TupleStreamTest, Test) {
  auto tp = std::make_tuple(1, 1.2, "ABC", std::string("XYZ"));
  std::stringstream ss;

  ss << tp;
  EXPECT_EQ(ss.str(), std::string("(1, 1.2, \"ABC\", \"XYZ\")"));
}

TEST(TupleStreamTest, NestedTest) {
  auto tpn = std::make_tuple("ABC", std::string("XYZ"));
  auto tp = std::make_tuple(1, 1.2, tpn);
  std::stringstream ss;

  ss << tp;
  EXPECT_EQ(ss.str(), std::string("(1, 1.2, (\"ABC\", \"XYZ\"))"));
}

TEST(CheckTest, Test) {
  EXPECT_THROW({
      DCPL_CHECK_EQ(1, 2) << "Fail";
    }, std::runtime_error);

  EXPECT_THROW({
      DCPL_CHECK_NE(1, 1) << "Fail";
    }, std::runtime_error);

  EXPECT_THROW({
      DCPL_CHECK_LT(11, 2) << "Fail";
    }, std::runtime_error);

  EXPECT_THROW({
      DCPL_CHECK_LE(17, 2) << "Fail";
    }, std::runtime_error);

  EXPECT_THROW({
      DCPL_CHECK_GT(1, 2) << "Fail";
    }, std::runtime_error);

  EXPECT_THROW({
      DCPL_CHECK_GE(1, 2) << "Fail";
    }, std::runtime_error);

  EXPECT_NO_THROW({
      DCPL_CHECK_GE(2, 2) << "Fail";
    });

  EXPECT_NO_THROW({
      DCPL_CHECK_GT(2, -1) << "Fail";
    });

  EXPECT_NO_THROW({
      DCPL_CHECK_LE(2, 2) << "Fail";
    });

  EXPECT_NO_THROW({
      DCPL_CHECK_LT(1, 2) << "Fail";
    });

  EXPECT_NO_THROW({
      DCPL_CHECK_NE(21, 2) << "Fail";
    });

  EXPECT_NO_THROW({
      DCPL_CHECK_EQ(2, 2) << "Fail";
    });
}

TEST(SpanTest, API) {
  int array[] = {1, 2, 3, 4, 5, 6, 7, 8};
  std::span<const int> sp_arr(array);

  EXPECT_EQ(sp_arr.size(), 8);
  EXPECT_EQ(sp_arr[1], 2);
  EXPECT_EQ(sp_arr[2], 3);

  std::span<const int> ssp_arr = sp_arr.subspan(2, 4);

  EXPECT_EQ(ssp_arr.size(), 4);
  EXPECT_EQ(ssp_arr[2], 5);
  EXPECT_EQ(ssp_arr[3], 6);

  std::span<int> wsp_arr(array);

  EXPECT_EQ(wsp_arr.size(), 8);
  wsp_arr[2] = -1;
  EXPECT_EQ(array[2], -1);

  std::stringstream ss;

  ss << sp_arr;
  EXPECT_EQ(ss.str(), std::string("(1, 2, -1, 4, 5, 6, 7, 8)"));
}

TEST(StorageSpanTest, API) {
  int array[] = {1, 2, 3, 4, 5, 6, 7, 8};
  dcpl::storage_span<int> ssp_arr(array);

  EXPECT_EQ(ssp_arr.size(), 8);
  EXPECT_EQ(ssp_arr.data().data(), array);

  dcpl::storage_span<int>
      vsp_arr(std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8});
  EXPECT_EQ(vsp_arr.size(), 8);
  EXPECT_NE(vsp_arr.storage(), nullptr);

  dcpl::storage_span<int> empty;
  empty = vsp_arr;
  EXPECT_EQ(empty.size(), 8);

  empty = ssp_arr;
  EXPECT_EQ(empty.size(), 8);
  EXPECT_EQ(empty.data().data(), array);
}

TEST(UtilTest, Argsort) {
  float array[] = {1.2, -0.8, 12.44, 8.9, 5.1, 16.25, 2.4};
  std::span<float> sp_arr(array);

  std::vector<std::size_t> indices = dcpl::argsort(sp_arr);
  for (std::size_t i = 1; i < indices.size(); ++i) {
    EXPECT_LE(sp_arr[indices[i - 1]], sp_arr[indices[i]]);
  }
}

TEST(UtilTest, ToVector) {
  float array[] = {1.2, -0.8, 12.44, 8.9, 5.1, 16.25, 2.4};
  std::span<const float> sp_arr(array);

  std::vector<float> vec = dcpl::to_vector<float>(sp_arr);

  ASSERT_EQ(vec.size(), sp_arr.size());
  for (std::size_t i = 0; i < vec.size(); ++i) {
    EXPECT_EQ(vec[i], sp_arr[i]);
  }
}

TEST(UtilTest, ReduceIndices) {
  std::size_t indices[] = {3, 1, 5, 2, 0, 4};
  dcpl::bitmap bmap(10, false);

  bmap[1] = true;
  bmap[3] = true;
  bmap[0] = true;
  bmap[8] = true;

  std::vector<std::size_t> rindices = dcpl::reduce_indices<std::size_t>(indices, bmap);

  EXPECT_EQ(rindices.size(), 3);
  EXPECT_EQ(rindices[0], 3);
  EXPECT_EQ(rindices[1], 1);
  EXPECT_EQ(rindices[2], 0);
}

TEST(UtilTest, Resample) {
  dcpl::rnd_generator gen;
  std::vector<std::size_t> indices = dcpl::resample(100, 90, &gen);

  EXPECT_LE(indices.size(), 90);
}

TEST(UtilTest, Take) {
  const std::size_t N = 20;
  dcpl::rnd_generator gen;
  std::vector<float> values = dcpl::randn<float>(N, &gen);
  std::vector<std::size_t> indices{2, 4, 7, 11};

  std::vector<float> tvalues = dcpl::take<float>(values, indices);
  ASSERT_EQ(tvalues.size(), indices.size());
  for (std::size_t i = 0; i < indices.size(); ++i) {
    EXPECT_EQ(tvalues[i], values[indices[i]]);
  }

  std::unique_ptr<float[]> buffer = std::make_unique<float[]>(N);
  std::span<float> tovalues =
      dcpl::take<float>(values, indices, std::span<float>(buffer.get(), N));
  ASSERT_EQ(tovalues.size(), indices.size());
  for (std::size_t i = 0; i < indices.size(); ++i) {
    EXPECT_EQ(tovalues[i], values[indices[i]]);
  }
}

TEST(UtilTest, ToString) {
  auto sp = dcpl::to_span("DCPL Test");
  std::string ssp = dcpl::to_string(sp);

  EXPECT_EQ(ssp, "DCPL Test");
}

TEST(UtilTest, ToNumber) {
  auto sp = dcpl::to_span("1234");

  EXPECT_EQ(dcpl::to_number<int>(sp), 1234);
}

TEST(IVectorTest, IntTest) {
  dcpl::ivector<int, 16> ivec;

  EXPECT_TRUE(ivec.is_array());
  ivec.push_back(17);
  ivec.push_back(21);
  EXPECT_TRUE(ivec.is_array());
  EXPECT_EQ(ivec[0], 17);
  EXPECT_EQ(ivec.at(0), 17);
  EXPECT_EQ(ivec.at(1), 21);

  int s = 0;

  for (auto i : ivec) {
    s += i;
  }
  EXPECT_EQ(s, 38);
}

TEST(IVectorTest, StringTest) {
  dcpl::ivector<std::string, 2> ivec;

  EXPECT_TRUE(ivec.is_array());
  ivec.push_back("ABC");
  ivec.push_back("DEF");
  EXPECT_TRUE(ivec.is_array());
  ivec.push_back("GHI");
  ivec.emplace_back("XYZ");
  EXPECT_FALSE(ivec.is_array());
  EXPECT_EQ(ivec[0], "ABC");
  EXPECT_EQ(ivec[1], "DEF");
  EXPECT_EQ(ivec.at(2), "GHI");
  EXPECT_EQ(ivec.at(3), "XYZ");
}

TEST(IVectorTest, CopyCTor) {
  dcpl::ivector<int, 2> ivec;

  ivec.push_back(17);
  ivec.push_back(21);

  decltype(ivec) ctor1(ivec);

  EXPECT_EQ(ctor1[0], 17);
  EXPECT_EQ(ctor1.at(0), 17);
  EXPECT_EQ(ctor1.at(1), 21);

  ctor1.push_back(34);

  decltype(ctor1) ctor2(ctor1);

  EXPECT_FALSE(ctor2.is_array());

  EXPECT_EQ(ctor2.at(0), 17);
  EXPECT_EQ(ctor2.at(1), 21);
  EXPECT_EQ(ctor2.at(2), 34);
}

TEST(IVectorTest, MoveCTor) {
  dcpl::ivector<int, 2> ivec;

  ivec.push_back(17);
  ivec.push_back(21);

  decltype(ivec) ctor1(std::move(ivec));

  EXPECT_EQ(ctor1[0], 17);
  EXPECT_EQ(ctor1.at(0), 17);
  EXPECT_EQ(ctor1.at(1), 21);

  ctor1.push_back(34);

  const int* ptr1 = ctor1.data();

  decltype(ctor1) ctor2(std::move(ctor1));

  EXPECT_FALSE(ctor2.is_array());

  EXPECT_EQ(ctor2.at(0), 17);
  EXPECT_EQ(ctor2.at(1), 21);
  EXPECT_EQ(ctor2.at(2), 34);

  EXPECT_EQ(ptr1, ctor2.data());
}

TEST(IVectorTest, Stream) {
  dcpl::ivector<int, 2> ivec;

  ivec.push_back(17);
  ivec.push_back(21);
  ivec.push_back(34);

  std::stringstream ss;

  ss << ivec;

  EXPECT_EQ(ss.str(), "(17, 21, 34)");
}

TEST(IVectorTest, Resize) {
  dcpl::ivector<int, 2> ivec;

  ivec.push_back(17);
  ivec.push_back(21);

  ivec.resize(1);
  EXPECT_EQ(ivec.size(), 1);

  ivec.resize(2);
  EXPECT_EQ(ivec.size(), 2);
  EXPECT_EQ(ivec[1], 0);

  ivec.resize(8);
  EXPECT_EQ(ivec.size(), 8);
  EXPECT_EQ(ivec[7], 0);
  EXPECT_FALSE(ivec.is_array());
}

TEST(IVectorTest, Insert) {
  dcpl::ivector<int, 8> ivec;

  ivec.push_back(17);
  ivec.push_back(21);
  ivec.push_back(34);

  dcpl::ilist<int> elems{1, 2, 3, 4};

  ivec.insert(ivec.end(), elems.begin(), elems.end());

  EXPECT_EQ(ivec.size(), 7);
  EXPECT_EQ(ivec.at(4), 2);
  EXPECT_TRUE(ivec.is_array());

  ivec.insert(ivec.begin() + 1, elems.begin(), elems.end());

  EXPECT_EQ(ivec.size(), 11);
  EXPECT_EQ(ivec.at(4), 4);
  EXPECT_FALSE(ivec.is_array());
}

TEST(IVectorTest, ContainerInit) {
  {
    dcpl::ilist<std::string> elems{"ABC", "DEF", "GHI"};
    dcpl::ivector<std::string, 4> svec(elems);

    EXPECT_EQ(svec.size(), 3);
    EXPECT_EQ(svec.at(1), "DEF");
    EXPECT_TRUE(svec.is_array());
  }
  {
    dcpl::ilist<std::string> elems{"ABC", "DEF", "GHI"};
    dcpl::ivector<std::string, 2> svec(elems);

    EXPECT_EQ(svec.size(), 3);
    EXPECT_EQ(svec.at(1), "DEF");
    EXPECT_FALSE(svec.is_array());
  }
}

TEST(ThreadPoolTest, API) {
  const std::size_t N = 200;
  dcpl::rnd_generator gen;
  std::vector<float> values = dcpl::randn<float>(N, &gen);

  const float ref = 2.3;
  std::function<float (const float&)> fn = [ref](const float& value) -> float {
    return ref + value;
  };

  std::vector<float> results = dcpl::map(fn, values.begin(), values.end());

  for (std::size_t i = 0; i < values.size(); ++i) {
    EXPECT_EQ(results[i], values[i] + ref);
  }
}

TEST(VarintTest, API) {
  std::vector<dcpl::umaxint_t> ivec{1234, 97, 91823, 4322, 0};
  std::vector<std::uint8_t> enc_data = dcpl::varint_vec_encode(ivec);
  std::vector<dcpl::umaxint_t> dvec = dcpl::varint_vec_decode<dcpl::umaxint_t>(enc_data);

  EXPECT_EQ(ivec, dvec);
}

TEST(ToStringTest, API) {
  dcpl::umaxint_t v = static_cast<dcpl::umaxint_t>(182738918229102212);
  std::stringstream ss;

  ss << v;
  EXPECT_EQ(ss.str(), std::string("182738918229102212"));
}

dcpl::coro::ns_coro<void*> CoroSelfHandle() {
  auto handle = co_await dcpl::coro::get_coro_handle();

  co_return handle.address();
}

TEST(Coro, SelfHandle) {
  auto cfn = CoroSelfHandle();

  EXPECT_EQ(cfn.value(), cfn.handle().address());
}

dcpl::coro::ns_coro<void*> CoroSelfPromise() {
  auto handle = co_await dcpl::coro::get_coro_handle();

  co_return static_cast<void*>(dcpl::coro::value_base_ptr<>(handle));
}

TEST(Coro, SelfPromise) {
  auto cfn = CoroSelfPromise();

  EXPECT_EQ(cfn.value(), static_cast<void*>(&cfn.promise()));
}

TEST(Memory, API) {
  static constexpr std::size_t buffer_size = 4096;
  std::unique_ptr<std::uint8_t[]> buffer =
      std::make_unique<std::uint8_t[]>(buffer_size);
  dcpl::memory mem(buffer.get(), buffer_size);

  mem.write(static_cast<char>(21));
  mem.seek(0);
  EXPECT_EQ(mem.read<char>(), 21);

  mem.seek(1);
  mem.write(17);
  mem.seek(1);
  EXPECT_EQ(mem.read<int>(), 17);

  mem.seek(1);
  mem.write(17.21f);
  mem.seek(1);
  EXPECT_EQ(mem.read<float>(), 17.21f);

  mem.seek(1);
  mem.write(17.21);
  mem.seek(1);
  EXPECT_EQ(mem.read<double>(), 17.21);

  mem.align(128);
  EXPECT_EQ(mem.tell(), 128);
}

TEST(JSON, API) {
  dcpl::json::json jd;

  jd["int17"] = 17;
  EXPECT_EQ(jd["int17"], 17);

  jd["int21"] = 21;
  EXPECT_EQ(jd["int21"], 21);

  jd["str_ABC"] = "ABC";
  EXPECT_EQ(jd["str_ABC"], "ABC");

  EXPECT_NE(dcpl::get_or<int>(jd, "int21", 111), 111);
  EXPECT_EQ(dcpl::get_or<int>(jd, "miss", 1721), 1721);
  EXPECT_EQ(dcpl::get_or<double>(jd, "missd", 17.21), 17.21);

  std::string jd_dump = jd.dump();
  dcpl::json::json pjd = dcpl::json::json::parse(jd_dump);

  EXPECT_EQ(pjd["int17"], 17);
  EXPECT_EQ(pjd["int21"], 21);
  EXPECT_EQ(pjd["str_ABC"], "ABC");
}

TEST(Any, API) {
  dcpl::any_map<std::string> am;

  am["int17"] = 17;
  EXPECT_EQ(am["int17"], 17);

  am["float21"] = 21.0f;
  EXPECT_EQ(am["float21"], 21.0f);

  am["double_pi"] = 3.14;
  EXPECT_EQ(am["double_pi"], 3.14);

  am["string"] = std::string("ABCD");
  EXPECT_EQ(am["string"], std::string("ABCD"));
  EXPECT_EQ(*am["string"].ptr_cast<const std::string>(), std::string("ABCD"));

  am["cstring"] = "ABCD";
  EXPECT_EQ(std::strcmp(am["cstring"], "ABCD"), 0);

  dcpl::any x = 17.21;
  EXPECT_EQ(x, 17.21);
  x = 17;
  EXPECT_EQ(x, 17);
}

TEST(TempFile, API) {
  std::string path;
  {
    dcpl::temp_file tmp({});

    tmp.file() << "Some random data\n";
    path = tmp.path();
  }

  EXPECT_FALSE(stdfs::exists(path));
}

TEST(FileView, Shared) {
  static const std::size_t size = 4096 * 1024;
  static const char* const wrdata = "WRITTEN!";
  dcpl::temp_path tmp;
  {
    dcpl::file file(tmp, dcpl::file::open_read | dcpl::file::open_write |
                    dcpl::file::open_create);

    file.truncate(size);

    dcpl::file::mmap mm = file.view(dcpl::file::mmap_read | dcpl::file::mmap_write, 0, 0);

    file.close();

    std::span<char> data = mm.data();

    EXPECT_EQ(data.size(), size);

    std::memcpy(data.data() + size / 2, wrdata, std::strlen(wrdata));

    mm.sync();

    EXPECT_EQ(stdfs::file_size(tmp), size);
  }
  {
    dcpl::file::mmap mm = dcpl::file::view(tmp, dcpl::file::mmap_read, 0, 0);
    std::span<char> data = mm.data();

    EXPECT_EQ(data.size(), size);

    EXPECT_EQ(std::memcmp(data.data() + size / 2, wrdata, std::strlen(wrdata)), 0);
  }
}

TEST(MMap, Anon) {
  static const std::size_t size = 4096 * 8;
  dcpl::file::mmap mm =
      dcpl::file::view(dcpl::file::mmap_read | dcpl::file::mmap_write, size);
  std::span<char> data = mm.data();

  for (std::size_t i = 0; i < size; i += 1024) {
    data[i] = static_cast<char>(i & 0xff);

    EXPECT_EQ(data[i], static_cast<char>(i & 0xff));
  }
}

TEST(FileFile, Private) {
  static const std::size_t size = 4096 * 1024;
  static const char* const wrdata = "WRITTEN!";
  dcpl::temp_path tmp;
  {
    dcpl::file file(tmp, dcpl::file::open_read | dcpl::file::open_write |
                    dcpl::file::open_create);

    file.truncate(size);

    dcpl::file::mmap mm = file.view(dcpl::file::mmap_read | dcpl::file::mmap_write |
                                    dcpl::file::mmap_priv, 0, 0);

    file.close();

    std::span<char> data = mm.data();

    EXPECT_EQ(data.size(), size);

    std::memcpy(data.data() + size / 2, wrdata, std::strlen(wrdata));

    mm.sync();

    EXPECT_EQ(stdfs::file_size(tmp), size);
  }
  {
    dcpl::file::mmap mm = dcpl::file::view(tmp, dcpl::file::mmap_read, 0, 0);
    std::span<char> data = mm.data();

    EXPECT_EQ(data.size(), size);

    EXPECT_EQ(std::memcmp(data.data() + size / 2, wrdata, std::strlen(wrdata)), 0);
  }
}

}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}

