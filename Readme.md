                          ┌──────────────────────┐
                          │        GUI           │
                          │  gui_config.json     │
                          └─────────┬────────────┘
                                    │
                    (protobuf cmd    │   / events)
                                    ▼
                          ┌──────────────────────┐
                          │      EventHub        │
                          │  hub_config.json     │
                          └───────┬──────────────┘
                                  │
        ┌──────────────┬──────────┴──────────┬──────────────┐
        │              │                     │              │
        ▼              ▼                     ▼              ▼
 ┌────────────┐ ┌────────────┐       ┌────────────┐ ┌────────────┐
 │ VideoWorker│ │SensorWorker│       │SystemWorker│ │ AI Worker  │
 │ video.json │ │sensor.json │       │system.json │ │ ai.json    │
 └─────┬──────┘ └─────┬──────┘       └─────┬──────┘ └────────────┘
       │              │                     │
       ▼              ▼                     ▼
   (frame port)   sensor data         system data