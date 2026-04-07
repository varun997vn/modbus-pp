// @ts-check
// `@type` JSDoc annotations allow editor autocompletion and type checking
// (when paired with `@ts-check`).
// There are two ways to add type annotations for your Docusaurus config.
// Can use JSDoc in a file with `@ts-check` and `@type` annotations:
/** @type {import('@docusaurus/types').Config} */

const config = {
  title: "modbus_pp",
  tagline: "Modern C++17 Modbus Library — Solving 10 Protocol Limitations",
  favicon: "img/favicon.ico",

  // Set the production url of your site here
  url: "https://varun997vn.github.io",
  // Set the /<baseUrl>/ pathname under which your site is served
  // For GitHub pages deployment, it is often '/<projectName>/'
  baseUrl: "/modbus-pp/",

  // GitHub pages deployment config.
  // If you aren't using GitHub pages, you don't need these.
  organizationName: "varun997vn",
  projectName: "modbus-pp",
  deploymentBranch: "gh-pages",

  onBrokenLinks: "warn",
  onBrokenMarkdownLinks: "warn",

  // Even if you don't use internationalization, you can use this field to set
  // useful metadata like html lang. For example, if your site is Chinese, you
  // may want to replace "en" with "zh-Hans".
  i18n: {
    defaultLocale: "en",
    locales: ["en"],
  },

  presets: [
    [
      "classic",
      /** @type {import('@docusaurus/preset-classic').Options} */
      ({
        docs: {
          sidebarPath: "./sidebars.js",
          editUrl: "https://github.com/varun997vn/modbus-pp/tree/main/docusaurus",
          showLastUpdateAuthor: false,
          showLastUpdateTime: false,
        },
        blog: false,
        theme: {
          customCss: "./src/css/custom.css",
        },
      }),
    ],
  ],

  themeConfig:
    /** @type {import('@docusaurus/preset-classic').ThemeConfig} */
    ({
      image: "img/docusaurus-social-card.jpg",
      navbar: {
        title: "modbus_pp",
        logo: {
          alt: "modbus_pp Logo",
          src: "img/logo.svg",
        },
        items: [
          {
            type: "docSidebar",
            sidebarId: "tutorialSidebar",
            position: "left",
            label: "Docs",
          },
          {
            href: "https://github.com/varun997vn/modbus-pp",
            label: "GitHub",
            position: "right",
          },
        ],
      },
      footer: {
        style: "dark",
        links: [
          {
            title: "Documentation",
            items: [
              {
                label: "Quick Start",
                to: "/docs/quick-start/introduction",
              },
              {
                label: "Concepts",
                to: "/docs/concepts/overview",
              },
              {
                label: "Features",
                to: "/docs/features/overview",
              },
              {
                label: "Architecture",
                to: "/docs/architecture/overview",
              },
            ],
          },
          {
            title: "References",
            items: [
              {
                label: "API Reference",
                to: "/docs/api/core-module",
              },
              {
                label: "Benchmarks",
                to: "/docs/benchmarks",
              },
              {
                label: "Test Coverage",
                to: "/docs/testing",
              },
            ],
          },
          {
            title: "More",
            items: [
              {
                label: "GitHub Repository",
                href: "https://github.com/varun997vn/modbus-pp",
              },
              {
                label: "MIT License",
                to: "/docs/license",
              },
              {
                label: "Examples",
                to: "/docs/examples/basic-client-server",
              },
            ],
          },
        ],
        copyright: `Copyright © ${new Date().getFullYear()} modbus_pp. Built with Docusaurus.`,
      },
      prism: {
        additionalLanguages: ["cpp", "cmake", "bash"],
        theme: require("prism-react-renderer").themes.github,
        darkTheme: require("prism-react-renderer").themes.dracula,
      },
    }),
};

module.exports = config;
