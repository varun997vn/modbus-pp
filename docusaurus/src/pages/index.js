import React from "react";
import Link from "@docusaurus/Link";
import useDocusaurusContext from "@docusaurus/useDocusaurusContext";
import Layout from "@theme/Layout";
import styles from "./index.module.css";

const features = [
  {
    title: "Type-Safe Registers",
    description:
      "Compile-time validated register descriptors for Float32, Int64, String and more. Catch byte-order bugs at build time, not in production.",
  },
  {
    title: "HMAC-SHA256 Security",
    description:
      "Challenge-response authentication baked into every frame. Retrofit security onto any Modbus deployment without changing slave firmware.",
  },
  {
    title: "Request Pipelining",
    description:
      "Send multiple requests without waiting for replies. Correlation IDs match responses to requests — up to 8x throughput on high-latency links.",
  },
  {
    title: "Pub/Sub Events",
    description:
      "Subscribe to register changes with dead-band filtering. Replace polling loops with event-driven callbacks and cut CPU load dramatically.",
  },
  {
    title: "Device Discovery",
    description:
      "Broadcast scans that detect Modbus devices and their capabilities automatically. No more manually configuring device addresses.",
  },
  {
    title: "100% Wire Compatible",
    description:
      "Every enhancement is backward compatible. Talk to any standard Modbus RTU/TCP device with zero protocol changes on the slave side.",
  },
];

function Feature({ title, description }) {
  return (
    <div className={styles.feature}>
      <h3>{title}</h3>
      <p>{description}</p>
    </div>
  );
}

const problems = [
  ["No authentication", "HMAC-SHA256 challenge-response handshake"],
  ["Weak data types", "Compile-time typed register descriptors"],
  ["Poll-based only", "Event-driven Pub/Sub with dead-band filtering"],
  ["253-byte frame limit", "Extended payloads — kilobytes per frame"],
  ["No timestamps", "Microsecond-precision timestamps in every frame"],
  ["Sequential throughput", "Pipelining with correlation IDs (8x speedup)"],
  ["Poor error codes", "25+ error codes with std::error_code integration"],
  ["Manual discovery", "Broadcast scanning with capability detection"],
  ["Batch inflexibility", "Automatic range merging for batch operations"],
  ["Byte order chaos", "4 byte orders, compile-time selection, zero runtime cost"],
];

export default function Home() {
  const { siteConfig } = useDocusaurusContext();
  return (
    <Layout
      title={siteConfig.title}
      description="Modern C++17 Modbus library solving 10 protocol limitations while keeping 100% wire compatibility"
    >
      {/* Hero */}
      <header className={styles.hero}>
        <div className={styles.heroInner}>
          <div className={styles.heroBadge}>C++17 · MIT License · v1.0.0</div>
          <h1 className={styles.heroTitle}>modbus_pp</h1>
          <p className={styles.heroSubtitle}>
            A modern C++17 Modbus library that solves{" "}
            <strong>10 fundamental protocol limitations</strong> — while staying
            100% wire-compatible with every standard Modbus device.
          </p>
          <div className={styles.heroStats}>
            <div className={styles.stat}><span>~4,300</span><label>lines of C++</label></div>
            <div className={styles.stat}><span>13</span><label>test suites</label></div>
            <div className={styles.stat}><span>7</span><label>examples</label></div>
            <div className={styles.stat}><span>5</span><label>benchmarks</label></div>
          </div>
          <div className={styles.heroButtons}>
            <Link className={styles.btnPrimary} to="/docs/quick-start/introduction">
              Get Started
            </Link>
            <Link className={styles.btnSecondary} to="/docs/concepts/overview">
              Core Concepts
            </Link>
            <Link
              className={styles.btnGhost}
              href="https://github.com/varun997vn/modbus-pp"
            >
              GitHub
            </Link>
          </div>
        </div>
      </header>

      <main>
        {/* Quick install */}
        <section className={styles.install}>
          <div className={styles.container}>
            <h2>Get started in seconds</h2>
            <pre className={styles.installCode}>{`git clone https://github.com/varun997vn/modbus-pp.git
cd modbus-pp && mkdir build && cd build
cmake .. && make -j$(nproc)`}</pre>
          </div>
        </section>

        {/* Features grid */}
        <section className={styles.featuresSection}>
          <div className={styles.container}>
            <h2>What makes modbus_pp different</h2>
            <div className={styles.featuresGrid}>
              {features.map((f) => (
                <Feature key={f.title} {...f} />
              ))}
            </div>
          </div>
        </section>

        {/* 10 Problems table */}
        <section className={styles.problemsSection}>
          <div className={styles.container}>
            <h2>10 Problems. 10 Solutions.</h2>
            <p className={styles.problemsSubtitle}>
              Every enhancement is a wire-level extension — standard Modbus
              slaves see a normal request and respond normally.
            </p>
            <table className={styles.problemsTable}>
              <thead>
                <tr>
                  <th>Protocol Problem</th>
                  <th>modbus_pp Solution</th>
                </tr>
              </thead>
              <tbody>
                {problems.map(([problem, solution]) => (
                  <tr key={problem}>
                    <td>{problem}</td>
                    <td>{solution}</td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </section>

        {/* CTA */}
        <section className={styles.cta}>
          <div className={styles.container}>
            <h2>Ready to build?</h2>
            <p>
              Read the docs, explore the examples, or dive straight into the API
              reference.
            </p>
            <div className={styles.ctaButtons}>
              <Link className={styles.btnPrimary} to="/docs/quick-start/installation">
                Installation Guide
              </Link>
              <Link className={styles.btnSecondary} to="/docs/examples/basic-client-server">
                View Examples
              </Link>
              <Link className={styles.btnSecondary} to="/docs/api/core-module">
                API Reference
              </Link>
            </div>
          </div>
        </section>
      </main>
    </Layout>
  );
}
