// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <string>

#include "fmt/format.h"
#include "protobuf_tests.pb.h"
#include "stout/gtest.h"
#include "stout/json.h"
#include "stout/jsonify.h"
#include "stout/protobuf.h"
#include "stout/strings.h"
#include "stout/uuid.h"

using std::string;

using google::protobuf::RepeatedPtrField;

namespace tests {

// Trivial equality operators to enable gtest macros.
bool operator==(const SimpleMessage& left, const SimpleMessage& right) {
  if (left.id() != right.id()
      || left.numbers().size() != right.numbers().size()) {
    return false;
  }

  return std::equal(
      left.numbers().begin(),
      left.numbers().end(),
      right.numbers().begin());
}


bool operator!=(const SimpleMessage& left, const SimpleMessage& right) {
  return !(left == right);
}

} // namespace tests


TEST(ProtobufTest, JSON) {
  tests::Message message;
  message.set_b(true);
  message.set_str("string");
  message.set_bytes("bytes");
  message.set_int32(-1);
  message.set_int64(-1);
  message.set_uint32(1);
  message.set_uint64(1);
  message.set_sint32(-1);
  message.set_sint64(-1);
  message.set_f(1.0);
  message.set_d(1.0);
  message.set_e(tests::ONE);
  message.mutable_nested()->set_str("nested");
  message.add_repeated_bool(true);
  message.add_repeated_string("repeated_string");
  message.add_repeated_bytes("repeated_bytes");
  message.add_repeated_int32(-2);
  message.add_repeated_int64(-2);
  message.add_repeated_uint32(2);
  message.add_repeated_uint64(2);
  message.add_repeated_sint32(-2);
  message.add_repeated_sint64(-2);
  message.add_repeated_float(1.0);
  message.add_repeated_double(1.0);
  message.add_repeated_double(2.0);
  message.add_repeated_enum(tests::TWO);
  message.add_repeated_nested()->set_str("repeated_nested");

  // TODO(bmahler): To dynamically generate a protobuf message,
  // see the commented-out code below.
  //
  //  DescriptorProto proto;
  //
  //  proto.set_name("Message");
  //
  //  FieldDescriptorProto* field = proto.add_field();
  //  field->set_name("str");
  //  field->set_type(FieldDescriptorProto::TYPE_STRING);
  //
  //  const Descriptor* descriptor = proto.descriptor();
  //
  //  DynamicMessageFactory factory;
  //  Message* message = factory.GetPrototype(descriptor);
  //
  //  Reflection* message.getReflection();

  // The keys are in alphabetical order.
  string expected =
      R"~(
    {
      "b": true,
      "bytes": "Ynl0ZXM=",
      "d": 1.0,
      "e": "ONE",
      "f": 1.0,
      "int32": -1,
      "int64": -1,
      "nested": { "str": "nested"},
      "optional_default": 42.0,
      "repeated_bool": [true],
      "repeated_bytes": ["cmVwZWF0ZWRfYnl0ZXM="],
      "repeated_double": [1.0, 2.0],
      "repeated_enum": ["TWO"],
      "repeated_float": [1.0],
      "repeated_int32": [-2],
      "repeated_int64": [-2],
      "repeated_nested": [ { "str": "repeated_nested" } ],
      "repeated_sint32": [-2],
      "repeated_sint64": [-2],
      "repeated_string": ["repeated_string"],
      "repeated_uint32": [2],
      "repeated_uint64": [2],
      "sint32": -1,
      "sint64": -1,
      "str": "string",
      "uint32": 1,
      "uint64": 1
    })~";

  // Remove ' ' and '\n' from `expected` so that we can compare
  // it with the JSON string parsed from protobuf message.
  expected.erase(
      std::remove_if(expected.begin(), expected.end(), ::isspace),
      expected.end());

  // The only difference between this JSON string and the above one is, all
  // the bools and numbers are in the format of strings.
  string accepted =
      R"~(
    {
      "b": "true",
      "bytes": "Ynl0ZXM=",
      "d": "1.0",
      "e": "ONE",
      "f": "1.0",
      "int32": "-1",
      "int64": "-1",
      "nested": { "str": "nested"},
      "optional_default": "42.0",
      "repeated_bool": ["true"],
      "repeated_bytes": ["cmVwZWF0ZWRfYnl0ZXM="],
      "repeated_double": ["1.0", "2.0"],
      "repeated_enum": ["TWO"],
      "repeated_float": ["1.0"],
      "repeated_int32": ["-2"],
      "repeated_int64": ["-2"],
      "repeated_nested": [ { "str": "repeated_nested" } ],
      "repeated_sint32": ["-2"],
      "repeated_sint64": ["-2"],
      "repeated_string": ["repeated_string"],
      "repeated_uint32": ["2"],
      "repeated_uint64": ["2"],
      "sint32": "-1",
      "sint64": "-1",
      "str": "string",
      "uint32": "1",
      "uint64": "1"
    })~";

  JSON::Object object = JSON::protobuf(message);

  EXPECT_EQ(expected, fmt::format("{}", object));

  // Test parsing too.
  Try<tests::Message> parse = protobuf::parse<tests::Message>(object);
  ASSERT_SOME(parse);

  EXPECT_EQ(object, JSON::protobuf(parse.get()));

  // Test all the bools and numbers in the JSON string `accepted` can be
  // successfully parsed.
  Try<JSON::Object> json = JSON::parse<JSON::Object>(accepted);
  ASSERT_SOME(json);

  parse = protobuf::parse<tests::Message>(json.get());
  ASSERT_SOME(parse);

  EXPECT_EQ(object, JSON::protobuf(parse.get()));

  // Modify the message to test (de-)serialization of random bytes generated
  // by UUID.
  message.set_bytes(id::UUID::random().toBytes());

  object = JSON::protobuf(message);

  // Test parsing too.
  parse = protobuf::parse<tests::Message>(object);
  ASSERT_SOME(parse);

  EXPECT_EQ(object, JSON::protobuf(parse.get()));

  // Now convert JSON to string and parse it back as JSON.
  ASSERT_SOME_EQ(object, JSON::parse(fmt::format("{}", object)));
}


