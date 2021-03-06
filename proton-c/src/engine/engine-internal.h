#ifndef _PROTON_ENGINE_INTERNAL_H
#define _PROTON_ENGINE_INTERNAL_H 1

/*
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 */

#include <proton/object.h>
#include <proton/buffer.h>
#include <proton/engine.h>
#include <proton/types.h>
#include "../dispatcher/dispatcher.h"
#include "../util.h"

typedef enum pn_endpoint_type_t {CONNECTION, SESSION, SENDER, RECEIVER} pn_endpoint_type_t;

typedef struct pn_endpoint_t pn_endpoint_t;

struct pn_condition_t {
  pn_string_t *name;
  pn_string_t *description;
  pn_data_t *info;
};

struct pn_endpoint_t {
  pn_endpoint_type_t type;
  pn_state_t state;
  pn_error_t *error;
  pn_condition_t condition;
  pn_condition_t remote_condition;
  pn_endpoint_t *endpoint_next;
  pn_endpoint_t *endpoint_prev;
  pn_endpoint_t *transport_next;
  pn_endpoint_t *transport_prev;
  bool modified;
};

typedef struct {
  pn_sequence_t id;
  bool sent;
  bool init;
} pn_delivery_state_t;

typedef struct {
  pn_sequence_t next;
  pn_hash_t *deliveries;
} pn_delivery_map_t;

typedef struct {
  // XXX: stop using negative numbers
  uint32_t local_handle;
  uint32_t remote_handle;
  pn_sequence_t delivery_count;
  pn_sequence_t link_credit;
} pn_link_state_t;

typedef struct {
  // XXX: stop using negative numbers
  uint16_t local_channel;
  uint16_t remote_channel;
  bool incoming_init;
  pn_delivery_map_t incoming;
  pn_delivery_map_t outgoing;
  pn_sequence_t incoming_transfer_count;
  pn_sequence_t incoming_window;
  pn_sequence_t remote_incoming_window;
  pn_sequence_t outgoing_transfer_count;
  pn_sequence_t outgoing_window;
  pn_hash_t *local_handles;
  pn_hash_t *remote_handles;

  uint64_t disp_code;
  bool disp_settled;
  bool disp_type;
  pn_sequence_t disp_first;
  pn_sequence_t disp_last;
  bool disp;
} pn_session_state_t;

#define SCRATCH (1024)

#include <proton/sasl.h>
#include <proton/ssl.h>

typedef struct pn_io_layer_t {
  void *context;
  struct pn_io_layer_t *next;
  ssize_t (*process_input)(struct pn_io_layer_t *io_layer, const char *, size_t);
  ssize_t (*process_output)(struct pn_io_layer_t *io_layer, char *, size_t);
  pn_timestamp_t (*process_tick)(struct pn_io_layer_t *io_layer, pn_timestamp_t);
  size_t (*buffered_output)(struct pn_io_layer_t *);  // how much output is held
  size_t (*buffered_input)(struct pn_io_layer_t *);   // how much input is held
} pn_io_layer_t;

struct pn_transport_t {
  pn_tracer_t *tracer;
  size_t header_count;
  pn_sasl_t *sasl;
  pn_ssl_t *ssl;
  pn_connection_t *connection;
  pn_dispatcher_t *disp;
  bool open_sent;
  bool open_rcvd;
  bool close_sent;
  bool close_rcvd;
  char *remote_container;
  char *remote_hostname;
  pn_data_t *remote_offered_capabilities;
  pn_data_t *remote_desired_capabilities;
  pn_data_t *remote_properties;
  pn_data_t *disp_data;
  //#define PN_DEFAULT_MAX_FRAME_SIZE (16*1024)
#define PN_DEFAULT_MAX_FRAME_SIZE (0)  /* for now, allow unlimited size */
  uint32_t   local_max_frame;
  uint32_t   remote_max_frame;
  pn_condition_t remote_condition;

#define PN_IO_SSL  0
#define PN_IO_SASL 1
#define PN_IO_AMQP 2
#define PN_IO_LAYER_CT (PN_IO_AMQP+1)
  pn_io_layer_t io_layers[PN_IO_LAYER_CT];

  /* dead remote detection */
  pn_millis_t local_idle_timeout;
  pn_timestamp_t dead_remote_deadline;
  uint64_t last_bytes_input;

  /* keepalive */
  pn_millis_t remote_idle_timeout;
  pn_timestamp_t keepalive_deadline;
  uint64_t last_bytes_output;

  pn_error_t *error;
  pn_hash_t *local_channels;
  pn_hash_t *remote_channels;
  pn_string_t *scratch;

  /* statistics */
  uint64_t bytes_input;
  uint64_t bytes_output;

  /* output buffered for send */
  size_t output_size;
  size_t output_pending;
  char *output_buf;

