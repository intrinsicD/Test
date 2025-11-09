# Logger Quick Reference

## Include
```cpp
#include "engine/core/log.hpp"
```

## Initialization (Optional - Auto-initializes)
```cpp
engine::core::Log::init();
engine::core::Log::shutdown();
```

## Basic Macros

### Engine Core Logger
```cpp
ENGINE_CORE_TRACE("message");
ENGINE_CORE_DEBUG("message");
ENGINE_CORE_INFO("message");
ENGINE_CORE_WARN("message");
ENGINE_CORE_ERROR("message");
ENGINE_CORE_CRITICAL("message");
```

### Application/Client Logger
```cpp
ENGINE_TRACE("message");
ENGINE_DEBUG("message");
ENGINE_INFO("message");
ENGINE_WARN("message");
ENGINE_ERROR("message");
ENGINE_CRITICAL("message");
```

## Formatted Logging
```cpp
ENGINE_INFO("Frame {}: FPS = {:.2f}", frame_num, fps);
ENGINE_CORE_DEBUG("Loading {}", filename);
ENGINE_ERROR("Failed with code: {}", error_code);
```

## Set Log Level
```cpp
#include <spdlog/spdlog.h>

engine::core::Log::set_level(spdlog::level::trace);  // Show everything
engine::core::Log::set_level(spdlog::level::debug);  // Hide trace
engine::core::Log::set_level(spdlog::level::info);   // Hide trace+debug
engine::core::Log::set_level(spdlog::level::warn);   // Only warn+error+critical
engine::core::Log::set_level(spdlog::level::err);    // Only error+critical
```

## Custom Logger for Modules
```cpp
class MyModule {
    std::shared_ptr<spdlog::logger> m_logger;
public:
    MyModule() {
        m_logger = engine::core::Log::create_logger("MYMODULE");
        m_logger->info("Module created");
    }
};
```

## Log Output

### Console (colored)
```
[12:34:56] ENGINE: Engine starting...
[12:34:56] APP: Application ready
```

### File (logs/engine.log)
```
[2025-11-09 12:34:56.123] [info] ENGINE: Engine starting...
[2025-11-09 12:34:56.124] [info] APP: Application ready
```

## When to Use Each Level

| Level    | Use Case |
|----------|----------|
| TRACE    | Very detailed, step-by-step execution flow |
| DEBUG    | Detailed information for debugging |
| INFO     | General informational messages |
| WARN     | Potentially harmful situations |
| ERROR    | Error events that might allow continued operation |
| CRITICAL | Severe errors that may cause termination |

