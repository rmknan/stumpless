// SPDX-License-Identifier: Apache-2.0

/*
 * Copyright 2018-2020 Joel E. Anderson
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

#include <stddef.h>
#include <stdio.h>
#include <stumpless/target.h>
#include <stumpless/target/stream.h>
#include "private/config/wrapper/locale.h"
#include "private/config/wrapper/thread_safety.h"
#include "private/error.h"
#include "private/inthelper.h"
#include "private/memory.h"
#include "private/target.h"
#include "private/target/stream.h"
#include "private/validate.h"

void
stumpless_close_stream_target( const struct stumpless_target *target ) {
  if( !target ) {
    raise_argument_empty( L10N_NULL_ARG_ERROR_MESSAGE( "target" ) );
    return;
  }

  if( target->type != STUMPLESS_STREAM_TARGET ) {
    raise_target_incompatible( L10N_INVALID_TARGET_TYPE_ERROR_MESSAGE );
    return;
  }

  clear_error(  );
  destroy_stream_target( target->id );
  destroy_target( target );
}

struct stumpless_target *
stumpless_open_stderr_target( const char *name ) {
  return stumpless_open_stream_target( name, stderr );
}

struct stumpless_target *
stumpless_open_stdout_target( const char *name ) {
  return stumpless_open_stream_target( name, stdout );
}

struct stumpless_target *
stumpless_open_stream_target( const char *name, FILE *stream ) {
  struct stumpless_target *target;

  clear_error(  );

  VALIDATE_ARG_NOT_NULL( name );
  VALIDATE_ARG_NOT_NULL( stream );

  target = new_target( STUMPLESS_STREAM_TARGET, name );

  if( !target ) {
    goto fail;
  }

  target->id = new_stream_target( stream );
  if( !target->id ) {
    goto fail_id;
  }

  stumpless_set_current_target( target );
  return target;

fail_id:
  destroy_target( target );
fail:
  return NULL;
}

/* private definitions */

void
destroy_stream_target( const struct stream_target *target ) {
  config_destroy_mutex( &target->stream_mutex );
  free_mem( target );
}

struct stream_target *
new_stream_target( FILE *stream ) {
  struct stream_target *target;

  target = alloc_mem( sizeof( *target ) );
  if( !target ) {
    return NULL;
  }

  config_init_mutex( &target->stream_mutex );
  target->stream = stream;

  return target;
}

int
sendto_stream_target( struct stream_target *target,
                      const char *msg,
                      size_t msg_length ) {
  size_t fwrite_result;

  config_lock_mutex( &target->stream_mutex );
  fwrite_result = fwrite( msg, sizeof( char ), msg_length, target->stream );
  config_unlock_mutex( &target->stream_mutex );

  if( fwrite_result != msg_length ) {
    goto write_failure;
  }

  return cap_size_t_to_int( fwrite_result + 1 );

write_failure:
  raise_stream_write_failure(  );
  return -1;
}
