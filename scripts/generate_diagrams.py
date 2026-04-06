#!/usr/bin/env python3
"""
Generate architecture and diagram assets for modbus_pp documentation.

Generates SVG diagrams without external dependencies.
"""

import os
from pathlib import Path


def create_directories():
    """Create required directories."""
    output_dir = Path(__file__).parent.parent / 'docusaurus' / 'static' / 'diagrams'
    output_dir.mkdir(parents=True, exist_ok=True)
    return output_dir


def generate_pdu_layout_diagram(output_dir):
    """Generate extended PDU layout as SVG (ASCII art style)."""
    svg_content = '''<svg xmlns="http://www.w3.org/2000/svg" width="1000" height="400" viewBox="0 0 1000 400">
  <defs>
    <style>
      .field-box { fill: #E8F4F8; stroke: #333; stroke-width: 2; }
      .field-label { font-family: monospace; font-size: 12px; font-weight: bold; }
      .field-size { font-family: monospace; font-size: 10px; fill: #666; }
      .header-text { font-family: Arial; font-size: 14px; font-weight: bold; }
      .section-title { font-family: Arial; font-size: 12px; font-weight: bold; fill: #333; }
    </style>
  </defs>

  <!-- Title -->
  <text x="500" y="25" class="header-text" text-anchor="middle">Extended modbus_pp PDU Layout (FC = 0x6E)</text>

  <!-- Standard PDU -->
  <text x="20" y="65" class="section-title">Standard PDU (backward compatible):</text>
  
  <rect x="20" y="80" width="80" height="40" class="field-box"/>
  <text x="60" y="103" class="field-label" text-anchor="middle">FC</text>
  <text x="60" y="118" class="field-size" text-anchor="middle">1B</text>

  <rect x="100" y="80" width="200" height="40" class="field-box"/>
  <text x="200" y="103" class="field-label" text-anchor="middle">Data</text>
  <text x="200" y="118" class="field-size" text-anchor="middle">0-252 B</text>

  <!-- Extended PDU -->
  <text x="20" y="175" class="section-title">Extended PDU (modbus_pp):</text>

  <rect x="20" y="190" width="40" height="40" class="field-box" fill="#FFB84D"/>
  <text x="40" y="213" class="field-label" text-anchor="middle">0x6E</text>
  <text x="40" y="228" class="field-size" text-anchor="middle">FC</text>

  <rect x="60" y="190" width="40" height="40" class="field-box"/>
  <text x="80" y="213" class="field-label" text-anchor="middle">Ver</text>
  <text x="80" y="228" class="field-size" text-anchor="middle">1B</text>

  <rect x="100" y="190" width="50" height="40" class="field-box"/>
  <text x="125" y="213" class="field-label" text-anchor="middle">Flags</text>
  <text x="125" y="228" class="field-size" text-anchor="middle">2B</text>

  <rect x="150" y="190" width="60" height="40" class="field-box"/>
  <text x="180" y="213" class="field-label" text-anchor="middle">CorrID</text>
  <text x="180" y="228" class="field-size" text-anchor="middle">2B</text>

  <rect x="210" y="190" width="70" height="40" class="field-box" fill="#FFCCCC" opacity="0.7"/>
  <text x="245" y="213" class="field-label" text-anchor="middle">TS?</text>
  <text x="245" y="228" class="field-size" text-anchor="middle">8B*</text>

  <rect x="280" y="190" width="50" height="40" class="field-box"/>
  <text x="305" y="213" class="field-label" text-anchor="middle">ExtFC</text>
  <text x="305" y="228" class="field-size" text-anchor="middle">1B</text>

  <rect x="330" y="190" width="60" height="40" class="field-box"/>
  <text x="360" y="213" class="field-label" text-anchor="middle">PayLen</text>
  <text x="360" y="228" class="field-size" text-anchor="middle">2B</text>

  <rect x="390" y="190" width="150" height="40" class="field-box"/>
  <text x="465" y="213" class="field-label" text-anchor="middle">Payload</text>
  <text x="465" y="228" class="field-size" text-anchor="middle">N B</text>

  <rect x="540" y="190" width="80" height="40" class="field-box" fill="#FFCCCC" opacity="0.7"/>
  <text x="580" y="213" class="field-label" text-anchor="middle">HMAC?</text>
  <text x="580" y="228" class="field-size" text-anchor="middle">32B*</text>

  <!-- Legend -->
  <text x="20" y="300" class="section-title">Legend:</text>
  
  <text x="20" y="325" font-family="monospace" font-size="11">
    <tspan>FC = Function Code (0x6E for extended)</tspan>
    <tspan x="20" dy="18">Ver = Version (currently 1)</tspan>
    <tspan x="20" dy="18">Flags = Bitmap for TS, HMAC presence</tspan>
    <tspan x="20" dy="18">CorrID = Correlation ID for pipelining (0x0000-0xFFFF)</tspan>
    <tspan x="20" dy="18">TS? = Optional timestamp (only if flag set)</tspan>
    <tspan x="20" dy="18">ExtFC = Extended function code (0x00-0xFF)</tspan>
    <tspan x="20" dy="18">PayLen = Length of payload in bytes</tspan>
    <tspan x="20" dy="18">Payload = Extended operation data</tspan>
    <tspan x="20" dy="18">HMAC? = Optional HMAC-SHA256 digest (only if flag set)</tspan>
    <tspan x="20" dy="18">* = Only present if corresponding flag is set</tspan>
  </text>
</svg>'''

    output_file = output_dir / 'pdu-layout.svg'
    with open(output_file, 'w') as f:
        f.write(svg_content)
    print(f"✓ Generated PDU layout diagram: {output_file.name}")