TEST(ProtobufTest, JSONArray) {
  tests::SimpleMessage message1;
  message1.set_id("message1");
  message1.add_numbers(1);
  message1.add_numbers(2);

  // Messages with different IDs are not equal.
  tests::SimpleMessage message2;
  message2.set_id("message2");
  message2.add_numbers(1);
  message2.add_numbers(2);

  // The keys are in alphabetical order.
  string expected =
      R"~(
    [
      {
        "id": "message1",
        "numbers": [1, 2]
      },
      {
        "id": "message2",
        "numbers": [1, 2]
      }
    ])~";

  // Remove ' ' and '\n' from `expected` so that we can compare
  // it with the JSON string parsed from protobuf message.
  expected.erase(
      std::remove_if(expected.begin(), expected.end(), ::isspace),
      expected.end());

  tests::ArrayMessage arrayMessage;
  arrayMessage.add_values()->CopyFrom(message1);
  arrayMessage.add_values()->CopyFrom(message2);

  JSON::Array array = JSON::protobuf(arrayMessage.values());

  EXPECT_EQ(expected, fmt::format("{}", array));
}


// Tests that integer precision is maintained between
// JSON <-> Protobuf conversions.
TEST(ProtobufTest, JsonLargeIntegers) {
  // These numbers are equal or close to the integer limits.
  tests::Message message;
  message.set_int32(-2147483647);
  message.set_int64(-9223372036854775807);
  message.set_uint32(4294967295U);
  message.set_uint64(9223372036854775807);
  message.set_sint32(-1234567890);
  message.set_sint64(-1234567890123456789);
  message.add_repeated_int32(-2000000000);
  message.add_repeated_int64(-9000000000000000000);
  message.add_repeated_uint32(3000000000U);
  message.add_repeated_uint64(7000000000000000000);
  message.add_repeated_sint32(-1000000000);
  message.add_repeated_sint64(-8000000000000000000);

  // Parts of the protobuf that are required.  Copied from the above test.
  message.set_b(true);
  message.set_str("string");
  message.set_bytes("bytes");
  message.set_f(1.0);
  message.set_d(1.0);
  message.set_e(tests::ONE);
  message.mutable_nested()->set_str("nested");

  // The keys are in alphabetical order.
  string expected =
      R"~(
    {
      "b": true,
      "bytes": "Ynl0ZXM=",
      "d": 1.0,
      "e": "ONE",
      "f": 1.0,
      "int32": -2147483647,
      "int64": -9223372036854775807,
      "nested": {"str": "nested"},
      "optional_default": 42.0,
      "repeated_int32": [-2000000000],
      "repeated_int64": [-9000000000000000000],
      "repeated_sint32": [-1000000000],
      "repeated_sint64": [-8000000000000000000],
      "repeated_uint32": [3000000000],
      "repeated_uint64": [7000000000000000000],
      "sint32": -1234567890,
      "sint64": -1234567890123456789,
      "str": "string",
      "uint32": 4294967295,
      "uint64": 9223372036854775807
    })~";

  // Remove ' ' and '\n' from `expected` so that we can compare
  // it with the JSON string parsed from protobuf message.
  expected.erase(
      std::remove_if(expected.begin(), expected.end(), ::isspace),
      expected.end());

  // Check JSON -> String.
  JSON::Object object = JSON::protobuf(message);
  EXPECT_EQ(expected, fmt::format("{}", object));

  // Check JSON -> Protobuf.
  Try<tests::Message> parse = protobuf::parse<tests::Message>(object);
  ASSERT_SOME(parse);

  // Check Protobuf -> JSON.
  EXPECT_EQ(object, JSON::protobuf(parse.get()));

  // Check String -> JSON.
  Try<JSON::Object> json = JSON::parse<JSON::Object>(expected);
  EXPECT_SOME_EQ(object, json);
}


