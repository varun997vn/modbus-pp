/**
 * Creating a sidebar enables you to:
 - create an ordered group of docs
 - render a set of docs in the sidebar
 - provide next/previous navigation

 The sidebars can be generated from the file structure, or explicitly defined here.

 Create as many sidebars as you want.
 */

// @ts-check

/** @type {import('@docusaurus/plugin-content-docs').SidebarsConfig} */
const sidebars = {
  tutorialSidebar: {
    "Quick Start": [
      "quick-start/introduction",
      "quick-start/installation",
      "quick-start/hello-world",
      "quick-start/next-steps",
    ],
    "Core Concepts": [
      "concepts/overview",
      "concepts/modbus-fundamentals",
      "concepts/problems-solved",
      "concepts/design-philosophy",
      "concepts/module-tour",
    ],
    "Features & Examples": [
      "features/overview",
      "features/typed-registers",
      "features/security-hmac",
      "features/pipelining",
      "features/pubsub",
      "features/discovery",
      "features/batch-operations",
      "features/byte-order-safety",
    ],
    Examples: [
      "examples/basic-client-server",
      "examples/typed-registers-example",
      "examples/failure-propagation",
      "examples/pipeline-throughput",
      "examples/pubsub-events",
      "examples/security-authentication",
      "examples/endian-byte-orders",
    ],
    "Architecture & Design": [
      "architecture/overview",
      "architecture/module-structure",
      "architecture/wire-format",
      "architecture/design-patterns",
      "architecture/transport-abstraction",
      "architecture/error-handling",
    ],
    "API Reference": [
      "api/core-module",
      "api/transport-module",
      "api/security-module",
      "api/pipeline-module",
      "api/pubsub-module",
      "api/discovery-module",
      "api/register-map-module",
      "api/client-server-facades",
    ],
    "Testing & Benchmarks": [
      "testing/overview",
      "testing/test-coverage",
      "benchmarks",
    ],
    Reference: ["glossary", "faq", "license"],
  },
};

module.exports = sidebars;
