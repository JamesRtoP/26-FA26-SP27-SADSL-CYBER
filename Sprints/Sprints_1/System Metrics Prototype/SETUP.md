# System Metrics Prototype

Qt desktop prototype that embeds a Kibana dashboard for local CPU, memory, disk, and network metrics.

## Prerequisites

- Docker Desktop
- Qt Creator
- Qt 6 with **MSVC 2022 64-bit**
- Qt WebEngine, Qt WebChannel, and Qt Positioning
- Visual Studio C++ build tools

## Run it

### 1. Start Elasticsearch and Kibana

From the repo root:

```powershell
docker compose up -d
```

Kibana should open at:

```text
http://localhost:5601
```

### 2. Import the Kibana dashboard

In Kibana:

**Stack Management → Saved Objects → Import**

Import:

```text
system-metrics-dashboard.ndjson
```

### 3. Start Elastic Agent

Download **Elastic Agent 9.5.4 for Windows x86_64** and extract it.

The download may create a nested folder. Open the **inner folder that contains `elastic-agent.exe`**.

Example:

```text
elastic-agent-9.5.4-windows-x86_64/
└── elastic-agent-9.5.4-windows-x86_64/
    └── elastic-agent.exe
```

Copy the repo's:

```text
elastic-agent.yml
```

into that same inner folder next to `elastic-agent.exe`.

Then, from that folder, run:

```powershell
.\elastic-agent.exe run -e
```

Leave that terminal open while using the app.

### 4. Run the Qt app

Open:

```text
Qt_Kibana_Integration/CMakeLists.txt
```

in Qt Creator.

Select the **MSVC 2022 64-bit** kit, then build and run.

## Stop

Stop Elastic Agent with `Ctrl+C`.

Stop Docker with:

```powershell
docker compose down
```

This keeps the Elasticsearch/Kibana data volume.