TEST(ProtobufTest, SimpleMessageEquals) {
  tests::SimpleMessage message1;
  message1.set_id("message1");
  message1.add_numbers(1);
  message1.add_numbers(2);

  // Obviously, a message should equal to itself.
  EXPECT_EQ(message1, message1);

  // Messages with different IDs are not equal.
  tests::SimpleMessage message2;
  message2.set_id("message2");
  message2.add_numbers(1);
  message2.add_numbers(2);

  EXPECT_NE(message1, message2);

  // Messages with not identical collection of numbers are not equal.
  tests::SimpleMessage message3;
  message3.set_id("message1");
  message3.add_numbers(1);

  EXPECT_NE(message1, message3);

  tests::SimpleMessage message4;
  message4.set_id("message1");
  message4.add_numbers(2);
  message4.add_numbers(1);

  EXPECT_NE(message1, message4);

  // Different messages with the same ID and collection of numbers should
  // be equal. Their JSON counterparts should be equal as well.
  tests::SimpleMessage message5;
  message5.set_id("message1");
  message5.add_numbers(1);
  message5.add_numbers(2);

  EXPECT_EQ(message1, message5);
  EXPECT_EQ(JSON::protobuf(message1), JSON::protobuf(message5));
}


TEST(ProtobufTest, ParseJSONArray) {
  tests::SimpleMessage message;
  message.set_id("message1");
  message.add_numbers(1);
  message.add_numbers(2);

  // Convert protobuf message to a JSON object.
  JSON::Object object = JSON::protobuf(message);

  // Populate JSON array with JSON objects, conversion JSON::Object ->
  // JSON::Value is implicit.
  JSON::Array array;
  array.values.push_back(object);
  array.values.push_back(object);

  // Parse JSON array into a collection of protobuf messages.
  auto parse =
      protobuf::parse<RepeatedPtrField<tests::SimpleMessage>>(array);
  ASSERT_SOME(parse);
  auto repeated = parse.get();

  // Make sure the parsed message equals to the original one.
  EXPECT_EQ(message, repeated.Get(0));
  EXPECT_EQ(message, repeated.Get(1));
}


TEST(ProtobufTest, ParseJSONNull) {
  tests::Nested nested;
  nested.set_str("value");

  // Test message with optional field set to 'null'.
  string message =
      R"~(
    {
      "str": "value",
      "optional_str": null
    })~";

  Try<JSON::Object> json = JSON::parse<JSON::Object>(message);
  ASSERT_SOME(json);

  Try<tests::Nested> parse = protobuf::parse<tests::Nested>(json.get());
  ASSERT_SOME(parse);

  EXPECT_EQ(parse->SerializeAsString(), nested.SerializeAsString());

  // Test message with repeated field set to 'null'.
  message =
      R"~(
    {
      "str": "value",
      "repeated_str": null
    })~";

  json = JSON::parse<JSON::Object>(message);
  ASSERT_SOME(json);

  parse = protobuf::parse<tests::Nested>(json.get());
  ASSERT_SOME(parse);

  EXPECT_EQ(parse->SerializeAsString(), nested.SerializeAsString());

  // Test message with required field set to 'null'.
  message =
      R"~(
    {
      "str": null
    })~";

  json = JSON::parse<JSON::Object>(message);
  ASSERT_SOME(json);

  EXPECT_ERROR(protobuf::parse<tests::Nested>(json.get()));
}


