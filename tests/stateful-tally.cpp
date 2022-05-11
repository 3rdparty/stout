#include "stout/stateful-tally.h"

#include "gtest/gtest.h"

enum class State : uint8_t {
  Readers,
  ReadersWithWriterWaiting,
  Writer,
};

TEST(StatefulTallyTest, Test) {
  stout::StatefulTally<State> tally(State::Readers);

  State state = State::Readers;

  EXPECT_TRUE(tally.Increment(state));
  EXPECT_EQ(State::Readers, tally.state());
  EXPECT_EQ(1, tally.count());

  EXPECT_TRUE(tally.Update(state, State::ReadersWithWriterWaiting));
  EXPECT_EQ(State::ReadersWithWriterWaiting, tally.state());
  EXPECT_EQ(1, tally.count());

  state = State::ReadersWithWriterWaiting;

  EXPECT_TRUE(tally.Increment(state));
  EXPECT_EQ(State::ReadersWithWriterWaiting, tally.state());
  EXPECT_EQ(2, tally.count());

  bool decrement = [&]() {
    auto [state, count] = tally.Decrement();
    return state == State::ReadersWithWriterWaiting && count == 1;
  }();
  EXPECT_TRUE(decrement);
  EXPECT_EQ(State::ReadersWithWriterWaiting, tally.state());
  EXPECT_EQ(1, tally.count());

  decrement = [&]() {
    auto [state, count] = tally.Decrement();
    return state == State::ReadersWithWriterWaiting && count == 0;
  }();
  EXPECT_TRUE(decrement);
  EXPECT_EQ(State::ReadersWithWriterWaiting, tally.state());
  EXPECT_EQ(0, tally.count());

  EXPECT_TRUE(tally.Update(state, State::Writer));
  EXPECT_EQ(State::Writer, tally.state());
  EXPECT_EQ(0, tally.count());

  bool reset = false;

  state = State::Writer;
  EXPECT_TRUE(tally.Reset(state, State::Readers, [&]() {
    reset = true;
  }));

  EXPECT_TRUE(reset);

  state = State::Readers;

  EXPECT_TRUE(tally.Increment(state));
  EXPECT_EQ(State::Readers, tally.state());
  EXPECT_EQ(1, tally.count());
}