def generate_benchmark_diagram(output_dir):
    """Generate benchmark comparison chart as SVG."""
    svg_content = '''<svg xmlns="http://www.w3.org/2000/svg" width="800" height="500" viewBox="0 0 800 500">
  <defs>
    <style>
      .axis { stroke: #333; stroke-width: 2; }
      .gridline { stroke: #ddd; stroke-width: 1; }
      .bar-sequential { fill: #FF9999; stroke: #cc0000; stroke-width: 2; }
      .bar-pipelined { fill: #99CC99; stroke: #00cc00; stroke-width: 2; }
      .label { font-family: Arial, sans-serif; font-size: 14px; font-weight: bold; text-anchor: middle; }
      .title { font-family: Arial, sans-serif; font-size: 18px; font-weight: bold; }
      .value-label { font-family: monospace; font-size: 12px; font-weight: bold; }
      .annotation { font-family: Arial; font-size: 14px; font-weight: bold; fill: #333; }
    </style>
  </defs>

  <!-- Title -->
  <text x="400" y="30" class="title" text-anchor="middle">Sequential vs. Pipelined: 16 Requests Over 10ms-Latency Link</text>

  <!-- Y-axis -->
  <line x1="80" y1="400" x2="80" y2="50" class="axis"/>
  <!-- X-axis -->
  <line x1="80" y1="400" x2="750" y2="400" class="axis"/>

  <!-- Grid lines -->
  <line x1="80" y1="350" x2="750" y2="350" class="gridline"/>
  <line x1="80" y1="300" x2="750" y2="300" class="gridline"/>
  <line x1="80" y1="250" x2="750" y2="250" class="gridline"/>
  <line x1="80" y1="200" x2="750" y2="200" class="gridline"/>
  <line x1="80" y1="150" x2="750" y2="150" class="gridline"/>
  <line x1="80" y1="100" x2="750" y2="100" class="gridline"/>

  <!-- Y-axis labels (ms) -->
  <text x="60" y="405" class="label" font-size="11">0</text>
  <text x="60" y="355" class="label" font-size="11">50</text>
  <text x="60" y="305" class="label" font-size="11">100</text>
  <text x="60" y="255" class="label" font-size="11">150</text>

  <!-- Bars -->
  <rect x="150" y="160" width="100" height="240" class="bar-sequential"/>
  <rect x="350" y="360" width="100" height="40" class="bar-pipelined"/>

  <!-- Bar labels -->
  <text x="200" y="420" class="label">Sequential</text>
  <text x="400" y="420" class="label">Pipelined</text>

  <!-- Value labels -->
  <text x="200" y="150" class="value-label" text-anchor="middle">160ms</text>
  <text x="400" y="350" class="value-label" text-anchor="middle">20ms</text>

  <!-- Speedup annotation -->
  <rect x="250" y="200" width="200" height="80" fill="yellow" opacity="0.2" stroke="#ff9900" stroke-width="2" rx="5"/>
  <text x="350" y="230" class="annotation" text-anchor="middle">8x</text>
  <text x="350" y="255" class="annotation" font-size="12" text-anchor="middle">Speedup</text>

  <!-- Y-axis label -->
  <text x="15" y="220" class="label" font-size="12">Time (ms)</text>
</svg>'''

    output_file = output_dir / 'pipelined-speedup.svg'
    with open(output_file, 'w') as f:
        f.write(svg_content)
    print(f"✓ Generated benchmark chart: {output_file.name}")


def main():
    """Generate all diagram assets."""
    print("Generating modbus_pp documentation diagrams...\n")
    
    output_dir = create_directories()
    
    generate_pdu_layout_diagram(output_dir)
    generate_benchmark_diagram(output_dir)
    
    print(f"\n✓ All diagrams generated successfully!")
    print(f"  Location: docusaurus/static/diagrams/")


if __name__ == '__main__':
    main()