TEST(ProtobufTest, ParseJSONNestedError) {
  // Here we trigger an error parsing the 'nested' message, i.e., set
  // the string type field `nested.str` to a number.
  string message =
      R"~(
    {
      "b": true,
      "str": "string",
      "bytes": "Ynl0ZXM=",
      "f": 1.0,
      "d": 1.0,
      "e": "ONE",
      "nested": {
          "str": 1.0
      }
    })~";

  Try<JSON::Object> json = JSON::parse<JSON::Object>(message);
  ASSERT_SOME(json);

  Try<tests::Message> parse = protobuf::parse<tests::Message>(json.get());
  ASSERT_ERROR(parse);

  EXPECT_TRUE(strings::contains(
      parse.error(),
      "Not expecting a JSON number for field"));
}


// Tests when parsing protobuf from JSON, for the optional enum field which
// has an unrecognized enum value, after the parsing the field will be unset
// and its getter will return the default enum value. For the repeated enum
// field which contains an unrecognized enum value, after the parsing the
// field will not contain that unrecognized value anymore.
TEST(ProtobufTest, ParseJSONUnrecognizedEnum) {
  string message =
      R"~(
    {
      "e1": "XXX",
      "e2": "",
      "repeated_enum": ["ONE", "XXX", "", "TWO"]
    })~";

  Try<JSON::Object> json = JSON::parse<JSON::Object>(message);
  ASSERT_SOME(json);

  Try<tests::EnumMessage> parse =
      protobuf::parse<tests::EnumMessage>(json.get());

  ASSERT_SOME(parse);

  EXPECT_FALSE(parse->has_e1());
  EXPECT_EQ(tests::UNKNOWN, parse->e1());
  EXPECT_FALSE(parse->has_e2());
  EXPECT_EQ(tests::UNKNOWN, parse->e2());

  EXPECT_EQ(2, parse->repeated_enum_size());
  EXPECT_EQ(tests::ONE, parse->repeated_enum(0));
  EXPECT_EQ(tests::TWO, parse->repeated_enum(1));
}


TEST(ProtobufTest, Jsonify) {
  tests::Message message;
  message.set_b(true);
  message.set_str("string");
  message.set_bytes("bytes");
  message.set_int32(-1);
  message.set_int64(-1);
  message.set_uint32(1);
  message.set_uint64(1);
  message.set_sint32(-1);
  message.set_sint64(-1);
  message.set_f(1.0);
  message.set_d(1.0);
  message.set_e(tests::ONE);
  message.mutable_nested()->set_str("nested");
  message.add_repeated_bool(true);
  message.add_repeated_string("repeated_string");
  message.add_repeated_bytes("repeated_bytes");
  message.add_repeated_int32(-2);
  message.add_repeated_int64(-2);
  message.add_repeated_uint32(2);
  message.add_repeated_uint64(2);
  message.add_repeated_sint32(-2);
  message.add_repeated_sint64(-2);
  message.add_repeated_float(1.0);
  message.add_repeated_double(1.0);
  message.add_repeated_double(2.0);
  message.add_repeated_enum(tests::TWO);
  message.add_repeated_nested()->set_str("repeated_nested");

  // TODO(bmahler): To dynamically generate a protobuf message,
  // see the commented-out code below.
  //  DescriptorProto proto;
  //
  //  proto.set_name("Message");
  //
  //  FieldDescriptorProto* field = proto.add_field();
  //  field->set_name("str");
  //  field->set_type(FieldDescriptorProto::TYPE_STRING);
  //
  //  const Descriptor* descriptor = proto.descriptor();
  //
  //  DynamicMessageFactory factory;
  //  Message* message = factory.GetPrototype(descriptor);
  //
  //  Reflection* message.getReflection();

  // The keys are in alphabetical order.
  string expected =
      R"~(
    {
      "b": true,
      "str": "string",
      "bytes": "Ynl0ZXM=",
      "int32": -1,
      "int64": -1,
      "uint32": 1,
      "uint64": 1,
      "sint32": -1,
      "sint64": -1,
      "f": 1.0,
      "d": 1.0,
      "e": "ONE",
      "nested": { "str": "nested"},
      "repeated_bool": [true],
      "repeated_string": ["repeated_string"],
      "repeated_bytes": ["cmVwZWF0ZWRfYnl0ZXM="],
      "repeated_int32": [-2],
      "repeated_int64": [-2],
      "repeated_uint32": [2],
      "repeated_uint64": [2],
      "repeated_sint32": [-2],
      "repeated_sint64": [-2],
      "repeated_float": [1.0],
      "repeated_double": [1.0, 2.0],
      "repeated_enum": ["TWO"],
      "repeated_nested": [ { "str": "repeated_nested" } ],
      "optional_default": 42.0
    })~";

  // Remove ' ' and '\n' from `expected` so that we can compare
  // it with the JSON string parsed from protobuf message.
  expected.erase(
      std::remove_if(expected.begin(), expected.end(), ::isspace),
      expected.end());

  EXPECT_EQ(expected, string(jsonify(JSON::Protobuf(message))));
}


