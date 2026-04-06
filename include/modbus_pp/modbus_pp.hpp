#pragma once

/// @file modbus_pp.hpp
/// @brief Umbrella header — includes all modbus_pp headers.

// Core
#include "core/types.hpp"
#include "core/endian.hpp"
#include "core/error.hpp"
#include "core/result.hpp"
#include "core/timestamp.hpp"
#include "core/function_codes.hpp"
#include "core/pdu.hpp"

// Register Map
#include "register_map/register_descriptor.hpp"
#include "register_map/register_map.hpp"
#include "register_map/type_codec.hpp"
#include "register_map/batch_request.hpp"

// Transport
#include "transport/transport.hpp"
#include "transport/frame_codec.hpp"
#include "transport/loopback_transport.hpp"
#include "transport/tcp_transport.hpp"
#include "transport/rtu_transport.hpp"

// Security
#include "security/hmac_auth.hpp"
#include "security/session.hpp"
#include "security/credential_store.hpp"

// Pipeline
#include "pipeline/pipeline.hpp"
#include "pipeline/correlation_id.hpp"
#include "pipeline/request_queue.hpp"

// Pub/Sub
#include "pubsub/subscription.hpp"
#include "pubsub/publisher.hpp"
#include "pubsub/subscriber.hpp"

// Discovery
#include "discovery/device_info.hpp"
#include "discovery/scanner.hpp"

// Client/Server Facades
#include "client/client.hpp"
#include "client/server.hpp"
