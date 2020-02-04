#include <folly/experimental/TestUtil.h>
#include <folly/io/RecordIO.h>
#include <folly/io/IOBufQueue.h>

#include <gtest/gtest.h>

folly::StringPiece sp(folly::ByteRange br) {
  return folly::StringPiece(br);
}

template <class T>
std::unique_ptr<folly::IOBuf> iobufs(std::initializer_list<T> ranges) {
  folly::IOBufQueue queue;
  for (auto& range : ranges) {
    folly::StringPiece r(range);
    queue.append(folly::IOBuf::wrapBuffer(r.data(), r.size()));
  }
  return queue.move();
}

TEST(Folly, RecordIOWriter) {
  folly::test::TemporaryFile file;
  {
    folly::RecordIOWriter writer(folly::File(file.fd()));
    writer.write(iobufs({"hello ", "world"}));
    writer.write(iobufs({"goodbye"}));
  }
  {
    folly::RecordIOReader reader(folly::File(file.fd()));
    auto it = reader.begin();
    ASSERT_FALSE(it == reader.end());
    EXPECT_EQ("hello world", sp((it++)->first));
    ASSERT_FALSE(it == reader.end());
    EXPECT_EQ("goodbye", sp((it++)->first));
    EXPECT_TRUE(it == reader.end());
  }
}