TEST(ProtobufTest, JsonifyArray) {
  tests::SimpleMessage message1;
  message1.set_id("message1");
  message1.add_numbers(1);
  message1.add_numbers(2);

  // Messages with different IDs are not equal.
  tests::SimpleMessage message2;
  message2.set_id("message2");
  message2.add_numbers(1);
  message2.add_numbers(2);

  // The keys are in alphabetical order.
  string expected =
      R"~(
    [
      {
        "id": "message1",
        "numbers": [1, 2]
      },
      {
        "id": "message2",
        "numbers": [1, 2]
      }
    ])~";

  // Remove ' ' and '\n' from `expected` so that we can compare
  // it with the JSON string parsed from protobuf message.
  expected.erase(
      std::remove_if(expected.begin(), expected.end(), ::isspace),
      expected.end());

  tests::ArrayMessage arrayMessage;
  arrayMessage.add_values()->CopyFrom(message1);
  arrayMessage.add_values()->CopyFrom(message2);

  string actual = jsonify([&arrayMessage](JSON::ArrayWriter* writer) {
    foreach (const tests::SimpleMessage& message, arrayMessage.values()) {
      writer->element(JSON::Protobuf(message));
    }
  });

  EXPECT_EQ(expected, actual);
}


// Tests that integer precision is maintained between
// JSON <-> Protobuf conversions.
TEST(ProtobufTest, JsonifyLargeIntegers) {
  // These numbers are equal or close to the integer limits.
  tests::Message message;
  message.set_int32(-2147483647);
  message.set_int64(-9223372036854775807);
  message.set_uint32(4294967295U);
  message.set_uint64(9223372036854775807);
  message.set_sint32(-1234567890);
  message.set_sint64(-1234567890123456789);
  message.add_repeated_int32(-2000000000);
  message.add_repeated_int64(-9000000000000000000);
  message.add_repeated_uint32(3000000000U);
  message.add_repeated_uint64(7000000000000000000);
  message.add_repeated_sint32(-1000000000);
  message.add_repeated_sint64(-8000000000000000000);

  // Parts of the protobuf that are required.  Copied from the above test.
  message.set_b(true);
  message.set_str("string");
  message.set_bytes("bytes");
  message.set_f(1.0);
  message.set_d(1.0);
  message.set_e(tests::ONE);
  message.mutable_nested()->set_str("nested");

  // The keys are in alphabetical order.
  string expected =
      R"~(
    {
      "b": true,
      "str": "string",
      "bytes": "Ynl0ZXM=",
      "int32": -2147483647,
      "int64": -9223372036854775807,
      "uint32": 4294967295,
      "uint64": 9223372036854775807,
      "sint32": -1234567890,
      "sint64": -1234567890123456789,
      "f": 1.0,
      "d": 1.0,
      "e": "ONE",
      "nested": {"str": "nested"},
      "repeated_int32": [-2000000000],
      "repeated_int64": [-9000000000000000000],
      "repeated_uint32": [3000000000],
      "repeated_uint64": [7000000000000000000],
      "repeated_sint32": [-1000000000],
      "repeated_sint64": [-8000000000000000000],
      "optional_default": 42.0
    })~";

  // Remove ' ' and '\n' from `expected` so that we can compare
  // it with the JSON string parsed from protobuf message.
  expected.erase(
      std::remove_if(expected.begin(), expected.end(), ::isspace),
      expected.end());

  // Check JSON -> String.
  EXPECT_EQ(expected, string(jsonify(JSON::Protobuf(message))));
}