  /* input from peer */
  size_t input_size;
  size_t input_pending;
  char *input_buf;
  bool tail_closed;      // input stream closed by driver

  void *context;
};

struct pn_connection_t {
  pn_endpoint_t endpoint;
  pn_endpoint_t *endpoint_head;
  pn_endpoint_t *endpoint_tail;
  pn_endpoint_t *transport_head;
  pn_endpoint_t *transport_tail;
  pn_list_t *sessions;
  pn_transport_t *transport;
  pn_delivery_t *work_head;
  pn_delivery_t *work_tail;
  pn_delivery_t *tpwork_head;
  pn_delivery_t *tpwork_tail;
  pn_string_t *container;
  pn_string_t *hostname;
  pn_data_t *offered_capabilities;
  pn_data_t *desired_capabilities;
  pn_data_t *properties;
  void *context;
};

struct pn_session_t {
  pn_endpoint_t endpoint;
  pn_connection_t *connection;
  pn_list_t *links;
  void *context;
  size_t incoming_capacity;
  pn_sequence_t incoming_bytes;
  pn_sequence_t outgoing_bytes;
  pn_sequence_t incoming_deliveries;
  pn_sequence_t outgoing_deliveries;
  pn_session_state_t state;
};

struct pn_terminus_t {
  pn_terminus_type_t type;
  pn_string_t *address;
  pn_durability_t durability;
  pn_expiry_policy_t expiry_policy;
  pn_seconds_t timeout;
  bool dynamic;
  pn_distribution_mode_t distribution_mode;
  pn_data_t *properties;
  pn_data_t *capabilities;
  pn_data_t *outcomes;
  pn_data_t *filter;
};

struct pn_link_t {
  pn_endpoint_t endpoint;
  pn_string_t *name;
  pn_session_t *session;
  pn_terminus_t source;
  pn_terminus_t target;
  pn_terminus_t remote_source;
  pn_terminus_t remote_target;
  pn_delivery_t *unsettled_head;
  pn_delivery_t *unsettled_tail;
  pn_delivery_t *current;
  pn_delivery_t *settled_head;
  pn_delivery_t *settled_tail;
  uint8_t snd_settle_mode;
  uint8_t rcv_settle_mode;
  uint8_t remote_snd_settle_mode;
  uint8_t remote_rcv_settle_mode;
  size_t unsettled_count;
  pn_sequence_t available;
  pn_sequence_t credit;
  pn_sequence_t queued;
  bool drain_flag_mode; // receiver only
  bool drain;
  int drained; // number of drained credits
  void *context;
  pn_link_state_t state;
};

struct pn_disposition_t {
  uint64_t type;
  pn_data_t *data;
  pn_data_t *annotations;
  pn_condition_t condition;
  uint32_t section_number;
  uint64_t section_offset;
  bool failed;
  bool undeliverable;
  bool settled;
};

struct pn_delivery_t {
  pn_link_t *link;
  pn_buffer_t *tag;
  pn_disposition_t local;
  pn_disposition_t remote;
  bool updated;
  bool settled; // tracks whether we're in the unsettled list or not
  pn_delivery_t *unsettled_next;
  pn_delivery_t *unsettled_prev;
  pn_delivery_t *settled_next;
  pn_delivery_t *settled_prev;
  pn_delivery_t *work_next;
  pn_delivery_t *work_prev;
  bool work;
  pn_delivery_t *tpwork_next;
  pn_delivery_t *tpwork_prev;
  bool tpwork;
  pn_buffer_t *bytes;
  bool done;
  void *context;
  pn_delivery_state_t state;
};

#define PN_SET_LOCAL(OLD, NEW)                                          \
  (OLD) = ((OLD) & PN_REMOTE_MASK) | (NEW)

#define PN_SET_REMOTE(OLD, NEW)                                         \
  (OLD) = ((OLD) & PN_LOCAL_MASK) | (NEW)

void pn_link_dump(pn_link_t *link);

void pn_dump(pn_connection_t *conn);
void pn_transport_sasl_init(pn_transport_t *transport);

ssize_t pn_io_layer_input_passthru(pn_io_layer_t *, const char *, size_t );
ssize_t pn_io_layer_output_passthru(pn_io_layer_t *, char *, size_t );
pn_timestamp_t pn_io_layer_tick_passthru(pn_io_layer_t *, pn_timestamp_t);

void pn_condition_init(pn_condition_t *condition);
void pn_condition_tini(pn_condition_t *condition);
void pn_modified(pn_connection_t *connection, pn_endpoint_t *endpoint);
void pn_real_settle(pn_delivery_t *delivery);
void pn_clear_tpwork(pn_delivery_t *delivery);
void pn_work_update(pn_connection_t *connection, pn_delivery_t *delivery);
void pn_clear_modified(pn_connection_t *connection, pn_endpoint_t *endpoint);

#endif /* engine-internal.h */
