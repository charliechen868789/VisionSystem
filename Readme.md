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




   gui_config.json
                         │
                    ┌────▼────┐
                    │   GUI   │  ← video settings (source, fps, resolution,
                    └────┬────┘    ai model, confidence, toggles)
                         │ CONTROL_ACTION (settings update)
                         ▼
                    ┌─────────────┐
                    │  EventHub   │ ← hub_config.json
                    └──┬──────┬───┘
                       │      │ forward settings to workers
              ┌────────▼─┐  ┌─▼──────────┐
              │VideoWorker│  │  AIWorker  │ ← ai.json
              │video.json │  │            │
              └────┬──────┘  └─────┬──────┘
                   │ raw frames    │ AI results
                   │ (port 9001)   │ (port 9004)
                   └──────┬────────┘
                           ▼
                      EventHub SUB
                      (merges streams)
                           │
                      GUI SUB (port 9005)
                      shows results + overlay




GUI toggle "Face Detection ON"
  → CONTROL_ACTION {action="face_detection", value="true"}
  → EventHub receives
  → EventHub forwards to VideoWorker AND AIWorker via their config socket (port 9010)
  → Workers update runtime config
  → Settings saved to video.json / ai.json