TEST(ProtobufTest, JsonifyMap) {
  tests::MapMessage message;
  (*message.mutable_string_to_bool())["key1"] = true;
  (*message.mutable_string_to_bool())["key2"] = false;
  (*message.mutable_string_to_bytes())["key"] = "bytes";
  (*message.mutable_string_to_double())["key"] = 1.0;
  (*message.mutable_string_to_enum())["key"] = tests::ONE;
  (*message.mutable_string_to_float())["key"] = 1.0;
  (*message.mutable_string_to_string())["key1"] = "value1";
  (*message.mutable_string_to_string())["key2"] = "value2";

  tests::Nested nested;
  nested.set_str("nested");
  (*message.mutable_string_to_nested())["key"] = nested;

  // These numbers are equal or close to the integer limits.
  (*message.mutable_string_to_int32())["key"] = -2147483647;
  (*message.mutable_string_to_int64())["key"] = -9223372036854775807;
  (*message.mutable_string_to_sint32())["key"] = -1234567890;
  (*message.mutable_string_to_sint64())["key"] = -1234567890123456789;
  (*message.mutable_string_to_uint32())["key"] = 4294967295;
  (*message.mutable_string_to_uint64())["key"] = 9223372036854775807;

  (*message.mutable_bool_to_string())[true] = "value1";
  (*message.mutable_bool_to_string())[false] = "value2";

  // These numbers are equal or close to the integer limits.
  (*message.mutable_int32_to_string())[-2147483647] = "value";
  (*message.mutable_int64_to_string())[-9223372036854775807] = "value";
  (*message.mutable_sint32_to_string())[-1234567890] = "value";
  (*message.mutable_sint64_to_string())[-1234567890123456789] = "value";
  (*message.mutable_uint32_to_string())[4294967295] = "value";
  (*message.mutable_uint64_to_string())[9223372036854775807] = "value";

  // The keys are in alphabetical order.
  // The value of `string_to_bytes` is base64 encoded.
  string expected =
      R"~(
    {
      "bool_to_string": {
        "false": "value2",
        "true": "value1"
      },
      "int32_to_string": {
        "-2147483647": "value"
      },
      "int64_to_string": {
        "-9223372036854775807": "value"
      },
      "sint32_to_string": {
        "-1234567890": "value"
      },
      "sint64_to_string": {
        "-1234567890123456789": "value"
      },
      "string_to_bool": {
        "key1": true,
        "key2": false
      },
      "string_to_bytes": {
        "key": "Ynl0ZXM="
      },
      "string_to_double": {
        "key": 1.0
      },
      "string_to_enum": {
        "key": "ONE"
      },
      "string_to_float": {
        "key": 1.0
      },
      "string_to_int32": {
        "key": -2147483647
      },
      "string_to_int64": {
        "key": -9223372036854775807
      },
      "string_to_nested": {
        "key": {
          "str": "nested"
        }
      },
      "string_to_sint32": {
        "key": -1234567890
      },
      "string_to_sint64": {
        "key": -1234567890123456789
      },
      "string_to_string": {
        "key1": "value1",
        "key2": "value2"
      },
      "string_to_uint32": {
        "key": 4294967295
      },
      "string_to_uint64": {
        "key": 9223372036854775807
      },
      "uint32_to_string": {
        "4294967295": "value"
      },
      "uint64_to_string": {
        "9223372036854775807": "value"
      }
    })~";

  // Remove ' ' and '\n' from `expected` so that we can compare
  // it with the JSON string parsed from protobuf message.
  expected.erase(
      std::remove_if(expected.begin(), expected.end(), ::isspace),
      expected.end());

  JSON::Object object = JSON::protobuf(message);
  EXPECT_EQ(expected, fmt::format("{}", object));

  // Test parsing too.
  Try<tests::MapMessage> parse = protobuf::parse<tests::MapMessage>(object);
  ASSERT_SOME(parse);

  EXPECT_EQ(object, JSON::protobuf(parse.get()));
}
