// SPDX-License-Identifier: Apache-2.0

/*
 * Copyright 2018-2024 Joel E. Anderson
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstdio>
#include <fstream>
#include <stddef.h>
#include <stdlib.h>
#include <string>
#include <stumpless.h>
#include <gtest/gtest.h>
#include "test/helper/assert.hpp"
#include "test/helper/fixture.hpp"
#include "test/helper/rfc5424.hpp"

namespace {
  int
  basic_log_function( const struct stumpless_target *target,
                      const struct stumpless_entry *entry ) {
    EXPECT_NOT_NULL( target );
    EXPECT_NOT_NULL( entry );
    return 0;
  }

  class StreamTargetTest : public::testing::Test {
    protected:
      const char *filename = "testfile.log";
      FILE *stream;
      struct stumpless_target *target;
      struct stumpless_entry *basic_entry;

    virtual void
    SetUp( void ) {
      struct stumpless_element *element;
      struct stumpless_param *param;

      stream = fopen( filename, "w+" );

      target = stumpless_open_stream_target( filename, stream );

      stumpless_set_target_default_app_name( target, "buffer-target-test" );
      stumpless_set_target_default_msgid( target, "default-message" );

      basic_entry = stumpless_new_entry( STUMPLESS_FACILITY_USER,
                                         STUMPLESS_SEVERITY_INFO,
                                        "stumpless-unit-test",
                                        "basic-entry",
                                        "basic test message" );

      element = stumpless_new_element( "basic-element" );
      stumpless_add_element( basic_entry, element );

      param = stumpless_new_param( "basic-param-name", "basic-param-value" );
      stumpless_add_param( element, param );
    }

    virtual void
    TearDown( void ) {
      stumpless_destroy_entry_and_contents( basic_entry );
      stumpless_close_stream_target( target );

      fclose( stream );
      remove( filename );
    }
  };

  TEST_F( StreamTargetTest, AddEntry ) {
    int result;

    SCOPED_TRACE( "EntryTargetTest.AddEntry" );

    result = stumpless_add_entry( target, basic_entry );
    EXPECT_GE( result, 0 );
    EXPECT_NO_ERROR;
  }

  /* non-fixture tests */

  TEST( StreamTargetCloseTest, Generic ) {
    const char *filename = "genericclosetest.log";
    FILE *stream;
    struct stumpless_target *target;

    stream = fopen( filename, "w+" );
    ASSERT_NOT_NULL( stream );

    target = stumpless_open_stream_target( filename, stream );
    EXPECT_NO_ERROR;
    EXPECT_NOT_NULL( target );
    EXPECT_EQ( stumpless_get_current_target(  ), target );

    stumpless_close_target( target );
    EXPECT_NO_ERROR;

    EXPECT_EQ( stumpless_get_current_target(  ),
               stumpless_get_default_target(  ) );
    EXPECT_STRNE( stumpless_get_current_target(  )->name,
                  filename );

    stumpless_free_all(  );
    remove( filename );
  }

  TEST( StreamTargetCloseTest, NullTarget ) {
    stumpless_close_stream_target( NULL );
    EXPECT_ERROR_ID_EQ( STUMPLESS_ARGUMENT_EMPTY );
  }

  TEST( StreamTargetCloseTest, WrongTargetType ) {
    const struct stumpless_target *target;

    target = stumpless_open_function_target( "not-a-stream-target",
                                             basic_log_function );
    ASSERT_NOT_NULL( target );

    stumpless_close_stream_target( target );
    EXPECT_ERROR_ID_EQ( STUMPLESS_TARGET_INCOMPATIBLE );

    stumpless_close_function_target( target );
  }

  TEST( StreamTargetFormat, NewlineSeparator ) {
    struct stumpless_target *target;
    struct stumpless_entry *entry;
    struct stumpless_element *element;
    struct stumpless_param *param;
    const char *filename = "filetargetformattest.log";
    FILE *stream;
    size_t line_count = 3;
    size_t i;

    stream = fopen( filename, "w+" );
    ASSERT_NOT_NULL( stream );

    target = stumpless_open_stream_target( filename, stream );
    ASSERT_NOT_NULL( target );

    entry = stumpless_new_entry( STUMPLESS_FACILITY_USER,
                                 STUMPLESS_SEVERITY_INFO,
                                "stumpless-unit-test",
                                "basic-entry",
                                "basic test message" );
    ASSERT_TRUE( entry != NULL );

    element = stumpless_new_element( "basic-element" );
    ASSERT_NOT_NULL( element );
    stumpless_add_element( entry, element );

    param = stumpless_new_param( "basic-param-name", "basic-param-value" );
    ASSERT_NOT_NULL( param );
    stumpless_add_param( element, param );

    for( i = 0; i < line_count; i++ ) {
      stumpless_add_entry( target, entry );
    }

    stumpless_destroy_entry_and_contents( entry );
    stumpless_close_stream_target( target );
    fclose( stream );

    std::ifstream infile( filename );
    std::string line;
    i = 0;
    while( std::getline( infile, line ) ) {
      TestRFC5424Compliance( line.c_str() );
      i++;
    }

    EXPECT_EQ( i, line_count );

    remove( filename );
  }

  TEST( StreamTargetOpenTest, MallocFailure ) {
    struct stumpless_target *target;
    const char *filename = "open-malloc-fail.log";
    FILE *stream;
    void *(*set_malloc_result)(size_t);

    set_malloc_result = stumpless_set_malloc( [](size_t size)->void *{ return NULL; } );
    ASSERT_NOT_NULL( set_malloc_result );

    stream = fopen( filename, "w+" );
    EXPECT_NOT_NULL( stream );

    target = stumpless_open_stream_target( filename, stream );
    EXPECT_NULL( target );
    EXPECT_ERROR_ID_EQ( STUMPLESS_MEMORY_ALLOCATION_FAILURE );

    fclose( stream );
    remove( filename );

    set_malloc_result = stumpless_set_malloc( malloc );
    ASSERT_TRUE( set_malloc_result == malloc );
  }

  TEST( StreamTargetOpenTest, NullName ) {
    struct stumpless_target *target;
    const char *filename = "null-name.log";
    FILE *stream;

    stream = fopen( filename, "w+" );
    ASSERT_NOT_NULL( stream );

    target = stumpless_open_stream_target( NULL, stream );
    EXPECT_NULL( target );
    EXPECT_ERROR_ID_EQ( STUMPLESS_ARGUMENT_EMPTY );

    fclose( stream );
    remove( filename );
  }

  TEST( StreamTargetOpenTest, NullStream ) {
    struct stumpless_target *target;

    target = stumpless_open_stream_target( "no-stream-provided", NULL );
    EXPECT_NULL( target );
    EXPECT_ERROR_ID_EQ( STUMPLESS_ARGUMENT_EMPTY );
  }

  TEST( StreamTargetStderrTest, NullName ) {
    const struct stumpless_target *target;

    target = stumpless_open_stderr_target( NULL );
    EXPECT_NULL( target );
    EXPECT_ERROR_ID_EQ( STUMPLESS_ARGUMENT_EMPTY );
  }

  TEST( StreamTargetStdoutTest, NullName ) {
    const struct stumpless_target *target;

    target = stumpless_open_stdout_target( NULL );
    EXPECT_NULL( target );
    EXPECT_ERROR_ID_EQ( STUMPLESS_ARGUMENT_EMPTY );
  }

  TEST( StreamTargetWriteTest, ReadOnlyStream ) {
    struct stumpless_target *target;
    struct stumpless_entry *basic_entry;
    const char *filename = "null-name.log";
    FILE *stream;
    int result;

    basic_entry = create_entry();
    ASSERT_NOT_NULL( basic_entry );

    stream = fopen( filename, "w+" );
    fclose( stream );

    stream = fopen( filename, "r" );
    ASSERT_NOT_NULL( stream );

    target = stumpless_open_stream_target( filename, stream );
    ASSERT_NOT_NULL( target );

    result = stumpless_add_entry( target, basic_entry );
    EXPECT_LT( result, 0 );
    EXPECT_ERROR_ID_EQ( STUMPLESS_STREAM_WRITE_FAILURE );

    stumpless_destroy_entry_and_contents( basic_entry );
    fclose( stream );
    stumpless_close_stream_target( target );

    remove( filename );
  }
}
